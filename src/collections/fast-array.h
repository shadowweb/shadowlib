#ifndef SW_COLLECTIONS_FASTARRAY_H
#define SW_COLLECTIONS_FASTARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/// This static array implementation is designed to be supper fast,
/// but provides little safe-guards, be careful using it
/// Note: this array does not preserve elements order upon removal

typedef struct swFastArray
{
  uint8_t *storage;
  uint32_t count;
  uint32_t size;
  size_t elementSize;
} swFastArray;

bool swFastArrayInit(swFastArray *array, size_t elementSize, size_t initialSize);
bool swFastArrayInitFromArray(swFastArray *to, swFastArray *from);
void swFastArrayClear(swFastArray *array);
bool swFastArrayResize(swFastArray *array, uint32_t index);

#define swFastArrayCount(a)             (a).count
#define swFastArraySize(a)              (a).size
#define swFastArrayData(a)              (a).storage

#define swFastArrayEnsureCapacity(a, i)     ( ((a).size > (i)) ? true : swFastArrayResize(&(a), i) )
#define swFastArrayGet(a, i, d)             ( ((a).size > (i)) ? ((d) = ((typeof(d) *)((a).storage))[i]), true : false )
#define swFastArrayGetPtr(a, i, t)          ( ((a).size > (i)) ? &((t *)((a).storage))[i] : NULL)
#define swFastArrayGetExistingPtr(a, i, t)  ( ((a).count > (i)) ? &((t *)((a).storage))[i] : NULL)
#define swFastArraySet(a, i, d)             ((swFastArrayEnsureCapacity((a), i)) ? ((typeof(d) *)((a).storage))[i] = (d), true : false )

#define swFastArrayPush(a, d)               ( swFastArraySet(a, (a).count, d) ? (++(a).count): false )
#define swFastArrayPop(a, d)                ( swFastArrayGet(a, (a).count - 1, d) ? ((a).count--) : false )
#define swFastRemove(a, t, i)               ( ((a).count > (i)) ? ( ((t *)((a).storage))[i] = ((t *)((a).storage))[--(a).count] ), true : false )

#endif // SW_COLLECTIONS_FASTARRAY_H
