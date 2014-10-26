#include "thread/mpsc-futex-ring-buffer.h"

#include "core/memory.h"
#include "thread/futex.h"

#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

static void *swMPSCFutexRingBufferRun(swMPSCFutexRingBuffer *ringBuffer)
{
  if (ringBuffer)
  {
    uint32_t currentTail = 0;
    struct timespec sleepInterval = { .tv_sec = 0, .tv_nsec = 1000000 };
    while(!ringBuffer->shutdown || (ringBuffer->currentTail != ringBuffer->head))
    {
      currentTail = ringBuffer->currentTail;
      if (ringBuffer->head != currentTail)
      {
        size_t size = (ringBuffer->head < currentTail)? (currentTail - ringBuffer->head) : (ringBuffer->size - ringBuffer->head) + currentTail;
        if (size && !ringBuffer->consumeFunc(&(ringBuffer->buffer[currentTail]), size, ringBuffer->data))
          break;
        ringBuffer->head = currentTail;
      }
      else
      {
        if (!swFutexWaitWithTimeout(&(ringBuffer->currentTail), currentTail, &sleepInterval))
          break;
      }
    }
  }
  return NULL;
}


static void swMPSCFutexRingBufferStop(swMPSCFutexRingBuffer *ringBuffer)
{
  ringBuffer->shutdown = true;
}

static void swMPSCFutexRingBufferDone(swMPSCFutexRingBuffer *ringBuffer, void *returnValue)
{
  ringBuffer->done = true;
}

swMPSCFutexRingBuffer *swMPSCFutexRingBufferNew(swThreadManager *threadManager, uint32_t pages, swMPSCFutexRingBufferConsumeFunction consumeFunc, void *data)
{
  swMPSCFutexRingBuffer *ringBuffer = swMemoryCacheAlignMalloc(sizeof(*ringBuffer));
  if (!swMPSCFutexRingBufferInit(ringBuffer, threadManager, pages, consumeFunc, data))
  {
    swMemoryFree(ringBuffer);
    ringBuffer = NULL;
  }
  return ringBuffer;
}

bool swMPSCFutexRingBufferInit(swMPSCFutexRingBuffer *ringBuffer, swThreadManager *threadManager, uint32_t pages, swMPSCFutexRingBufferConsumeFunction consumeFunc, void *data)
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
                ringBuffer->head = 0;
                ringBuffer->candidateTail = 0;
                ringBuffer->currentTail = 0;
                ringBuffer->threadManager = threadManager;
                ringBuffer->consumeFunc = consumeFunc;
                ringBuffer->shutdown = false;
                ringBuffer->done = false;
                ringBuffer->data = data;
                rtn = swThreadManagerStartThread(threadManager, (swThreadRunFunction)swMPSCFutexRingBufferRun, (swThreadStopFunction)swMPSCFutexRingBufferStop, (swThreadDoneFunction)swMPSCFutexRingBufferDone, ringBuffer);
                if (!rtn)
                  munmap(upperData, ringBuffer->size);
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

void swMPSCFutexRingBufferRelease(swMPSCFutexRingBuffer *ringBuffer)
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

void swMPSCFutexRingBufferDelete(swMPSCFutexRingBuffer *ringBuffer)
{
  if (ringBuffer)
  {
    swMPSCFutexRingBufferRelease(ringBuffer);
    swMemoryFree(ringBuffer);
  }
}

bool swMPSCFutexRingBufferProduceAcquire(swMPSCFutexRingBuffer *ringBuffer, uint8_t **buffer, size_t size)
{
  bool rtn = false;
  if (ringBuffer && buffer && size)
  {
    uint32_t candidateTail = 0;
    uint32_t newCandidateTail = 0;
    uint32_t head = 0;
    size_t sizeAvailable = 0;
    swSpinLockLock(&(ringBuffer->tailLock));
    head = ringBuffer->head;
    candidateTail = ringBuffer->candidateTail;
    sizeAvailable = (candidateTail > head)?
        (size_t)((ringBuffer->size - candidateTail) + head - 1) :
        ((candidateTail < head)? (size_t)(head - candidateTail - 1) : (ringBuffer->size - 1));
    if (sizeAvailable >= size)
    {
      newCandidateTail = candidateTail + size;
      newCandidateTail = (newCandidateTail < ringBuffer->size)? newCandidateTail : (newCandidateTail - ringBuffer->size);
      ringBuffer->candidateTail = newCandidateTail;
      *buffer = &(ringBuffer->buffer[candidateTail]);
      rtn = true;
    }
    swSpinLockUnlock(&(ringBuffer->tailLock));
  }
  return rtn;
}

bool swMPSCFutexRingBufferProduceRelease(swMPSCFutexRingBuffer *ringBuffer, uint8_t *buffer, size_t size)
{
  bool rtn = false;
  if (ringBuffer && buffer && size)
  {
    uint32_t expectedCurrentTail = (uint32_t)(buffer - ringBuffer->buffer);
    uint32_t newCurrentTail = expectedCurrentTail + size;
    newCurrentTail = (newCurrentTail < ringBuffer->size)? newCurrentTail : (newCurrentTail - ringBuffer->size);
    do
    {
      uint32_t currentTail = ringBuffer->currentTail;
      if (currentTail != expectedCurrentTail)
      {
        if (!swFutexWait(&(ringBuffer->currentTail), currentTail))
          return false;
      }
      else
        break;
    } while (true);
    while(__sync_val_compare_and_swap(&(ringBuffer->currentTail), expectedCurrentTail, newCurrentTail) != expectedCurrentTail);
    if (swFutexWakeupAll(&(ringBuffer->currentTail)))
      rtn = true;
  }
  return rtn;
}
