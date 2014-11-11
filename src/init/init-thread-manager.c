#include "init/init-thread-manager.h"

#include "command-line/option-category.h"

int64_t threadJoinWaitInterval = 1000;

swOptionCategoryModuleDeclare(swThreadManagerOptions, "Thread Manager Options",
  swOptionDeclareScalar("term-wait-interval",    "Amount of time in milliseconds to wait for threads to join before cancelling them",
    NULL,   &threadJoinWaitInterval,              swOptionValueTypeInt, false)
);

static void *threadManagerArrayData[2] = {NULL};

static bool swInitThreadManagerStart()
{
  bool rtn = false;
  swEdgeLoop **loopPtr = threadManagerArrayData[0];
  swThreadManager **threadManagerPtr = threadManagerArrayData[1];
  if (loopPtr && *loopPtr && threadManagerPtr && !(*threadManagerPtr))
  {
    if (threadJoinWaitInterval > 0)
    {
      if ((*threadManagerPtr = swThreadManagerNew(*loopPtr, threadJoinWaitInterval)))
        rtn = true;
    }
  }
  return rtn;
}

static void swInitThreadManagerStop()
{
  swThreadManager **threadManagerPtr = threadManagerArrayData[1];
  if (threadManagerPtr && *threadManagerPtr)
    swThreadManagerDelete(*threadManagerPtr);
  threadManagerArrayData[0] = NULL;
  threadManagerArrayData[1] = NULL;
}

static swInitData threadManagerData = {.startFunc = swInitThreadManagerStart, .stopFunc = swInitThreadManagerStop, .name = "Thread Manager"};

swInitData *swInitThreadManagerDataGet(swEdgeLoop **loopPtr, swThreadManager **threadManagerPtr)
{
  threadManagerArrayData[0] = loopPtr;
  threadManagerArrayData[1] = threadManagerPtr;
  return &threadManagerData;
}
