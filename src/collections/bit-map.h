#ifndef SW_COLLECTIONS_BITMAP_H
#define SW_COLLECTIONS_BITMAP_H

#include <stdbool.h>
#include <stdint.h>

#include "core/memory.h"

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

// TODO: implement larger bit map
typedef struct __attribute__ ((__packed__)) swBitMap {
  uint16_t bitCount;
  uint8_t bits[];
} swBitMap;

#define swBitMapMaxBitCount       ((1 << 16) - 16)
#define swBitMapMaxByteCount      (swBitMaxBitCount/8)
// bit position in a byte can only use values from 0 to 7, can be extracted from
// bit map positin by doing & with mask 7
#define swBitMapBitPositionMask   ((1 << 3) - 1)  // this is 7

static inline bool swBitMapIsSet(swBitMap *map, uint16_t bit)
{
  if (map && bit < map->bitCount)
    return (map->bits[(bit >> 3)] & (1 << (bit & swBitMapBitPositionMask)));
  return false;
}

static inline void swBitMapSet(swBitMap *map, uint16_t bit)
{
  if (map && bit < map->bitCount)
    map->bits[(bit >> 3)] |= (1 << (bit & swBitMapBitPositionMask));
}

static inline void swBitMapClear(swBitMap *map, uint16_t bit)
{
  if (map && bit < map->bitCount)
    map->bits[(bit >> 3)] &= (~(1 << (bit & swBitMapBitPositionMask)));
}

static inline void swBitMapClearAll(swBitMap *map)
{
  if (map)
  {
    uint16_t byteCount = (map->bitCount >> 3) + 1;
    for (uint16_t i = 0; i < byteCount; i++)
      map->bits[i] = 0;
  }
}

static inline swBitMap *swBitMapNew(uint16_t bitCount)
{
  swBitMap *rtn = NULL;
  if (bitCount <= swBitMapMaxBitCount)
  {
    uint16_t byteCount = ((bitCount - 1) >> 3) + 1;
    rtn = swMemoryMalloc(sizeof(swBitMap) + byteCount);
    if (rtn)
      rtn->bitCount = bitCount;
  }
  return rtn;
}

static inline void swBitMapDelete(swBitMap *map)
{
  if (map)
    swMemoryFree(map);
}

#endif  // SW_COLLECTIONS_BITMAP_H
