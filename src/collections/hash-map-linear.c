#include "hash-map-linear.h"

#include <string.h>

#include <core/memory.h>

static inline void swHashMapLinearClearInternal(swHashMapLinear *map)
{
  map->count = 0;
  map->used = 0;

  if (map->keyDelete)
  {
    for (uint32_t i = 0; i < map->size; i++)
    {
      if (swHashIsReal(map->hashes[i]))
        map->keyDelete(map->keys[i]);
    }
  }
  if (map->valueDelete)
  {
    for (uint32_t i = 0; i < map->size; i++)
    {
      if (swHashIsReal(map->hashes[i]))
        map->valueDelete(map->values[i]);
    }
  }
  memset (map->hashes, 0, map->size * sizeof (uint32_t));
  memset (map->keys, 0, map->size * sizeof (void *));
  memset (map->values, 0, map->size * sizeof (void *));
}

swHashMapLinear *swHashMapLinearNew(swHashKeyHashFunction keyHash, swHashKeyEqualFunction keyEqual, swHashKeyDeleteFunction keyDelete, swHashValueDeleteFunction valueDelete)
{
  swHashMapLinear *rtn = swMemoryCalloc(1, sizeof(swHashMapLinear));
  if (!swHashMapLinearInit(rtn, keyHash, keyEqual, keyDelete, valueDelete))
  {
    swMemoryFree(rtn);
    rtn = NULL;
  }
  return rtn;
}

bool swHashMapLinearInit(swHashMapLinear *map, swHashKeyHashFunction keyHash, swHashKeyEqualFunction keyEqual, swHashKeyDeleteFunction keyDelete, swHashValueDeleteFunction valueDelete)
{
  bool rtn = false;
  if (map)
  {
    swHashShiftSet (SW_HASH_MIN_SHIFT, &map->size, &map->mod, &map->mask);
    if ((map->hashes = swMemoryCalloc(map->size, sizeof(uint32_t))))
    {
      if ((map->keys = swMemoryCalloc(map->size, sizeof(void *))))
      {
        if ((map->values = swMemoryCalloc(map->size, sizeof(void *))))
        {
          map->keyEqual     = keyEqual;
          map->keyHash      = (keyHash) ? keyHash : swHashPointerHash;
          map->keyDelete    = keyDelete;
          map->valueDelete  = valueDelete;
          rtn = true;
        }
      }
    }
    if (!rtn)
      swHashMapLinearRelease(map);
  }
  return rtn;
}

void swHashMapLinearDelete(swHashMapLinear *map)
{
  if (map)
  {
    swHashMapLinearRelease(map);
    swMemoryFree(map);
  }
}

void swHashMapLinearRelease(swHashMapLinear *map)
{
  if (map)
  {
    swHashMapLinearClearInternal(map);
    if (map->values)
      swMemoryFree(map->values);
    if (map->keys)
      swMemoryFree(map->keys);
    if (map->hashes)
      swMemoryFree(map->hashes);
    memset(map, 0, sizeof(*map));
  }
}


static inline uint32_t swHashMapLinearHashGet(swHashMapLinear *map, void *key)
{
  uint32_t hashValue = map->keyHash (key);
  if (!swHashIsReal(hashValue))
    hashValue = 2;
  return hashValue;
}

static inline bool swHashMapLinearInsertPositionFind(swHashMapLinear* map, void *key, uint32_t keyHash, uint32_t *position, bool replace)
{
  bool rtn = true;

  uint32_t nodeIndex = keyHash % map->mod;
  uint32_t nodeHash = map->hashes[nodeIndex];
  size_t step = 0;
  while (swHashIsReal(nodeHash))
  {
    if ((keyHash == nodeHash) && ((key == map->keys[nodeIndex]) || (map->keyEqual? map->keyEqual(key, map->keys[nodeIndex]) : false)))
    {
      if (!replace)
        rtn = false;
      break;
    }
    step++;
    // make sure we do not loop forever
    if (step > map->size)
    {
      rtn = false;
      break;
    }
    nodeIndex += step;
    nodeIndex &= map->mask;
    nodeHash = map->hashes[nodeIndex];
  }

  if (rtn)
    *position = nodeIndex;

  return rtn;
}

