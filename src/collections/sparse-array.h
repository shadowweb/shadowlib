#ifndef SW_COLLECTIONS_SPARSEARRAY_H
#define SW_COLLECTIONS_SPARSEARRAY_H

#include "collections/bit-map.h"
#include "collections/dynamic-array.h"
#include "collections/static-array.h"

typedef struct swSparseArrayBlockInfo
{
  uint8_t        *data;
  swBitMapLongInt usedMap;
  uint32_t        firstFree;
} swSparseArrayBlockInfo;

typedef struct swSparseArray
{
  swDynamicArray metaData;
  size_t elementSize;
  uint32_t firstFree;
  uint32_t count;
  uint32_t mask;
  uint32_t shift;
  uint32_t blockElementsMax;
  uint32_t blocksCount;
} swSparseArray;

#define swSparseArrayIsPowerOfTwo(x)    ((x) && !((x) & ((x) - 1)))
#define swSparseArrayCount(a)           (a).count

swSparseArray *swSparseArrayNew(size_t elementSize, uint32_t blockElementsMax, uint32_t blocksCount);
bool swSparseArrayInit(swSparseArray *array, size_t elementSize, uint32_t blockElementsMax, uint32_t blocksCount);
void swSparseArrayRelease(swSparseArray *array);
void swSparseArrayFree(swSparseArray *array);

bool swSparseArrayAcquireFirstFree(swSparseArray *array, uint32_t *index, void **data);
bool swSparseArrayRemove(swSparseArray *array, uint32_t index);
bool swSparseArrayExtract(swSparseArray *array, uint32_t index, void *data);
bool swSparseArrayWalk(swSparseArray *array, bool (*func)(void *ptr));

#endif  // SW_COLLECTIONS_SPARSEARRAY_H
