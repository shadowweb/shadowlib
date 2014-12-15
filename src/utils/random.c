#include "utils/random.h"
#include <stdio.h>

static inline bool swUtilFillRandomInternal(swDynamicBuffer* buffer, size_t bytesNeeded, const char *randomFileName)
{
  bool rtn = false;
  if (swDynamicBufferEnsureCapacity(buffer, (buffer->len + bytesNeeded)))
  {
    FILE *file = fopen(randomFileName, "r");
    if (file)
    {
      if (fread(buffer->data + buffer->len, bytesNeeded, 1, file))
      {
        buffer->len += bytesNeeded;
        rtn = true;
      }
      fclose(file);
    }
  }
  return rtn;
}

bool swUtilFillRandom(swDynamicBuffer *buffer, size_t bytesNeeded)
{
  bool rtn = false;
  if (buffer && bytesNeeded)
    rtn = swUtilFillRandomInternal(buffer, bytesNeeded, "/dev/random");
  return rtn;
}

bool swUtilFillURandom(swDynamicBuffer *buffer, size_t bytesNeeded)
{
  bool rtn = false;
  if (buffer && bytesNeeded)
    rtn = swUtilFillRandomInternal(buffer, bytesNeeded, "/dev/urandom");
  return rtn;
}