static bool swHashMapLinearResize(swHashMapLinear *map)
{
  bool rtn = false;

  uint32_t shift = swHashClosestShiftFind (map->count * 2);
  shift = (shift > SW_HASH_MIN_SHIFT)? ((shift < SW_HASH_MAX_SHIFT)? shift : SW_HASH_MAX_SHIFT): SW_HASH_MIN_SHIFT;

  size_t newSize = 0;
  uint32_t newMod = 0, newMask = 0;
  swHashShiftSet(shift, &newSize, &newMod, &newMask);

  void **newKeys = swMemoryCalloc(newSize, sizeof(void *));
  if (newKeys)
  {
    void **newValues = swMemoryCalloc(newSize, sizeof(void *));
    if (newValues)
    {
      uint32_t *newHashes = swMemoryCalloc(newSize, sizeof(uint32_t));
      if (newHashes)
      {
        rtn = true;
        for (uint32_t i = 0; i < map->size; i++)
        {
          uint32_t keyHash = map->hashes[i];
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
            newKeys[nodeIndex] = map->keys[i];
            newValues[nodeIndex] = map->values[i];
          }
          else
            break;
        }

        if (rtn)
        {
          swMemoryFree(map->keys);
          swMemoryFree(map->values);
          swMemoryFree(map->hashes);

          map->keys = newKeys;
          map->values = newValues;
          map->hashes = newHashes;
          map->size = newSize;
          map->mod = newMod;
          map->mask = newMask;
          map->used = map->count;
        }
        else
          swMemoryFree(newHashes);
      }
      if (!rtn)
        swMemoryFree (newValues);
    }
    if (!rtn)
      swMemoryFree (newKeys);
  }
  return rtn;
}

static void swHashMapLinearMaybeResize(swHashMapLinear *map)
{
  size_t used = map->used;
  size_t size = map->size;

  if (((size > (map->count * 4)) && (size > (1 << SW_HASH_MIN_SHIFT))) ||   // shrink
      ((size < (used + (size / 4))) && (size < (1UL << SW_HASH_MAX_SHIFT))))  // grow
    swHashMapLinearResize(map);
}

static bool swHashMapLinearInsertAtPosition(swHashMapLinear *map, void *key, void *value, uint32_t nodeIndex, uint32_t keyHash)
{
  bool rtn = false;

  uint32_t oldHash = map->hashes[nodeIndex];
  map->hashes[nodeIndex] = keyHash;
  map->keys[nodeIndex] = key;
  map->values[nodeIndex] = value;
  map->count++;
  if (swHashIsUnused(oldHash))
  {
    map->used++;
    swHashMapLinearMaybeResize (map);
  }
  rtn = true;
  return rtn;
}

bool swHashMapLinearInsert(swHashMapLinear *map, void *key, void *value)
{
  bool rtn = false;
  if (map && key)
  {
    uint32_t keyHash = swHashMapLinearHashGet(map, key);
    uint32_t nodeIndex = 0;
    if (swHashMapLinearInsertPositionFind(map, key, keyHash, &nodeIndex, false))
      rtn = swHashMapLinearInsertAtPosition(map, key, value, nodeIndex, keyHash);
  }
  return rtn;
}

static bool swHashMapLinearUpsertAtPosition(swHashMapLinear *map, void *key, void *value, uint32_t nodeIndex, uint32_t keyHash)
{
  bool rtn = false;

  uint32_t oldHash = map->hashes[nodeIndex];
  if (swHashIsReal(oldHash))
  {
    if (key != map->keys[nodeIndex])
    {
      if (map->keyDelete)
        map->keyDelete(map->keys[nodeIndex]);
      map->keys[nodeIndex] = key;
    }
    if (value != map->values[nodeIndex])
    {
      if (map->valueDelete)
        map->valueDelete(map->values[nodeIndex]);
      map->values[nodeIndex] = value;
    }
    rtn = true;
  }
  else
  {
    map->hashes[nodeIndex] = keyHash;
    map->keys[nodeIndex] = key;
    map->values[nodeIndex] = value;
    map->count++;
    if (swHashIsUnused(oldHash))
    {
      map->used++;
      swHashMapLinearMaybeResize (map);
    }
    rtn = true;
  }
  return rtn;
}

bool swHashMapLinearUpsert(swHashMapLinear *map, void *key, void *value)
{
  bool rtn = false;
  if (map && key)
  {
    uint32_t keyHash = swHashMapLinearHashGet(map, key);
    uint32_t nodeIndex = 0;
    if (swHashMapLinearInsertPositionFind(map, key, keyHash, &nodeIndex, true))
      rtn = swHashMapLinearUpsertAtPosition(map, key, value, nodeIndex, keyHash);
  }
  return rtn;
}

static inline bool swHashMapLinearRemovePositionFind(swHashMapLinear* map, void *key, uint32_t keyHash, uint32_t *position)
{
  bool rtn = false;

  uint32_t nodeIndex = keyHash % map->mod;
  uint32_t nodeHash = map->hashes[nodeIndex];
  size_t step = 0;
  while (!swHashIsUnused(nodeHash))
  {
    if (swHashIsReal(nodeHash) && (keyHash == nodeHash) &&
        ((key == map->keys[nodeIndex]) || (map->keyEqual? map->keyEqual(key, map->keys[nodeIndex]) : false)))
    {
      rtn = true;
      break;
    }
    step++;
    // make sure we do not loop forever
    if (step > map->size)
      break;
    nodeIndex += step;
    nodeIndex &= map->mask;
    nodeHash = map->hashes[nodeIndex];
  }

  if (rtn)
    *position = nodeIndex;

  return rtn;
}

