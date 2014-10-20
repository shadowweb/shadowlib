#include "thread/thread-manager.h"

#include "core/memory.h"

#include <errno.h>
#include <string.h>

swThreadManager *swThreadManagerNew(swEdgeLoop *loop, uint64_t joinWaitInterval)
{
  swThreadManager *manager = swMemoryMalloc(sizeof(*manager));
  if (!swThreadManagerInit(manager, loop, joinWaitInterval))
  {
    swMemoryFree(manager);
    manager = NULL;
  }
  return manager;
}

bool swThreadManagerInit(swThreadManager *manager, swEdgeLoop *loop, uint64_t joinWaitInterval)
{
  bool rtn = false;
  if (manager && loop)
  {
    memset(manager, 0, sizeof(*manager));
    if ((swSparseArrayInit(&(manager->threadInfoArray), sizeof(swThreadInfo), 64, 4)))
    {
      manager->joinWaitInterval = joinWaitInterval;
      manager->loop = loop;
      rtn = true;
    }
  }
  return rtn;
}

static void swThreadInfoClean(swThreadInfo *threadInfo)
{
  if (threadInfo->doneFunc)
    threadInfo->doneFunc(threadInfo->data, threadInfo->returnValue);
  swEdgeAsyncClose(&(threadInfo->doneEvent));
  swSparseArrayRemove(&(threadInfo->manager->threadInfoArray), threadInfo->position);
}

static void swThreadInfoForceJoin(swThreadInfo *threadInfo)
{
  if (pthread_cancel(threadInfo->threadId))
  {
    pthread_join(threadInfo->threadId, &(threadInfo->returnValue));
    swThreadInfoClean(threadInfo);
  }
}

static void swThreadInfoJoinWaitCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  if (timer)
  {
    swThreadInfo *threadInfo = swEdgeWatcherDataGet(timer);
    swEdgeTimerClose(timer);
    if (threadInfo)
    {
      if (pthread_tryjoin_np(threadInfo->threadId, &(threadInfo->returnValue)) == 0)
        swThreadInfoClean(threadInfo);
      else
        swThreadInfoForceJoin(threadInfo);
    }
  }
}

static bool swThreadInfoFinalizeThread(swThreadInfo *threadInfo)
{
  bool rtn = false;
  if (threadInfo)
  {
    int error = pthread_tryjoin_np(threadInfo->threadId, &(threadInfo->returnValue));
    if (error == 0)
    {
      rtn = true;
      if (threadInfo->joinWaitTimer.watcher.loop)
        swEdgeTimerClose(&(threadInfo->joinWaitTimer));
      swThreadInfoClean(threadInfo);
    }
    else if (error == EBUSY)
    {
      if (!threadInfo->joinWaitTimer.watcher.loop)
      {
        if (swEdgeTimerInit(&(threadInfo->joinWaitTimer), swThreadInfoJoinWaitCallback, false))
        {
          swEdgeWatcherDataSet(&(threadInfo->joinWaitTimer), threadInfo);
          if (swEdgeTimerStart(&(threadInfo->joinWaitTimer), threadInfo->manager->loop, threadInfo->manager->joinWaitInterval, 0, false))
            rtn = true;
          else
            swEdgeTimerClose(&(threadInfo->joinWaitTimer));
        }
      }
      else
        rtn = true;
    }
  }
  return rtn;
}

static void swThreadInfoDoneCallback(swEdgeAsync *asyncEvent, eventfd_t eventCount, uint32_t events)
{
  if (asyncEvent)
  {
    swThreadInfo *threadInfo = swEdgeWatcherDataGet(asyncEvent);
    if (!swThreadInfoFinalizeThread(threadInfo))
      swThreadInfoForceJoin(threadInfo);
  }
}

static void *swThreadInfoRun(void *data)
{
  void *rtn = NULL;
  // we are going to be in really bad shape if data is NULL, so I prefer we code dump here
  swThreadInfo *threadInfo = data;
  rtn = threadInfo->runFunc(threadInfo->data);
  // not checking return status as there is nothing I can do here
  swEdgeAsyncSend(&(threadInfo->doneEvent));
  return rtn;
}

bool swThreadManagerStartThread(swThreadManager *manager, swThreadRunFunction runFunc, swThreadStopFunction stopFunc, swThreadDoneFunction doneFunc, void *data)
{
  bool rtn = false;
  if (manager && runFunc)
  {
    swThreadInfo *threadInfo = NULL;
    uint32_t position = 0;
    if (swSparseArrayAcquireFirstFree(&(manager->threadInfoArray), &position, (void **)(&threadInfo)))
    {
      memset(threadInfo, 0, sizeof(*threadInfo));
      threadInfo->position = position;
      threadInfo->runFunc = runFunc;
      threadInfo->stopFunc = stopFunc;
      threadInfo->doneFunc = doneFunc;
      threadInfo->data = data;
      threadInfo->manager = manager;
      if (swEdgeAsyncInit(&(threadInfo->doneEvent), swThreadInfoDoneCallback))
      {
        swEdgeWatcherDataSet(&(threadInfo->doneEvent), threadInfo);
        if (swEdgeAsyncStart(&(threadInfo->doneEvent), manager->loop))
        {
          if (!pthread_create(&(threadInfo->threadId), NULL, swThreadInfoRun, threadInfo))
            rtn = true;
        }
      }
      if (!rtn)
      {
        swEdgeAsyncClose(&(threadInfo->doneEvent));
        swSparseArrayRemove(&(manager->threadInfoArray), position);
      }
    }
  }
  if (!rtn && doneFunc)
    doneFunc(data, NULL);
  return rtn;
}

static bool swThreadInfoStop(swThreadInfo *threadInfo)
{
  bool rtn = false;
  if (threadInfo)
  {
    if (threadInfo->stopFunc)
      threadInfo->stopFunc(threadInfo->data);
    rtn = swThreadInfoFinalizeThread(threadInfo);
  }
  return rtn;
}

void swThreadManagerRelease(swThreadManager *manager)
{
  if (manager)
  {
    manager->terminate = true;
    if (swSparseArrayWalk(&(manager->threadInfoArray), (swSparseArrayWalkFunction)swThreadInfoStop))
    {
      while (swSparseArrayCount(manager->threadInfoArray))
        swEdgeLoopRun(manager->loop, true);
    }
    else
      swSparseArrayWalk(&(manager->threadInfoArray), (swSparseArrayWalkFunction)swThreadInfoForceJoin);
    swSparseArrayRelease(&(manager->threadInfoArray));
  }
}

void swThreadManagerDelete(swThreadManager *manager)
{
  if (manager)
  {
    swThreadManagerRelease(manager);
    swMemoryFree(manager);
  }
}
