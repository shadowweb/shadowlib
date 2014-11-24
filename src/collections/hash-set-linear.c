#include "hash-set-linear.h"

#include <string.h>

#include <core/memory.h>

static inline void swHashSetLinearClearInternal(swHashSetLinear *set)
{
  set->count = 0;
  set->used = 0;

  if (set->keyDelete)
  {
    for (uint32_t i = 0; i < set->size; i++)
    {
      if (swHashIsReal(set->hashes[i]))
        set->keyDelete(set->keys[i]);
    }
  }
  memset (set->hashes, 0, set->size * sizeof (uint32_t));
  memset (set->keys, 0, set->size * sizeof (void *));
}

swHashSetLinear *swHashSetLinearNew(swHashKeyHashFunction keyHash, swHashKeyEqualFunction keyEqual, swHashKeyDeleteFunction keyDelete)
{
  swHashSetLinear *rtn = NULL;
  swHashSetLinear *set = swMemoryCalloc(1, sizeof(swHashSetLinear));
  if (set)
  {
    swHashShiftSet (SW_HASH_MIN_SHIFT, &set->size, &set->mod, &set->mask);
    if ((set->hashes = swMemoryCalloc(set->size, sizeof(uint32_t))))
    {
      if ((set->keys = swMemoryCalloc(set->size, sizeof(void *))))
      {
        set->keyEqual    = keyEqual;
        set->keyHash     = (keyHash) ? keyHash : swHashPointerHash;
        set->keyDelete   = keyDelete;
        rtn = set;
      }
    }
    if (!rtn)
      swHashSetLinearDelete(set);
  }
  return rtn;
}

void swHashSetLinearDelete (swHashSetLinear *set)
{
  if (set)
  {
    swHashSetLinearClearInternal(set);
    if (set->keys)
      swMemoryFree(set->keys);
    if (set->hashes)
      swMemoryFree(set->hashes);
    swMemoryFree(set);
  }
}

static inline uint32_t swHashSetLinearHashGet(swHashSetLinear *set, void *key)
{
  uint32_t hashValue = set->keyHash (key);
  if (!swHashIsReal(hashValue))
    hashValue = 2;
  return hashValue;
}

static inline bool swHashSetLinearInsertPositionFind(swHashSetLinear* set, void *key, uint32_t keyHash, uint32_t *position, bool replace)
{
  bool rtn = true;

  uint32_t nodeIndex = keyHash % set->mod;
  uint32_t nodeHash = set->hashes[nodeIndex];
  size_t step = 0;
  while (swHashIsReal(nodeHash))
  {
    if ((keyHash == nodeHash) && ((key == set->keys[nodeIndex]) || (set->keyEqual? set->keyEqual(key, set->keys[nodeIndex]) : false)))
    {
      if (!replace)
        rtn = false;
      break;
    }
    step++;
    // make sure we do not loop forever
    if (step > set->size)
    {
      rtn = false;
      break;
    }
    nodeIndex += step;
    nodeIndex &= set->mask;
    nodeHash = set->hashes[nodeIndex];
  }

  if (rtn)
    *position = nodeIndex;

  return rtn;
}

static bool swHashSetLinearResize(swHashSetLinear *set)
{
  bool rtn = false;

  uint32_t shift = swHashClosestShiftFind (set->count * 2);
  shift = (shift > SW_HASH_MIN_SHIFT)? ((shift < SW_HASH_MAX_SHIFT)? shift : SW_HASH_MAX_SHIFT): SW_HASH_MIN_SHIFT;

  size_t newSize = 0;
  uint32_t newMod = 0, newMask = 0;
  swHashShiftSet(shift, &newSize, &newMod, &newMask);

  void **newKeys = swMemoryCalloc(newSize, sizeof(void *));
  if (newKeys)
  {
    uint32_t *newHashes = swMemoryCalloc(newSize, sizeof(uint32_t));
    if (newHashes)
    {
      rtn = true;
      for (uint32_t i = 0; i < set->size; i++)
      {
        uint32_t keyHash = set->hashes[i];
        if (!swHashIsReal(keyHash))
            continue;

        uint32_t nodeIndex = keyHash % newMod;
        uint32_t nodeHash = newHashes[nodeIndex];
        uint32_t step = 0;
        while (swHashIsReal(nodeHash))
        {
          step++;
          // make sure we do not loop forever
          if (step > newSize)
          {
            rtn = false;
            break;
          }
          nodeIndex += step;
          nodeIndex &= newMask;
          nodeHash = newHashes[nodeIndex];
        }

        if (rtn)
        {
          newHashes[nodeIndex] = keyHash;
          newKeys[nodeIndex] = set->keys[i];
        }
        else
          break;
      }

      if (rtn)
      {
        swMemoryFree(set->keys);
        swMemoryFree(set->hashes);

        set->keys = newKeys;
        set->hashes = newHashes;
        set->size = newSize;
        set->mod = newMod;
        set->mask = newMask;
        set->used = set->count;
      }
      else
        swMemoryFree(newHashes);
    }
    if (!rtn)
      swMemoryFree (newKeys);
  }
  return rtn;
}

