#ifndef SW_COLLECTIONS_SPARSEARRAY_H
#define SW_COLLECTIONS_SPARSEARRAY_H

#include "collections/dynamic-array.h"
#include "collections/static-array.h"

#define SW_SPARSE_ARRAY_MAX_BLOCK_SIZE  64

typedef struct swSparseArrayBlockInfo
{
  uint8_t *data;
  uint64_t usedMap;
  // uint32_t count;
  uint32_t firstFree;
} swSparseArrayBlockInfo;

typedef struct swSparseArray
{
  swDynamicArray metaData;
  size_t elementSize;
  // TODO: think of a better name, this one sucks
  uint32_t firstFree;
  uint32_t count;
  uint32_t mask;
  // TODO: the rest can be smaller than 32 bit (16 perhaps)
  uint32_t groupSize;
  uint32_t shift;
  uint32_t groupCount;
} swSparseArray;

#define swSparseArrayIsPowerOfTwo(x)    ((x) && !((x) & ((x) - 1)))
#define swSparseArrayCount(a)           (a).count

swSparseArray *swSparseArrayNew(size_t elementSize, uint32_t groupSize, uint32_t groupCount);
bool swSparseArrayInit(swSparseArray *array, size_t elementSize, uint32_t groupSize, uint32_t groupCount);
void swSparseArrayRelease(swSparseArray *array);
void swSparseArrayFree(swSparseArray *array);

bool swSparseArrayAcquireFirstFree(swSparseArray *array, uint32_t *index, void **data);
bool swSparseArrayRemove(swSparseArray *array, uint32_t index);
bool swSparseArrayWalk(swSparseArray *array, bool (*func)(void *ptr));

#endif  // SW_COLLECTIONS_SPARSEARRAY_H
