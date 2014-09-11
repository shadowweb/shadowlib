#ifndef SW_COLLECTIONS_DYNAMICARRAY_H
#define SW_COLLECTIONS_DYNAMICARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "collections/static-array.h"

typedef struct swDynamicArray
{
  size_t elementSize;
  uint8_t *data;
  uint32_t count;
  uint32_t size;
} swDynamicArray;

#define swDynamicArrayData(a)             (a).data
#define swDynamicArrayCount(a)            (a).count
#define swDynamicArraySize(a)             (a).size

#define swDynamicArrayInitEmpty(es)       { .elementSize = (es) }
#define swDynamicArraySetEmpty(es)        *(swDynamicArray[]){swDynamicArrayInitEmpty(es)}

swDynamicArray *swDynamicArrayNew(size_t elementSize, uint32_t size);
void swDynamicArrayDelete(swDynamicArray *array);

swDynamicArray *swDynamicArrayNewFromStaticArray(const swStaticArray *staticArray);
bool swDynamicArraySetFromStaticArray(swDynamicArray *dynamicArray, const swStaticArray *staticArray);

bool swDynamicArrayInit(swDynamicArray *array, size_t elementSize, uint32_t size);
void swDynamicArrayClear(swDynamicArray *array);
void swDynamicArrayRelease(swDynamicArray *array);

bool swDynamicArrayAppendStaticArray(swDynamicArray *dynamicArray, const swStaticArray *staticArray);
bool swDynamicArraySet(swDynamicArray *dynamicArray, uint32_t position, void *element);
void *swDynamicArrayGet(swDynamicArray *dynamicArray, uint32_t position);
bool swDynamicArrayPush(swDynamicArray *dynamicArray, void *element);
bool swDynamicArrayPop(swDynamicArray *dynamicArray, void *element);

#endif // SW_COLLECTIONS_DYNAMICARRAY_H
