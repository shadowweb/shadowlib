#include "thread/mpsc-ring-buffer.h"

#include "core/memory.h"
#include "core/time.h"
#include "unittest/unittest.h"

bool ringBufferConsumeFunction(uint8_t *data, size_t size, void *arg)
{
  return true;
}

void ringBufferSetUp(swTestSuite *suite)
{
  swEdgeLoop *loop = swEdgeLoopNew();
  ASSERT_NOT_NULL(loop);
  swThreadManager *manager = swThreadManagerNew(loop, 1000);
  ASSERT_NOT_NULL(manager);
  swMPSCRingBuffer *ringBuffer = swMPSCRingBufferNew(manager, 4, ringBufferConsumeFunction, NULL);
  ASSERT_NOT_NULL(ringBuffer);
  swTestSuiteDataSet(suite, ringBuffer);
}

void ringBufferTearDown(swTestSuite *suite)
{
  swMPSCRingBuffer *ringBuffer = swTestSuiteDataGet(suite);
  swThreadManager *manager = ringBuffer->threadManager;
  swEdgeLoop *loop = manager->loop;
  swMPSCRingBufferDelete(ringBuffer);
  swThreadManagerDelete(manager);
  swEdgeLoopDelete(loop);
}

typedef struct swRingBufferTestData
{
  swMPSCRingBuffer *ringBuffer;
  size_t    bytesAcquired;
  uint64_t  acquireSuccess;
  uint64_t  acquireFailure;
  uint64_t  executionCPUTime;
  uint64_t  executionTotalTime;
} swRinfBufferTestData;

static const size_t acquireBytesTotal = 64 * 1024 * 1024;
static const size_t acquireBytes      = 64;

void *testRunFunction(swRinfBufferTestData *testData)
{
  if (testData)
  {
    uint64_t threadCPUTime  = swTimeGet(CLOCK_THREAD_CPUTIME_ID);
    uint64_t totalTime      = swTimeGet(CLOCK_MONOTONIC_RAW);
    uint8_t *buffer = NULL;
    while (testData->bytesAcquired < acquireBytesTotal)
    {
      if (swMPSCRingBufferProduceAcquire(testData->ringBuffer, &buffer, acquireBytes))
      {
        testData->acquireSuccess++;
        testData->bytesAcquired += acquireBytes;
        if (!swMPSCRingBufferProduceRelease(testData->ringBuffer, buffer, acquireBytes))
          break;
      }
      else
      {
        testData->acquireFailure++;
        // WARNING: does not behave well in valgrind without this nanosleep, causes starvation of the consumer thread
        struct timespec sleepInterval = { .tv_sec = 0, .tv_nsec = 5 };
        nanosleep(&sleepInterval, NULL);
      }
    }
    testData->executionTotalTime  = swTimeGet(CLOCK_MONOTONIC_RAW) - totalTime;
    testData->executionCPUTime    = swTimeGet(CLOCK_THREAD_CPUTIME_ID) - threadCPUTime;
  }
  return NULL;
}

void testDoneFunction(swRinfBufferTestData *testData, void *returnValue)
{
  ASSERT_NOT_NULL(testData);
  if (testData)
  {
    ASSERT_EQUAL(testData->bytesAcquired, acquireBytesTotal);
    swEdgeLoop *loop = testData->ringBuffer->threadManager->loop;
    swEdgeLoopBreak(loop);
  }
}

void testSetUp(swTestSuite *suite, swTest *test)
{
  swMPSCRingBuffer *ringBuffer = swTestSuiteDataGet(suite);
  swThreadManager *manager = ringBuffer->threadManager;
  swRinfBufferTestData *testData = swMemoryCalloc(1, sizeof(*testData));
  ASSERT_NOT_NULL(testData);
  if (testData)
  {
    testData->ringBuffer = ringBuffer;
    ASSERT_TRUE (swThreadManagerStartThread(manager, (swThreadRunFunction)testRunFunction, NULL, (swThreadDoneFunction)testDoneFunction, testData));
  }
  swTestDataSet(test, testData);
}

void testTearDown(swTestSuite *suite, swTest *test)
{
  swRinfBufferTestData *testData = swTestDataGet(test);
  ASSERT_NOT_NULL(testData);
  if (testData)
  {
    swTestLogLine("bytes acquired = %zu, success/failure attepts = %lu/%lu\n", testData->bytesAcquired, testData->acquireSuccess, testData->acquireFailure);
    swTestLogLine("thread time = %lu ns, total time = %lu ns\n", testData->executionCPUTime, testData->executionTotalTime);
    swMemoryFree(testData);
  }
  swTestDataSet(test, NULL);
}

swTestDeclare(SingleThread, testSetUp, testTearDown, swTestRun)
{
  bool rtn = false;

  swMPSCRingBuffer *ringBuffer = swTestSuiteDataGet(suite);
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

swTestSuiteStructDeclare(MPSCRingBufferSimpleTest, ringBufferSetUp, ringBufferTearDown, swTestRun,
                         &SingleThread);

// TODO: add a test with multiple producer threads
