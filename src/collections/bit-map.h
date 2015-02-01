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

typedef struct __attribute__ ((__packed__)) swBitMap {
  uint16_t bitSize;
  uint16_t bitCount;
  uint8_t bytes[];
} swBitMap;

#define swBitMapMaxBitSize       ((1 << 16) - 16)
#define swBitMapMaxByteSize      (swBitMaxBitSize/8)
// bit position in a byte can only use values from 0 to 7, can be extracted from
// bit map positin by doing & with mask 7
#define swBitMapBitPositionMask   ((1 << 3) - 1)  // this is 7
#define swBitMapByteSize(b)       (((b) >> 3) + (((b) & swBitMapBitPositionMask) > 0))

#define swBitMapSize(m)     ((m)? m->bitSize : 0)
#define swBitMapCount(m)    ((m)? m->bitCount : 0)

static inline bool swBitMapIsSet(swBitMap *map, uint16_t bitPosition)
{
  if (map && bitPosition < map->bitSize)
    return (map->bytes[(bitPosition >> 3)] & (1 << (bitPosition & swBitMapBitPositionMask)));
  return false;
}

static inline void swBitMapSet(swBitMap *map, uint16_t bitPosition)
{
  if (map && bitPosition < map->bitSize)
  {
    uint16_t bytePosition = bitPosition >> 3;
    uint8_t bitMask = (1 << (bitPosition & swBitMapBitPositionMask));
    // NOTE: this piece of code might not be immidiately apparent what it is doing
    // subtract old bit value from the bit mask value; this equals either 0 if the values
    // are the same or some power of two value if the values are different;
    // comparison allows to quickly convert it to 0 or 1 respectively;
    // adding it to bitCount increments bitCount when we actually set previously unset bit
    map->bitCount += ((bitMask - (map->bytes[bytePosition] & bitMask)) > 0);
    map->bytes[bytePosition] |= bitMask;
  }
}

static inline void swBitMapClear(swBitMap *map, uint16_t bitPosition)
{
  if (map && bitPosition < map->bitSize)
  {
    uint16_t bytePosition = bitPosition >> 3;
    uint8_t bitMask = (1 << (bitPosition & swBitMapBitPositionMask));
    // NOTE: this piece of code might not be immidiately apparent what it is doing
    // we need to know if the bit was set or not; only if it is set, the bit count needs to
    // be decremented; so we take the bit value and do "> 0" comparison that gives us the value
    // to correctly subtract from the bitCount
    map->bitCount -= ((map->bytes[bytePosition] & bitMask) > 0);
    map->bytes[bytePosition] &= ~bitMask;
  }
}

static inline void swBitMapClearAll(swBitMap *map)
{
  if (map)
  {
    uint16_t byteSize = swBitMapByteSize(map->bitSize);
    uint8_t *currentBytePtr = map->bytes;
    const uint8_t *endBytePtr = currentBytePtr + byteSize;
    if ((endBytePtr - currentBytePtr) >= (ssize_t)sizeof(uint32_t))
    {
      *((uint32_t *)currentBytePtr) = 0;
      currentBytePtr += sizeof(uint32_t);
      while ((endBytePtr - currentBytePtr) >= (ssize_t)sizeof(uint64_t))
      {
        *((uint64_t *)currentBytePtr) = 0;
        currentBytePtr += sizeof(uint64_t);
      }
    }
    while (currentBytePtr < endBytePtr)
    {
      uint16_t bytesIncrement = 0;
      switch (endBytePtr - currentBytePtr)
      {
        case 1:
          *currentBytePtr = 0;
          bytesIncrement = sizeof(uint8_t);
          break;
        case 2:
        case 3:
          *((uint16_t *)currentBytePtr) = 0;
          bytesIncrement = sizeof(uint16_t);
          break;
        case 4:
        case 5:
        case 6:
        case 7:
          *((uint32_t *)currentBytePtr) = 0;
          bytesIncrement = sizeof(uint32_t);
          break;
      }
      currentBytePtr += bytesIncrement;
    }
    map->bitCount = 0;
  }
}

static inline void swBitMapSetAll(swBitMap *map)
{
  if (map)
  {
    uint16_t byteSize = swBitMapByteSize(map->bitSize);
    uint8_t *currentBytePtr = map->bytes;
    const uint8_t *endBytePtr = currentBytePtr + byteSize;
    if ((endBytePtr - currentBytePtr) >= (ssize_t)sizeof(uint32_t))
    {
      *((uint32_t *)currentBytePtr) = UINT32_MAX;
      currentBytePtr += sizeof(uint32_t);
      while ((endBytePtr - currentBytePtr) >= (ssize_t)sizeof(uint64_t))
      {
        *((uint64_t *)currentBytePtr) = UINT64_MAX;
        currentBytePtr += sizeof(uint64_t);
      }
    }
    while (currentBytePtr < endBytePtr)
    {
      uint16_t bytesIncrement = 0;
      switch (endBytePtr - currentBytePtr)
      {
        case 1:
          *currentBytePtr = UINT8_MAX;
          bytesIncrement = sizeof(uint8_t);
          break;
        case 2:
        case 3:
          *((uint16_t *)currentBytePtr) = UINT16_MAX;
          bytesIncrement = sizeof(uint16_t);
          break;
        case 4:
        case 5:
        case 6:
        case 7:
          *((uint32_t *)currentBytePtr) = UINT32_MAX;
          bytesIncrement = sizeof(uint32_t);
          break;
      }
      currentBytePtr += bytesIncrement;
    }
    map->bitCount = map->bitSize;
  }
}

static inline swBitMap *swBitMapNew(uint16_t bitSize)
{
  swBitMap *rtn = NULL;
  if (bitSize <= swBitMapMaxBitSize)
  {
    uint16_t byteSize = swBitMapByteSize(bitSize);
    // we really need calloc here to start with the clear bit map
    rtn = swMemoryCalloc(1, sizeof(swBitMap) + byteSize);
    if (rtn)
      rtn->bitSize = bitSize;
  }
  return rtn;
}

static inline void swBitMapDelete(swBitMap *map)
{
  if (map)
    swMemoryFree(map);
}

bool swBitMapFindFirstSet(swBitMap *map, uint16_t *position);
bool swBitMapFindFirstClear(swBitMap *map, uint16_t *position);

#endif  // SW_COLLECTIONS_BITMAP_H
