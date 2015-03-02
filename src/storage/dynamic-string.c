#include "storage/dynamic-string.h"

#include "core/memory.h"
#include "core/time.h"

#include <stdarg.h>

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

bool swDynamicStringEnsureSize(swDynamicString *string, size_t size)
{
  bool rtn = false;
  if (string && size)
  {
    if (size > string->size)
    {
      void *newData = swMemoryRealloc(string->data, size);
      if (newData)
      {
        string->data = newData;
        string->size = size;
        rtn = true;
      }
    }
    else
      rtn = true;
  }
  return rtn;
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

swDynamicString *swDynamicStringNewFromFormat(const char *format, ...)
{
  swDynamicString *rtn = NULL;
  if (format)
  {
    va_list argList;
    va_list argListCopy;
    va_start(argList, format);
    va_copy(argListCopy, argList);

    int sizeNeeded = vsnprintf(NULL, 0, format, argList);
    if (sizeNeeded > 0)
    {
      swDynamicString *newString = swDynamicStringNew(sizeNeeded + 1);
      if (newString)
      {
        // the last character will be '\0'
        int sizePrinted = vsnprintf(newString->data, sizeNeeded + 1, format, argListCopy);
        if (sizePrinted == sizeNeeded)
        {
          newString->len = sizeNeeded;
          rtn = newString;
        }
        if (!rtn)
          swDynamicStringDelete(newString);
      }
    }
    va_end(argListCopy);
    va_end(argList);
  }
  return rtn;
}

bool swDynamicStringSetFromFormat(swDynamicString *dynamicStr, const char *format, ...)
{
  bool rtn = false;
  if (dynamicStr && format)
  {
    va_list argList;
    va_list argListCopy;
    va_start(argList, format);
    va_copy(argListCopy, argList);

    int sizeNeeded = vsnprintf(NULL, 0, format, argList);
    if (sizeNeeded > 0)
    {
      if (dynamicStr->size <= (size_t)sizeNeeded)
      {
        char *data = dynamicStr->data;
        if ((data = swMemoryRealloc(data, sizeNeeded + 1)))
        {
          dynamicStr->data = data;
          dynamicStr->size = sizeNeeded + 1;
        }
        if (data)
        {
          // the last character will be '\0'
          int sizePrinted = vsnprintf(data, sizeNeeded + 1, format, argListCopy);
          if (sizePrinted == sizeNeeded)
          {
            dynamicStr->len = sizeNeeded;
            rtn = true;
          }
        }
      }
    }
    va_end(argListCopy);
    va_end(argList);
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

bool swDynamicStringAppendTime(swDynamicString *dynamicStr)
{
  bool rtn = false;
  if (dynamicStr && (dynamicStr->size - dynamicStr->len) > SW_TIME_STRING_SIZE)
  {
    struct timespec timeValue = {0};
    if (!clock_gettime(CLOCK_REALTIME, &timeValue))
    {
      uint64_t millisec = swTimeNSecToMSec(timeValue.tv_nsec);
      struct tm brokenDownTime = {0};
      if (gmtime_r(&(timeValue.tv_sec), &brokenDownTime))
      {
        int printedLength = strftime(&(dynamicStr->data[dynamicStr->len]), (dynamicStr->size - dynamicStr->len), "%FT%T", &brokenDownTime);
        if (printedLength == (SW_TIME_STRING_SIZE - 4))
        {
          printedLength += sprintf(&(dynamicStr->data[dynamicStr->len + printedLength]), ".%03lu", millisec);
          if (printedLength == SW_TIME_STRING_SIZE)
          {
            dynamicStr->len += printedLength;
            rtn = true;
          }
        }
      }
    }
  }
  return rtn;
}
