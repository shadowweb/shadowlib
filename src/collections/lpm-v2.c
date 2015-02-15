#include "collections/lpm-v2.h"
#include "core/memory.h"

#include <string.h>
#include <sys/param.h>

#define SW_LPM_PREFIX_STORAGE_POSITION(i, v, c, p)  ((!(i))? (v) : ((c) - (2 << ((p) + 1)) + (v)))
#define SW_LPM_PREFIX_STORAGE_INDEX(p, f)           (!((p) == (uint32_t)((f) - 1)))
// mask for bit positions in 64 bit word ((1 << 6) - 1)
#define SW_LPM_BIT_POSITION_MASK                    0x3F

static inline swLPMV2Prefix *swLPMV2NodeGetPrefix(swLPMV2Node *node, uint32_t prefixPosition, uint8_t value, uint16_t nodeCount, uint8_t factor)
{
  swLPMV2Prefix *rtn = NULL;
  uint8_t storageIndex = SW_LPM_PREFIX_STORAGE_INDEX(prefixPosition, factor);
  if (node->prefix[storageIndex])
  {
    uint16_t storagePosition = SW_LPM_PREFIX_STORAGE_POSITION(storageIndex, value, nodeCount, prefixPosition);
    rtn = node->prefix[storageIndex][storagePosition];
  }
  return rtn;
}

static inline bool swLPMV2NodeSetPrefix(swLPMV2Node *node, uint32_t prefixPosition, uint8_t value, uint16_t nodeCount, uint8_t factor, swLPMV2Prefix *prefix)
{
  bool rtn = false;
  uint8_t storageIndex = SW_LPM_PREFIX_STORAGE_INDEX(prefixPosition, factor);
  if (!node->prefix[storageIndex])
    node->prefix[storageIndex] = swMemoryCalloc(nodeCount, sizeof(swLPMV2Prefix *));
  if (node->prefix[storageIndex])
  {
    uint16_t storagePosition = SW_LPM_PREFIX_STORAGE_POSITION(storageIndex, value, nodeCount, prefixPosition);
    node->prefix[storageIndex][storagePosition] = prefix;
    uint8_t index = storagePosition >> 6;
    uint8_t bitPosition = storagePosition & SW_LPM_BIT_POSITION_MASK;
    uint64_t mask = (uint64_t)1UL << bitPosition;
    if (!(node->bitMaps[storageIndex][index] & mask))
    {
      node->bitMaps[storageIndex][index] |= mask;
      node->prefixCount[storageIndex]++;
    }
    rtn = true;
  }
  return rtn;
}

static inline void swLPMV2NodeClearPrefix(swLPMV2Node *node, uint32_t prefixPosition, uint8_t value, uint16_t nodeCount, uint8_t factor)
{
  uint8_t storageIndex = SW_LPM_PREFIX_STORAGE_INDEX(prefixPosition, factor);
  if (node->prefix[storageIndex])
  {
    uint16_t storagePosition = SW_LPM_PREFIX_STORAGE_POSITION(storageIndex, value, nodeCount, prefixPosition);
    node->prefix[storageIndex][storagePosition] = NULL;
    uint8_t index = storagePosition >> 6;
    uint8_t bitPosition = storagePosition & SW_LPM_BIT_POSITION_MASK;
    uint64_t mask = (uint64_t)1UL << bitPosition;
    if ((node->bitMaps[storageIndex][index] & mask))
    {
      node->bitMaps[storageIndex][index] &= (~mask);
      node->prefixCount[storageIndex]--;
    }
  }
}

static inline bool swLPMV2NodePrefixIsClear(swLPMV2Node *node, uint32_t prefixPosition, uint8_t value, uint16_t nodeCount, uint8_t factor)
{
  bool rtn = false;
  uint8_t storageIndex = SW_LPM_PREFIX_STORAGE_INDEX(prefixPosition, factor);
  if (node->prefix[storageIndex])
  {
    uint16_t storagePosition = SW_LPM_PREFIX_STORAGE_POSITION(storageIndex, value, nodeCount, prefixPosition);
    uint8_t index = storagePosition >> 6;
    uint8_t bitPosition = storagePosition & SW_LPM_BIT_POSITION_MASK;
    uint64_t mask = (uint64_t)1UL << bitPosition;
    if (!(node->bitMaps[storageIndex][index] & mask))
      rtn = true;
  }
  else
    rtn = true;
  return rtn;
}

