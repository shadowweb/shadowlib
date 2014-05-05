#ifndef SW_COLLECTIONS_HASHSETLINEAR_H
#define SW_COLLECTIONS_HASHSETLINEAR_H

#include "hash-common.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct swHashSetLinear
{
  uint32_t *hashes;
  void    **keys;

  swHashKeyEqualFunction  keyEqual;
  swHashKeyHashFunction   keyHash;
  swHashKeyDeleteFunction keyDelete;

  size_t    size;   // total nodes allocated
  size_t    count;  // nodes used by real values
  size_t    used;   // nodes used (real + tombstones)
  uint32_t  mod;
  uint32_t  mask;
} swHashSetLinear;

typedef struct swHashSetLinearIterator
{
  swHashSetLinear *set;
  size_t position;
} swHashSetLinearIterator;

swHashSetLinear *swHashSetLinearNew(swHashKeyHashFunction keyHash, swHashKeyEqualFunction keyEqual, swHashKeyDeleteFunction keyDelete);
void    swHashSetLinearDelete(swHashSetLinear *set);
bool    swHashSetLinearInsert(swHashSetLinear *set, void *key);
bool    swHashSetLinearUpsert(swHashSetLinear *set, void *key);
bool    swHashSetLinearRemove(swHashSetLinear *set, void *key);
void    swHashSetLinearClear(swHashSetLinear *set);
bool    swHashSetLinearContains(swHashSetLinear *set, void *key);
void   *swHashSetLinearExtract(swHashSetLinear *set, void *key);
size_t  swHashSetLinearCount(swHashSetLinear *set);

swHashSetLinearIterator *swHashSetLinearIteratorNew(swHashSetLinear *set);
bool    swHashSetLinearIteratorInit(swHashSetLinearIterator *iter, swHashSetLinear *set);
void   *swHashSetLinearIteratorNext(swHashSetLinearIterator *iter);
bool    swHashSetLinearIteratorReset(swHashSetLinearIterator *iter);
void    swHashSetLinearIteratorDelete(swHashSetLinearIterator *iter);


#endif // SW_COLLECTIONS_HASHSETLINEAR_H