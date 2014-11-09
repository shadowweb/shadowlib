#include "thread/threaded-test.h"

#include "core/memory.h"
#include "core/time.h"

static swThreadedTestSuite threadedTestGlobal __attribute__ ((section(".thrededtest"))) = { 0 };

void swThreadedTestSetup(swTestSuite *suite, swTest *test);
void swThreadedTestTeardown(swTestSuite *suite, swTest *test);
bool swThreadedTestRun(swTestSuite *suite, swTest *test);

swTest *swThreadedTestNew(const char *name, uint32_t threadCount)
{
  swTest *test = swMemoryCalloc(1, sizeof(*test));
  if (test)
  {
    swDynamicString *testName = swDynamicStringNewFromFormat("%s.TestThreads.%u", name, threadCount);
    if (testName)
    {
      test->testName = testName->data;
      swMemoryFree(testName);
      test->setupFunc = swThreadedTestSetup;
      test->teardownFunc = swThreadedTestTeardown;
      test->runFunc = swThreadedTestRun;
    }
    else
    {
      swMemoryFree(test);
      test = NULL;
    }
  }
  return test;
}

void swThreadedTestDelete(swTest *test)
{
  if (test)
  {
    swMemoryFree((char *)(test->testName));
    swMemoryFree(test);
  }
}

bool swThreadedTestSuiteInit(swThreadedTestSuite *test)
{
  bool rtn = false;
  if (test)
  {
    uint32_t numberRuns = test->threadCounts.count;
    uint32_t *numberThreads = (uint32_t *)(test->threadCounts.data);
    if ((test->tests = swMemoryCalloc(numberRuns + 1, sizeof(swTest *))))
    {
      test->currentTest = 0;
      uint32_t i = 0;
      for (; i < numberRuns; i++)
      {
        if (!(test->tests[i] = swThreadedTestNew(test->testName, numberThreads[i])))
          break;
      }
      if (i == numberRuns)
        rtn = true;
      else
      {
        while (i > 0)
        {
          i--;
          swThreadedTestDelete(test->tests[i]);
        }
        swMemoryFree(test->tests);
        test->tests = NULL;
      }
    }
  }
  return rtn;
}

void swThreadedTestSuiteRelease(swThreadedTestSuite *test)
{
  if (test)
  {
    uint32_t numberRuns = test->threadCounts.count;
    for (uint32_t i = 0; i < numberRuns; i++)
      swThreadedTestDelete(test->tests[i]);
    swMemoryFree(test->tests);
    test->tests = NULL;
  }
}

void *testRunFunction(swThreadedTestThreadData *threadData)
{
  if (threadData)
  {
    uint64_t threadCPUTime  = swTimeGet(CLOCK_THREAD_CPUTIME_ID);
    uint64_t totalTime      = swTimeGet(CLOCK_MONOTONIC_RAW);
    if (threadData->test->threadRunFunc)
      ASSERT_TRUE(threadData->test->threadRunFunc(&(threadData->test->testData), threadData));
    threadData->executionTotalTime  = swTimeGet(CLOCK_MONOTONIC_RAW) - totalTime;
    threadData->executionCPUTime    = swTimeGet(CLOCK_THREAD_CPUTIME_ID) - threadCPUTime;
  }
  return NULL;
}

void testStopFunction(swThreadedTestThreadData *threadData)
{
  if (threadData)
    threadData->shutdown = true;
}

void testDoneFunction(swThreadedTestThreadData *threadData, void *returnValue)
{
  ASSERT_NOT_NULL(threadData);
  if (threadData)
  {
    threadData->done = true;
    if (threadData->test->threadTeardownFunc)
      threadData->test->threadTeardownFunc(&(threadData->test->testData), threadData);
  }
}

void killLoop(swEdgeAsync *asyncWatcher, eventfd_t eventCount, uint32_t events)
{
  if (asyncWatcher)
    swEdgeLoopBreak(asyncWatcher->watcher.loop);
}

