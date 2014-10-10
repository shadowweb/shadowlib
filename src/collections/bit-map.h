#ifndef SW_COLLECTIONS_BITMAP_H
#define SW_COLLECTIONS_BITMAP_H

#include <stdint.h>

typedef uint64_t swBitMapLongInt;

static uint32_t swBitMapLongIntBitCount = 8 * sizeof(swBitMapLongInt);

#define swBitMapLongIntMask(v)      ((swBitMapLongInt)1 << ((v) & (swBitMapLongIntBitCount - 1)))
#define swBitMapLongIntSet(v, b)    ((void) ((v) |= swBitMapLongIntMask(b)))
#define swBitMapLongIntClear(v, b)  ((void) ((v) &= ~swBitMapLongIntMask(b)))
#define swBitMapLongIntIsSet(v, b)  (((v) & swBitMapLongIntMask(b)) != 0)

static inline uint32_t swBitMapLongIntGetNextFalse(swBitMapLongInt map, uint32_t startPosition, uint32_t maxPosition)
{
  uint32_t rtn = startPosition + 1;
  uint64_t mask = swBitMapLongIntMask(startPosition);
  while (rtn < maxPosition)
  {
    mask <<= 1;
    if (!(map & mask))
      break;
    rtn++;
  }
  return rtn;
}

#endif  // SW_COLLECTIONS_BITMAP_H
