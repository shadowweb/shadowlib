#ifndef SW_COLLECTIONS_ARRAY_H
#define SW_COLLECTIONS_ARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/// This static array implementation is designed to be supper fast,
/// but provides little safe-guards, be careful using it

typedef struct swStaticArray
{
  uint8_t *storage;
  uint32_t count;
  uint32_t size;
  size_t elementSize;
} swStaticArray;

bool swStaticArrayInit(swStaticArray *array, size_t elementSize, size_t initialSize);
void swStaticArrayClear(swStaticArray *array);
bool swStaticArrayResize(swStaticArray *array, uint32_t index);

#define swStaticArrayCount(a)             (a).count
#define swStaticArraySize(a)              (a).size
#define swStaticArrayData(a)              (a).storage

#define swStaticArrayEnsureCapacity(a, i) ( ((a).size > (i)) ? true : swStaticArrayResize(&(a), i) )
#define swStaticArrayGet(a, i, d)         ( ((a).size > (i)) ? ((d) = ((typeof(d) *)((a).storage))[i]), true : false )
#define swStaticArraySet(a, i, d)         ((swStaticArrayEnsureCapacity((a), i)) ? ((typeof(d) *)((a).storage))[i] = (d), true : false )

#define swStaticArrayPush(a, d)           ( swStaticArraySet(a, (a).count, d) ? (++(a).count): false )
#define swStaticArrayPop(a, d)            ( swStaticArrayGet(a, (a).count - 1, d) ? ((a).count--) : false )
#define swStaticRemove(a, t, i)           ( ((a).count > (i)) ? ( ((t *)((a).storage))[i] = ((t *)((a).storage))[--(a).count] ) : false )

#endif // SW_COLLECTIONS_ARRAY_H