static void swHashSetLinearMaybeResize(swHashSetLinear *set)
{
  size_t used = set->used;
  size_t size = set->size;

  if (((size > (set->count * 4)) && (size > (1 << SW_HASH_MIN_SHIFT))) ||   // shrink
      ((size < (used + (size / 4))) && (size < (1UL << SW_HASH_MAX_SHIFT))))  // grow
    swHashSetLinearResize(set);
}

static bool swHashSetLinearInsertAtPosition(swHashSetLinear *set, void *key, uint32_t nodeIndex, uint32_t keyHash)
{
  bool rtn = false;

  uint32_t oldHash = set->hashes[nodeIndex];
  set->hashes[nodeIndex] = keyHash;
  set->keys[nodeIndex] = key;
  set->count++;
  if (swHashIsUnused(oldHash))
  {
    set->used++;
    swHashSetLinearMaybeResize (set);
  }
  rtn = true;
  return rtn;
}

bool swHashSetLinearInsert(swHashSetLinear *set, void *key)
{
  bool rtn = false;
  if (set && key)
  {
    uint32_t keyHash = swHashSetLinearHashGet(set, key);
    uint32_t nodeIndex = 0;
    if (swHashSetLinearInsertPositionFind(set, key, keyHash, &nodeIndex, false))
      rtn = swHashSetLinearInsertAtPosition(set, key, nodeIndex, keyHash);
  }
  return rtn;
}

static bool swHashSetLinearUpsertAtPosition(swHashSetLinear *set, void *key, uint32_t nodeIndex, uint32_t keyHash)
{
  bool rtn = false;

  uint32_t oldHash = set->hashes[nodeIndex];
  if (swHashIsReal(oldHash))
  {
    if (key != set->keys[nodeIndex])
    {
      if (set->keyDelete)
        set->keyDelete(set->keys[nodeIndex]);
      set->keys[nodeIndex] = key;
    }
    rtn = true;
  }
  else
  {
    set->hashes[nodeIndex] = keyHash;
    set->keys[nodeIndex] = key;
    set->count++;
    if (swHashIsUnused(oldHash))
    {
      set->used++;
      swHashSetLinearMaybeResize (set);
    }
    rtn = true;
  }
  return rtn;
}

bool swHashSetLinearUpsert(swHashSetLinear *set, void *key)
{
  bool rtn = false;
  if (set && key)
  {
    uint32_t keyHash = swHashSetLinearHashGet(set, key);
    uint32_t nodeIndex = 0;
    if (swHashSetLinearInsertPositionFind(set, key, keyHash, &nodeIndex, true))
      rtn = swHashSetLinearUpsertAtPosition(set, key, nodeIndex, keyHash);
  }
  return rtn;
}

static inline bool swHashSetLinearRemovePositionFind(swHashSetLinear* set, void *key, uint32_t keyHash, uint32_t *position)
{
  bool rtn = false;

  uint32_t nodeIndex = keyHash % set->mod;
  uint32_t nodeHash = set->hashes[nodeIndex];
  size_t step = 0;
  while (!swHashIsUnused(nodeHash))
  {
    if (swHashIsReal(nodeHash) && (keyHash == nodeHash) &&
        ((key == set->keys[nodeIndex]) || (set->keyEqual? set->keyEqual(key, set->keys[nodeIndex]) : false)))
    {
      rtn = true;
      break;
    }
    step++;
    // make sure we do not loop forever
    if (step > set->size)
      break;
    nodeIndex += step;
    nodeIndex &= set->mask;
    nodeHash = set->hashes[nodeIndex];
  }

  if (rtn)
    *position = nodeIndex;

  return rtn;
}

