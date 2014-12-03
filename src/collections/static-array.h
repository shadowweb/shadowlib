#ifndef SW_COLLECTIONS_STATICARRAY_H
#define SW_COLLECTIONS_STATICARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct swStaticArray
{
  size_t elementSize;
  uint8_t *data;
  uint32_t count;
} swStaticArray;

#define swStaticArrayDefineEmpty                {.elementSize = 0,         .data = NULL,           .count = 0}
#define swStaticArrayDefine(d, t)               {.elementSize = sizeof(t), .data = (uint8_t *)(d), .count = sizeof(d)/sizeof(t)}
#define swStaticArrayDefineWithCount(d, t, c)   {.elementSize = sizeof(t), .data = (uint8_t *)(d), .count = (c)}

#define swStaticArraySetWithCount(d, t, c)      *(swStaticArray[]){{.elementSize = sizeof(t), .data = (uint8_t *)(d), .count = (c)}}
#define swStaticArraySet(d, t)                  *(swStaticArray[]){{.elementSize = sizeof(t), .data = (uint8_t *)(d), .count = sizeof(d)/sizeof(t)}}
#define swStaticArraySetEmpty                   *(swStaticArray[]){{.elementSize = 0,         .data = NULL,           .count = 0}}

#define swStaticArrayData(a)              (a).data
#define swStaticArrayCount(a)             (a).count

#endif // SW_COLLECTIONS_STATICARRAY_H
