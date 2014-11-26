#include "collections/call-tree.h"
#include "collections/hash-set-linear.h"
#include "command-line/option-category.h"
#include "command-line/command-line.h"
#include "core/memory.h"
#include "init/init.h"
#include "init/init-command-line.h"
#include "init/init-log-manager.h"
#include "log/log-manager.h"
#include "utils/file.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

swLoggerDeclareWithLevel(buildCallTreeLogger, "CallTree", swLogLevelInfo);

swStaticString inputFileName  = swStaticStringDefineEmpty;

swStaticArray threadIdList    = swStaticArrayDefineEmpty;

swOptionCategoryMainDeclare(swBuildCallTreeMainOptions, "Build Call Tree Tool Main Options",
  swOptionDeclareScalar("input-file|in",  "Input file name",
    NULL,                   &inputFileName,     swOptionValueTypeString,                                  true),
  swOptionDeclareArray("input-threads",   "Comma separated list of integer thread ids that need to be processed",
    "thread1,thread2,...",  &threadIdList,  0,  swOptionValueTypeInt,    swOptionArrayTypeCommaSeparated, true)
);

typedef struct swBuildCallTreeThreadData
{
  swCallTree *callTree;
  swDynamicArray *callStack;
  pid_t threadId;
} swBuildCallTreeThreadData;

void swBuildCallTreeThreadDataDelete(swBuildCallTreeThreadData *data)
{
  if (data)
  {
    if (data->callStack)
      swDynamicArrayDelete(data->callStack);
    if (data->callTree)
      swCallTreeDelete(data->callTree);
    swMemoryFree(data);
  }
}

swBuildCallTreeThreadData *swBuildCallTreeThreadDataNew(pid_t threadId)
{
  swBuildCallTreeThreadData *rtn = NULL;
  if (threadId)
  {
    swBuildCallTreeThreadData *data = swMemoryCalloc(1, sizeof(*data));
    if (data)
    {
      if ((data->callTree = swCallTreeNew(0)))
      {
        if ((data->callStack = swDynamicArrayNew(sizeof(swCallTree *), 64)))
        {
          if (swDynamicArrayPush(data->callStack, &(data->callTree)))
          {
            data->threadId = threadId;
            rtn = data;
          }
        }
      }
      if (!rtn)
        swBuildCallTreeThreadDataDelete(data);
    }
  }
  return rtn;
}

#define SW_FUNCTION_END    0x8000000000000000UL
static swHashMapLinear *threadMap = NULL;
static swHashSetLinear *functions = NULL;

static void swBuildCallTreeStop()
{
  if (functions)
  {
    swHashSetLinearDelete(functions);
    functions = NULL;
  }
  if (threadMap)
  {
    swHashMapLinearDelete(threadMap);
    threadMap = NULL;
  }
}

static uint32_t swBuildCallTreeThreadDataKeyHash(pid_t *key)
{
  return swMurmurHash3_32(key, sizeof(*key));
}

static inline bool swBuildCallTreeThreadDataKeyEqual(pid_t *key1, pid_t *key2)
{
  return (*key1 == *key2);
}

static uint32_t swBuildCallTreeFunctionHash(uint64_t *key)
{
  return swMurmurHash3_32(key, sizeof(*key));
}

static inline bool swBuildCallTreeFunctionEqual(uint64_t *key1, uint64_t *key2)
{
  return (*key1 == *key2);
}