swLPMV2Node *swLPMV2NodeNew(uint16_t nodeCount, uint8_t factor)
{
  swLPMV2Node *rtn = NULL;
  if (nodeCount)
  {
    swLPMV2Node *node = swMemoryCalloc(1, sizeof(swLPMV2Node) + nodeCount * sizeof(swLPMV2Node *));
    if (node)
      rtn = node;
  }
  return rtn;
}

void swLPMV2NodeDelete(swLPMV2Node *lpmNode, uint16_t nodeCount, uint8_t factor, bool freeNode)
{
  if (lpmNode && nodeCount && factor)
  {
    for (uint8_t j = 0; j < 2; j++)
    {
      if (lpmNode->prefix[j])
        swMemoryFree(lpmNode->prefix[j]);
    }
    swLPMV2Node **nodes = lpmNode->nodes;
    for (uint16_t i = 0; i < nodeCount; i++)
    {
      if (nodes[i])
        swLPMV2NodeDelete(nodes[i], nodeCount, factor, true);
    }
    if (freeNode)
      swMemoryFree(lpmNode);
  }
}

swLPMV2 *swLPMV2New(uint8_t factor)
{
  swLPMV2 *rtn = NULL;
  if (factor >= SW_LPM_MIN_FACTOR && factor <= SW_LPM_MAX_FACTOR)
  {
    uint16_t nodeCount = (uint16_t)1 << factor;
    swLPMV2 *lpm = swMemoryCalloc(1, sizeof(swLPMV2) + nodeCount * sizeof(swLPMV2Node *));
    if (lpm)
    {
      lpm->nodeCount  = nodeCount;
      lpm->factor     = factor;
      rtn = lpm;
    }
  }
  return rtn;
}

void swLPMV2Delete(swLPMV2 *lpm)
{
  if (lpm)
  {
    swLPMV2NodeDelete(&(lpm->rootNode), lpm->nodeCount, lpm->factor, false);
    swMemoryFree(lpm);
  }
}

#define SW_LPM_PREFIX_BYTES_FROM_LEN(l)  (((l) >> 3) + (((l) & 7) > 0))

static inline bool _getSlice(uint8_t *data, size_t totalBytes, size_t totalBits, uint16_t startPosition, uint8_t factor, uint8_t *value)
{
  bool rtn = false;
  if (data && totalBytes && factor && startPosition <totalBits && value)
  {
    uint8_t startByte = startPosition >> 3;
    // uint8_t endByte = endPosition >> 3; // it can only be startByte + 1
    uint8_t startBit = 7 - (startPosition & 7);
    uint16_t endPosition = startPosition + factor - 1;
    if (endPosition >= totalBits)
      endPosition = totalBits - 1;
    uint8_t endBit = 7 - (endPosition & 7);
    uint16_t adjustedFactor = endPosition - startPosition + 1;
    uint16_t mask = (1 << adjustedFactor) - 1;
    if (startBit >= endBit)
      *value = (data[startByte] >> endBit) & mask;
    else
      *value = ((data[startByte] << (adjustedFactor - startBit - 1)) + (data[startByte + 1] >> endBit)) & mask;
    rtn = true;
  }
  return rtn;
}

static inline bool swLPMV2PrefixGetSlice(swLPMV2Prefix *prefix, uint16_t startPosition, uint8_t factor, uint8_t *value)
{
  bool rtn = 0;
  // TODO: store totalBytes inside prefix (that way we do not need to recalculate it every time)
  if (prefix)
    rtn = _getSlice(prefix->prefixBytes, swLPMV2PrefixBytes(prefix), prefix->len, startPosition, factor, value);
  return rtn;
}

#define SW_LPM_PREFIX_IS_FINAL(x)       (!((uint64_t)(x) & 0x01))
#define SW_LPM_PREFIX_CLEAR_FLAG(x)     (swLPMV2Prefix *)((uint64_t)(x) & (~1UL))
#define SW_LPM_PREFIX_SET_FLAG(x, n)    (swLPMV2Prefix *)((uint64_t)(x) | (n))

