#include "static-string.h"
#include <string.h>

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
