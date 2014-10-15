#include "thread/thread-manager.h"
#include "unittest/unittest.h"
#include <core/memory.h>

#include <unistd.h>

void threadManagerSetup(swTestSuite *suite)
{
  swEdgeLoop *loop = swEdgeLoopNew();
  ASSERT_NOT_NULL(loop);
  swThreadManager *manager = swThreadManagerNew(loop);
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

void threadCreateSetup(swTestSuite *suite, swTest *test)
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
  testData->terminate = arg;
  *(testData->terminate) = 1L;
}

void doneFunction(void *arg, void *returnValue)
{
  swTestLogLine("Done thread\n");
  swTreadTestData *testData = arg;
  ASSERT_NOT_NULL(testData);
  swEdgeLoopBreak(testData->manager->loop);
}

swTestDeclare(ThreadCreate, threadCreateSetup, threadCreateTearDown, swTestRun)
{
  swThreadManager *manager = swTestSuiteDataGet(suite);
  uint64_t *terminate = swTestDataGet(test);
  swTreadTestData testData = {.manager = manager, .terminate = terminate};
  // TODO: trigger thread exit somehow
  ASSERT_TRUE(swThreadManagerStartThread(manager, runFunction, stopFunction, doneFunction, &testData));
  swEdgeLoopRun(manager->loop, false);
  return true;
}

swTestSuiteStructDeclare(ThreadManagerTest, threadManagerSetup, threadManagerTearDown, swTestRun,
                         &ThreadCreate);