static void swHashMapLinearRemoveAtPosition(swHashMapLinear *map, uint32_t nodeIndex)
{
  // Erect tombstone
  map->hashes[nodeIndex] = SW_HASH_TOMBSTONE;
  map->count--;

  void *key = map->keys[nodeIndex];
  map->keys[nodeIndex] = NULL;
  if (map->keyDelete)
    map->keyDelete(key);

  void *value = map->values[nodeIndex];
  map->values[nodeIndex] = NULL;
  if (map->valueDelete)
    map->valueDelete(value);

  swHashMapLinearMaybeResize(map);
}

bool swHashMapLinearRemove(swHashMapLinear *map, void *key)
{
  bool rtn = false;
  if (map && key)
  {
    uint32_t keyHash = swHashMapLinearHashGet(map, key);
    uint32_t nodeIndex = 0;
    if ((rtn = swHashMapLinearRemovePositionFind(map, key, keyHash, &nodeIndex)))
      swHashMapLinearRemoveAtPosition(map, nodeIndex);
  }
  return rtn;
}

void swHashMapLinearClear(swHashMapLinear *map)
{
  if (map)
  {
    swHashMapLinearClearInternal(map);
    swHashMapLinearResize(map);
  }
}

bool swHashMapLinearValueGet(swHashMapLinear *map, void *key, void **value)
{
  bool rtn = false;
  if (map && key)
  {
    uint32_t keyHash = swHashMapLinearHashGet(map, key);
    uint32_t nodeIndex = 0;
    if ((rtn = swHashMapLinearRemovePositionFind(map, key, keyHash, &nodeIndex)))
    {
      if (value)
        *value = map->values[nodeIndex];
    }
  }
  return rtn;
}

static void *swHashMapLinearExtractAtPosition(swHashMapLinear *map, uint32_t nodeIndex, void **value)
{
  // Erect tombstone
  map->hashes[nodeIndex] = SW_HASH_TOMBSTONE;
  map->count--;

  void *key = map->keys[nodeIndex];
  map->keys[nodeIndex] = NULL;

  if (value)
    *value = map->values[nodeIndex];
  else
  {
    if (map->valueDelete)
      map->valueDelete(map->values[nodeIndex]);
  }
  map->values[nodeIndex] = NULL;

  swHashMapLinearMaybeResize(map);
  return key;
}

void *swHashMapLinearExtract(swHashMapLinear *map, void *key, void **value)
{
  void *rtn = NULL;
  if (map && key)
  {
    uint32_t keyHash = swHashMapLinearHashGet(map, key);
    uint32_t nodeIndex = 0;
    if (swHashMapLinearRemovePositionFind(map, key, keyHash, &nodeIndex))
      rtn = swHashMapLinearExtractAtPosition(map, nodeIndex, value);
  }
  return rtn;
}

size_t swHashMapLinearCount(swHashMapLinear *map)
{
  if (map)
    return map->count;
  return 0;
}

swHashMapLinearIterator *swHashMapLinearIteratorNew(swHashMapLinear *map)
{
  swHashMapLinearIterator *rtn = NULL;
  if (map)
  {
    swHashMapLinearIterator *iter = swMemoryCalloc(1, sizeof(swHashMapLinearIterator));
    if (iter)
    {
      if (swHashMapLinearIteratorInit(iter, map))
        rtn = iter;
      else
        swMemoryFree(iter);
    }
  }
  return rtn;
}

bool swHashMapLinearIteratorInit(swHashMapLinearIterator *iter, swHashMapLinear *map)
{
  bool rtn = false;
  if (iter && map)
  {
    iter->map = map;
    iter->position = SW_HASH_ITER_END_POSITION;
  }
  return rtn;
}

void *swHashMapLinearIteratorNext(swHashMapLinearIterator *iter, void **value)
{
  void *rtn = NULL;
  if (iter && iter->map->count)
  {
    iter->position++;
    while (iter->position < iter->map->size)
    {
      if (swHashIsReal(iter->map->hashes[iter->position]))
        break;
      iter->position++;
    }
    if (iter->position < iter->map->size)
    {
      rtn = iter->map->keys[iter->position];
      if (value)
        *value = iter->map->values[iter->position];
    }
  }
  return rtn;
}

bool swHashMapLinearIteratorReset(swHashMapLinearIterator *iter)
{
  bool rtn = false;
  if (iter)
  {
    iter->position = SW_HASH_ITER_END_POSITION;
    rtn = true;
  }
  return rtn;
}

void swHashMapLinearIteratorDelete(swHashMapLinearIterator *iter)
{
  if (iter)
    swMemoryFree(iter);
}
