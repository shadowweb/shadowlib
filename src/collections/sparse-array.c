#include "collections/sparse-array.h"

#include "collections/hash-common.h"
#include "core/memory.h"

#include <limits.h>
#include <string.h>

static bool swSparseArrayBlockInfoInit(swSparseArrayBlockInfo *info, size_t elementSize, uint32_t blockSize)
{
  bool rtn = false;
  if (info && elementSize && blockSize)
  {
    memset (info, 0, sizeof(*info));
    if ((info->data = swMemoryMalloc(elementSize * blockSize)))
      rtn = true;
  }
  return rtn;
}

static void swSpareArrayBlockInfoRelease(swSparseArrayBlockInfo *info)
{
  if (info && info->data)
  {
    swMemoryFree(info->data);
    info->data = NULL;
  }
}

swSparseArray *swSparseArrayNew(size_t elementSize, uint32_t groupSize, uint32_t groupCount)
{
  swSparseArray *rtn = swMemoryMalloc(sizeof(swSparseArray *));
  if (rtn && !swSparseArrayInit(rtn, elementSize, groupSize, groupCount))
  {
    swMemoryFree(rtn);
    rtn = NULL;
  }
  return rtn;
}

bool swSparseArrayInit(swSparseArray *array, size_t elementSize, uint32_t groupSize, uint32_t groupCount)
{
  bool rtn = false;
  if (array && elementSize && swSparseArrayIsPowerOfTwo(groupSize) && groupSize <= SW_SPARSE_ARRAY_MAX_BLOCK_SIZE && groupCount)
  {
    if (swDynamicArrayInit(&(array->metaData), sizeof(swSparseArrayBlockInfo), groupCount))
    {
      array->elementSize = elementSize;
      array->groupSize = groupSize;
      array->groupCount = groupCount;
      array->shift = swHashClosestShiftFind(groupSize) - 1;
      array->mask = swHashGetMask(array->shift);
      array->count = 0;
      array->firstFree = 0;
      rtn = true;
    }
  }
  return rtn;
}

void swSparseArrayRelease(swSparseArray *array)
{
  if (array)
  {
    if (array->count)
    {
      swSparseArrayBlockInfo *metaData = (swSparseArrayBlockInfo *)(array->metaData.data);
      swSparseArrayBlockInfo *metaDataLast = metaData + array->metaData.count;
      while (metaData < metaDataLast)
      {
        swSpareArrayBlockInfoRelease(metaData);
        metaData++;
      }
    }
    swDynamicArrayRelease(&(array->metaData));
    memset(array, 0 , sizeof(*array));
  }
}

void swSparseArrayFree(swSparseArray *array)
{
  if (array)
  {
    swSparseArrayRelease(array);
    swMemoryFree(array);
  }
}

// -------------
// TODO: move this into a separate header file
#define SW_BF_NBITS            (8 * sizeof (uint64_t))
// #define SW_BF_ELT(d)           ((d) / SW_BF_NBITS)
#define SW_BF_MASK(d)          ((uint64_t) 1 << ((d) % SW_BF_NBITS))
#define SW_BF_SET(d, bits)     ((void) ((bits) |= SW_BF_MASK (d)))
#define SW_BF_CLR(d, bits)     ((void) ((bits) &= ~SW_BF_MASK (d)))
#define SW_BF_ISSET(d, bits)   (((bits) & SW_BF_MASK (d)) != 0)

static uint32_t swBitFieldGetNextFree(uint64_t bitField, uint32_t startPosition, uint32_t maxPosition)
{
  uint32_t rtn = startPosition + 1;
  uint64_t mask = SW_BF_MASK(startPosition);
  while (rtn < maxPosition)
  {
    mask <<= 1;
    if (!(bitField & mask))
      break;
    rtn++;
  }
  return rtn;
}
// ------------------

static swSparseArrayBlockInfo emptyBlock = { NULL };