static bool swBuildCallTreeThreadDataAppend(swBuildCallTreeThreadData *threadData, uint64_t functionAddress, bool end)
{
  bool rtn = false;
  if (functionAddress && threadData)
  {
    swCallTree *currentParent = NULL;
    if (end)
    {
      swCallTree *lastChild = NULL;
      if (swDynamicArrayPeek(threadData->callStack, &lastChild) && lastChild)
      {
        if (functionAddress == lastChild->funcAddress)
        {
          swDynamicArrayPop(threadData->callStack, &lastChild);
          if (swDynamicArrayPeek(threadData->callStack, &currentParent) && currentParent)
          {
            if (currentParent->count > 1)
            {
              if (swCallTreeCompare(&(currentParent->children[currentParent->count - 2]), lastChild, true) == 0)
              {
                currentParent->count--;
                currentParent->children[currentParent->count - 1].repeatCount++;
                swCallTreeDelete(lastChild);
              }
              rtn = true;
            }
            else
              rtn = true;
          }
          else
            SW_LOG_ERROR(&buildCallTreeLogger, "Failed to find current parent on the stack: lastChild = %p, currentParent = %p", (void *)lastChild, (void *)currentParent);
        }
        else
        {
          SW_LOG_ERROR(&buildCallTreeLogger, "threadId %u: lastChild = %p, currentParent = %p, function address mismatch 0x%lx != 0x%lx",
                        threadData->threadId, (void *)lastChild, (void *)currentParent, functionAddress, lastChild->funcAddress);
          // WARNING: this is a hack; I just need to see how misalligned everything is going to be after I added
          if (swCallTreeAddNext(lastChild, functionAddress))
            rtn = true;
          else
            SW_LOG_ERROR(&buildCallTreeLogger, "Failed to add bad function call: lastChild = %p, function = 0x%lx", (void *)lastChild, functionAddress);
        }
      }
      else
        SW_LOG_ERROR(&buildCallTreeLogger, "Stack is empty: lastChild = %p", (void *)lastChild);

      /*
      if (swDynamicArrayPop(threadData->callStack, &lastChild) && lastChild && swDynamicArrayPeek(threadData->callStack, &currentParent) && currentParent)
      {
        if (functionAddress == lastChild->funcAddress)
        {
          if (currentParent->count > 1)
          {
            if (swCallTreeCompare(&(currentParent->children[currentParent->count - 2]), lastChild, true) == 0)
            {
              currentParent->count--;
              currentParent->children[currentParent->count - 1].repeatCount++;
              swCallTreeDelete(lastChild);
            }
            rtn = true;
          }
          else
            rtn = true;
        }
        else
        {
          SW_LOG_ERROR(&buildCallTreeLogger, "threadId %u: lastChild = %p, currentParent = %p, function address mismatch 0x%lX != 0x%lX",
                        threadData->threadId, (void *)lastChild, (void *)currentParent, functionAddress, lastChild->funcAddress);
        }
      }
      else
        SW_LOG_ERROR(&buildCallTreeLogger, "lastChild = %p, currentParent = %p", (void *)lastChild, (void *)currentParent);
      */
    }
    else
    {
      if (swDynamicArrayPeek(threadData->callStack, &currentParent) && currentParent)
      {
        swCallTree *nextChild = swCallTreeAddNext(currentParent, functionAddress);
        if (nextChild)
        {
          rtn = swDynamicArrayPush(threadData->callStack, &nextChild);
          if (!rtn)
            SW_LOG_ERROR(&buildCallTreeLogger, "Failed to push next child to stack");
        }
        else
          SW_LOG_ERROR(&buildCallTreeLogger, "Failed to add next child to currentParent");
      }
      else
        SW_LOG_ERROR(&buildCallTreeLogger, "No parent on the stack");
    }
  }
  return rtn;
}

static bool swBuildCallTreeProcessThreadFile(int64_t threadId)
{
  bool rtn = false;
  if (threadId > 0)
  {
    swDynamicString *threadFile = swDynamicStringNewFromFormat("%.*s.%lu", (int)(inputFileName.len), inputFileName.data, (uint64_t)threadId);
    if (threadFile)
    {
      size_t fileSize = swFileGetSize(threadFile->data);
      if (fileSize)
      {
        int fd = open(threadFile->data, O_RDONLY, 0);
        if (fd > -1)
        {
          swBuildCallTreeThreadData *threadData = swBuildCallTreeThreadDataNew((pid_t)threadId);
          if (threadData)
          {
            if (swHashMapLinearInsert(threadMap, &(threadData->threadId), threadData))
            {
              off_t mapOffset = 0;
              size_t mapSize = (size_t)getpagesize() * 1024;
              char *dataAddressesEnd = NULL;
              uint64_t *currentAddress = NULL;
              uint64_t lastAddr = 0;
              while (mapOffset < (off_t)fileSize)
              {
                mapSize = ((fileSize >= (mapOffset + mapSize))? mapSize : (fileSize - mapOffset));
                char *dataAddressesStart = (char *)mmap(NULL, mapSize, (PROT_READ), (MAP_PRIVATE | MAP_POPULATE), fd, mapOffset);
                if (dataAddressesStart)
                {
                  dataAddressesEnd = dataAddressesStart + mapSize;
                  currentAddress = (uint64_t *)dataAddressesStart;
                  while (currentAddress < (uint64_t *)dataAddressesEnd)
                  {
                    uint64_t addr = (*currentAddress) & (~SW_FUNCTION_END);
                    bool end = (((*currentAddress) & SW_FUNCTION_END) != 0 );
                    if (swBuildCallTreeThreadDataAppend(threadData, addr, end))
                      currentAddress++;
                    else
                    {
                      SW_LOG_ERROR(&buildCallTreeLogger, "thread %lu: failed to append %s address 0x%lX to the tree; offset = 0x%lX", threadId, ((end)? "END" : "START"), addr,
                        mapOffset + (uint64_t)currentAddress - (uint64_t)dataAddressesStart);
                      lastAddr = addr;
                      break;
                    }
                  }
                  if (currentAddress < (uint64_t *)dataAddressesEnd)
                  {
                    SW_LOG_ERROR(&buildCallTreeLogger, "currentAddress(%p) < dataAddressesEnd(%p)", currentAddress, dataAddressesEnd);
                    break;
                  }
                  munmap(dataAddressesStart, mapSize);
                  mapOffset += mapSize;
                }
                else
                {
                  SW_LOG_ERROR(&buildCallTreeLogger, "mmap failed to create memory map of size %zu with offset %ld, \'%s\'", mapSize, mapOffset, strerror(errno));
                  break;
                }
              }
              // if NULL address is encountered, we are done
              // the reason for it is that we can't expect the thread to terminate properly and
              // cleanup its thread specific data when running trace
              if (mapOffset == (off_t)fileSize || !lastAddr)
                rtn = true;
            }
            else
              swBuildCallTreeThreadDataDelete(threadData);
          }
          close(fd);
        }
      }
      swDynamicStringDelete(threadFile);
    }
  }
  return rtn;
}