static bool swLPMV2NodeInsert(swLPMV2 *lpm, swLPMV2Node *node, swLPMV2Prefix *prefix, swLPMV2Prefix **foundPrefix, uint16_t offset)
{
  bool rtn = false;
  swLPMV2Node *currentNode = node;
  uint16_t i = offset;
  uint8_t value = 0;
  bool final = false;
  bool success = false;
  uint32_t prefixPosition = 0;
  swLPMV2Prefix *storedPrefix = NULL;
  bool storedPrefixFinal = false;
  while ((i < prefix->len) && currentNode)
  {
    value = 0;
    final = (i + lpm->factor >= prefix->len);
    if ((success = swLPMV2PrefixGetSlice(prefix, i, lpm->factor, &value)))
    {
      prefixPosition = (!final)? (lpm->factor - 1) : (prefix->len - 1 - i);
      if ((storedPrefix = swLPMV2NodeGetPrefix(currentNode, prefixPosition, value, lpm->nodeCount, lpm->factor)))
      {
        storedPrefixFinal = SW_LPM_PREFIX_IS_FINAL(storedPrefix);
        storedPrefix = SW_LPM_PREFIX_CLEAR_FLAG(storedPrefix);

        if (final)
        {
          // fail insertion; the same prefix is already found
          if (storedPrefixFinal)
          {
            if (foundPrefix)
              *foundPrefix = storedPrefix;
            break;
          }
          // if storedPrefix is not final, then it can only be in the last array, so value can address the nodes
          if (!currentNode->nodes[value])
          {
            if ((currentNode->nodes[value] = swLPMV2NodeNew(lpm->nodeCount, lpm->factor)))
              currentNode->nodeCount++;
          }
          if ((success = swLPMV2NodeInsert(lpm, currentNode->nodes[value], storedPrefix, NULL, i + lpm->factor)))
            success = rtn = swLPMV2NodeSetPrefix(currentNode, prefixPosition, value, lpm->nodeCount, lpm->factor, SW_LPM_PREFIX_SET_FLAG(prefix, !final));
        }
        else
        {
          if (!currentNode->nodes[value])
          {
            if ((currentNode->nodes[value] = swLPMV2NodeNew(lpm->nodeCount, lpm->factor)))
              currentNode->nodeCount++;
          }
          // keep current prefix
          if (storedPrefixFinal)
            currentNode = currentNode->nodes[value];
          else
          {
            if ((success = swLPMV2NodeInsert(lpm, currentNode->nodes[value], storedPrefix, NULL, i + lpm->factor)))
            {
              // clear prefix
              swLPMV2NodeClearPrefix(currentNode, prefixPosition, value, lpm->nodeCount, lpm->factor);
              currentNode = currentNode->nodes[value];
            }
          }
        }
      }
      else
      {
        if (!final && currentNode->nodes[value])
          currentNode = currentNode->nodes[value];
        else
          success = rtn = swLPMV2NodeSetPrefix(currentNode, prefixPosition, value, lpm->nodeCount, lpm->factor, SW_LPM_PREFIX_SET_FLAG(prefix, !final));
      }
    }
    if (rtn || !success)
      break;
    i += lpm->factor;
  }
  return rtn;
}

bool swLPMV2Insert(swLPMV2 *lpm, swLPMV2Prefix *prefix, swLPMV2Prefix **foundPrefix)
{
  bool rtn = false;
  if (lpm && prefix)
  {
    if ((rtn = swLPMV2NodeInsert(lpm, &(lpm->rootNode), prefix, foundPrefix, 0)))
      lpm->count++;
  }
  return rtn;
}

