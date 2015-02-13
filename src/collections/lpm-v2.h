#ifndef SW_COLLECTIONS_LPMV3_H
#define SW_COLLECTIONS_LPMV3_H

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

typedef swBitMapLongInt swLPMV2BitMap[4];

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
  uint16_t prefixCount[2];
  uint16_t nodeCount;         // number of node pointers stored in the node
  swLPMV2BitMap bitMaps[2];   // bitMap is used to easily find out which element in the prefix array is still set
                              // it is 256 bit long because the max lenght of the each prefix array is 256, which
                              // corresponds to 4 64-bit words
  // prefix requires detailed explanation
  // for factor == 1 only prefix[0] is used and has 2 elements allocated (same as lpm->nodeCount)
  // for all other factors both prefix[0] and prefix[1] may exist and will alsways has the same number of elements
  // as lpm->nodeCount; prefix[0] always contain prefixes that produced value of factor length for the given level
  // (either final or not final), everything else will be stored in prefix[1] which has enough length to accomodate
  // the all the other level; all the prefixes stored here will be final
  // for factor == 2, prefix[1] will contain 4 elements, but only 2 will be used for values of 1 bit long
  // for factor == 3, prefix[1] will contain 8 elements, but only 6 will be used
  //      4 - for values 2 bits long
  //      2 - for values 1 bit long
  // for factor == 4, prefix[1] will contain 16 elements. but only 14 will be used
  //      8 - for values 3 bits long
  //      4 - for values 2 bits long
  //      2 - for values 1 bit long
  // etc. till factor 8
  // prefixes with higher bit length are stored first
  swLPMV2Prefix **prefix[2];
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

#endif  // SW_COLLECTIONS_LPMV3_H

