#ifndef SW_COLLECTIONS_HASHFUNCTIONS_H
#define SW_COLLECTIONS_HASHFUNCTIONS_H

#include <stdint.h>

typedef unsigned __int128 uint128_t;

uint32_t  swMurmurHash3_32(const void *key, int len);
uint64_t  swMurmurHash3_64(const void *key, int len);
uint128_t swMurmurHash3_128(const void *key, int len);

uint32_t swDJBAlgoHash(const void *key, uint32_t len);

#endif // SW_COLLECTIONS_HASHFUNCTIONS_H