bool swLPMV2Find(swLPMV2 *lpm, swLPMV2Prefix *prefix, swLPMV2Prefix **foundPrefix)
{
  bool rtn = false;
  if (lpm && prefix)
  {
    swLPMV2Prefix *storedPrefix = NULL;
    swLPMV2Node *currentNode = &(lpm->rootNode);
    uint16_t i = 0;
    uint8_t value = 0;
    bool final = false;
    bool success = false;
    uint32_t prefixPosition = 0;
    bool storedPrefixFinal = false;
    while ((i < prefix->len) && currentNode)
    {
      value = 0;
      final = (i + lpm->factor >= prefix->len);
      if ((success = swLPMV2PrefixGetSlice(prefix, i, lpm->factor, &value)))
      {
        prefixPosition = (!final)? (lpm->factor - 1) : (prefix->len - 1 - i);
        storedPrefix = swLPMV2NodeGetPrefix(currentNode, prefixPosition, value, lpm->nodeCount, lpm->factor);
        if (storedPrefix)
        {
          storedPrefixFinal = SW_LPM_PREFIX_IS_FINAL(storedPrefix);
          storedPrefix = SW_LPM_PREFIX_CLEAR_FLAG(storedPrefix);
          if (final)
          {
            if (storedPrefixFinal)
              rtn = true;
            else
              break;
          }
          else
          {
            if (!storedPrefixFinal && swLPMV2PrefixEqual(prefix, storedPrefix))
              rtn = true;
          }
        }
      }
      if (rtn || !success)
        break;
      i += lpm->factor;
      currentNode = currentNode->nodes[value];
    }
    if (rtn && foundPrefix)
      *foundPrefix = storedPrefix;
  }
  return rtn;
}

static inline swLPMV2Prefix *swLPMV2NodeGetLastPrefix(swLPMV2Node *node, uint8_t factor)
{
  swLPMV2Prefix *rtn = NULL;
  if (((node->prefixCount[0] + node->prefixCount[1]) == 1) && !(node->nodeCount))
  {
    uint8_t storageIndex = (node->prefixCount[0])? 0: 1;
    uint32_t storagePosition = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
      if (swBitMapLongIntFindFirstSet(node->bitMaps[storageIndex][i], &storagePosition))
      {
        rtn = node->prefix[storageIndex][storagePosition];
        node->prefix[storageIndex][storagePosition] = NULL;
        swBitMapLongIntClear(node->bitMaps[storageIndex][i], storagePosition);
        node->prefixCount[storageIndex]--;
        break;
      }
    }
  }
  return rtn;
}

static inline void swLPMV2NodeDeleteChild(swLPMV2Node *node, uint16_t nodeCount, uint8_t factor, uint8_t value)
{
  swLPMV2NodeDelete(node->nodes[value], nodeCount, factor, true);
  node->nodes[value] = NULL;
  node->nodeCount--;
}

static inline bool swLPMV2NodeCleanupLastPrefix(swLPMV2Node *node, uint16_t nodeCount, uint8_t factor, uint8_t value, bool *done)
{
  bool rtn = false;
  swLPMV2Prefix *lastPrefix = swLPMV2NodeGetLastPrefix(node->nodes[value], factor);
  if (lastPrefix)
  {
    swLPMV2NodeDeleteChild(node, nodeCount, factor, value);
    rtn = swLPMV2NodeSetPrefix(node, factor - 1, value, nodeCount, factor, SW_LPM_PREFIX_SET_FLAG(lastPrefix, 1));
    if (done)
      *done = rtn;
  }
  else
    rtn = true;
  return rtn;
}

// the assumption here is that we manager the prefix storage separately, so that if we remove from LPM, the memory for the prefix object
// will be released by something else
// the easiest thing to do here is to traverse the tree, trying to find exact prefix match and then set the stored pointer to 0
// if no match found, return true but set foundPrefix to NULL, return false for failures
#define SW_LPM_MAX_DEPTH  256

typedef struct swLPMV2NodeValue
{
  swLPMV2Node *node;
  uint8_t value;
} swLPMV2NodeValue;

