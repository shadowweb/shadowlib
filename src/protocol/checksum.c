#include "protocol/checksum.h"

#include <stdio.h>

uint16_t swChecksumCalculate(swStaticBuffer *buffer)
{
  size_t left = buffer->len;
  uint32_t sum = 0;
  uint16_t *current_ptr = (uint16_t *)(buffer->data);

  while (left > 1) {
    sum += *current_ptr++;
    left -= sizeof (uint16_t);
  }
  if (left == 1) {
    uint32_t tmp = 0;
    *(uint8_t *) (&tmp) = *(uint8_t *) current_ptr;
    sum += tmp;
  }
  // printf ("'%s': before complement checksum = %08x\n", __func__, sum);

  sum = (sum >> 16) + (sum & 0xFFFF);
  // potentially, this can leave 1 in the upper 16 bits,
  // but it will be ignored when it is converted to 16 bit integer
  sum += (sum >> 16);
  return 0xFFFF & ~((uint16_t)sum);
  // printf ("'%s': checksum = %08x\n", __func__, sum);
  // return sum;
}

static inline uint32_t swChecksumInternalAdd(uint32_t checksum, swStaticBuffer *buffer)
{
  size_t left = buffer->len;
  uint16_t *currentPtr = (uint16_t *)(buffer->data);

  while (left > 1)
  {
    checksum += *currentPtr;
    ++currentPtr;
    left -= sizeof (uint16_t);
  }
  if (left == 1)
  {
    uint32_t tmp = 0;
    *(uint8_t *) (&tmp) = *(uint8_t *) currentPtr;
    checksum += tmp;
  }
  // printf ("'%s': checksum = %x\n", __func__, checksum);
  return checksum;
}

static inline uint32_t swChecksumInternalSubtract(uint32_t checksum, swStaticBuffer *buffer)
{
  size_t left = buffer->len;
  uint16_t *currentPtr = (uint16_t *)(buffer->data);

  while (left > 1)
  {
    checksum += 0xFFFF & ~(*currentPtr);
    ++currentPtr;
    left -= sizeof (uint16_t);
  }
  if (left == 1)
  {
    uint32_t tmp = 0;
    *(uint8_t *) (&tmp) = *(uint8_t *)currentPtr;
    checksum += 0xFFFF & ~tmp;
  }
  return checksum;
}

void swChecksumAdd(swChecksum *checksum, swStaticBuffer *buffer)
{
  if (checksum && buffer)
  {
    uint32_t checksumValue = checksum->checksum;
    // printf ("'%s': Start checksum = %08x\n", __func__, checksumValue);
    if (checksum->final)
      checksumValue = 0xFFFF & ~((uint16_t)checksumValue);

    checksum->checksum = checksumValue = swChecksumInternalAdd(checksumValue, buffer);

    if (checksum->final)
    {
      checksumValue = (checksumValue >> 16) + (checksumValue & 0xFFFF);
      checksumValue = (checksumValue >> 16) + (checksumValue & 0xFFFF);
      checksum->checksum = 0xFFFF & ~((uint16_t)checksumValue);
    }
    // printf ("'%s': End checksum = %08x\n", __func__, checksum->checksum);
  }
}

void swChecksumReplace(swChecksum *checksum, swStaticBuffer *oldBuffer, swStaticBuffer *newBuffer)
{
  if (checksum && oldBuffer && newBuffer && (oldBuffer->len == newBuffer->len))
  {
    uint32_t checksumValue = checksum->checksum;
    // printf ("'%s': Start checksum = %08x\n", __func__, checksumValue);
    if (checksum->final)
      checksumValue = 0xFFFF & ~((uint16_t)checksumValue);

    checksumValue = swChecksumInternalSubtract(checksumValue, oldBuffer);
    // printf ("'%s': Intermediate checksum = %08x\n", __func__, checksumValue);
    checksumValue = swChecksumInternalAdd(checksumValue, newBuffer);
    checksum->checksum = checksumValue;

    if (checksum->final)
    {
      checksumValue = (checksumValue >> 16) + (checksumValue & 0xFFFF);
      checksumValue = (checksumValue >> 16) + (checksumValue & 0xFFFF);
      checksum->checksum = 0xFFFF & ~((uint16_t)checksumValue);
    }
    // printf ("'%s': End checksum = %08x\n", __func__, checksum->checksum);
  }
}
