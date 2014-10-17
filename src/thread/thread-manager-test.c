#include "core/memory.h"
#include "core/time.h"
#include "thread/thread-manager.h"
#include "unittest/unittest.h"

#include <unistd.h>

void threadManagerSetUp(swTestSuite *suite)
{
  swEdgeLoop *loop = swEdgeLoopNew();
  ASSERT_NOT_NULL(loop);
  swThreadManager *manager = swThreadManagerNew(loop, 1000);
  ASSERT_NOT_NULL(manager);
  swTestSuiteDataSet(suite, manager);
}

void threadManagerTearDown(swTestSuite *suite)
{
  swThreadManager *manager = swTestSuiteDataGet(suite);
  swEdgeLoop *loop = manager->loop;
  swThreadManagerDelete(manager);
  swEdgeLoopDelete(loop);
}

void threadCreateSetUp(swTestSuite *suite, swTest *test)
{
  uint64_t *terminate = swMemoryMalloc(sizeof(*terminate));
  ASSERT_NOT_NULL(terminate);
  *terminate = 0L;
  swTestDataSet(test, terminate);
}

void threadCreateTearDown(swTestSuite *suite, swTest *test)
{
  uint64_t *terminate = swTestDataGet(test);
  ASSERT_NOT_NULL(terminate);
  swMemoryFree(terminate);
}

typedef struct swTreadTestData
{
  swThreadManager *manager;
  uint64_t *terminate;
  swEdgeTimer timer;
} swTreadTestData;

void *runFunction(void *arg)
{
  swTreadTestData *testData = arg;
  ASSERT_NOT_NULL(testData->terminate);
  while (!(*(testData->terminate)))
    sleep(2);
  return NULL;
}

void stopFunction(void *arg)
{
  swTestLogLine("Terminating thread\n");
  swTreadTestData *testData = arg;
  ASSERT_NOT_NULL(testData);
  ASSERT_NOT_NULL(testData->terminate);
  *(testData->terminate) = 1L;
}

void doneFunction(void *arg, void *returnValue)
{
  swTestLogLine("Done thread\n");
  swTreadTestData *testData = arg;
  ASSERT_NOT_NULL(testData);
  swEdgeLoopBreak(testData->manager->loop);
}

void timerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swTreadTestData *testData = swEdgeWatcherDataGet(timer);
  if (testData)
    *(testData->terminate) = true;
}

swTestDeclare(RunOneThread, threadCreateSetUp, threadCreateTearDown, swTestRun)
{
  bool rtn = false;
  swThreadManager *manager = swTestSuiteDataGet(suite);
  uint64_t *terminate = swTestDataGet(test);
  swTreadTestData testData = {.manager = manager, .terminate = terminate};
  if (swEdgeTimerInit(&(testData.timer), timerCallback, false))
  {
    if (swEdgeTimerStart(&(testData.timer), manager->loop, 2000, 0, false))
    {
      swEdgeWatcherDataSet(&(testData.timer), &testData);
      ASSERT_TRUE(swThreadManagerStartThread(manager, runFunction, stopFunction, doneFunction, &testData));
      swEdgeLoopRun(manager->loop, false);
      swEdgeTimerStop(&(testData.timer));
      rtn = true;
    }
    swEdgeTimerClose(&(testData.timer));
  }
  return rtn;
}

swTestSuiteStructDeclare(ThreadManagerSimpleTest, threadManagerSetUp, threadManagerTearDown, swTestRun,
                         &RunOneThread);

// WARNING: valgrind does not like too many threads, settled on 500 for this test
#define SW_NUM_THREADS 500

typedef struct swManyTreadsTestData
{
  swThreadManager manager;
  swEdgeTimer timer;
  uint32_t threadIds[SW_NUM_THREADS];
} swManyTreadsTestData;

void manyThreadsSetUpSuite(swTestSuite *suite)
{
  bool rtn = false;
  swEdgeLoop *loop = swEdgeLoopNew();
  if (loop)
  {
    swManyTreadsTestData *data = swMemoryCalloc(1, sizeof(*data));
    if (data)
    {
      if (swThreadManagerInit(&(data->manager), loop, 1000))
      {
        for (uint32_t i = 0; i < SW_NUM_THREADS; i++)
          data->threadIds[i] = i;
        swTestSuiteDataSet(suite, data);
        rtn = true;
      }
      if (!rtn)
        swMemoryFree(data);
    }
    if (!rtn)
      swEdgeLoopDelete(loop);
  }
  ASSERT_TRUE(rtn);
}

void manyThreadsTearDownSuite(swTestSuite *suite)
{
  swManyTreadsTestData *data = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(data);
  if (data)
  {
    swEdgeLoop *loop = data->manager.loop;
    swThreadManagerRelease(&(data->manager));
    swMemoryFree(data);
    swEdgeLoopDelete(loop);
  }
}

void *threadRunFunction(void *arg)
{
  uint32_t *threadId = arg;
  ASSERT_NOT_NULL(threadId);
  int64_t sleepInterval = rand() % 2000;
  struct timespec interval = {.tv_sec = swTimeMSecToSec(sleepInterval), .tv_nsec = swTimeMSecToNSec(swTimeMSecToSecRem(sleepInterval))};

  nanosleep(&interval, NULL);
  return NULL;
}

void threadDoneFunction(void *arg, void *returnValue)
{
  uint32_t *threadId = arg;
  ASSERT_NOT_NULL(threadId);
}

void manyThreadsSetUp(swTestSuite *suite, swTest *test)
{
  swManyTreadsTestData *data = swTestSuiteDataGet(suite);
  uint32_t i = 0;
  while (i < SW_NUM_THREADS)
  {
    if (swThreadManagerStartThread(&(data->manager), threadRunFunction, NULL, threadDoneFunction, &(data->threadIds[i])))
      i++;
    else
      break;
  }
  ASSERT_TRUE (i == SW_NUM_THREADS);
}

void manyThreadsTearDown(swTestSuite *suite, swTest *test)
{
  swManyTreadsTestData *data = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(data);
}

void manyThreadsTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  static uint32_t timesCalled = 0;
  timesCalled++;
  swManyTreadsTestData *data = swEdgeWatcherDataGet(timer);
  if (!swSparseArrayCount(data->manager.threadInfoArray) || timesCalled == 5)
    swEdgeLoopBreak(data->manager.loop);
}

swTestDeclare(RunManyThreads, manyThreadsSetUp, manyThreadsTearDown, swTestRun)
{
  srand(time(NULL));
  uint32_t threadCount = SW_NUM_THREADS;
  swManyTreadsTestData *data = swTestSuiteDataGet(suite);
  if (swEdgeTimerInit(&(data->timer), manyThreadsTimerCallback, false))
  {
    if (swEdgeTimerStart(&(data->timer), data->manager.loop, 1000, 1000, false))
    {
      swEdgeWatcherDataSet(&(data->timer), data);
      swEdgeLoopRun(data->manager.loop, false);
      threadCount = swSparseArrayCount(data->manager.threadInfoArray);
      ASSERT_EQUAL(threadCount, 0);
      swEdgeTimerStop(&(data->timer));
    }
    swEdgeTimerClose(&(data->timer));
  }
  return (threadCount == 0);
}

swTestSuiteStructDeclare(ThreadManagerTest, manyThreadsSetUpSuite, manyThreadsTearDownSuite, swTestRun,
                         &RunManyThreads);