bool swLPMV2Remove(swLPMV2 *lpm, swLPMV2Prefix *prefix, swLPMV2Prefix **foundPrefix)
{
  bool rtn = false;
  if (lpm && lpm->count && prefix)
  {
    swLPMV2NodeValue currentNodePairs[SW_LPM_MAX_DEPTH];
    uint32_t currentNodePosition = 0;

    swLPMV2Prefix *storedPrefix = NULL;
    swLPMV2Node *currentNode = &(lpm->rootNode);
    currentNodePairs[currentNodePosition].node = currentNode;
    uint32_t prefixPosition = 0;
    uint8_t value = 0;
    bool final = false;
    bool success = false;
    bool storedPrefixFinal = false;
    uint16_t i = 0;
    while ((i < prefix->len) && currentNode)
    {
      value = 0;
      final = (i + lpm->factor >= prefix->len);
      if ((success = swLPMV2PrefixGetSlice(prefix, i, lpm->factor, &value)))
      {
        currentNodePairs[currentNodePosition].value = value;
        prefixPosition = (!final)? (lpm->factor - 1) : (prefix->len - 1 - i);
        storedPrefix = swLPMV2NodeGetPrefix(currentNode, prefixPosition, value, lpm->nodeCount, lpm->factor);
        if (storedPrefix)
        {
          storedPrefixFinal = SW_LPM_PREFIX_IS_FINAL(storedPrefix);
          storedPrefix = SW_LPM_PREFIX_CLEAR_FLAG(storedPrefix);
          if (final)
          {
            if (storedPrefixFinal)
            {
              swLPMV2NodeClearPrefix(currentNode, prefixPosition, value, lpm->nodeCount, lpm->factor);
              rtn = true;
            }
            else
              break;
          }
          else
          {
            if (!storedPrefixFinal && swLPMV2PrefixEqual(prefix, storedPrefix))
            {
              swLPMV2NodeClearPrefix(currentNode, prefixPosition, value, lpm->nodeCount, lpm->factor);
              rtn = true;
            }
          }
        }
      }
      if (rtn || !success)
        break;
      i += lpm->factor;
      currentNode = currentNode->nodes[value];
      currentNodePairs[++currentNodePosition].node = currentNode;
    }
    if (rtn)
    {
      lpm->count--;

      if ((prefixPosition == (uint32_t)(lpm->factor - 1)) && (currentNode->nodes[value]))
        rtn = swLPMV2NodeCleanupLastPrefix(currentNode, lpm->nodeCount, lpm->factor, value, NULL);
      uint32_t prefixCount = currentNode->prefixCount[0] + currentNode->prefixCount[1];
      while (rtn && (currentNode->nodeCount == 0) && (prefixCount <= 1) && (currentNodePosition > 0))
      {
        currentNodePosition--;
        value = currentNodePairs[currentNodePosition].value;
        currentNode = currentNodePairs[currentNodePosition].node;
        if (prefixCount == 1)
        {
          if (swLPMV2NodePrefixIsClear(currentNode, lpm->factor - 1, value, lpm->nodeCount, lpm->factor))
          {
            bool done = false;
            rtn = swLPMV2NodeCleanupLastPrefix(currentNode, lpm->nodeCount, lpm->factor, value, &done);
            if (done)
              continue;
          }
          break;
        }
        else
          swLPMV2NodeDeleteChild(currentNode, lpm->nodeCount, lpm->factor, value);
        prefixCount = currentNode->prefixCount[0] + currentNode->prefixCount[1];
      }
      if (rtn && foundPrefix)
        *foundPrefix = storedPrefix;
    }
  }
  return rtn;
}

static inline bool swStaticBufferGetBitSlice(swStaticBuffer *buffer, uint16_t startPosition, uint8_t factor, uint8_t *value)
{
  bool rtn = 0;
  if (buffer)
    rtn = _getSlice((uint8_t *)(buffer->data), buffer->len, buffer->len * 8, startPosition, factor, value);
  return rtn;
}

static void swLPMV2PrefixPrintBuffer(swStaticBuffer *value) __attribute__((unused));
static void swLPMV2PrefixPrintBuffer(swStaticBuffer *value)
{
  if (value)
  {
    printf("%p: len = %zu, ", value, value->len);
    for (uint8_t i = 0; i < value->len; i++)
    {
      for (uint8_t j = 0; j < 8; j++)
        printf("%u", !!((value->data[i] << j) & 0x80));
      printf(" ");
    }
    printf("\n");
  }
}

