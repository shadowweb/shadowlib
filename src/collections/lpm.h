#ifndef SW_COLLECTIONS_LPM_H
#define SW_COLLECTIONS_LPM_H

#include "storage/static-buffer.h"

#include <stdbool.h>
#include <stdint.h>

#define SW_LPM_MIN_FACTOR 1
#define SW_LPM_MAX_FACTOR 8

typedef struct swLPMPrefix
{
  uint16_t len;           // number of relevant bits
  uint8_t prefixBytes[];  // length of the array is len/8 + (len%8 > 0)
} __attribute__((aligned(2))) swLPMPrefix;

int  swLPMPrefixCompare(swLPMPrefix *p1, swLPMPrefix *p2);
bool swLPMPrefixEqual(swLPMPrefix *p1, swLPMPrefix *p2);
bool swLPMPrefixInit(swLPMPrefix *prefix, uint8_t *bytes, uint16_t len);
void swLPMPrefixPrint(swLPMPrefix *prefix);

#define swLPMPrefixBytes(p)   (((p)->len >> 3) + (((p)->len & 7) > 0))

typedef struct swLPMNode
{
  // each element is a 64-bit integer that represnts a prefix pointer and 1 bit indicating if the prefix is final
  // use either bit 0 or bit 63 to indicate if the value is final
  // 1 - final means that the prefix is in the right place
  // 0 - intermidiate means that the prefix is here just to avoid creation of extra layers
  //      in the trie because if we do, then all we find will be just this prefix
  //      this also means that if the node is not 0, then intermidiate prefix should not be set
  // can contain up to "factor" number of prefixes since factor determines the number of bits
  // that each node represents
  swLPMPrefix ***prefix;      // this is the pointer to the array of prefix arrays; the first array has 2 - elements
                              // the second has 4, the third has 8, etc., the number of elements in this array is the same
                              // is the number factor value in swLPM
  struct swLPMNode  *nodes[]; // number of nodes is 2 to the power of factor
} swLPMNode;

typedef struct swLPM
{
  uint64_t    count;      // number of elements inserted into the trie
  uint16_t    nodeCount;  // derived from factor
  uint8_t     factor;     // SW_LPM_MIN_FACTOR <= factor <= SW_LPM_MAX_FACTOR
  swLPMNode   rootNode;   // rootNode
} swLPM;

swLPM *swLPMNew(uint8_t factor);
void swLPMDelete(swLPM *lpm);

bool swLPMInsert(swLPM *lpm, swLPMPrefix *prefix, swLPMPrefix **foundPrefix);
bool swLPMFind(swLPM *lpm, swLPMPrefix *prefix, swLPMPrefix **foundPrefix);
bool swLPMRemove(swLPM *lpm, swLPMPrefix *prefix, swLPMPrefix **foundPrefix);

bool swLPMMatch(swLPM *lpm, swStaticBuffer *value, swLPMPrefix **prefix);
bool swLPMValidate(swLPM *lpm, bool print);

#endif  // SW_COLLECTIONS_LPM_H