bool swSparseArrayAcquireFirstFree(swSparseArray *array, uint32_t *index, void **data)
{
  bool rtn = false;
  if (array && (index || data))
  {
    uint32_t memBlockId = (array->firstFree) >> array->shift;
    uint32_t blockPosition = array->firstFree & array->mask;
    if ((memBlockId < array->metaData.size) || swDynamicArrayEnsureCapacity(&(array->metaData), (memBlockId + 1)))
    {
      while ((array->metaData.count <= memBlockId) && swDynamicArrayPush(&(array->metaData), &emptyBlock));
      if (array->metaData.count > memBlockId)
      {
        swSparseArrayBlockInfo *metaData = swDynamicArrayGet(&(array->metaData), memBlockId);
        if (metaData)
        {
          if (metaData->data || swSparseArrayBlockInfoInit(metaData, array->elementSize, array->groupSize))
          {
            if (index)
              *index = array->firstFree;
            if (data)
              *data = (void *)(metaData->data + blockPosition * array->elementSize);
            SW_BF_SET(metaData->firstFree, metaData->usedMap);
            metaData->firstFree = swBitFieldGetNextFree(metaData->usedMap, blockPosition, array->groupSize);
            if (metaData->firstFree < SW_SPARSE_ARRAY_MAX_BLOCK_SIZE)
              array->firstFree = memBlockId * array->groupSize + metaData->firstFree;
            else
            {
              memBlockId++;
              while (memBlockId < array->metaData.count)
              {
                metaData++;
                // metaData = swDynamicArrayGet(&(array->metaData), memBlockId);
                if (metaData->usedMap < ULONG_MAX)
                {
                  array->firstFree = memBlockId * array->groupSize + metaData->firstFree;
                  break;
                }
                memBlockId++;
              }
              if (memBlockId == array->metaData.count)
                array->firstFree = memBlockId * array->groupSize;
            }
            array->count++;
            rtn = true;
          }
        }
      }
    }
  }
  return rtn;
}

bool swSparseArrayRemove(swSparseArray *array, uint32_t index)
{
  return swSparseArrayExtract(array, index, NULL);
}

bool swSparseArrayExtract(swSparseArray *array, uint32_t index, void *data)
{
  bool rtn = false;
  if (array && array->count)
  {
    uint32_t memBlockId = index >> array->shift;
    uint32_t blockPosition = index & array->mask;
    swSparseArrayBlockInfo *blockInfo = swDynamicArrayGet(&(array->metaData), memBlockId);
    if (blockInfo)
    {
      if (data)
        memcpy(data, (void *)(blockInfo->data + array->elementSize * blockPosition), array->elementSize);
      rtn = true;
      SW_BF_CLR(blockPosition, blockInfo->usedMap);
      if (index < array->firstFree)
        array->firstFree = index;
      array->count--;
      if (blockPosition < blockInfo->firstFree)
        blockInfo->firstFree = blockPosition;
      if (!(blockInfo->usedMap))
      {
        swSpareArrayBlockInfoRelease(blockInfo);
        if (!(array->count))
        {
          array->metaData.count = 0;
          rtn = swDynamicArrayEnsureCapacity(&(array->metaData), array->groupCount);
        }
        else if ((memBlockId + 1) == array->metaData.count)
        {
          while (--(array->metaData.count) && --blockInfo && !(blockInfo->usedMap));
          rtn = swDynamicArrayEnsureCapacity(&(array->metaData), (array->metaData.count > array->groupCount)? array->metaData.count : array->groupCount);
        }
      }
    }
  }
  return rtn;
}

bool swSparseArrayWalk(swSparseArray *array, bool (*func)(void *ptr))
{
  bool rtn = false;
  if (array && func)
  {
    uint32_t i = 0;
    swSparseArrayBlockInfo *metaData = (swSparseArrayBlockInfo *)(array->metaData.data);
    swSparseArrayBlockInfo *metaDataLast = metaData + array->metaData.count;
    while (metaData < metaDataLast)
    {
      if (metaData->usedMap)
      {
        uint32_t blockPosition = 0;
        uint64_t usedMap = metaData->usedMap;
        bool failed = false;
        while (usedMap)
        {
          if (usedMap & 1)
          {
            if (!func((void *)(metaData->data + blockPosition * array->elementSize)))
            {
              failed = true;
              break;
            }
            i++;
          }
          usedMap >>= 1;
        }
        if (failed)
          break;
      }
      metaData++;
    }
    if ((metaData == metaDataLast) && (i == array->count))
      rtn = true;
  }
  return rtn;
}
