#include "core/memory.h"
#include "core/time.h"
#include "log/log-manager.h"
#include "thread/threaded-test.h"

swLoggerDeclareWithLevel(asyncLogger, "AsyncLogger", swLogLevelInfo);

typedef struct swAsyncWriterTestThreadData
{
  uint64_t  logMessagesTotal;
  uint64_t  logMessagesLogged;
  uint64_t  logMessagesFailed;
} swAsyncWriterTestThreadData;

typedef struct swAsyncWriterTestData
{
  swLogManager *logManager;
  swAsyncWriterTestThreadData *threadData;

  uint32_t logMessagesPerThread;
  uint32_t logMessagesTotal;
} swAsyncWriterTestData;

static const size_t logMessagesTotal = 4 * 1024 * 1024;

void swAsyncWriterDataSetup(swThreadedTestData *data)
{
  bool success = false;
  swAsyncWriterTestData *testData = swMemoryCalloc(1, sizeof(*testData));
  if (testData)
  {
    if ((testData->logManager = swLogManagerNew(swLogLevelInfo)))
    {
      swLogSink sink = {NULL};
      swLogFormatter formatter = {NULL};
      swStaticString fileName = swStaticStringDefine("/tmp/AsyncWriterTest");
      if (swLogBufferFormatterInit(&formatter) && swLogFileSinkInit(&sink, &(data->threadManager), 32*1024*1024, 2, &fileName))
      {
        swLogWriter writer;
        if (swLogWriterInit(&writer, sink, formatter) && swLogManagerWriterAdd(testData->logManager, writer))
        {
          testData->logMessagesPerThread = logMessagesTotal / data->numThreads;
          testData->logMessagesTotal = logMessagesTotal;
          data->expectedEventCount = data->numThreads;
          if ((testData->threadData = swMemoryCalloc(data->numThreads, sizeof(swAsyncWriterTestThreadData))))
          {
            for(uint32_t i = 0; i < data->numThreads; i++)
              testData->threadData[i].logMessagesTotal = testData->logMessagesPerThread;
            success = true;
            swThreadedTestDataSet(data, testData);
          }
        }
      }
      if (!success)
        swLogManagerDelete(testData->logManager);
    }
    if (!success)
      swMemoryFree(testData);
  }
  ASSERT_TRUE(success);
}

void swAsyncWriterDataTeardown(swThreadedTestData *data)
{
  swAsyncWriterTestData *testData = swThreadedTestDataGet(data);
  if (testData)
  {
    for (uint32_t i = 0; i < data->numThreads; i++)
    {
      swAsyncWriterTestThreadData *threadData = &(testData->threadData[i]);
      swTestLogLine("Thread %u: messages logged = %lu, failed = %lu\n", i, threadData->logMessagesLogged, threadData->logMessagesFailed);
      swTestLogLine("Thread %u: thread time = %lu ns, total time = %lu ns\n", i, data->threadData[i].executionCPUTime, data->threadData[i].executionTotalTime);
    }
    swTestLogLine("Messages Logged Total %zu\n", testData->logMessagesTotal);

    if (testData->logManager)
      swLogManagerDelete(testData->logManager);
    if (testData->threadData)
      swMemoryFree(testData->threadData);
    swMemoryFree(testData);
    swThreadedTestDataSet(data, NULL);
  }
}

void swAsyncWriterThreadDataSetup(swThreadedTestData *data, swThreadedTestThreadData *threadData)
{
  swAsyncWriterTestData *testData = swThreadedTestDataGet(data);
  swThreadedTestThreadDataSet(threadData, &(testData->threadData[threadData->id]));
}

void swAsyncWriterThreadDataTeardown(swThreadedTestData *data, swThreadedTestThreadData *threadData)
{
  swThreadedTestThreadDataSet(threadData, NULL);
}

bool swAsyncWriterThreadDataRun(swThreadedTestData *data, swThreadedTestThreadData *threadData)
{
  bool rtn = true;
  swAsyncWriterTestThreadData *localThreadData = swThreadedTestThreadDataGet(threadData);
  while (!threadData->shutdown && localThreadData->logMessagesLogged < localThreadData->logMessagesTotal)
  {
    if (swLoggerLog(&asyncLogger, swLogLevelInfo, __FILE__, __FUNCTION__, __LINE__, "Thread %u logs message %lu\n", threadData->id, localThreadData->logMessagesLogged))
      localThreadData->logMessagesLogged++;
    else
      localThreadData->logMessagesFailed++;
  }
  if (localThreadData->logMessagesLogged == localThreadData->logMessagesTotal)
    swEdgeAsyncSend(&(data->killLoop));
  return rtn;
}

static uint32_t threadCounts[] = {1, 2, 4, 8, 16};

swThreadedTestDeclare(AsyncWriter, swAsyncWriterDataSetup, swAsyncWriterDataTeardown,
                   swAsyncWriterThreadDataSetup, swAsyncWriterThreadDataTeardown, swAsyncWriterThreadDataRun,
                   threadCounts);
