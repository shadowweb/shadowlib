#include "collections/lpm.h"
#include "core/memory.h"

#include <string.h>
#include <sys/param.h>

swLPMNode *swLPMNodeNew(uint16_t nodeCount, uint8_t factor)
{
  swLPMNode *rtn = NULL;
  if (nodeCount)
  {
    swLPMNode *node = swMemoryCalloc(1, sizeof(swLPMNode) + nodeCount * sizeof(swLPMNode *));
    if (node)
    {
      if ((node->prefix = swMemoryCalloc(factor, sizeof(swLPMPrefix **))))
        rtn = node;
      else
        swMemoryFree(node);
    }
  }
  return rtn;
}

void swLPMNodeDelete(swLPMNode *lpmNode, uint16_t nodeCount, uint8_t factor, bool freeNode)
{
  if (lpmNode && nodeCount && factor)
  {
    if (lpmNode->prefix)
    {
      for (uint8_t j = 0; j < factor; j++)
      {
        if (lpmNode->prefix[j])
          swMemoryFree(lpmNode->prefix[j]);
      }
      swMemoryFree(lpmNode->prefix);
    }
    swLPMNode **nodes = lpmNode->nodes;
    for (uint16_t i = 0; i < nodeCount; i++)
    {
      if (nodes[i])
        swLPMNodeDelete(nodes[i], nodeCount, factor, true);
    }
    if (freeNode)
      swMemoryFree(lpmNode);
  }
}

swLPM *swLPMNew(uint8_t factor)
{
  swLPM *rtn = NULL;
  if (factor >= SW_LPM_MIN_FACTOR && factor <= SW_LPM_MAX_FACTOR)
  {
    uint16_t nodeCount = (uint16_t)1 << factor;
    swLPM *lpm = swMemoryCalloc(1, sizeof(swLPM) + nodeCount * sizeof(swLPMNode *));
    if (lpm)
    {
      if ((lpm->rootNode.prefix = swMemoryCalloc(factor, sizeof(swLPMPrefix **))))
      {
        lpm->nodeCount  = nodeCount;
        lpm->factor     = factor;
        rtn = lpm;
      }
      else
        swLPMNodeDelete(&(lpm->rootNode), nodeCount, factor, false);
    }
  }
  return rtn;
}

void swLPMDelete(swLPM *lpm)
{
  if (lpm)
  {
    swLPMNodeDelete(&(lpm->rootNode), lpm->nodeCount, lpm->factor, false);
    swMemoryFree(lpm);
  }
}

// #define SW_LPM_PREFIX_BYTES(x)  (((x)->len >> 3) + (((x)->len & 7) > 0))
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

static inline bool swLPMPrefixGetSlice(swLPMPrefix *prefix, uint16_t startPosition, uint8_t factor, uint8_t *value)
{
  bool rtn = 0;
  // TODO: store totalBytes inside prefix (that way we do not need to recalculate it every time)
  if (prefix)
    rtn = _getSlice(prefix->prefixBytes, swLPMPrefixBytes(prefix), prefix->len, startPosition, factor, value);
  return rtn;
}

#define SW_LPM_PREFIX_IS_FINAL(x)       (!((uint64_t)(x) & 0x01))
#define SW_LPM_PREFIX_CLEAR_FLAG(x)     (swLPMPrefix *)((uint64_t)(x) & (~1UL))
#define SW_LPM_PREFIX_SET_FLAG(x, n)    (swLPMPrefix *)((uint64_t)(x) + (n))

