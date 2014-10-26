#include "thread/mpsc-futex-ring-buffer.h"

#include "core/memory.h"
#include "core/time.h"
#include "unittest/unittest.h"

typedef struct swRingBufferTestThreadData
{
  swMPSCFutexRingBuffer *ringBuffer;
  size_t    bytesAcquired;
  size_t    acquireBytesTotal;
  uint64_t  acquireSuccess;
  uint64_t  acquireFailure;
  uint64_t  executionCPUTime;
  uint64_t  executionTotalTime;
  uint32_t  id;
  bool      shutdown;
  bool      done;
} swRingBufferTestThreadData;

typedef struct swRingBufferTestData
{
  swMPSCFutexRingBuffer *ringBuffer;
  swRingBufferTestThreadData *threadData;
  swEdgeAsync killLoop;
  size_t acquireBytesPerThread;
  size_t acquireBytesTotal;
  size_t consumedBytesTotal;
  size_t producedBytesTotal;
  uint32_t numThreads;
} swRingBufferTestData;

static const size_t acquireBytesTotal = 64 * 1024 * 1024;
static const size_t acquireBytes      = 64;

void swRingBufferTestDataDelete(swRingBufferTestData *testData)
{
  if (testData)
  {
    if (testData->killLoop.watcher.loop)
      swEdgeAsyncClose(&(testData->killLoop));
    swMPSCFutexRingBuffer *ringBuffer = testData->ringBuffer;
    swThreadManager *manager = ringBuffer->threadManager;
    swEdgeLoop *loop = manager->loop;
    swMPSCFutexRingBufferDelete(ringBuffer);
    swThreadManagerDelete(manager);
    if (testData->threadData)
      swMemoryFree(testData->threadData);
    swEdgeLoopDelete(loop);
    swMemoryFree(testData);
  }
}

bool ringBufferConsumeFunction(uint8_t *buffer, size_t size, void *data)
{
  swRingBufferTestData *testData = data;
  if (testData)
  {
    testData->consumedBytesTotal += size;
    if (testData->consumedBytesTotal == testData->acquireBytesTotal)
      swEdgeAsyncSend(&(testData->killLoop));
  }
  return true;
}

void *testRunFunction(swRingBufferTestThreadData *threadData)
{
  if (threadData)
  {
    uint64_t threadCPUTime  = swTimeGet(CLOCK_THREAD_CPUTIME_ID);
    uint64_t totalTime      = swTimeGet(CLOCK_MONOTONIC_RAW);
    uint8_t *buffer = NULL;
    while (!threadData->shutdown && threadData->bytesAcquired < threadData->acquireBytesTotal)
    {
      if (swMPSCFutexRingBufferProduceAcquire(threadData->ringBuffer, &buffer, acquireBytes))
      {
        threadData->acquireSuccess++;
        threadData->bytesAcquired += acquireBytes;
        if (!swMPSCFutexRingBufferProduceRelease(threadData->ringBuffer, buffer, acquireBytes))
          break;
        __sync_add_and_fetch(&(((swRingBufferTestData *)(threadData->ringBuffer->data))->producedBytesTotal), acquireBytes);
      }
      else
      {
        threadData->acquireFailure++;
        pthread_yield();
      }
    }
    threadData->executionTotalTime  = swTimeGet(CLOCK_MONOTONIC_RAW) - totalTime;
    threadData->executionCPUTime    = swTimeGet(CLOCK_THREAD_CPUTIME_ID) - threadCPUTime;
  }
  return NULL;
}

void testStopFunction(swRingBufferTestThreadData *threadData)
{
  if (threadData)
    threadData->shutdown = true;
}

void testDoneFunction(swRingBufferTestThreadData *threadData, void *returnValue)
{
  ASSERT_NOT_NULL(threadData);
  if (threadData)
  {
    threadData->done = true;
    ASSERT_EQUAL(threadData->bytesAcquired, threadData->acquireBytesTotal);
  }
}

void killLoop(swEdgeAsync *asyncWatcher, eventfd_t eventCount, uint32_t events)
{
  if (asyncWatcher)
    swEdgeLoopBreak(asyncWatcher->watcher.loop);
}

bool swRingBufferTestDataSetTest(swRingBufferTestData *testData, size_t acquireBytesTotal, uint32_t numThreads)
{
  bool rtn = false;
  if (testData && acquireBytesTotal && numThreads)
  {
    if ((testData->threadData = swMemoryCalloc(numThreads, sizeof(swRingBufferTestThreadData))))
    {
      swThreadManager *manager = testData->ringBuffer->threadManager;
      testData->acquireBytesPerThread = acquireBytesTotal / numThreads;
      testData->acquireBytesTotal = acquireBytesTotal;
      testData->consumedBytesTotal = 0;
      testData->numThreads = numThreads;
      if (swEdgeAsyncInit(&(testData->killLoop), killLoop))
      {
        if (swEdgeAsyncStart(&(testData->killLoop), manager->loop))
        {
          uint32_t i = 0;
          for(; i < numThreads; i++)
          {
            testData->threadData[i].ringBuffer = testData->ringBuffer;
            testData->threadData[i].acquireBytesTotal = testData->acquireBytesPerThread;
            testData->threadData[i].id = i;
            if (!swThreadManagerStartThread(manager, (swThreadRunFunction)testRunFunction, (swThreadStopFunction)testStopFunction, (swThreadDoneFunction)testDoneFunction, &(testData->threadData[i])))
              break;
          }
          if (i == numThreads)
            rtn = true;
          else
          {
            while (i > 0)
            {
              while (!testData->threadData[i - 1].done)
              {
                testStopFunction(&testData->threadData[i - 1]);
                swEdgeLoopRun(manager->loop, true);
              }
              i--;
            }
          }
        }
        if (!rtn)
          swEdgeAsyncClose(&(testData->killLoop));
      }
      if (!rtn)
      {
        swMemoryFree(testData->threadData);
        testData->threadData = NULL;
      }
    }
  }
  return rtn;
}

