#include "log/log-manager.h"

#include "core/memory.h"
#include "storage/dynamic-string.h"
#include "thread/mpsc-ring-buffer.h"
#include "thread/mpsc-futex-ring-buffer.h"

#include <unistd.h>

typedef struct swLogFileSinkData
{
  swMPSCRingBuffer ringBuffer;
  // swMPSCFutexRingBuffer ringBuffer;
  swDynamicString *baseFileName;
  FILE     *stream;
  size_t    maxFileSize;
  size_t    currentFileSize;
  uint32_t  maxFileCount;
  uint32_t  currentFileCount;
} swLogFileSinkData;

static bool swLogFileSinkAcquire(swLogSink *sink, size_t sizeNeeded, uint8_t **buffer)
{
  bool rtn = false;
  if (sink && sizeNeeded && buffer)
  {
    swLogFileSinkData *sinkData = (swLogFileSinkData *)swLogSinkDataGet(sink);
    if (swMPSCRingBufferProduceAcquire(&(sinkData->ringBuffer), buffer, sizeNeeded))
    // if (swMPSCFutexRingBufferProduceAcquire(&(sinkData->ringBuffer), buffer, sizeNeeded))
      rtn = true;
  }
  return rtn;
}

static bool swLogFileSinkRelease(swLogSink *sink, size_t sizeNeeded, uint8_t  *buffer)
{
  bool rtn = false;
  if (sink && sizeNeeded && buffer)
  {
    swLogFileSinkData *sinkData = (swLogFileSinkData *)swLogSinkDataGet(sink);
    if (swMPSCRingBufferProduceRelease(&(sinkData->ringBuffer), buffer, sizeNeeded))
    // if (swMPSCFutexRingBufferProduceRelease(&(sinkData->ringBuffer), buffer, sizeNeeded))
      rtn = true;
  }
  return rtn;
}

static void swLogFileSinkClear(swLogSink *sink)
{
  if (sink)
  {
    swLogFileSinkData *sinkData = (swLogFileSinkData *)swLogSinkDataGet(sink);
    swLogSinkDataSet(sink, NULL);
    swMPSCRingBufferRelease(&(sinkData->ringBuffer));
    // swMPSCFutexRingBufferRelease(&(sinkData->ringBuffer));
    if (sinkData->stream)
    {
      fclose(sinkData->stream);
      char tmpFileName[sinkData->baseFileName->len + 20 + 1];
      snprintf(tmpFileName, sizeof(tmpFileName), "%.*s.%u", (int)(sinkData->baseFileName->len), sinkData->baseFileName->data, sinkData->currentFileCount);
      rename(sinkData->baseFileName->data, tmpFileName);
    }
    swDynamicStringDelete(sinkData->baseFileName);
    swMemoryFree(sinkData);
  }
}

static bool swLogFileSinkConsume(uint8_t *buffer, size_t size, void *data)
{
  bool rtn = false;
  swLogFileSinkData *sinkData = (swLogFileSinkData *)data;
  if (sinkData && buffer && size)
  {
    if (!(sinkData->stream))
    {
      if ((sinkData->stream = fopen(sinkData->baseFileName->data, "w+")))
      {
        setlinebuf(sinkData->stream);
        sinkData->currentFileSize = 0;
      }
    }
    if (sinkData->stream)
    {
      fwrite((void *)buffer, size, 1, sinkData->stream);
      sinkData->currentFileSize += size;
      if (sinkData->currentFileSize >= sinkData->maxFileSize)
      {
        fclose(sinkData->stream);
        sinkData->stream = NULL;
        char tmpFileName[sinkData->baseFileName->len + 20 + 1];
        snprintf(tmpFileName, sizeof(tmpFileName), "%.*s.%u", (int)(sinkData->baseFileName->len), sinkData->baseFileName->data, sinkData->currentFileCount);

        if (!rename(sinkData->baseFileName->data, tmpFileName))
        {
          sinkData->currentFileCount++;
          if (sinkData->maxFileCount && (sinkData->currentFileCount > sinkData->maxFileCount))
          {
            snprintf(tmpFileName, sizeof(tmpFileName), "%.*s.%u", (int)(sinkData->baseFileName->len), sinkData->baseFileName->data, sinkData->currentFileCount - sinkData->maxFileCount - 1);
            unlink(tmpFileName);
          }
          rtn = true;
        }
      }
      else
        rtn = true;
    }
  }
  return rtn;
}

// if maxFileCount == 0, no files will be cleaned up
bool swLogFileSinkInit(swLogSink *sink, swThreadManager *threadManager, size_t maxFileSize, uint32_t maxFileCount, swStaticString *baseFileName)
{
  bool rtn = false;
  if (sink && threadManager && maxFileSize && baseFileName)
  {
    swLogFileSinkData *sinkData = swMemoryCalloc(1, sizeof(*sinkData));
    if (sinkData)
    {
      if ((sinkData->baseFileName = swDynamicStringNewFromFormat("%.*s.%d.log", (int)(baseFileName->len), (baseFileName->data), getpid())))
      {
        if (swMPSCRingBufferInit(&(sinkData->ringBuffer), threadManager, 1024, swLogFileSinkConsume, sinkData))
        // if (swMPSCFutexRingBufferInit(&(sinkData->ringBuffer), threadManager, 1024, swLogFileSinkConsume, sinkData))
        {
          memset(sink, 0, sizeof(*sink));
          sink->acquireFunc = swLogFileSinkAcquire;
          sink->releaseFunc = swLogFileSinkRelease;
          sink->clearFunc   = swLogFileSinkClear;
          swLogSinkDataSet(sink, sinkData);
          sinkData->maxFileSize = maxFileSize;
          sinkData->maxFileCount = maxFileCount;
          rtn = true;
        }
        if (!rtn)
          swDynamicStringDelete(sinkData->baseFileName);
      }
      if (!rtn)
        swMemoryFree(sinkData);
    }
  }
  return rtn;
}
