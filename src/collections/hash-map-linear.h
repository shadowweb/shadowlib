#ifndef SW_COLLECTIONS_HASHMAPLINEAR_H
#define SW_COLLECTIONS_HASHMAPLINEAR_H

#include "hash-common.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct swHashMapLinear
{
  uint32_t *hashes;
  void    **keys;
  void    **values;

  swHashKeyEqualFunction    keyEqual;
  swHashKeyHashFunction     keyHash;
  swHashKeyDeleteFunction   keyDelete;
  swHashValueDeleteFunction valueDelete;

  size_t    size;   // total nodes allocated
  size_t    count;  // nodes used by real values
  size_t    used;   // nodes used (real + tombstones)
  uint32_t  mod;
  uint32_t  mask;
} swHashMapLinear;

typedef struct swHashMapLinearIterator
{
  swHashMapLinear *map;
  size_t position;
} swHashMapLinearIterator;

swHashMapLinear *swHashMapLinearNew(swHashKeyHashFunction keyHash, swHashKeyEqualFunction keyEqual, swHashKeyDeleteFunction keyDelete, swHashValueDeleteFunction valueDelete);
void    swHashMapLinearDelete(swHashMapLinear *map);
bool    swHashMapLinearInsert(swHashMapLinear *map, void *key, void *value);
bool    swHashMapLinearUpsert(swHashMapLinear *map, void *key, void *value);
bool    swHashMapLinearRemove(swHashMapLinear *map, void *key);
void    swHashMapLinearClear(swHashMapLinear *map);
bool    swHashMapLinearValueGet(swHashMapLinear *map, void *key, void **value);
void   *swHashMapLinearExtract(swHashMapLinear *map, void *key, void **value);
size_t  swHashMapLinearCount(swHashMapLinear *map);

swHashMapLinearIterator *swHashMapLinearIteratorNew(swHashMapLinear *map);
bool    swHashMapLinearIteratorInit(swHashMapLinearIterator *iter, swHashMapLinear *map);
void   *swHashMapLinearIteratorNext(swHashMapLinearIterator *iter, void **value);
bool    swHashMapLinearIteratorReset(swHashMapLinearIterator *iter);
void    swHashMapLinearIteratorDelete(swHashMapLinearIterator *iter);

#endif // SW_COLLECTIONS_HASHMAPLINEAR_H