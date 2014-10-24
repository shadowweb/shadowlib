#include "core/memory.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static swMemoryManager defaultManager = {malloc, calloc, realloc, free, posix_memalign};
static swMemoryManager *currentManager = &defaultManager;

swMemoryMallocFunction       *swMemoryMalloc       = malloc;
swMemoryCallocFunction       *swMemoryCalloc       = calloc;
swMemoryReallocFunction      *swMemoryRealloc      = realloc;
swMemoryFreeFunction         *swMemoryFree         = free;
swMemoryAlignMallocFunction  *swMemoryAlignMalloc  = posix_memalign;

void swMemoryManagerSet(swMemoryManager *manager)
{
  if (manager)
  {
    currentManager       = manager;
    swMemoryMalloc       = currentManager->malloc;
    swMemoryCalloc       = currentManager->calloc;
    swMemoryRealloc      = currentManager->realloc;
    swMemoryFree         = currentManager->free;
    swMemoryAlignMalloc  = currentManager->alignMalloc;
  }
}

void swMemoryManagerDefaultSet()
{
  currentManager = &defaultManager;
}

swMemoryManager *swMemoryManagerGet()
{
  return currentManager;
}

void *swMemoryDuplicate(void *src, size_t size)
{
  void *rtn = NULL;
  if (src)
  {
    rtn = swMemoryMalloc(size);
    if (rtn)
      memcpy(rtn, src, size);
  }
  return rtn;
}

static size_t cacheSize = 64;

void *swMemoryCacheAlignMalloc(size_t size)
{
  void *rtn = NULL;
  if (cacheSize || ((cacheSize = sysconf(_SC_LEVEL1_DCACHE_LINESIZE)) > 0))
  {
    if (swMemoryAlignMalloc(&rtn, cacheSize, size) != 0)
      rtn = NULL;
  }
  return rtn;
}
