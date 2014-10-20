#ifndef SW_THREAD_MPSCRINGBUFFER_H
#define SW_THREAD_MPSCRINGBUFFER_H

#include "thread/thread-manager.h"

#include <stdbool.h>
#include <stdint.h>

// based on ring buffer implemntation published here
// http://en.wikipedia.org/wiki/Circular_buffer#Optimized_POSIX_implementation
// some idiot decided that it violates WP:OR rules of the wikipedia and
// removed the source code sample while the whole world out there references this article
// for the sake of this code sample; anyway, the code sample can be found
// by digging throug the article change history
// also found vrb library that implements the same with extra safeguards
// can be found here: http://vrb.slashusr.org/
// it also shows how the same can be done using shared memory

// MPSC stands for Multiple Producer Single Consumer
struct swMPSCRingBuffer;

typedef bool (*swMPSCRingBufferConsumeFunction)(uint8_t *buffer, size_t size, void *data);

typedef struct swMPSCRingBuffer
{
  swThreadManager *threadManager;
  swMPSCRingBufferConsumeFunction consumeFunc;
  void *data;
  size_t size;
  uint8_t *buffer;
  uint8_t *bufferEnd;
  uint8_t *head;
  uint8_t *candidateTail;
  uint8_t *currentTail;
  bool shutdown;
  bool done;
} swMPSCRingBuffer;

swMPSCRingBuffer *swMPSCRingBufferNew(swThreadManager *threadManager, uint32_t pages, swMPSCRingBufferConsumeFunction consumeFunc, void *data);
bool swMPSCRingBufferInit(swMPSCRingBuffer *ringBuffer, swThreadManager *threadManager, uint32_t pages, swMPSCRingBufferConsumeFunction consumeFunc, void *data);
void swMPSCRingBufferRelease(swMPSCRingBuffer *ringBuffer);
void swMPSCRingBufferDelete(swMPSCRingBuffer *ringBuffer);
bool swMPSCRingBufferProduceAcquire(swMPSCRingBuffer *ringBuffer, uint8_t **buffer, size_t size);
bool swMPSCRingBufferProduceRelease(swMPSCRingBuffer *ringBuffer, uint8_t *buffer, size_t size);

#endif  // SW_THREAD_MPSCRINGBUFFER_H