static bool swBuildCallTreeStart()
{
  bool rtn = false;

  if (inputFileName.len && threadIdList.count)
  {
    if ((functions = swHashSetLinearNew((swHashKeyHashFunction)swBuildCallTreeFunctionHash, (swHashKeyEqualFunction)swBuildCallTreeFunctionEqual, NULL)))
    {
      if ((threadMap = swHashMapLinearNew((swHashKeyHashFunction)swBuildCallTreeThreadDataKeyHash, (swHashKeyEqualFunction)swBuildCallTreeThreadDataKeyEqual, NULL, (swHashValueDeleteFunction)swBuildCallTreeThreadDataDelete)))
      {
        int64_t *threadIds = (int64_t *)threadIdList.data;
        uint32_t i = 0;
        for(; i < threadIdList.count; i++)
        {
          if (!swBuildCallTreeProcessThreadFile(threadIds[i]))
            break;
        }
        if (i == threadIdList.count)
          rtn = true;
        else
        {
          swHashMapLinearDelete(threadMap);
          threadMap = 0;
        }
      }
      if (!rtn)
      {
        swHashSetLinearDelete(functions);
        functions = NULL;
      }
    }
  }
  return rtn;
}

static void swBuildCallTreePrintFunction(swCallTree *node, uint32_t level, void *data)
{
  FILE *out = (FILE *)data;
  fprintf(out, "%u %#lx %u\n", level, node->funcAddress, node->repeatCount);
  swHashSetLinearInsert(functions, &(node->funcAddress));
}

static bool swBuildCallTreeFunctionPrint()
{
  bool rtn = false;
  swHashSetLinearIterator iter;
  if (swHashSetLinearIteratorInit(&iter, functions))
  {
    swDynamicString *functionFile = swDynamicStringNewFromFormat("%.*s.addr", (int)(inputFileName.len), inputFileName.data);
    if (functionFile)
    {
      FILE *outStream = fopen(functionFile->data, "w");
      if (outStream)
      {
        uint64_t *functionAddress = NULL;
        while ((functionAddress = swHashSetLinearIteratorNext(&iter)))
          fprintf(outStream, "%#lx\n", *functionAddress);
        fclose(outStream);
        rtn = true;
      }
      swDynamicStringDelete(functionFile);
    }
  }
  return rtn;
}


static bool swBuildCallTreePrint(pid_t threadId, swCallTree *callTree)
{
  bool rtn = false;
  if (threadId && callTree)
  {
    swDynamicString *threadFile = swDynamicStringNewFromFormat("%.*s.%lu.out", (int)(inputFileName.len), inputFileName.data, (uint64_t)threadId);
    if (threadFile)
    {
      FILE *outStream = fopen(threadFile->data, "w");
      if (outStream)
      {
        swCallTreeWalk(callTree, swBuildCallTreePrintFunction, outStream);
        fclose(outStream);
        rtn = true;
      }
      swDynamicStringDelete(threadFile);
    }
  }
  return rtn;
}

static bool swBuildCallTreeProcess()
{
  bool rtn = false;
  swHashMapLinearIterator iter;
  if (swHashMapLinearIteratorInit(&iter, threadMap))
  {
    rtn = true;
    swBuildCallTreeThreadData *threadData = NULL;
    while(rtn && swHashMapLinearIteratorNext(&iter, (void **)&threadData))
      rtn = swBuildCallTreePrint(threadData->threadId, threadData->callTree);
    if (rtn)
      rtn = swBuildCallTreeFunctionPrint();
  }
  return rtn;
}

static swInitData buildCallTreeData = {.startFunc = swBuildCallTreeStart, .stopFunc = swBuildCallTreeStop, .name = "Build Call Tree"};

swInitData *swBuildCallTreeDataGet()
{
  return &buildCallTreeData;
}

int main (int argc, char *argv[])
{
  int rtn = EXIT_FAILURE;
  swInitData *initData[] =
  {
    swInitCommandLineDataGet(&argc, argv, "Build Call Tree Tool", NULL),
    swInitLogManagerDataGet(NULL),
    swBuildCallTreeDataGet(),
    NULL
  };

  if (swInitStart (initData))
  {
    if (swBuildCallTreeProcess())
      rtn = EXIT_SUCCESS;
    swInitStop(initData);
  }

  return rtn;
}
