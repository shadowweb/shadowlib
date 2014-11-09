#include "core/memory.h"
#include "core/time.h"
#include "thread/threaded-test.h"
#include "thread/mpsc-futex-ring-buffer.h"

typedef struct swFutexRingBufferTestThreadData
{
  swMPSCFutexRingBuffer *ringBuffer;
  size_t    bytesAcquired;
  size_t    acquireBytesTotal;
  uint64_t  acquireSuccess;
  uint64_t  acquireFailure;
} swFutexRingBufferTestThreadData;

typedef struct swFutexRingBufferTestData
{
  swMPSCFutexRingBuffer *ringBuffer;
  swFutexRingBufferTestThreadData *threadData;
  size_t acquireBytesPerThread;
  size_t acquireBytesTotal;
  size_t consumedBytesTotal;
} swFutexRingBufferTestData;

static const size_t acquireBytesTotal = 64 * 1024 * 1024;
static const size_t acquireBytes      = 64;

bool ringBufferConsumeFunction(uint8_t *buffer, size_t size, void *data)
{
  swThreadedTestData *testThreadData = data;
  swFutexRingBufferTestData *testData = swThreadedTestDataGet(testThreadData);
  if (testData)
  {
    testData->consumedBytesTotal += size;
    if (testData->consumedBytesTotal == testData->acquireBytesTotal)
      swEdgeAsyncSend(&(testThreadData->killLoop));
  }
  return true;
}

void swMPSCFutexRingBufferDataSetup(swThreadedTestData *data)
{
  bool success = false;
  swFutexRingBufferTestData *testData = swMemoryCalloc(1, sizeof(*testData));
  if (testData)
  {
    if ((testData->ringBuffer = swMPSCFutexRingBufferNew(&(data->threadManager), 4, ringBufferConsumeFunction, data)))
    {
      testData->acquireBytesPerThread = acquireBytesTotal / data->numThreads;
      testData->acquireBytesTotal = acquireBytesTotal;
      testData->consumedBytesTotal = 0;
      if ((testData->threadData = swMemoryCalloc(data->numThreads, sizeof(swFutexRingBufferTestThreadData))))
      {
        for(uint32_t i = 0; i < data->numThreads; i++)
        {
          testData->threadData[i].ringBuffer = testData->ringBuffer;
          testData->threadData[i].acquireBytesTotal = testData->acquireBytesPerThread;
        }
        success = true;
        swThreadedTestDataSet(data, testData);
      }
      if (!success)
        swMPSCFutexRingBufferDelete(testData->ringBuffer);
    }
    if (!success)
      swMemoryFree(testData);
  }
  ASSERT_TRUE(success);
}

void swMPSCFutexRingBufferDataTeardown(swThreadedTestData *data)
{
  swFutexRingBufferTestData *testData = swThreadedTestDataGet(data);
  if (testData)
  {
    for (uint32_t i = 0; i < data->numThreads; i++)
    {
      swFutexRingBufferTestThreadData *threadData = &(testData->threadData[i]);
      swTestLogLine("Thread %u: bytes acquired = %zu, success/failure attepts = %lu/%lu\n", i, threadData->bytesAcquired, threadData->acquireSuccess, threadData->acquireFailure);
      swTestLogLine("Thread %u: thread time = %lu ns, total time = %lu ns\n", i, data->threadData[i].executionCPUTime, data->threadData[i].executionTotalTime);
    }
    swTestLogLine("Acquire Total %zu, Consumed Total %zu\n", testData->acquireBytesTotal, testData->consumedBytesTotal);

    if (testData->ringBuffer)
      swMPSCFutexRingBufferDelete(testData->ringBuffer);
    if (testData->threadData)
      swMemoryFree(testData->threadData);
    swMemoryFree(testData);
    swThreadedTestDataSet(data, NULL);
  }
}

void swMPSCFutexRingBufferThreadDataSetup(swThreadedTestData *data, swThreadedTestThreadData *threadData)
{
  swFutexRingBufferTestData *testData = swThreadedTestDataGet(data);
  swThreadedTestThreadDataSet(threadData, &(testData->threadData[threadData->id]));
}

void swMPSCFutexRingBufferThreadDataTeardown(swThreadedTestData *data, swThreadedTestThreadData *threadData)
{
  swThreadedTestThreadDataSet(threadData, NULL);
}

bool swMPSCFutexRingBufferThreadDataRun(swThreadedTestData *data, swThreadedTestThreadData *threadData)
{
  bool rtn = true;
  swFutexRingBufferTestThreadData *localThreadData = swThreadedTestThreadDataGet(threadData);
  // struct timespec sleepInterval = { .tv_sec = 0, .tv_nsec = 1000 };
  uint8_t *buffer = NULL;
  while (!threadData->shutdown && localThreadData->bytesAcquired < localThreadData->acquireBytesTotal)
  {
    if (swMPSCFutexRingBufferProduceAcquire(localThreadData->ringBuffer, &buffer, acquireBytes))
    {
      localThreadData->acquireSuccess++;
      localThreadData->bytesAcquired += acquireBytes;
      if (!swMPSCFutexRingBufferProduceRelease(localThreadData->ringBuffer, buffer, acquireBytes))
      {
        rtn = false;
        break;
      }
    }
    else
    {
      localThreadData->acquireFailure++;
      // WARNING: does not behave well in valgrind test without this nanosleep
      // nanosleep(&sleepInterval, NULL);
      pthread_yield();
    }
  }
  return rtn;
}

static uint32_t threadCounts[] = {1, 2, 4, 8, 16};

swThreadedTestDeclare(MPSCFutexRingBuffer, swMPSCFutexRingBufferDataSetup, swMPSCFutexRingBufferDataTeardown,
                   swMPSCFutexRingBufferThreadDataSetup, swMPSCFutexRingBufferThreadDataTeardown, swMPSCFutexRingBufferThreadDataRun,
                   threadCounts);
