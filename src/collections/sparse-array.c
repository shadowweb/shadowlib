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

swSparseArray *swSparseArrayNew(size_t elementSize, uint32_t blockElementsMax, uint32_t blocksCount)
{
  swSparseArray *rtn = swMemoryMalloc(sizeof(swSparseArray *));
  if (rtn && !swSparseArrayInit(rtn, elementSize, blockElementsMax, blocksCount))
  {
    swMemoryFree(rtn);
    rtn = NULL;
  }
  return rtn;
}

bool swSparseArrayInit(swSparseArray *array, size_t elementSize, uint32_t blockElementsMax, uint32_t blocksCount)
{
  bool rtn = false;
  if (array && elementSize && swSparseArrayIsPowerOfTwo(blockElementsMax) && (blockElementsMax <= swBitMapLongIntBitCount) && blocksCount)
  {
    if (swDynamicArrayInit(&(array->metaData), sizeof(swSparseArrayBlockInfo), blocksCount))
    {
      array->elementSize = elementSize;
      array->blockElementsMax = blockElementsMax;
      array->blocksCount = blocksCount;
      array->shift = swHashClosestShiftFind(blockElementsMax) - 1;
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

static void swSparseArraySetNextFree(swSparseArray *array, swSparseArrayBlockInfo *blockInfo, uint32_t memBlockId, uint32_t blockPosition)
{
  blockInfo->firstFree = swBitMapLongIntGetNextFalse(blockInfo->usedMap, blockPosition, array->blockElementsMax);
  if (blockInfo->firstFree < array->blockElementsMax)
    array->firstFree = memBlockId * array->blockElementsMax + blockInfo->firstFree;
  else
  {
    memBlockId++;
    while (memBlockId < array->metaData.count)
    {
      blockInfo++;
      if (blockInfo->usedMap < ULONG_MAX)
      {
        array->firstFree = memBlockId * array->blockElementsMax + blockInfo->firstFree;
        break;
      }
      memBlockId++;
    }
    if (memBlockId == array->metaData.count)
      array->firstFree = memBlockId * array->blockElementsMax;
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
          if (blockInfo->data || swSparseArrayBlockInfoInit(blockInfo, array->elementSize, array->blockElementsMax))
          {
            if (index)
              *index = array->firstFree;
            if (data)
              *data = (void *)(blockInfo->data + blockPosition * array->elementSize);
            swBitMapLongIntSet(blockInfo->usedMap, blockInfo->firstFree);
            array->count++;
            swSparseArraySetNextFree(array, blockInfo, memBlockId, blockPosition);
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
          rtn = swDynamicArrayEnsureCapacity(&(array->metaData), array->blocksCount);
        }
        else if ((memBlockId + 1) == array->metaData.count)
        {
          while (--(array->metaData.count) && --blockInfo && !(blockInfo->usedMap));
          rtn = swDynamicArrayEnsureCapacity(&(array->metaData), (array->metaData.count > array->blocksCount)? array->metaData.count : array->blocksCount);
        }
      }
    }
  }
  return rtn;
}

bool swSparseArrayWalk(swSparseArray *array, swSparseArrayWalkFunction walkFunc)
{
  bool rtn = false;
  if (array && walkFunc)
  {
    uint32_t i = 0;
    swSparseArrayBlockInfo *blockInfo = (swSparseArrayBlockInfo *)(array->metaData.data);
    swSparseArrayBlockInfo *blockInfoLast = blockInfo + array->metaData.count;
    uint32_t count = array->count;
    while (blockInfo < blockInfoLast)
    {
      uint64_t usedMap = blockInfo->usedMap;
      uint32_t blockPosition = 0;
      while (usedMap)
      {
        if (usedMap & 1)
        {
          if (!walkFunc((void *)(blockInfo->data + blockPosition * array->elementSize)))
            break;
          i++;
        }
        usedMap >>= 1;
      }
      if (usedMap)
        break;
      blockInfo++;
    }
    if ((blockInfo == blockInfoLast) && (i == count))
      rtn = true;
  }
  return rtn;
}
