#include "fast-array.h"

#include "collections/hash-common.h"
#include "core/memory.h"

#include <string.h>

bool swFastArrayInit(swFastArray *array, size_t elementSize, size_t initialSize)
{
  bool rtn = false;
  if (array && elementSize)
  {
    uint32_t size = 1 << swHashClosestShiftFind((initialSize)? initialSize : 8);
    if ((array->storage = swMemoryMalloc(size * elementSize)))
    {
      array->size = size;
      array->elementSize = elementSize;
      array->count = 0;
      rtn = true;
    }
  }
  return rtn;
}

bool swFastArrayInitFromArray(swFastArray *to, swFastArray *from)
{
  bool rtn = false;
  if (to && from)
  {
    uint32_t size = 1 << swHashClosestShiftFind((from->count)? from->count : 8);
    if ((to->storage = swMemoryMalloc(size * from->elementSize)))
    {
      to->size = size;
      to->elementSize = from->elementSize;
      to->count = from->count;
      memcpy(to->storage, from->storage, from->count * from->elementSize);
      rtn = true;
    }
  }
  return rtn;
}

void swFastArrayClear(swFastArray *array)
{
  if (array)
  {
    if (array->storage)
    {
      swMemoryFree(array->storage);
      array->storage = NULL;
    }
    array->count = array->size = 0;
    array->elementSize = 0;
  }
}

bool swFastArrayResize(swFastArray* array, uint32_t index)
{
  bool rtn = false;
  if (array)
  {
    uint32_t newSize = 1 << swHashClosestShiftFind(index + 1);
    uint8_t *newStorage = swMemoryRealloc(array->storage, array->elementSize * newSize);
    if (newStorage)
    {
      array->storage = newStorage;
      array->size = newSize;
      rtn = true;
    }
  }
  return rtn;
}