static void swHashSetLinearRemoveAtPosition(swHashSetLinear *set, uint32_t nodeIndex)
{
  // Erect tombstone
  set->hashes[nodeIndex] = SW_HASH_TOMBSTONE;
  set->count--;

  void *key = set->keys[nodeIndex];
  set->keys[nodeIndex] = NULL;
  if (set->keyDelete)
    set->keyDelete(key);

  swHashSetLinearMaybeResize(set);
}

bool swHashSetLinearRemove(swHashSetLinear *set, void *key)
{
  bool rtn = false;
  if (set && key)
  {
    uint32_t keyHash = swHashSetLinearHashGet(set, key);
    uint32_t nodeIndex = 0;
    if ((rtn = swHashSetLinearRemovePositionFind(set, key, keyHash, &nodeIndex)))
      swHashSetLinearRemoveAtPosition(set, nodeIndex);
  }
  return rtn;
}

void swHashSetLinearClear(swHashSetLinear *set)
{
  if (set)
  {
    swHashSetLinearClearInternal(set);
    swHashSetLinearResize(set);
  }
}

bool swHashSetLinearContains(swHashSetLinear *set, void *key)
{
  bool rtn = false;
  if (set && key)
  {
    uint32_t keyHash = swHashSetLinearHashGet(set, key);
    uint32_t nodeIndex = 0;
    rtn = swHashSetLinearRemovePositionFind(set, key, keyHash, &nodeIndex);
  }
  return rtn;
}

static void *swHashSetLinearExtractAtPosition(swHashSetLinear *set, uint32_t nodeIndex)
{
  // Erect tombstone
  set->hashes[nodeIndex] = SW_HASH_TOMBSTONE;
  set->count--;

  void *key = set->keys[nodeIndex];
  set->keys[nodeIndex] = NULL;

  swHashSetLinearMaybeResize(set);
  return key;
}

void *swHashSetLinearExtract(swHashSetLinear *set, void *key)
{
  void *rtn = NULL;
  if (set && key)
  {
    uint32_t keyHash = swHashSetLinearHashGet(set, key);
    uint32_t nodeIndex = 0;
    if (swHashSetLinearRemovePositionFind(set, key, keyHash, &nodeIndex))
      rtn = swHashSetLinearExtractAtPosition(set, nodeIndex);
  }
  return rtn;
}

size_t swHashSetLinearCount(swHashSetLinear *set)
{
  if (set)
    return set->count;
  return 0;
}

swHashSetLinearIterator *swHashSetLinearIteratorNew(swHashSetLinear *set)
{
  swHashSetLinearIterator *rtn = NULL;
  if (set)
  {
    swHashSetLinearIterator *iter = swMemoryCalloc(1, sizeof(swHashSetLinearIterator));
    if (iter)
    {
      if (swHashSetLinearIteratorInit(iter, set))
        rtn = iter;
      else
        swMemoryFree(iter);
    }
  }
  return rtn;
}

bool swHashSetLinearIteratorInit(swHashSetLinearIterator *iter, swHashSetLinear *set)
{
  bool rtn = false;
  if (iter && set)
  {
    iter->set = set;
    iter->position = SW_HASH_ITER_END_POSITION;
    rtn = true;
  }
  return rtn;
}

void *swHashSetLinearIteratorNext(swHashSetLinearIterator *iter)
{
  void *rtn = NULL;
  if (iter && iter->set->count)
  {
    iter->position++;
    while (iter->position < iter->set->size)
    {
      if (swHashIsReal(iter->set->hashes[iter->position]))
        break;
      iter->position++;
    }
    if (iter->position < iter->set->size)
      rtn = iter->set->keys[iter->position];
  }
  return rtn;
}

bool swHashSetLinearIteratorReset(swHashSetLinearIterator *iter)
{
  bool rtn = false;
  if (iter)
  {
    iter->position = SW_HASH_ITER_END_POSITION;
    rtn = true;
  }
  return rtn;
}

void swHashSetLinearIteratorDelete(swHashSetLinearIterator *iter)
{
  if (iter)
    swMemoryFree(iter);
}
