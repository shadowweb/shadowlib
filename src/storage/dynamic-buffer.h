#ifndef SW_STORAGE_DYNAMICBUFFER_H
#define SW_STORAGE_DYNAMICBUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "storage/static-buffer.h"

// TODO: create swFixedBuffer that looks the same, but allocates the whole data structure with the
//        buffer in a single malloc

typedef struct swDynamicBuffer
{
  size_t   len;
  uint8_t *data;
  size_t   size;
} swDynamicBuffer;

static inline uint32_t swDynamicBufferHash(const swDynamicBuffer *buffer)
{
  return swStaticBufferHash((swStaticBuffer *)(buffer));
}

static inline int swDynamicBufferCompare(const swDynamicBuffer *b1, const swDynamicBuffer *b2)
{
  return swStaticBufferCompare((swStaticBuffer *)(b1), (swStaticBuffer *)(b2));
}

static inline bool swDynamicBufferEqual(const swDynamicBuffer *b1, const swDynamicBuffer *b2)
{
  return swStaticBufferEqual((swStaticBuffer *)(b1), (swStaticBuffer *)(b2));
}

static inline bool swDynamicBufferSame(const swDynamicBuffer *b1, const swDynamicBuffer *b2)
{
  return swStaticBufferSame((swStaticBuffer *)(b1), (swStaticBuffer *)(b2));
}

swDynamicBuffer *swDynamicBufferNew(size_t size);
bool swDynamicBufferInit(swDynamicBuffer *buffer, size_t size);
void swDynamicBufferDelete(swDynamicBuffer *buffer);

swDynamicBuffer *swDynamicBufferNewFromStaticBuffer(const swStaticBuffer *staticBuf);
bool swDynamicBufferSetFromStaticBuffer(swDynamicBuffer *dynamicBuf, const swStaticBuffer *staticBuf);
swDynamicBuffer *swDynamicBufferNewFromCBuffer(const uint8_t *cBuf, size_t size);
bool swDynamicBufferSetFromCBuffer(swDynamicBuffer *dynamicBuf, const uint8_t *cBuf, size_t size);

void swDynamicBufferClear(swDynamicBuffer *buffer);
void swDynamicBufferRelease(swDynamicBuffer *buffer);

bool swDynamicBufferAppendStaticString(swDynamicBuffer *dynamicBuf, const swStaticBuffer *staticBuf);
bool swDynamicBufferAppendCBuffer(swDynamicBuffer *dynamicBuf, const uint8_t *cBuf, size_t size);

bool swDynamicBufferEnsureCapacity(swDynamicBuffer *dynamicBuf, size_t size);


// TODO: implement to and from hex, to and from base64

#endif // SW_STORAGE_DYNAMICBUFFER_H