// TODO: maybe use sparse arrays here for storing prefixes, to save space and to know how many elements are there
static bool swLPMNodeInsert(swLPM *lpm, swLPMNode *node, swLPMPrefix *prefix, swLPMPrefix **foundPrefix, uint16_t offset)
{
  bool rtn = false;
  // printf("Inserting node prefix for offset %u: ", offset);
  // swLPMPrefixPrint(prefix);
  swLPMNode *currentNode = node;
  uint16_t i = offset;
  while ((i < prefix->len) && currentNode)
  {
    uint8_t value = 0;
    bool final = (i + lpm->factor >= prefix->len);
    bool success = false;
    if ((success = swLPMPrefixGetSlice(prefix, i, lpm->factor, &value)))
    {
      uint32_t prefixPosition = (!final)? (lpm->factor - 1) : (lpm->factor - (i + lpm->factor - prefix->len) - 1);
      uint32_t elementCount = (2 << prefixPosition);
      // printf ("\tslice value = %u, i = %u, prefix position = %u, elementCount = %u, final = %d\n", value, i, prefixPosition, elementCount, final);
      if (!currentNode->prefix[prefixPosition])
        currentNode->prefix[prefixPosition] = swMemoryCalloc(elementCount, sizeof(swLPMPrefix *));
      swLPMPrefix **prefixArray = currentNode->prefix[prefixPosition];
      if ((success = (prefixArray != NULL)))
      {
        // printf ("\tprefixArray = %p\n", prefixArray);
        swLPMPrefix *storedPrefix = prefixArray[value];
        if (storedPrefix)
        {
          bool storedPrefixFinal = SW_LPM_PREFIX_IS_FINAL(storedPrefix);
          // printf ("\tstoredPrefix = %p, final = %d\n", storedPrefix, storedPrefixFinal);
          storedPrefix = SW_LPM_PREFIX_CLEAR_FLAG(storedPrefix);

          if (final)
          {
            // fail insertion; the same prefix is already found
            if (storedPrefixFinal)
            {
              // printf("Failed inserting the same prefix as existing one\n");
              if (foundPrefix)
                *foundPrefix = storedPrefix;
              break;
            }
            // if storedPrefix is not final, then it can only be in the last array, so value can address the nodes
            if (!currentNode->nodes[value])
              currentNode->nodes[value] = swLPMNodeNew(lpm->nodeCount, lpm->factor);
            if ((success = swLPMNodeInsert(lpm, currentNode->nodes[value], storedPrefix, NULL, i + lpm->factor)))
            {
              // printf("-- Inserted after eviction!!\n");
              prefixArray[value] = SW_LPM_PREFIX_SET_FLAG(prefix, !final);
              rtn = true;
            }
            else
              printf ("1: Failed while moving stored prefix down the tree\n");
          }
          else
          {
            if (!currentNode->nodes[value])
              currentNode->nodes[value] = swLPMNodeNew(lpm->nodeCount, lpm->factor);
            // keep current prefix
            if (storedPrefixFinal)
              currentNode = currentNode->nodes[value];
            else
            {
              if ((success = swLPMNodeInsert(lpm, currentNode->nodes[value], storedPrefix, NULL, i + lpm->factor)))
              {
                // clear prefix
                prefixArray[value] = NULL;
                currentNode = currentNode->nodes[value];
              }
              else
                printf ("2: Failed while moving stored prefix down the tree\n");
            }
          }
        }
        else
        {
          // printf ("\tno storedPrefix\n");
          if (!final && currentNode->nodes[value])
            currentNode = currentNode->nodes[value];
          else
          {
            // store current prefix here
            // printf("-- Inserted without eviction: final = %d!!\n", final);
            prefixArray[value] = SW_LPM_PREFIX_SET_FLAG(prefix, !final);
            rtn = true;
          }
        }
      }
      else
        printf ("Failed to create prefix array for position %u\n", prefixPosition);
    }
    else
      printf ("Failed to get slice\n");
    if (rtn || !success)
      break;
    i += lpm->factor;
  }
  // printf("Done inserting node prefix at offset %u: ", i);
  // swLPMPrefixPrint(prefix);
  return rtn;
}

bool swLPMInsert(swLPM *lpm, swLPMPrefix *prefix, swLPMPrefix **foundPrefix)
{
  bool rtn = false;
  if (lpm && prefix)
  {
    // printf("Inserting prefix: ");
    // swLPMPrefixPrint(prefix);
    if ((rtn = swLPMNodeInsert(lpm, &(lpm->rootNode), prefix, foundPrefix, 0)))
      lpm->count++;
  }
  return rtn;
}

