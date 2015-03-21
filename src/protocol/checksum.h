#ifndef SW_PROTOCOL_CHECKSUM_H
#define SW_PROTOCOL_CHECKSUM_H

#include "storage/static-buffer.h"

#include <stdio.h>

typedef struct swChecksum
{
  unsigned int final : 1;
  unsigned int checksum : 31;
} swChecksum;

#define swChecksumSetEmpty      *(swChecksum[]){{.final = 0, .checksum = 0 }}
#define swChecksumSet(c, f)     *(swChecksum[]){{.final = (f), .checksum = (c) }}
#define swChecksumGet(c)        (uint16_t)((c).checksum)

uint16_t swChecksumCalculate(swStaticBuffer *buffer);

void swChecksumAdd(swChecksum *checksum, swStaticBuffer *buffer);
void swChecksumReplace(swChecksum *checksum, swStaticBuffer *oldBuffer, swStaticBuffer *newBuffer);
static inline void swChecksumFinalize(swChecksum *checksum)
{
  if (checksum && !checksum->final)
  {
    uint32_t checksumValue = checksum->checksum;
    checksumValue = (checksumValue >> 16) + (checksumValue & 0xFFFF);
    checksumValue = (checksumValue >> 16) + (checksumValue & 0xFFFF);
    checksum->checksum = 0xFFFF & ~((uint16_t)checksumValue);
    // printf ("'%s': checksum = %08x\n", __func__, checksum->checksum);
    checksum->final = true;
  }
}

#endif  // SW_PROTOCOL_CHECKSUM_H