swRingBufferTestData *swRingBufferTestDataNew(size_t acquireBytesTotal, uint32_t numThreads)
{
  swRingBufferTestData *rtn = NULL;
  if (acquireBytesTotal && numThreads)
  {
    swRingBufferTestData *testData = swMemoryCalloc(1, sizeof(*testData));
    if (testData)
    {
      swEdgeLoop *loop = swEdgeLoopNew();
      if (loop)
      {
        swThreadManager *manager = swThreadManagerNew(loop, 1000);
        if (manager)
        {
          if ((testData->ringBuffer = swMPSCFutexRingBufferNew(manager, 4, ringBufferConsumeFunction, testData)))
          {
            if (swRingBufferTestDataSetTest(testData, acquireBytesTotal, numThreads))
              rtn = testData;
          }
          if (!rtn)
            swThreadManagerDelete(manager);
        }
        if (!rtn)
          swEdgeLoopDelete(loop);
      }
      if (!rtn)
        swMemoryFree(testData);
    }
  }
  return rtn;
}

static inline void setUpAnyTest(swTest *test, uint32_t numThreads)
{
  swRingBufferTestData *testData = swRingBufferTestDataNew(acquireBytesTotal, numThreads);
  ASSERT_NOT_NULL(testData);
  if (testData)
    swTestDataSet(test, testData);
}

void testSetUpOneThread(swTestSuite *suite, swTest *test)
{
  setUpAnyTest(test, 1);
}

void testSetUpTwoThreads(swTestSuite *suite, swTest *test)
{
  setUpAnyTest(test, 2);
}

void testSetUpFourThreads(swTestSuite *suite, swTest *test)
{
  setUpAnyTest(test, 4);
}

void testSetUpEightThreads(swTestSuite *suite, swTest *test)
{
  setUpAnyTest(test, 8);
}

void testSetUpSixteenThreads(swTestSuite *suite, swTest *test)
{
  setUpAnyTest(test, 16);
}

void testTearDown(swTestSuite *suite, swTest *test)
{
  swRingBufferTestData *testData = swTestDataGet(test);
  if (testData)
  {
    for (uint32_t i = 0; i < testData->numThreads; i++)
    {
      swRingBufferTestThreadData *threadData = &(testData->threadData[i]);
      swTestLogLine("Thread %u: bytes acquired = %zu, success/failure attepts = %lu/%lu\n", i, threadData->bytesAcquired, threadData->acquireSuccess, threadData->acquireFailure);
      swTestLogLine("Thread %u: thread time = %lu ns, total time = %lu ns\n", i, threadData->executionCPUTime, threadData->executionTotalTime);
    }
    swTestLogLine("Acquire Total %zu, Consumed Total %zu\n", testData->acquireBytesTotal, testData->consumedBytesTotal);
    swRingBufferTestDataDelete(testData);
    swTestDataSet(test, NULL);
  }
}

static inline bool runAnyTest(swTest *test)
{
  bool rtn = false;
  swRingBufferTestData *testData = swTestDataGet(test);
  swMPSCFutexRingBuffer *ringBuffer = testData->ringBuffer;
  if (ringBuffer)
  {
    swThreadManager *manager = ringBuffer->threadManager;
    if (manager)
    {
      swEdgeLoopRun(manager->loop, false);
      rtn = true;
    }
  }
  return rtn;
}

swTestDeclare(OneThread, testSetUpOneThread, testTearDown, swTestRun)
{
  return runAnyTest(test);
}

swTestDeclare(TwoThreads, testSetUpTwoThreads, testTearDown, swTestRun)
{
  return runAnyTest(test);
}

swTestDeclare(FourThreads, testSetUpFourThreads, testTearDown, swTestRun)
{
  return runAnyTest(test);
}

swTestDeclare(EightThreads, testSetUpEightThreads, testTearDown, swTestRun)
{
  return runAnyTest(test);
}

swTestDeclare(SixteenThreads, testSetUpSixteenThreads, testTearDown, swTestRun)
{
  return runAnyTest(test);
}

swTestSuiteStructDeclare(MPSCRingBufferSimpleTest, NULL, NULL, swTestRun,
                         &OneThread, &TwoThreads, &FourThreads, &EightThreads, &SixteenThreads);
