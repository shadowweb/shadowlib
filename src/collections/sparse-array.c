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
  if (array && elementSize && swSparseArrayIsPowerOfTwo(groupSize) && (groupSize <= swBitMapLongIntBitCount) && groupCount)
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
        swSparseArrayBlockInfo *blockInfo = swDynamicArrayGet(&(array->metaData), memBlockId);
        if (blockInfo)
        {
          if (blockInfo->data || swSparseArrayBlockInfoInit(blockInfo, array->elementSize, array->groupSize))
          {
            if (index)
              *index = array->firstFree;
            if (data)
              *data = (void *)(blockInfo->data + blockPosition * array->elementSize);
            swBitMapLongIntSet(blockInfo->usedMap, blockInfo->firstFree);
            blockInfo->firstFree = swBitMapLongIntGetNextFalse(blockInfo->usedMap, blockPosition, array->groupSize);
            if (blockInfo->firstFree < swBitMapLongIntBitCount)
              array->firstFree = memBlockId * array->groupSize + blockInfo->firstFree;
            else
            {
              memBlockId++;
              while (memBlockId < array->metaData.count)
              {
                blockInfo++;
                // metaData = swDynamicArrayGet(&(array->metaData), memBlockId);
                if (blockInfo->usedMap < ULONG_MAX)
                {
                  array->firstFree = memBlockId * array->groupSize + blockInfo->firstFree;
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
      swBitMapLongIntClear(blockInfo->usedMap, blockPosition);
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