bool swLPMV2Match(swLPMV2 *lpm, swStaticBuffer *value, swLPMV2Prefix **prefix)
{
  bool rtn = false;
  if (lpm && value && prefix)
  {
    swLPMV2Prefix *currentPrefix = NULL;
    swLPMV2Prefix *storedPrefix = NULL;
    swLPMV2Node *currentNode = &(lpm->rootNode);
    uint32_t prefixPosition = 0;
    uint16_t maxBits = value->len * 8;
    uint16_t i = 0;
    uint8_t valueSlice = 0;
    bool final = false;
    while (currentNode && (i < maxBits))
    {
      valueSlice = 0;
      final = (i + lpm->factor >= maxBits);
      if (swStaticBufferGetBitSlice(value, i, lpm->factor, &valueSlice))
      {
        prefixPosition = (!final)? (lpm->factor - 1) : (maxBits - 1 - i);
        for (uint16_t j = 0; j <= prefixPosition; j++)
        {
          storedPrefix = swLPMV2NodeGetPrefix(currentNode, j, (valueSlice >> (prefixPosition - j)), lpm->nodeCount, lpm->factor);
          if (storedPrefix)
            currentPrefix = SW_LPM_PREFIX_CLEAR_FLAG(storedPrefix);
        }
        if (final)
          break;
      }
      else
      {
        currentPrefix = NULL;
        break;
      }
      currentNode = currentNode->nodes[valueSlice];
      i += lpm->factor;
    }
    if (currentPrefix)
    {
      *prefix = currentPrefix;
      rtn = true;
    }
  }
  return rtn;
}

int swLPMV2PrefixCompare(swLPMV2Prefix *p1, swLPMV2Prefix *p2)
{
  int rtn = 0;
  if (p1 != p2)
  {
    if (!p1 || !p2)
      return ((p1 > p2) - (p2 < p1));
    else
    {
      uint64_t minLen = MIN(p1->len, p2->len);
      rtn = memcmp ((void *)(p1->prefixBytes), (void *)(p2->prefixBytes), SW_LPM_PREFIX_BYTES_FROM_LEN(minLen));
      if ((rtn == 0) && (p1->len != p2->len))
        rtn = (p1->len > p2->len) - (p2->len > p1->len);
    }
  }
  return rtn;
}

bool swLPMV2PrefixEqual(swLPMV2Prefix *p1, swLPMV2Prefix *p2)
{
  bool rtn = false;
  if (p1 == p2)
    rtn = true;
  else
  {
    if (p1 && p2 && p1->len == p2->len && (memcmp((void *)(p1->prefixBytes), (void *)(p2->prefixBytes), swLPMV2PrefixBytes(p1)) == 0))
      rtn = true;
  }
  return rtn;
}

bool swLPMV2PrefixInit(swLPMV2Prefix *prefix, uint8_t *bytes, uint16_t len)
{
  bool rtn = false;
  if (prefix && bytes && len)
  {
    uint8_t numberOfBytes = (len >> 3) + ((len & 7) > 0);
    memcpy(prefix->prefixBytes, bytes, numberOfBytes);
    if (((len & 7) > 0))
    {
      uint16_t bytesMissing = 8 - (len & 7);
      uint16_t mask = ~((1 << bytesMissing) - 1);
      prefix->prefixBytes[numberOfBytes - 1] &= mask;
    }
    prefix->len = len;
    rtn = true;
  }
  return rtn;
}

void swLPMV2PrefixPrint(swLPMV2Prefix *prefix)
{
  if (prefix)
  {
    printf("%p: len = %3u, ", prefix, prefix->len);
    uint8_t bytes = swLPMV2PrefixBytes(prefix);
    for (uint8_t i = 0; i < bytes; i++)
    {
      for (uint8_t j = 0; j < 8; j++)
        printf("%u", !!((prefix->prefixBytes[i] << j) & 0x80));
      printf(" ");
    }
    printf("\n");
  }
}

//                                          0         1         2         3         4         5         6         7         8         9        10        11        12
//                                          01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567
static char *swLPMV2ValidateOffsetString = "                                                                                                                                ";

