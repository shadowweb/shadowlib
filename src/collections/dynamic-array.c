#include <string.h>

#include "collections/dynamic-array.h"
#include "collections/hash-common.h"
#include "core/memory.h"

#define swDynamicArrayGetNextFreePtr(d) (d->data + (d->count * d->elementSize))
#define swDynamicArrayGetPositionPtr(d, p) (d->data + (p * d->elementSize))

swDynamicArray *swDynamicArrayNew(size_t elementSize, uint32_t size)
{
  swDynamicArray *rtn = swMemoryCalloc(1, sizeof(swDynamicArray));
  if (rtn)
  {
    if (!swDynamicArrayInit(rtn, elementSize, size))
    {
      swDynamicArrayDelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

void swDynamicArrayDelete(swDynamicArray *array)
{
  if (array)
  {
    if (array->data)
      swMemoryFree(array->data);
    swMemoryFree(array);
  }
}

static bool swDynamicArrayResize(swDynamicArray *dynamicArray, uint32_t index)
{
  bool rtn = false;
  if (dynamicArray)
  {
    uint32_t newSize = 1 << swHashClosestShiftFind(index + 1);
    if (newSize != dynamicArray->size)
    {
      uint8_t *newData = swMemoryRealloc(dynamicArray->data, dynamicArray->elementSize * newSize);
      if (newData)
      {
        dynamicArray->data = newData;
        dynamicArray->size = newSize;
        rtn = true;
      }
    }
    else
      rtn = true;
  }
  return rtn;
}

swDynamicArray *swDynamicArrayNewFromStaticArray(const swStaticArray *staticArray)
{
  swDynamicArray *rtn = NULL;
  if (staticArray)
  {
    swDynamicArray *newDynamicArray = swDynamicArrayNew(staticArray->elementSize, staticArray->count);
    if (newDynamicArray)
    {
      memcpy(newDynamicArray->data, staticArray->data, staticArray->count * staticArray->elementSize);
      newDynamicArray->count = staticArray->count;
      rtn = newDynamicArray;
    }
  }
  return rtn;
}

bool swDynamicArraySetFromStaticArray(swDynamicArray *dynamicArray, const swStaticArray *staticArray)
{
  bool rtn = false;
  if (dynamicArray && staticArray)
  {
    swDynamicArray tempDynamicArray = {0};
    if (swDynamicArrayInit(&tempDynamicArray, staticArray->elementSize, staticArray->count))
    {
      memcpy(tempDynamicArray.data, staticArray->data, staticArray->count * staticArray->elementSize);
      tempDynamicArray.count = staticArray->count;
      swDynamicArrayRelease(dynamicArray);
      *dynamicArray = tempDynamicArray;
      rtn = true;
    }
  }
  return rtn;
}

bool swDynamicArrayInit(swDynamicArray *array, size_t elementSize, uint32_t size)
{
  bool rtn = false;
  if (array && elementSize && size)
  {
    if ((array->data = swMemoryMalloc(size * elementSize)))
    {
      array->elementSize = elementSize;
      array->size = size;
      array->count = 0;
      rtn = true;
    }
  }
  return rtn;
}

void swDynamicArrayClear(swDynamicArray *array)
{
  if (array)
    array->count = 0;
}

void swDynamicArrayRelease(swDynamicArray *array)
{
  if (array)
  {
    if (array->data)
    {
      swMemoryFree(array->data);
      array->data = NULL;
    }
    array->count = 0;
    array->size = 0;
    array->elementSize = 0;
  }
}

bool swDynamicArrayEnsureCapacity(swDynamicArray *array, uint32_t size)
{
  bool rtn = false;
  if (array && size && (array->count <= size))
  {
    if (array->size < size)
      rtn = swDynamicArrayResize(array, size);
    else if (array->count < size)
      rtn = swDynamicArrayResize(array, size);
    else
      rtn = true;
  }
  return rtn;
}

bool swDynamicArrayAppendStaticArray(swDynamicArray *dynamicArray, const swStaticArray *staticArray)
{
  bool rtn = false;
  if (dynamicArray && staticArray && (dynamicArray->elementSize == staticArray->elementSize))
  {
    uint32_t newCount = dynamicArray->count + staticArray->count;
    if ((newCount <= dynamicArray->size) || swDynamicArrayResize(dynamicArray, newCount))
    {
      uint8_t *startPtr = swDynamicArrayGetNextFreePtr(dynamicArray);
      memcpy(startPtr, staticArray->data, staticArray->count * staticArray->elementSize);
      dynamicArray->count = newCount;
      rtn = true;
    }
  }
  return rtn;
}

bool swDynamicArraySet(swDynamicArray *dynamicArray, uint32_t position, void *element)
{
  bool rtn = false;
  if (dynamicArray && (position <= dynamicArray->count) && element)
  {
    uint32_t newCount = dynamicArray->count + (position == dynamicArray->count);
    if ((newCount <= dynamicArray->size) || swDynamicArrayResize(dynamicArray, newCount))
    {
      uint8_t *startPtr = swDynamicArrayGetPositionPtr(dynamicArray, position);
      memcpy(startPtr, element, dynamicArray->elementSize);
      dynamicArray->count = newCount;
      rtn = true;
    }
  }
  return rtn;
}

void *swDynamicArrayGet(swDynamicArray *dynamicArray, uint32_t position)
{
  if (dynamicArray && (position < dynamicArray->count))
    return dynamicArray->data + (dynamicArray->elementSize * position);
  return NULL;
}


bool swDynamicArrayPush(swDynamicArray *dynamicArray, void *element)
{
  bool rtn = false;
  if (dynamicArray && element)
  {
    uint32_t newCount = dynamicArray->count + 1;
    if ((newCount <= dynamicArray->size) || swDynamicArrayResize(dynamicArray, newCount))
    {
      uint8_t *startPtr = swDynamicArrayGetNextFreePtr(dynamicArray);
      memcpy(startPtr, element, dynamicArray->elementSize);
      dynamicArray->count = newCount;
      rtn = true;
    }
  }
  return rtn;
}

bool swDynamicArrayPop(swDynamicArray *dynamicArray, void *element)
{
  bool rtn = false;
  if (dynamicArray && element && dynamicArray->count)
  {
    uint32_t newCount = dynamicArray->count - 1;
    if ((newCount <= dynamicArray->size) || swDynamicArrayResize(dynamicArray, newCount))
    {
      uint8_t *startPtr = swDynamicArrayGetPositionPtr(dynamicArray, newCount);
      memcpy(element, startPtr, dynamicArray->elementSize);
      dynamicArray->count = newCount;
      rtn = true;
    }
  }
  return rtn;
}