bool swLPMFind(swLPM *lpm, swLPMPrefix *prefix, swLPMPrefix **foundPrefix)
{
  bool rtn = false;
  if (lpm && prefix)
  {
    swLPMPrefix *storedPrefix = NULL;
    swLPMNode *currentNode = &(lpm->rootNode);
    uint16_t i = 0;
    while ((i < prefix->len) && currentNode)
    {
      uint8_t value = 0;
      bool final = (i + lpm->factor >= prefix->len);
      bool success = false;
      if ((success = swLPMPrefixGetSlice(prefix, i, lpm->factor, &value)))
      {
        uint32_t prefixPosition = (!final)? (lpm->factor - 1) : (lpm->factor - (i + lpm->factor - prefix->len) - 1);
        swLPMPrefix **prefixArray = currentNode->prefix[prefixPosition];
        if(prefixArray)
        {
          storedPrefix = prefixArray[value];
          if (storedPrefix)
          {
            bool storedPrefixFinal = SW_LPM_PREFIX_IS_FINAL(storedPrefix);
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
              if (!storedPrefixFinal && swLPMPrefixEqual(prefix, storedPrefix))
                rtn = true;
            }
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


// the assumption here is that we manager the prefix storage separately, so that if we remove from LPM, the memory for the prefix object
// will be released by something else
// the easiest thing to do here is to traverse the tree, trying to find exact prefix match and then set the stored pointer to 0
// if no match found, return true but set foundPrefix to NULL, return false for failures
bool swLPMRemove(swLPM *lpm, swLPMPrefix *prefix, swLPMPrefix **foundPrefix)
{
  bool rtn = false;
  if (lpm && prefix)
  {
    swLPMPrefix *storedPrefix = NULL;
    swLPMNode *currentNode = &(lpm->rootNode);
    uint16_t i = 0;
    while ((i < prefix->len) && currentNode)
    {
      uint8_t value = 0;
      bool final = (i + lpm->factor >= prefix->len);
      bool success = false;
      if ((success = swLPMPrefixGetSlice(prefix, i, lpm->factor, &value)))
      {
        uint32_t prefixPosition = (!final)? (lpm->factor - 1) : (lpm->factor - (i + lpm->factor - prefix->len) - 1);
        swLPMPrefix **prefixArray = currentNode->prefix[prefixPosition];
        if(prefixArray)
        {
          storedPrefix = prefixArray[value];
          if (storedPrefix)
          {
            bool storedPrefixFinal = SW_LPM_PREFIX_IS_FINAL(storedPrefix);
            storedPrefix = SW_LPM_PREFIX_CLEAR_FLAG(storedPrefix);
            if (final)
            {
              if (storedPrefixFinal)
              {
                prefixArray[value] = NULL;
                rtn = true;
              }
              else
                break;
            }
            else
            {
              if (!storedPrefixFinal && swLPMPrefixEqual(prefix, storedPrefix))
              {
                prefixArray[value] = NULL;
                rtn = true;
              }
            }
          }
        }
      }
      if (rtn || !success)
        break;
      i += lpm->factor;
      currentNode = currentNode->nodes[value];
    }
    // return true if the matching prefix is not found, i.e., removed
    if ((i >= prefix->len) || !currentNode)
      rtn = true;
    if (rtn && foundPrefix)
    {
      *foundPrefix = storedPrefix;
      lpm->count--;
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

bool swLPMMatch(swLPM *lpm, swStaticBuffer *value, swLPMPrefix **prefix)
{
  bool rtn = false;
  if (lpm && value && prefix)
  {
    swLPMPrefix *currentPrefix = NULL;
    swLPMNode *currentNode = &(lpm->rootNode);
    uint16_t maxBits = value->len * 8;
    uint16_t i = 0;
    while (currentNode && (i < maxBits))
    {
      uint8_t valueSlice = 0;
      bool final = (i + lpm->factor >= maxBits);
      if (swStaticBufferGetBitSlice(value, i, lpm->factor, &valueSlice))
      {
        swLPMPrefix *storedPrefix = NULL;
        uint32_t prefixPosition = (!final)? (lpm->factor - 1) : (lpm->factor - (i + lpm->factor - maxBits) - 1);
        for (uint16_t i = 0; i <= prefixPosition; i++)
        {
          swLPMPrefix **prefixArray = currentNode->prefix[i];
          if (prefixArray)
            storedPrefix = SW_LPM_PREFIX_CLEAR_FLAG(prefixArray[ (valueSlice >> (prefixPosition - i)) ]);
        }
        // I do not think we need to compare the prefixes here because if it found non-final prefix, no child nodes should exist
        // for this value slice
        if (storedPrefix)
          currentPrefix = storedPrefix;
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

int swLPMPrefixCompare(swLPMPrefix *p1, swLPMPrefix *p2)
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

bool swLPMPrefixEqual(swLPMPrefix *p1, swLPMPrefix *p2)
{
  bool rtn = false;
  if (p1 == p2)
    rtn = true;
  else
  {
    if (p1 && p2 && p1->len == p2->len && (memcmp((void *)(p1->prefixBytes), (void *)(p2->prefixBytes), swLPMPrefixBytes(p1)) == 0))
      rtn = true;
  }
  return rtn;
}

bool swLPMPrefixInit(swLPMPrefix *prefix, uint8_t *bytes, uint16_t len)
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

void swLPMPrefixPrint(swLPMPrefix *prefix)
{
  if (prefix)
  {
    printf("%p: len = %u, ", prefix, prefix->len);
    uint8_t bytes = swLPMPrefixBytes(prefix);
    for (uint8_t i = 0; i < bytes; i++)
    {
      for (uint8_t j = 0; j < 8; j++)
        printf("%u", !!((prefix->prefixBytes[i] << j) & 0x80));
      printf(" ");
    }
    printf("\n");
  }
}

// Walk prefix array in pre-order order
// 0: [0:0]                                       [1:15]
// 1: [0:1]               [1:8]                   [2:16]                    [3:23]
// 2: [0:2]     [1:5]     [2:9]        [3:12]     [4:17]      [5:20]        [6:24]         [7:27]
// 3: [0:3][1:4][2:6][3:7][4:10][5:11][6:13][7:14][8:18][9:19][10:21][11:22][12:25][13:26][14:28][15:29]
static bool swLPMNodeValidate(swLPMNode *node, uint64_t *count, swLPMPrefix **prevPrefix, uint16_t nodeCount, uint8_t factor, bool print)
{
  bool rtn = false;
  if (node && count && prevPrefix)
  {
    uint8_t nextPosition[SW_LPM_MAX_FACTOR] = {0};
    swLPMPrefix *currentPrefix = NULL;
    rtn = true;
    for (uint16_t i = 0; (i < nodeCount) && rtn; i++)
    {
      for (uint8_t j = 0; j < factor; j++)
      {
        uint16_t position = i >> (factor - j - 1);
        if (nextPosition[j] == position)
        {
          if (node->prefix[j])
          {
            currentPrefix = node->prefix[j][position];
            if (currentPrefix)
            {
              (*count)++;
              bool isFinal = SW_LPM_PREFIX_IS_FINAL(currentPrefix);
              currentPrefix = SW_LPM_PREFIX_CLEAR_FLAG(currentPrefix);
              if (print)
                swLPMPrefixPrint(currentPrefix);
              if (*prevPrefix)
              {
                // current prefix should be greate than the previous one
                if (swLPMPrefixCompare(currentPrefix, *prevPrefix) < 1)
                {
                  printf("Failed comparison\n");
                  rtn = false;
                  break;
                }
              }
              // if the prefix is not final, it can only be in the last row of node prefixArray
              // and the node pointer in this position of the node array should be NULL
              if (!isFinal && ((j < (factor - 1)) || node->nodes[i]))
              {
                printf("Failed not final check\n");
                rtn = false;
                break;
              }
              *prevPrefix = currentPrefix;
            }
            nextPosition[j]++;
          }
        }
      }
      if (rtn)
      {
        if (node->nodes[i])
          rtn = swLPMNodeValidate(node->nodes[i], count, prevPrefix, nodeCount, factor, print);
      }
    }
  }
  return rtn;
}

bool swLPMValidate(swLPM *lpm, bool print)
{
  bool rtn = false;
  if (lpm)
  {
    uint64_t count = 0;
    swLPMPrefix *currentPrefix = NULL;
    if (swLPMNodeValidate(&(lpm->rootNode), &count, &currentPrefix, lpm->nodeCount, lpm->factor, print) && (count == lpm->count))
      rtn = true;
  }
  return rtn;
}