// Walk prefix array in pre-order order
// 0: [0:0]                                       [1:15]
// 1: [0:1]               [1:8]                   [2:16]                    [3:23]
// 2: [0:2]     [1:5]     [2:9]       [3:12]      [4:17]      [5:20]        [6:24]        [7:27]
// 3: [0:3][1:4][2:6][3:7][4:10][5:11][6:13][7:14][8:18][9:19][10:21][11:22][12:25][13:26][14:28][15:29]
static bool swLPMV2NodeValidate(swLPMV2Node *node, uint64_t *count, swLPMV2Prefix **prevPrefix, uint16_t nodeCount, uint8_t factor, uint16_t level, bool print)
{
  bool rtn = false;
  if (node && count && prevPrefix)
  {
    if (print)
      printf("%.*slevel = %3u, node = %p: prefixCount = (%u, %u), nodeCount = %u\n", level, swLPMV2ValidateOffsetString, level, (void *)node, node->prefixCount[0], node->prefixCount[1], node->nodeCount);
    uint8_t nextPosition[SW_LPM_MAX_FACTOR] = {0};
    swLPMV2Prefix *currentPrefix = NULL;
    uint16_t currentPrefixCount = 0;
    uint16_t currentNodeCount = 0;
    rtn = true;
    for (uint16_t i = 0; (i < nodeCount) && rtn; i++)
    {
      for (uint8_t j = 0; j < factor; j++)
      {
        uint16_t position = i >> (factor - j - 1);
        if (nextPosition[j] == position)
        {
          currentPrefix = swLPMV2NodeGetPrefix(node, j, position, nodeCount, factor);
          if (currentPrefix)
          {
            currentPrefixCount++;
            (*count)++;
            bool isFinal = SW_LPM_PREFIX_IS_FINAL(currentPrefix);
            currentPrefix = SW_LPM_PREFIX_CLEAR_FLAG(currentPrefix);
            if (print)
            {
              printf("%.*slevel = %3u, prefix level = %1u, position = %3u, final = %d: ", level, swLPMV2ValidateOffsetString, level, j, position, isFinal);
              swLPMV2PrefixPrint(currentPrefix);
            }
            if (*prevPrefix)
            {
              // current prefix should be greater than the previous one
              if (swLPMV2PrefixCompare(currentPrefix, *prevPrefix) < 1)
              {
                rtn = false;
                break;
              }
            }
            // if the prefix is not final, it can only be in the last row of node prefixArray
            // and the node pointer in this position of the node array should be NULL
            if (!isFinal && ((j < (factor - 1)) || node->nodes[i]))
            {
              rtn = false;
              break;
            }
            *prevPrefix = currentPrefix;
          }
          if (j == (factor - 1) && node->nodes[position] && swLPMV2NodePrefixIsClear(node, j, position, nodeCount, factor)
                                && node->nodes[position]->nodeCount == 0 && (node->nodes[position]->prefixCount[0] + node->nodes[position]->prefixCount[1]) == 1)
          {
            rtn = false;
            break;
          }
          nextPosition[j]++;
        }
      }
      if (rtn && node->nodes[i])
      {
        currentNodeCount++;
        rtn = swLPMV2NodeValidate(node->nodes[i], count, prevPrefix, nodeCount, factor, level + 1, print);
      }
    }
    if (rtn && ((currentNodeCount != node->nodeCount) || (currentPrefixCount != (node->prefixCount[0] + node->prefixCount[1]))))
    {
      printf ("'%s': Problem with the counts\n", __func__);
      rtn = false;
    }
  }
  return rtn;
}

bool swLPMV2Validate(swLPMV2 *lpm, bool print)
{
  bool rtn = false;
  if (lpm)
  {
    uint64_t count = 0;
    if (print)
      printf ("\nLPM: nodeCount = %u, factor = %u, count = %lu, \n", lpm->nodeCount, lpm->factor, lpm->count);
    swLPMV2Prefix *currentPrefix = NULL;
    if (swLPMV2NodeValidate(&(lpm->rootNode), &count, &currentPrefix, lpm->nodeCount, lpm->factor, 0, print) && (count == lpm->count))
      rtn = true;
  }
  return rtn;
}
