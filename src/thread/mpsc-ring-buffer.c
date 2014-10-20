#include "core/memory.h"
#include "thread/mpsc-ring-buffer.h"

#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

// for futex: this won't work anyway as futex expects pointer to an int, not to a pointer to pointer
// #include <linux/futex.h>
// #include <sys/time.h>

static void *swMPSCRingBufferRun(swMPSCRingBuffer *ringBuffer)
{
  if (ringBuffer)
  {
    uint8_t *currentTail = NULL;
    struct timespec sleepInterval = { .tv_sec = 0, .tv_nsec = 5 };
    while(!ringBuffer->shutdown || (ringBuffer->currentTail != ringBuffer->head))
    {
      currentTail = ringBuffer->currentTail;
      if (ringBuffer->head != currentTail)
      {
        size_t size = (ringBuffer->head < currentTail)? (currentTail - ringBuffer->head) : (ringBuffer->bufferEnd - ringBuffer->head) + (currentTail - ringBuffer->buffer);
        if (size && !ringBuffer->consumeFunc(currentTail, size, ringBuffer->data))
          break;
        ringBuffer->head = currentTail;
      }
      else
      {
        nanosleep(&sleepInterval, NULL);
        // futex(&(ringBuffer->currentTail), FUTEX_WAIT, currentTail, &sleepInterval, NULL, 0);
      }
    }
  }
  return NULL;
}

static void swMPSCRingBufferStop(swMPSCRingBuffer *ringBuffer)
{
  ringBuffer->shutdown = true;
}

static void swMPSCRingBufferDone(swMPSCRingBuffer *ringBuffer, void *returnValue)
{
  ringBuffer->done = true;
}

swMPSCRingBuffer *swMPSCRingBufferNew(swThreadManager *threadManager, uint32_t pages, swMPSCRingBufferConsumeFunction consumeFunc, void *data)
{
  swMPSCRingBuffer *ringBuffer = swMemoryMalloc(sizeof(*ringBuffer));
  if (!swMPSCRingBufferInit(ringBuffer, threadManager, pages, consumeFunc, data))
  {
    swMemoryFree(ringBuffer);
    ringBuffer = NULL;
  }
  return ringBuffer;
}

bool swMPSCRingBufferInit(swMPSCRingBuffer *ringBuffer, swThreadManager *threadManager, uint32_t pages, swMPSCRingBufferConsumeFunction consumeFunc, void *data)
{
  bool rtn = false;
  if (ringBuffer && threadManager && pages && consumeFunc)
  {
    memset(ringBuffer, 0, sizeof(*ringBuffer));
    ringBuffer->size = getpagesize() * pages;
    if((ringBuffer->buffer = (uint8_t *)mmap(NULL, ringBuffer->size << 1, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)))
    {
      char tempFile[] = "/dev/shm/mpscbuffer-XXXXXX";
      int fd = mkstemp(tempFile);
      if (fd >= 0)
      {
        if (unlink(tempFile) == 0)
        {
          if (ftruncate(fd, ringBuffer->size) == 0)
          {
            uint8_t *lowerData = ringBuffer->buffer;
            if (mmap(lowerData, ringBuffer->size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0) == lowerData)
            {
              uint8_t *upperData = ringBuffer->buffer + ringBuffer->size;
              if (mmap(upperData, ringBuffer->size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0) == upperData)
              {
                ringBuffer->bufferEnd = ringBuffer->buffer + ringBuffer->size;
                ringBuffer->threadManager = threadManager;
                ringBuffer->consumeFunc = consumeFunc;
                ringBuffer->head = ringBuffer->buffer;
                ringBuffer->candidateTail = ringBuffer->buffer;
                ringBuffer->currentTail = ringBuffer->buffer;
                ringBuffer->shutdown = false;
                ringBuffer->done = false;
                ringBuffer->data = data;
                rtn = swThreadManagerStartThread(threadManager, (swThreadRunFunction)swMPSCRingBufferRun, (swThreadStopFunction)swMPSCRingBufferStop, (swThreadDoneFunction)swMPSCRingBufferDone, ringBuffer);
              }
              if (!rtn)
                munmap(lowerData, ringBuffer->size);
            }
          }
        }
        close(fd);
      }
      if (!rtn)
        munmap(ringBuffer->buffer, ringBuffer->size << 1);
    }
  }
  return rtn;
}

void swMPSCRingBufferRelease(swMPSCRingBuffer *ringBuffer)
{
  if (ringBuffer)
  {
    while (!(ringBuffer->done))
    {
      ringBuffer->shutdown = true;
      swEdgeLoopRun(ringBuffer->threadManager->loop, true);
    }
    uint8_t *upperData = ringBuffer->buffer + ringBuffer->size;
    munmap(upperData, ringBuffer->size);
    munmap(ringBuffer->buffer, ringBuffer->size);
    munmap(ringBuffer->buffer, ringBuffer->size << 1);
  }
}

void swMPSCRingBufferDelete(swMPSCRingBuffer *ringBuffer)
{
  if (ringBuffer)
  {
    swMPSCRingBufferRelease(ringBuffer);
    swMemoryFree(ringBuffer);
  }
}

bool swMPSCRingBufferProduceAcquire(swMPSCRingBuffer *ringBuffer, uint8_t **buffer, size_t size)
{
  bool rtn = false;
  if (ringBuffer && buffer && size)
  {
    uint8_t *candidateData = NULL;
    uint8_t *candidateTail = NULL;
    uint8_t *head = NULL;
    size_t sizeAvailable = 0;
    do
    {
      head = ringBuffer->head;
      candidateData = ringBuffer->candidateTail;
      sizeAvailable = (candidateData > head)?
          (size_t)((ringBuffer->bufferEnd - candidateData) + (head - ringBuffer->buffer) - 1) :
          ((candidateData < head)? (size_t)(head - candidateData - 1) : ringBuffer->size);
      if (sizeAvailable < size)
        break;
      candidateTail = candidateData + size;
      candidateTail = (candidateTail <= ringBuffer->bufferEnd)? candidateTail : (uint8_t *)(candidateTail - ringBuffer->size);
      if (__sync_bool_compare_and_swap(&(ringBuffer->candidateTail), candidateData, candidateTail))
      {
        *buffer = candidateData;
        rtn = true;
        break;
      }
    } while (true);
  }
  return rtn;
}

bool swMPSCRingBufferProduceRelease(swMPSCRingBuffer *ringBuffer, uint8_t *buffer, size_t size)
{
  bool rtn = false;
  if (ringBuffer && buffer && size)
  {
    uint8_t *currentTail = buffer + size;
    currentTail = (currentTail <= ringBuffer->bufferEnd)? currentTail : (currentTail - ringBuffer->size);
    while (__sync_val_compare_and_swap(&(ringBuffer->currentTail), buffer, currentTail) != buffer);
    // futex(&(ringBuffer->currentTail), FUTEX_WAKE, 1, NULL, NULL, 0);
    rtn = true;
  }
  return rtn;
}
