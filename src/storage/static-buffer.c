#include "static-buffer.h"

#include <string.h>

#include <collections/hash-functions.h>

uint32_t swStaticBufferHash(const swStaticBuffer *buffer)
{
  if (buffer)
    return swMurmurHash3_32(buffer->data, buffer->len);
  return 0;
}

int swStaticBufferCompare(const swStaticBuffer *b1, const swStaticBuffer *b2)
{
  int ret = 0;
  if (b1 == b2)
    ret = 0;
  else if (b1 && b2)
  {
    if (b1->len == b2->len)
      ret = memcmp(b1->data, b2->data, b1->len);
    else
    {
      if (b1->len < b2->len)
      {
        ret = memcmp(b1->data, b2->data, b1->len);
        if (ret == 0)
          ret = -1;
      }
      else
      {
        ret = memcmp(b1->data, b2->data, b2->len);
        if (ret == 0)
          ret = 1;
      }
    }
  }
  else
    ret = (b1)? 1 : -1;
  return ret;
}

bool swStaticBufferEqual(const swStaticBuffer *b1, const swStaticBuffer *b2)
{
  bool ret = false;
  if (b1 == b2)
    ret = true;
  else if (b1 && b2)
  {
    if (b1->len == b2->len)
      ret = (memcmp(b1->data, b2->data, b1->len) == 0);
  }
  return ret;
}
