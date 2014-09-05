#include "storage/dynamic-string.h"

#include "core/memory.h"

swDynamicString *swDynamicStringNew(size_t size)
{
  swDynamicString *rtn = swMemoryCalloc(1, sizeof(swDynamicString));
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

void swDynamicStringDelete(swDynamicString *string)
{
  if (string)
  {
    if (string->data)
      swMemoryFree(string->data);
    swMemoryFree(string);
  }
}

swDynamicString *swDynamicStringNewFromStaticString(const swStaticString *staticStr)
{
  swDynamicString *rtn = NULL;
  if (staticStr)
  {
    size_t len = staticStr->len;
    if ((rtn = swDynamicStringNew(len + 1)))
    {
      memcpy(rtn->data, staticStr->data, len);
      rtn->len = len;
      rtn->data[len] = '\0';
    }
  }
  return rtn;
}

bool swDynamicStringSetFromStaticString(swDynamicString *dynamicStr, const swStaticString *staticStr)
{
  bool rtn = false;
  if (dynamicStr && staticStr)
  {
    char *data = dynamicStr->data;
    if (dynamicStr->size < (staticStr->len + 1))
    {
      if ((data = swMemoryRealloc(dynamicStr->data, staticStr->len + 1)))
      {
        dynamicStr->data = data;
        dynamicStr->size = staticStr->len + 1;
      }
    }
    if (data)
    {
      memcpy(data, staticStr->data, staticStr->len);
      dynamicStr->len = staticStr->len;
      data[staticStr->len] = '\0';
      rtn = true;
    }
  }
  return rtn;
}

swDynamicString *swDynamicStringNewFromCString(const char *cStr)
{
  swDynamicString *rtn = NULL;
  if (cStr)
  {
    size_t len = strlen(cStr);
    if ((rtn = swDynamicStringNew(len + 1)))
    {
      memcpy(rtn->data, cStr, len);
      rtn->len = len;
      rtn->data[len] = '\0';
    }
  }
  return rtn;
}

bool swDynamicStringSetFromCString(swDynamicString *dynamicStr, const char *cStr)
{
  bool rtn = false;
  if (dynamicStr && cStr)
  {
    size_t len = strlen(cStr);
    char *data = dynamicStr->data;
    if (dynamicStr->size < (len + 1))
    {
      if ((data = swMemoryRealloc(dynamicStr->data, len + 1)))
      {
        dynamicStr->data = data;
        dynamicStr->size = len + 1;
      }
    }
    if (data)
    {
      memcpy(data, cStr, len);
      dynamicStr->len = len;
      data[len] = '\0';
      rtn = true;
    }
  }
  return rtn;
}

void swDynamicStringClear(swDynamicString *string)
{
  if (string)
    string->len = 0;
}

void swDynamicStringRelease(swDynamicString *string)
{
  if (string)
  {
    if (string->data)
      swMemoryFree(string->data);
    memset(string, 0, sizeof(swDynamicString));
  }
}

bool swDynamicStringAppendStaticString(swDynamicString *dynamicStr, const swStaticString *staticStr)
{
  bool rtn = false;
  if (dynamicStr && staticStr)
  {
    size_t newLen = dynamicStr->len + staticStr->len;
    char *data = dynamicStr->data;
    if (newLen >= dynamicStr->size)
    {
      if ((data = swMemoryRealloc(data, newLen + 1)))
      {
        dynamicStr->data = data;
        dynamicStr->size = newLen + 1;
      }
    }
    if (data)
    {
      memcpy(&(data[dynamicStr->len]), staticStr->data, staticStr->len);
      dynamicStr->len = newLen;
      data[newLen] = '\0';
      rtn = true;
    }
  }
  return rtn;
}

bool swDynamicStringAppendCString(swDynamicString *dynamicStr, const char *cStr)
{
  bool rtn = false;
  if (dynamicStr && cStr)
  {
    size_t len = strlen(cStr);
    size_t newLen = dynamicStr->len + len;
    char *data = dynamicStr->data;
    if (newLen >= dynamicStr->size)
    {
      if ((data = swMemoryRealloc(data, newLen + 1)))
      {
        dynamicStr->data = data;
        dynamicStr->size = newLen + 1;
      }
    }
    if (data)
    {
      memcpy(&(data[dynamicStr->len]), cStr, len);
      dynamicStr->len = newLen;
      data[newLen] = '\0';
      rtn = true;
    }
  }
  return rtn;
}
