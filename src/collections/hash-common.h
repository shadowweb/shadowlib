#ifndef SW_COLLECTIONS_HASHCOMMON_H
#define SW_COLLECTIONS_HASHCOMMON_H

#include "hash-functions.h"

#include <stdbool.h>
#include <stddef.h>

typedef bool     (*swHashKeyEqualFunction)(const void *key1, const void *key2);
typedef uint32_t (*swHashKeyHashFunction)(const void * key);
typedef void     (*swHashKeyDeleteFunction)(void *data);
typedef void     (*swHashValueDeleteFunction)(void *data);

#define SW_HASH_UNUSED      0
#define SW_HASH_TOMBSTONE   1

#define swHashIsUnused(h)     ((h) == SW_HASH_UNUSED)
#define swHashIsTombstone(h)  ((h) == SW_HASH_TOMBSTONE)
#define swHashIsReal(h)       ((h) > SW_HASH_TOMBSTONE)

#define SW_HASH_MIN_SHIFT 3  // 1 << 3 == 8
#define SW_HASH_MAX_SHIFT 32  // 1 << 32 == 2**32

#define SW_HASH_ITER_END_POSITION   (~0UL)

void      swHashShiftSet (uint32_t shift, size_t *size, uint32_t *mod, uint32_t *mask);
uint32_t  swHashClosestShiftFind (uint32_t n);

uint32_t  swHashPointerHash(const void *data);

/*
typedef enum
{
  swHashLinear,
  swHashQuadratic,
  swHashBucket
} swHashType;
*/

#endif
