#include "static-string.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>

#include <collections/hash-functions.h>

uint32_t swStaticStringHash(const swStaticString *string)
{
  if (string)
    return swMurmurHash3_32(string->data, string->len);
  return 0;
}

int swStaticStringCompare(const swStaticString *s1, const swStaticString *s2)
{
  int ret = 0;
  if (s1 == s2)
    ret = 0;
  else if (s1 && s2)
  {
    if (s1->len == s2->len)
      ret = memcmp(s1->data, s2->data, s1->len);
    else
    {
      if (s1->len < s2->len)
      {
        ret = memcmp(s1->data, s2->data, s1->len);
        if (ret == 0)
          ret = -1;
      }
      else
      {
        ret = memcmp(s1->data, s2->data, s2->len);
        if (ret == 0)
          ret = 1;
      }
    }
  }
  else
    ret = (s1)? 1 : -1;
  return ret;
}

int swStaticStringCompareCaseless(const swStaticString *s1, const swStaticString *s2)
{
  int ret = 0;
  if (s1 == s2)
    ret = 0;
  else if (s1 && s2)
  {
    if (s1->len == s2->len)
      ret = strncasecmp(s1->data, s2->data, s1->len);
    else
    {
      if (s1->len < s2->len)
      {
        ret = strncasecmp(s1->data, s2->data, s1->len);
        if (ret == 0)
          ret = -1;
      }
      else
      {
        ret = strncasecmp(s1->data, s2->data, s2->len);
        if (ret == 0)
          ret = 1;
      }
    }
  }
  else
    ret = (s1)? 1 : -1;
  return ret;

}

bool swStaticStringEqual(const swStaticString *s1, const swStaticString *s2)
{
  bool ret = false;
  if (s1 == s2)
    ret = true;
  else if (s1 && s2)
  {
    if (s1->len == s2->len)
      ret = (memcmp(s1->data, s2->data, s1->len) == 0);
  }
  return ret;
}

bool swStaticStringSame(const swStaticString *s1, const swStaticString *s2)
{
  bool ret = false;
  if ((s1 == s2) || (s1 && s2 && s1->data == s2->data && s1->len == s2->len))
    ret = true;
  return ret;
}

bool swStaticStringFindChar(const swStaticString *string, char c, size_t *position)
{
  bool rtn = false;
  if (string && c && position)
  {
    char *charPosition = memchr(string->data, c, string->len);
    if(charPosition)
    {
      *position = charPosition - string->data;
      rtn = true;
    }
  }
  return rtn;
}

bool swStaticStringSetSubstring(const swStaticString *string, swStaticString *subString, size_t start, size_t end)
{
  bool rtn = false;
  if (string && subString && (start < string->len) && (end <= string->len) && (start <= end))
  {
    if ((subString->len = end - start))
    {
      subString->data = &string->data[start];
      rtn = true;
    }
  }
  return rtn;
}

bool swStaticStringSplitChar(const swStaticString *string, char c, swStaticString *slices, uint32_t slicesCount, uint32_t *foundSlicesCount, uint32_t flags)
{
  bool rtn = false;
  if (string && string->len && c && slices && slicesCount)
  {
    uint32_t slicesFound = 0;
    char *startPtr = string->data;
    char *endPtr = startPtr + string->len;
    char *currentPtr = startPtr;
    bool allowFirst = flags & swStaticStringSearchAllowFirst;
    bool allowLast = flags & swStaticStringSearchAllowLast;
    if ((allowFirst || (*startPtr != c)) && (allowLast || (*(endPtr - 1) != c)))
    {
      bool allowAdjacent = flags & swStaticStringSearchAllowAdjacent;
      while ((slicesFound < slicesCount) && (currentPtr < endPtr))
      {
        if (*currentPtr == c)
        {
          size_t len = currentPtr - startPtr;
          if (!len && startPtr != string->data && !allowAdjacent)
            break;
          slices[slicesFound] = swStaticStringSetWithLength(startPtr, len);
          slicesFound++;
          currentPtr++;
          startPtr = currentPtr;
        }
        else
          currentPtr++;
      }
      if ((currentPtr >= endPtr) || (slicesFound >= slicesCount))
      {
        if (slicesFound < slicesCount && startPtr < endPtr && currentPtr == endPtr)
        {
          slices[slicesFound] = swStaticStringSetWithLength(startPtr, endPtr - startPtr);
          slicesFound++;
        }
        if (foundSlicesCount)
          *foundSlicesCount = slicesFound;
        rtn = true;
      }
    }
  }
  return rtn;
}

bool swStaticStringCountChar(const swStaticString *string, char c, uint32_t *foundCount)
{
  bool rtn = false;
  if (string && c && foundCount)
  {
    char *fromPtr = string->data;
    char *endPtr = fromPtr + string->len;
    uint32_t charCount = 0;
    while (fromPtr < endPtr)
    {
      rtn = true;
      char *nextPtr = memchr(fromPtr, c, endPtr - fromPtr);
      if(nextPtr)
      {
        charCount++;
        fromPtr = nextPtr + 1;
      }
      else
        break;
    }
    if (rtn)
      *foundCount = charCount;
  }
  return rtn;

}

