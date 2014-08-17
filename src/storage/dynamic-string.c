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
    if (dynamicStr->size < (staticStr->len + 1))
    {
      if ((dynamicStr->data = swMemoryRealloc(dynamicStr->data, staticStr->len + 1)))
        dynamicStr->size = staticStr->len + 1;
    }
    if (dynamicStr->data)
    {
      memcpy(dynamicStr->data, staticStr->data, staticStr->len);
      dynamicStr->len = staticStr->len;
      dynamicStr->data[staticStr->len] = '\0';
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
    if (dynamicStr->size < (len + 1))
    {
      if ((dynamicStr->data = swMemoryRealloc(dynamicStr->data, len + 1)))
        dynamicStr->size = len + 1;
    }
    if (dynamicStr->data)
    {
      memcpy(dynamicStr->data, cStr, len);
      dynamicStr->len = len;
      dynamicStr->data[len] = '\0';
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

