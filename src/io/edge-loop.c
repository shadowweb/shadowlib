#include "edge-loop.h"
#include "edge-timer.h"
#include "edge-signal.h"
#include "edge-event.h"

#include <core/memory.h>

#include <unistd.h>

#define SW_EPOLLEVENTS_SIZE 64

typedef bool (*swEdgeWatcherProcess)(swEdgeWatcher *watcher, uint32_t events);

static swEdgeWatcherProcess watcherProcess[swWatcherTypeMax] =
{
  [swWatcherTypeTimer]          = (swEdgeWatcherProcess)swEdgeTimerProcess,
  [swWatcherTypePeriodicTimer]  = (swEdgeWatcherProcess)swEdgeTimerProcess,
  [swWatcherTypeSignal]         = (swEdgeWatcherProcess)swEdgeSignalProcess,
  [swWatcherTypeEvent]          = (swEdgeWatcherProcess)swEdgeEventProcess,
};

swEdgeLoop *swEdgeLoopNew()
{
  swEdgeLoop *rtn = NULL;
  swEdgeLoop *newLoop = swMemoryCalloc(1, sizeof(swEdgeLoop));
  if (newLoop)
  {
    if (swStaticArrayInit(&(newLoop->epollEvents), sizeof(struct epoll_event), SW_EPOLLEVENTS_SIZE))
    {
      if ((newLoop->fd = epoll_create1(EPOLL_CLOEXEC)) >= 0)
      {
        rtn = newLoop;
      }
    }
    if (!rtn)
      swEdgeLoopDelete(newLoop);
  }
  return rtn;
}

void swEdgeLoopDelete(swEdgeLoop *loop)
{
  if (loop)
  {
    if (swStaticArraySize(loop->epollEvents))
      swStaticArrayClear(&(loop->epollEvents));
    if (loop->fd >= 0)
      close(loop->fd);
    swMemoryFree(loop);
  }
}

bool swEdgeLoopWatcherAdd(swEdgeLoop* loop, swEdgeWatcher* watcher)
{
  bool rtn = false;
  if (loop && watcher)
  {
    if (epoll_ctl(loop->fd, EPOLL_CTL_ADD, watcher->fd, &(watcher->event)) == 0)
      rtn = true;
  }
  return rtn;
}

bool swEdgeLoopWatcherRemove(swEdgeLoop *loop, swEdgeWatcher *watcher)
{
  bool rtn = false;
  if (loop && watcher)
  {
    if (epoll_ctl(loop->fd, EPOLL_CTL_DEL, watcher->fd, &(watcher->event)) == 0)
      rtn = true;
  }
  return rtn;
}

// TODO: add function calls for running event loop once

void swEdgeLoopRun(swEdgeLoop *loop)
{
  if (loop)
  {
    loop->shutdown = false;
    int defaultTimeout = 1000;  // 1 second
    bool run = true;
    while (run)
    {
      int eventCount = epoll_wait(loop->fd, (struct epoll_event *)swStaticArrayData(loop->epollEvents), swStaticArraySize(loop->epollEvents), defaultTimeout);
      // if (eventCount > 0)
      //   printf("'%s': received %d events\n", __func__, eventCount);
      if (eventCount >= 0)
      {
        // process epollEvents
        for (int i = 0; i < eventCount; i++)
        {
          swEdgeWatcher *watcher = ((struct epoll_event *)swStaticArrayData(loop->epollEvents))[i].data.ptr;
          if (watcher->type < swWatcherTypeMax && watcherProcess[watcher->type])
            watcherProcess[watcher->type](watcher, ((struct epoll_event *)swStaticArrayData(loop->epollEvents))[i].events);
        }
        // increase array size to accomodate more events
        if ((uint32_t)eventCount == swStaticArraySize(loop->epollEvents) && !swStaticArrayResize(&(loop->epollEvents), swStaticArraySize(loop->epollEvents)))
          run = false;
        if (loop->shutdown)
          run = !loop->shutdown;
      }
      else
      {
        run = false;
      }
    }
  }
}

void swEdgeLoopBreak(swEdgeLoop *loop)
{
  if (loop)
    loop->shutdown = true;
}

