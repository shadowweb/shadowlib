#include "collections/call-tree.h"

#include "core/memory.h"

#include <string.h>

#define SW_CALLTREE_ROOT_FLAG 0x8000000000000000UL

swCallTree *swCallTreeNew(uint64_t funcAddress)
{
  swCallTree *tree = swMemoryCalloc(1, sizeof(swCallTree));
  if (tree)
  {
    tree->funcAddress = funcAddress;
    tree->repeatCount = 1;
    tree->funcAddress |= SW_CALLTREE_ROOT_FLAG;
  }
  return tree;
}

void swCallTreeDelete(swCallTree *tree)
{
  if (tree)
  {
    if (tree->children)
    {
      for (uint32_t i = 0; i < tree->count; i++)
        swCallTreeDelete(&tree->children[i]);
      swMemoryFree(tree->children);
    }
    if (tree->funcAddress & SW_CALLTREE_ROOT_FLAG)
      swMemoryFree(tree);
    else
      memset(tree, 0, sizeof(swCallTree));
  }
}

swCallTree *swCallTreeAddNext(swCallTree *parent, uint64_t funcAddress)
{
  swCallTree *child = NULL;
  if (parent)
  {
    bool canAdd = false;
    if (parent->count < parent->size)
      canAdd = true;
    else
    {
      uint32_t newSize = (parent->size)? (parent->size * 2) : 1;
      swCallTree *newChildren = swMemoryRealloc(parent->children, (size_t)newSize * sizeof(swCallTree));
      if (newChildren)
      {
        memset (&(newChildren[parent->count]), 0, ((newSize - parent->count) * sizeof(swCallTree)));
        parent->children = newChildren;
        parent->size = newSize;
        canAdd = true;
      }
    }
    if (canAdd)
    {
      child = &parent->children[parent->count];
      parent->count++;
      child->funcAddress = funcAddress;
      child->repeatCount = 1;
    }
  }
  return child;
}

int swCallTreeCompare(swCallTree *node1, swCallTree *node2, bool skipRepeat)
{
  int rtn = 0;
  if (node1 && node2)
  {
    rtn = (node1->funcAddress > node2->funcAddress) - (node1->funcAddress < node2->funcAddress);
    if (!rtn)
    {
      rtn = (skipRepeat)? 0 : (node1->repeatCount > node2->repeatCount) - (node1->repeatCount < node2->repeatCount);
      if (!rtn)
      {
        rtn = (node1->count > node2->count) - (node1->count < node2->count);
        for (uint32_t i = 0; !rtn && (i < node1->count); i++)
          rtn = swCallTreeCompare(&node1->children[i], &node2->children[i], false);
      }
    }
  }
  else
      rtn = (node1 > node2) - (node1 < node2);
  return rtn;

}

static void swCallTreeWalkNode(swCallTree *node, swCallTreeWalkCB *walkCB, void *data, uint32_t level)
{
  walkCB(node->funcAddress, node->repeatCount, level, data);
  for (uint32_t i = 0; i < node->count; i++)
    swCallTreeWalkNode(&(node->children[i]), walkCB, data, level + 1);
}

void swCallTreeWalk(swCallTree *root, swCallTreeWalkCB *walkCB, void *data)
{
  if (root && walkCB)
  {
    uint32_t level = 0;
    if (!(root->funcAddress & SW_CALLTREE_ROOT_FLAG))
    {
      walkCB(root->funcAddress, root->repeatCount, level, data);
      level++;
    }
    for (uint32_t i = 0; i < root->count; i++)
      swCallTreeWalkNode(&(root->children[i]), walkCB, data, level);
  }
}

/*
static void swCallTreeNodeDump(swCallTree *node, uint32_t offset)
{
  printf("%*u: Node %p: children=%p, count=%u, size=%u\n", offset*2, offset, node, node->children, node->count, node->size);
  for (uint32_t i = 0; i < node->count; i++)
    swCallTreeNodeDump(&node->children[i], (offset + 1));
}

void swCallTreeDump(swCallTree *root)
{
  if (root)
  {
    printf("Root %p: children=%p, count=%u, size=%u\n", root, root->children, root->count, root->size);
    for (uint32_t i = 0; i < root->count; i++)
      swCallTreeNodeDump(&root->children[i], 1);
    printf("\n");
  }
}
*/
