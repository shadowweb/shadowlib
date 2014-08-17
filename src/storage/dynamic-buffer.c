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
    if (dynamicBuf->size < staticBuf->len)
    {
      if ((dynamicBuf->data = swMemoryRealloc(dynamicBuf->data, staticBuf->len)))
        dynamicBuf->size = staticBuf->len;
    }
    if (dynamicBuf->data)
    {
      memcpy(dynamicBuf->data, staticBuf->data, staticBuf->len);
      dynamicBuf->len = staticBuf->len;
      dynamicBuf->data[staticBuf->len] = '\0';
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
    if (dynamicBuf->size < size)
    {
      if ((dynamicBuf->data = swMemoryRealloc(dynamicBuf->data, size)))
        dynamicBuf->size = size;
    }
    if (dynamicBuf->data)
    {
      memcpy(dynamicBuf->data, cBuf, size);
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
