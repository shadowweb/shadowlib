#include "storage/dynamic-buffer.h"

#include "core/memory.h"

swDynamicBuffer *swDynamicBufferNew(size_t size)
{
  swDynamicBuffer *rtn = swMemoryCalloc(1, sizeof(swDynamicBuffer));
  if (rtn)
  {
    if (!size || (rtn->data = swMemoryMalloc(size)))
      rtn->size = size;
    else
    {
      swMemoryFree(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

void swDynamicBufferDelete(swDynamicBuffer *buffer)
{
  if (buffer)
  {
    if (buffer->data)
      swMemoryFree(buffer->data);
    swMemoryFree(buffer);
  }
}

swDynamicBuffer *swDynamicBufferNewFromStaticBuffer(const swStaticBuffer *staticBuf)
{
  swDynamicBuffer *rtn = NULL;
  if (staticBuf)
  {
    size_t len = staticBuf->len;
    if ((rtn = swDynamicBufferNew(len)))
    {
      memcpy(rtn->data, staticBuf->data, len);
      rtn->len = len;
    }
  }
  return rtn;
}

bool swDynamicBufferSetFromStaticBuffer(swDynamicBuffer *dynamicBuf, const swStaticBuffer *staticBuf)
{
  bool rtn = false;
  if (dynamicBuf && staticBuf)
  {
    char *data = dynamicBuf->data;
    if (dynamicBuf->size < staticBuf->len)
    {
      if ((data = swMemoryRealloc(dynamicBuf->data, staticBuf->len)))
      {
        dynamicBuf->data = data;
        dynamicBuf->size = staticBuf->len;
      }
    }
    if (data)
    {
      memcpy(data, staticBuf->data, staticBuf->len);
      dynamicBuf->len = staticBuf->len;
      rtn = true;
    }
  }
  return rtn;
}

swDynamicBuffer *swDynamicBufferNewFromCBuffer(const char *cBuf, size_t size)
{
  swDynamicBuffer *rtn = NULL;
  if (cBuf && size)
  {
    if ((rtn = swDynamicBufferNew(size)))
    {
      memcpy(rtn->data, cBuf, size);
      rtn->len = size;
    }
  }
  return rtn;
}

bool swDynamicBufferSetFromCString(swDynamicBuffer *dynamicBuf, const char *cBuf, size_t size)
{
  bool rtn = false;
  if (dynamicBuf && cBuf && size)
  {
    char *data = dynamicBuf->data;
    if (dynamicBuf->size < size)
    {
      if ((data = swMemoryRealloc(dynamicBuf->data, size)))
      {
        dynamicBuf->data = data;
        dynamicBuf->size = size;
      }
    }
    if (data)
    {
      memcpy(data, cBuf, size);
      dynamicBuf->len = size;
      rtn = true;
    }
  }
  return rtn;
}

void swDynamicBufferClear(swDynamicBuffer *buffer)
{
  if (buffer)
    buffer->len = 0;
}

void swDynamicBufferRelease(swDynamicBuffer *buffer)
{
  if (buffer)
  {
    if (buffer->data)
      swMemoryFree(buffer->data);
    memset(buffer, 0, sizeof(swDynamicBuffer));
  }
}

bool swDynamicBufferAppendStaticString(swDynamicBuffer *dynamicBuf, const swStaticBuffer *staticBuf)
{
  bool rtn = false;
  if (dynamicBuf && staticBuf)
  {
    size_t newLen = dynamicBuf->len + staticBuf->len;
    char *data = dynamicBuf->data;
    if (newLen > dynamicBuf->size)
    {
      if ((data = swMemoryRealloc(data, newLen)))
      {
        dynamicBuf->data = data;
        dynamicBuf->size = newLen + 1;
      }
      if (data)
      {
        memcpy(&(data[dynamicBuf->len]), staticBuf->data, staticBuf->len);
        dynamicBuf->len = newLen;
        rtn = true;
      }
    }
  }
  return rtn;

}

bool swDynamicBufferAppendCBuffer(swDynamicBuffer *dynamicBuf, const char *cBuf, size_t size)
{
  bool rtn = false;
  if (dynamicBuf && cBuf && size)
  {
    size_t newLen = dynamicBuf->len + size;
    char *data = dynamicBuf->data;
    if (newLen > dynamicBuf->size)
    {
      if ((data = swMemoryRealloc(data, newLen)))
      {
        dynamicBuf->data = data;
        dynamicBuf->size = newLen;
      }
      if (data)
      {
        memcpy(&(data[dynamicBuf->len]), cBuf, size);
        dynamicBuf->len = newLen;
        rtn = true;
      }
    }
  }
  return rtn;

}

