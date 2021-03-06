#ifndef SW_CORE_MEMORY_H
#define SW_CORE_MEMORY_H

#include <stdio.h>

typedef void *(swMemoryMallocFunction)(size_t size);
typedef void *(swMemoryCallocFunction)(size_t nmemb, size_t size);
typedef void *(swMemoryReallocFunction)(void *ptr, size_t size);
typedef void (swMemoryFreeFunction)(void *ptr);
typedef int (swMemoryAlignMallocFunction)(void **memptr, size_t alignment, size_t size);

typedef struct swMemoryManager
{
  swMemoryMallocFunction      *malloc;
  swMemoryCallocFunction      *calloc;
  swMemoryReallocFunction     *realloc;
  swMemoryFreeFunction        *free;
  swMemoryAlignMallocFunction *alignMalloc;
} swMemoryManager;

void swMemoryManagerSet(swMemoryManager *manager);
void swMemoryManagerDefaultSet();
swMemoryManager *swMemoryManagerGet();

extern swMemoryMallocFunction  *swMemoryMalloc;
extern swMemoryCallocFunction  *swMemoryCalloc;
extern swMemoryReallocFunction *swMemoryRealloc;
extern swMemoryFreeFunction    *swMemoryFree;

void *swMemoryDuplicate(void *src, size_t size);
void *swMemoryCacheAlignMalloc(size_t size);

#endif // SW_CORE_MEMORY_H
