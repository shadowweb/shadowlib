#include "collections/bit-map.h"

#include <endian.h>

bool swBitMapFindFirstSet(swBitMap *map, uint16_t *position)
{
  bool rtn = false;
  if (map && (map->bitCount > 0) && position)
  {
    uint16_t byteSize = map->bitSize >> 3;
    uint8_t *currentBytePtr = map->bytes;
    const uint8_t *endBytePtr = currentBytePtr + byteSize;
    uint16_t foundPosition = 0;
    if ((endBytePtr - currentBytePtr) >= (ssize_t)sizeof(uint32_t))
    {
      if (*((uint32_t *)currentBytePtr) > 0)
      {
        foundPosition = __builtin_ctz(htole32(*((uint32_t *)currentBytePtr)));
        rtn = true;
      }
      else
      {
        currentBytePtr += sizeof(uint32_t);
        while ((endBytePtr - currentBytePtr) >= (ssize_t)sizeof(uint64_t))
        {
          if (*((uint64_t *)currentBytePtr) > 0)
          {
            foundPosition = (currentBytePtr - (uint8_t *)(map->bytes)) * 8 + __builtin_ctzl(htole64(*((uint64_t *)currentBytePtr)));
            rtn = true;
            break;
          }
          currentBytePtr += sizeof(uint64_t);
        }
      }
    }
    if (!rtn)
    {
      while (currentBytePtr < endBytePtr)
      {
        uint16_t bytesIncrement = 0;
        switch (endBytePtr - currentBytePtr)
        {
          case 1:
            bytesIncrement = sizeof(uint8_t);
            if (*currentBytePtr > 0)
            {
              foundPosition = (currentBytePtr - (uint8_t *)(map->bytes)) * 8 + __builtin_ctz(*currentBytePtr);
              rtn = true;
            }
            break;
          case 2:
          case 3:
            bytesIncrement = sizeof(uint16_t);
            if (*((uint16_t *)currentBytePtr) > 0)
            {
              foundPosition = (currentBytePtr - (uint8_t *)(map->bytes)) * 8 + __builtin_ctz(htole16(*((uint16_t *)currentBytePtr)));
              rtn = true;
            }
            break;
          case 4:
          case 5:
          case 6:
          case 7:
            bytesIncrement = sizeof(uint32_t);
            if (*((uint32_t *)currentBytePtr) > 0)
            {
              foundPosition = (currentBytePtr - (uint8_t *)(map->bytes)) * 8 + __builtin_ctz(htole32(*((uint32_t *)currentBytePtr)));
              rtn = true;
            }
            break;
        }
        if (rtn)
          break;
        currentBytePtr += bytesIncrement;
      }
      uint8_t bitsLeft = map->bitSize & swBitMapBitPositionMask;
      if (!rtn && (bitsLeft > 0))
      {
        uint8_t value = *currentBytePtr;
        for (uint8_t i = 0; i < bitsLeft; i++)
        {
          if (value & 0x01)
          {
            foundPosition = byteSize * 8 + i;
            rtn = true;
            break;
          }
          value = value >> 1;
        }
      }
    }
    if (rtn)
      *position = foundPosition;
  }
  return rtn;
}

bool swBitMapFindFirstClear(swBitMap *map, uint16_t *position)
{
  bool rtn = false;
  if (map && (map->bitCount < map->bitSize) && position)
  {
    uint16_t byteSize = map->bitSize >> 3;
    uint8_t *currentBytePtr = map->bytes;
    const uint8_t *endBytePtr = currentBytePtr + byteSize;
    uint16_t foundPosition = 0;
    if ((endBytePtr - currentBytePtr) >= (ssize_t)sizeof(uint32_t))
    {
      if (*((uint32_t *)currentBytePtr) < UINT32_MAX)
      {
        foundPosition = __builtin_ctz(~htole32(*((uint32_t *)currentBytePtr)));
        rtn = true;
      }
      else
      {
        currentBytePtr += sizeof(uint32_t);
        while ((endBytePtr - currentBytePtr) >= (ssize_t)sizeof(uint64_t))
        {
          if (*((uint64_t *)currentBytePtr) < UINT64_MAX)
          {
            foundPosition = (currentBytePtr - (uint8_t *)(map->bytes)) * 8 + __builtin_ctzl(~htole64(*((uint64_t *)currentBytePtr)));
            rtn = true;
            break;
          }
          currentBytePtr += sizeof(uint64_t);
        }
      }
    }
    if (!rtn)
    {
      while (currentBytePtr < endBytePtr)
      {
        uint16_t bytesIncrement = 0;
        switch (endBytePtr - currentBytePtr)
        {
          case 1:
            bytesIncrement = sizeof(uint8_t);
            if (*currentBytePtr < UINT8_MAX)
            {
              foundPosition = (currentBytePtr - (uint8_t *)(map->bytes)) * 8 + __builtin_ctz(~(*currentBytePtr));
              rtn = true;
            }
            break;
          case 2:
          case 3:
            bytesIncrement = sizeof(uint16_t);
            if (*((uint16_t *)currentBytePtr) < UINT16_MAX)
            {
              foundPosition = (currentBytePtr - (uint8_t *)(map->bytes)) * 8 + __builtin_ctz(~htole16(*((uint16_t *)currentBytePtr)));
              rtn = true;
            }
            break;
          case 4:
          case 5:
          case 6:
          case 7:
            bytesIncrement = sizeof(uint32_t);
            if (*((uint32_t *)currentBytePtr) < UINT32_MAX)
            {
              foundPosition = (currentBytePtr - (uint8_t *)(map->bytes)) * 8 + __builtin_ctz(~htole32(*((uint32_t *)currentBytePtr)));
              rtn = true;
            }
            break;
        }
        if (rtn)
          break;
        currentBytePtr += bytesIncrement;
      }
      uint8_t bitsLeft = map->bitSize & swBitMapBitPositionMask;
      if (!rtn && (bitsLeft > 0))
      {
        uint8_t value = ~(*currentBytePtr);
        for (uint8_t i = 0; i < bitsLeft; i++)
        {
          if (value & 0x01)
          {
            foundPosition = byteSize * 8 + i;
            rtn = true;
            break;
          }
          value = value >> 1;
        }
      }
    }
    if (rtn)
      *position = foundPosition;
  }
  return rtn;
}
