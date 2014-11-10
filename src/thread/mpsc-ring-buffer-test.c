#include "core/memory.h"
#include "core/time.h"
#include "thread/threaded-test.h"
#include "thread/mpsc-ring-buffer.h"

typedef struct swRingBufferTestThreadData
{
  swMPSCRingBuffer *ringBuffer;
  size_t    bytesAcquired;
  size_t    acquireBytesTotal;
  uint64_t  acquireSuccess;
  uint64_t  acquireFailure;
  // uint64_t  unused1;
  // uint64_t  unused2;
  // uint64_t  unused3;
} swRingBufferTestThreadData;

typedef struct swRingBufferTestData
{
  swMPSCRingBuffer *ringBuffer;
  swRingBufferTestThreadData *threadData;
  size_t acquireBytesPerThread;
  size_t acquireBytesTotal;
  size_t consumedBytesTotal;
} swRingBufferTestData;

static const size_t acquireBytesTotal = 64 * 1024 * 1024;
static const size_t acquireBytes      = 64;

bool ringBufferConsumeFunction(uint8_t *buffer, size_t size, void *data)
{
  swThreadedTestData *testThreadData = data;
  swRingBufferTestData *testData = swThreadedTestDataGet(testThreadData);
  if (testData)
  {
    testData->consumedBytesTotal += size;
    if (testData->consumedBytesTotal == testData->acquireBytesTotal)
      swEdgeAsyncSend(&(testThreadData->killLoop));
  }
  return true;
}

void swMPSCRingBufferDataSetup(swThreadedTestData *data)
{
  bool success = false;
  swRingBufferTestData *testData = swMemoryCalloc(1, sizeof(*testData));
  if (testData)
  {
    if ((testData->ringBuffer = swMPSCRingBufferNew(&(data->threadManager), 4, ringBufferConsumeFunction, data)))
    {
      testData->acquireBytesPerThread = acquireBytesTotal / data->numThreads;
      testData->acquireBytesTotal = acquireBytesTotal;
      testData->consumedBytesTotal = 0;
      if ((testData->threadData = swMemoryCalloc(data->numThreads, sizeof(swRingBufferTestThreadData))))
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
        swMPSCRingBufferDelete(testData->ringBuffer);
    }
    if (!success)
      swMemoryFree(testData);
  }
  ASSERT_TRUE(success);
}

void swMPSCRingBufferDataTeardown(swThreadedTestData *data)
{
  swRingBufferTestData *testData = swThreadedTestDataGet(data);
  if (testData)
  {
    for (uint32_t i = 0; i < data->numThreads; i++)
    {
      swRingBufferTestThreadData *threadData = &(testData->threadData[i]);
      swTestLogLine("Thread %u: bytes acquired = %zu, success/failure attepts = %lu/%lu\n", i, threadData->bytesAcquired, threadData->acquireSuccess, threadData->acquireFailure);
      swTestLogLine("Thread %u: thread time = %lu ns, total time = %lu ns\n", i, data->threadData[i].executionCPUTime, data->threadData[i].executionTotalTime);
    }
    swTestLogLine("Acquire Total %zu, Consumed Total %zu\n", testData->acquireBytesTotal, testData->consumedBytesTotal);

    if (testData->ringBuffer)
      swMPSCRingBufferDelete(testData->ringBuffer);
    if (testData->threadData)
      swMemoryFree(testData->threadData);
    swMemoryFree(testData);
    swThreadedTestDataSet(data, NULL);
  }
}

void swMPSCRingBufferThreadDataSetup(swThreadedTestData *data, swThreadedTestThreadData *threadData)
{
  swRingBufferTestData *testData = swThreadedTestDataGet(data);
  swThreadedTestThreadDataSet(threadData, &(testData->threadData[threadData->id]));
}

void swMPSCRingBufferThreadDataTeardown(swThreadedTestData *data, swThreadedTestThreadData *threadData)
{
  swThreadedTestThreadDataSet(threadData, NULL);
}

bool swMPSCRingBufferThreadDataRun(swThreadedTestData *data, swThreadedTestThreadData *threadData)
{
  bool rtn = true;
  swRingBufferTestThreadData *localThreadData = swThreadedTestThreadDataGet(threadData);
  // struct timespec sleepInterval = { .tv_sec = 0, .tv_nsec = 1000 };
  uint8_t *buffer = NULL;
  while (!threadData->shutdown && localThreadData->bytesAcquired < localThreadData->acquireBytesTotal)
  {
    if (swMPSCRingBufferProduceAcquire(localThreadData->ringBuffer, &buffer, acquireBytes))
    {
      localThreadData->acquireSuccess++;
      localThreadData->bytesAcquired += acquireBytes;
      if (!swMPSCRingBufferProduceRelease(localThreadData->ringBuffer, buffer, acquireBytes))
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

swThreadedTestDeclare(MPSCRingBuffer, swMPSCRingBufferDataSetup, swMPSCRingBufferDataTeardown,
                   swMPSCRingBufferThreadDataSetup, swMPSCRingBufferThreadDataTeardown, swMPSCRingBufferThreadDataRun,
                   threadCounts);
