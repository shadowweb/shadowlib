#ifndef SW_COLLECTIONS_CALLTREE_H
#define SW_COLLECTIONS_CALLTREE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct swCallTree
{
  struct swCallTree *children;
  uint32_t count;
  uint32_t size;
  uint64_t funcAddress;
  uint32_t repeatCount;
} swCallTree;

typedef void swCallTreeWalkCB(swCallTree *subTree, uint32_t level, void *data);

swCallTree *swCallTreeNew(uint64_t funcAddress);
void swCallTreeDelete(swCallTree *tree);

swCallTree *swCallTreeAddNext(swCallTree *parent, uint64_t funcAddress);
int swCallTreeCompare(swCallTree *node1, swCallTree *node2, bool skipRepeat);
void swCallTreeWalk(swCallTree *root, swCallTreeWalkCB *walkCB, void *data);

// void swCallTreeDump(swCallTree *root);

#endif  // SW_COLLECTIONS_CALLTREE_H
