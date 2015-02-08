#ifndef SW_COLLECTIONS_LPMV2_H
#define SW_COLLECTIONS_LPMV2_H

#include "collections/bit-map.h"
#include "storage/static-buffer.h"

#include <stdbool.h>
#include <stdint.h>

#define SW_LPM_MIN_FACTOR 1
#define SW_LPM_MAX_FACTOR 8

typedef struct swLPMV2Prefix
{
  uint16_t len;           // number of relevant bits
  uint8_t prefixBytes[];  // length of the array is len/8 + (len%8 > 0)
} __attribute__((aligned(2))) swLPMV2Prefix;

int  swLPMV2PrefixCompare(swLPMV2Prefix *p1, swLPMV2Prefix *p2);
bool swLPMV2PrefixEqual(swLPMV2Prefix *p1, swLPMV2Prefix *p2);
bool swLPMV2PrefixInit(swLPMV2Prefix *prefix, uint8_t *bytes, uint16_t len);
void swLPMV2PrefixPrint(swLPMV2Prefix *prefix);

#define swLPMV2PrefixBytes(p)   (((p)->len >> 3) + (((p)->len & 7) > 0))

typedef struct swLPMV2PrefixStorage
{
  swLPMV2Prefix **prefix;
  swBitMap      bitMap;
} swLPMV2PrefixStorage;

typedef struct swLPMV2Node
{
  // each element is a 64-bit integer that represnts a prefix pointer and 1 bit indicating if the prefix is final
  // use either bit 0 or bit 63 to indicate if the value is final
  // 1 - final means that the prefix is in the right place
  // 0 - intermidiate means that the prefix is here just to avoid creation of extra layers
  //      in the trie because if we do, then all we find will be just this prefix
  //      this also means that if the node is not 0, then intermidiate prefix should not be set
  // can contain up to "factor" number of prefixes since factor determines the number of bits
  // that each node represents
  uint16_t prefixCount;       // number of prefixes stored in the node
  uint16_t nodeCount;         // number of node pointers stored in the node
  // there is a gap here of 32 bits, but it can't be helped
  /*
  swLPMV2Prefix ***prefix;      // this is the pointer to the array of prefix arrays; the first array has 2 - elements
                              // the second has 4, the third has 8, etc., the number of elements in this array is the same
                              // is the number factor value in swLPMV2
  */
  swLPMV2PrefixStorage **storage;
  struct swLPMV2Node  *nodes[]; // number of nodes is 2 to the power of factor
} swLPMV2Node;

typedef struct swLPMV2
{
  uint64_t    count;      // number of elements inserted into the trie
  uint16_t    nodeCount;  // derived from factor
  uint8_t     factor;     // SW_LPM_MIN_FACTOR <= factor <= SW_LPM_MAX_FACTOR
  swLPMV2Node   rootNode;   // rootNode
} swLPMV2;

swLPMV2 *swLPMV2New(uint8_t factor);
void swLPMV2Delete(swLPMV2 *lpm);

bool swLPMV2Insert(swLPMV2 *lpm, swLPMV2Prefix *prefix, swLPMV2Prefix **foundPrefix);
bool swLPMV2Find(swLPMV2 *lpm, swLPMV2Prefix *prefix, swLPMV2Prefix **foundPrefix);
bool swLPMV2Remove(swLPMV2 *lpm, swLPMV2Prefix *prefix, swLPMV2Prefix **foundPrefix);

bool swLPMV2Match(swLPMV2 *lpm, swStaticBuffer *value, swLPMV2Prefix **prefix);
bool swLPMV2Validate(swLPMV2 *lpm, bool print);

#endif  // SW_COLLECTIONS_LPMV2_H

