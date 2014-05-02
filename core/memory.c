#include <stdlib.h>
#include <string.h>
#include "memory.h"

static swMemoryManager defaultManager = {malloc, calloc, realloc, free};
static swMemoryManager *currentManager = &defaultManager;

swMemoryMallocFunction  *swMemoryMalloc   = malloc;
swMemoryCallocFunction  *swMemoryCalloc   = calloc;
swMemoryReallocFunction *swMemoryRealloc  = realloc;
swMemoryFreeFunction    *swMemoryFree     = free;

void swMemoryManagerSet(swMemoryManager *manager)
{
  if (manager)
  {
    currentManager   = manager;
    swMemoryMalloc   = currentManager->malloc;
    swMemoryCalloc   = currentManager->calloc;
    swMemoryRealloc  = currentManager->realloc;
    swMemoryFree     = currentManager->free;
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
