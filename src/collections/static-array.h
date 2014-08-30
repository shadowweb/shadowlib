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

#define swStaticArrayDefineEmpty                {.elementSize = 0,    .data = NULL, .count = 0}
#define swStaticArrayDefine(d, es)              {.elementSize = (es), .data = (d),  .count = sizeof(d)/(es)}
#define swStaticArrayDefineWithCount(d, es, c)  {.elementSize = (es), .data = (d),  .count = (c)}

#define swStaticArraySetWithCount(d, es, c)     *(swStaticArray[]){{.elementSize = (es), .data = (d),  .count = (c)}}
#define swStaticArraySet(d, es)                 *(swStaticArray[]){{.elementSize = (es), .data = (d),  .count = sizeof(d)/(es)}}
#define swStaticArraySetEmpty                   *(swStaticArray[]){{.elementSize = 0,    .data = NULL, .count = 0}}

#define swStaticArrayData(a)              (a).data
#define swStaticArrayCount(a)             (a).count

#endif // SW_COLLECTIONS_STATICARRAY_H