void swThreadedTestSetup(swTestSuite *suite, swTest *test)
{
  swThreadedTestSuite *testSuiteData = swTestSuiteDataGet(suite);
  uint32_t numberRuns = testSuiteData->threadCounts.count;
  uint32_t *numberThreads = (uint32_t *)(testSuiteData->threadCounts.data);
  if (testSuiteData->currentTest < numberRuns)
  {
    bool success = false;
    swThreadedTestData *testData = &(testSuiteData->testData);
    testData->numThreads = numberThreads[testSuiteData->currentTest];
    if ((testData->threadData = swMemoryCalloc(testData->numThreads, sizeof(swThreadedTestThreadData))))
    {
      swEdgeLoop *loop = swEdgeLoopNew();
      if (loop)
      {
        if (swThreadManagerInit(&(testData->threadManager), loop, 1000))
        {
          if (testSuiteData->setupFunc)
            testSuiteData->setupFunc(testData);
          if (swEdgeAsyncInit(&(testData->killLoop), killLoop))
          {
            if (swEdgeAsyncStart(&(testData->killLoop), loop))
            {
              uint32_t i = 0;
              for(; i < testData->numThreads; i++)
              {
                testData->threadData[i].id = i;
                testData->threadData[i].test = testSuiteData;
                if (testSuiteData->threadSetupFunc)
                  testSuiteData->threadSetupFunc(testData, &(testData->threadData[i]));
                if (!swThreadManagerStartThread(&(testData->threadManager), (swThreadRunFunction)testRunFunction, (swThreadStopFunction)testStopFunction, (swThreadDoneFunction)testDoneFunction, &(testData->threadData[i])))
                  break;
              }
              if (i == testData->numThreads)
                success = true;
              else
              {
                while (i > 0)
                {
                  i--;
                  while (!testData->threadData[i].done)
                  {
                    testStopFunction(&testData->threadData[i]);
                    swEdgeLoopRun(loop, true);
                  }
                }
              }
            }
            if (!success)
              swEdgeAsyncClose(&(testData->killLoop));
          }
          if (!success)
            swThreadManagerRelease(&(testData->threadManager));
        }
        if (!success)
          swEdgeLoopDelete(loop);
      }
      if (!success)
      {
        swMemoryFree(testData->threadData);
        testData->threadData = NULL;
      }
    }
  }
  else
    ASSERT_TRUE(false);
}

void swThreadedTestTeardown(swTestSuite *suite, swTest *test)
{
  swThreadedTestSuite *testSuiteData = swTestSuiteDataGet(suite);
  uint32_t numberRuns = testSuiteData->threadCounts.count;
  if (testSuiteData->currentTest < numberRuns)
  {
    swThreadedTestData *testData = &(testSuiteData->testData);
    swEdgeLoop *loop = testData->threadManager.loop;

    if (testData->killLoop.watcher.loop)
      swEdgeAsyncClose(&(testData->killLoop));
    swThreadManagerRelease(&(testData->threadManager));
    if (testSuiteData->teardownFunc)
      testSuiteData->teardownFunc(testData);
    swEdgeLoopDelete(loop);
    if (testData->threadData)
    {
      swMemoryFree(testData->threadData);
      testData->threadData = NULL;
    }
    testSuiteData->currentTest++;
  }
}

bool swThreadedTestRun(swTestSuite *suite, swTest *test)
{
  bool rtn = false;
  swThreadedTestSuite *testSuiteData = swTestSuiteDataGet(suite);
  swThreadedTestData *testData = &(testSuiteData->testData);
  if (testData)
  {
    swEdgeLoopRun(testData->threadManager.loop, false);
    rtn = true;
  }
  return rtn;
}

void swThreadedTestSuiteSetup(swTestSuite *suite)
{
  // create test test suite
  if (suite)
  {
    swThreadedTestSuite *testBegin = &threadedTestGlobal;
    swThreadedTestSuite *testEnd = &threadedTestGlobal;

    uint32_t testCount = 0;
    // find begin and end of section by comparing magics
    while (1)
    {
      swThreadedTestSuite *current = testBegin - 1;
      if (current->magic != SW_BENCHMARK_MAGIC)
        break;
      testBegin--;
      testCount++;
    }
    while (1)
    {
      testEnd++;
      if (testEnd->magic != SW_BENCHMARK_MAGIC)
        break;
      testCount++;
    }
    if (testCount == 1)
    {
      swThreadedTestSuite *test = testBegin;
      if (test == &threadedTestGlobal)
        test++;
      if (swThreadedTestSuiteInit(test))
      {
        suite->suiteName = test->testName;
        suite->tests = test->tests;
        swTestSuiteDataSet(suite, test);
      }
      else
        ASSERT_TRUE(false);
    }
    else
      ASSERT_EQUAL(testCount, 1);
  }
}

void swThreadedTestSuiteTeardown(swTestSuite *suite)
{
  // tear down test test suite
  swThreadedTestSuite *testSuiteData = swTestSuiteDataGet(suite);
  swThreadedTestSuiteRelease(testSuiteData);
}

void swThreadedTestDataSet(swThreadedTestData *testData, void *data)
{
  if(testData)
    testData->data = data;
}

void *swThreadedTestDataGet(swThreadedTestData *testData)
{
  return (testData)? testData->data : NULL;
}

void swThreadedTestThreadDataSet(swThreadedTestThreadData *testThreadData, void *data)
{
  if(testThreadData)
    testThreadData->data = data;
}

void *swThreadedTestThreadDataGet(swThreadedTestThreadData *testThreadData)
{
  return (testThreadData)? testThreadData->data : NULL;
}

static swTestSuite testTest __attribute__ ((unused, section(".unittest"))) =
{
  .suiteName = "ThreadedTest",
  .setupFunc = swThreadedTestSuiteSetup,
  .teardownFunc = swThreadedTestSuiteTeardown,
  .data = NULL,
  .tests = (swTest *[]){NULL},
  .magic = SW_TEST_MAGIC,
  .skip = false
};
