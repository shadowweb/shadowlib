#ifndef SW_THREAD_MPSCFUTEXRINGBUFFER_H
#define SW_THREAD_MPSCFUTEXRINGBUFFER_H

#include "thread/spin-lock.h"
#include "thread/thread-manager.h"

#include <stdbool.h>
#include <stdint.h>

// this version is implemented with futex, it is slower, but runs well in valgrind

// MPSC stands for Multiple Producer Single Consumer
struct swMPSCFutexRingBuffer;

typedef bool (*swMPSCFutexRingBufferConsumeFunction)(uint8_t *buffer, size_t size, void *data);

typedef struct swMPSCFutexRingBuffer
{
  swThreadManager *threadManager;
  swMPSCFutexRingBufferConsumeFunction consumeFunc;
  uint8_t *buffer;
  void *data;
  size_t size;
  uint32_t head;
  bool shutdown;
  bool done;
  uint64_t _fill1;
  uint64_t _fill2;
  uint32_t candidateTail;
  uint32_t currentTail;
  swSpinLock tailLock;
} swMPSCFutexRingBuffer;

swMPSCFutexRingBuffer *swMPSCFutexRingBufferNew(swThreadManager *threadManager, uint32_t pages, swMPSCFutexRingBufferConsumeFunction consumeFunc, void *data);
bool swMPSCFutexRingBufferInit(swMPSCFutexRingBuffer *ringBuffer, swThreadManager *threadManager, uint32_t pages, swMPSCFutexRingBufferConsumeFunction consumeFunc, void *data);
void swMPSCFutexRingBufferRelease(swMPSCFutexRingBuffer *ringBuffer);
void swMPSCFutexRingBufferDelete(swMPSCFutexRingBuffer *ringBuffer);
bool swMPSCFutexRingBufferProduceAcquire(swMPSCFutexRingBuffer *ringBuffer, uint8_t **buffer, size_t size);
bool swMPSCFutexRingBufferProduceRelease(swMPSCFutexRingBuffer *ringBuffer, uint8_t *buffer, size_t size);

#endif  // SW_THREAD_MPSCFUTEXRINGBUFFER_H
