#include "murmur-hash3.h"
#include "hash-functions.h"

static uint32_t seed = 0xB0F57EE3;

uint32_t swMurmurHash3_32(const void *key, int len)
{
  uint32_t ret;
  swMurmurHash3_x86_32(key, len, seed, &ret);
  return ret;
}

uint64_t swMurmurHash_3_64(const void *key, int len)
{
  uint64_t out[2];
  swMurmurHash3_x64_128(key, len, seed, out);
  return out[0];
}

uint128_t swMurmurHash3_128(const void *key, int len)
{
  uint128_t ret;
  swMurmurHash3_x64_128(key, len, seed, &ret);
  return ret;
}

uint32_t swDJBAlgoHash(const void *key, uint32_t len)
{
  const signed char *pointer = key;
  uint32_t hash = 5381;

  for (uint32_t i = 0; i < len; i++)
    hash = (hash << 5) + hash + *pointer++;
  return hash;
}