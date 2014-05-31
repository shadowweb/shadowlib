#include "edge-loop.h"
#include "edge-timer.h"
#include "edge-signal.h"
#include "edge-event.h"
#include "edge-io.h"

#include <core/memory.h>

#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define SW_EPOLLEVENTS_SIZE 64

typedef bool (*swEdgeWatcherProcess)(swEdgeWatcher *watcher, uint32_t events);

static inline bool swEdgeLoopTimerProcess(swEdgeTimer *timerWatcher, uint32_t events)
{
  bool rtn = false;
  if (timerWatcher)
  {
    if (events & EPOLLIN)
    {
      swEdgeWatcher *watcher = (swEdgeWatcher *)timerWatcher;
      int readSize = 0;
      uint64_t expiredCount = 0;
      while ((readSize = read(watcher->fd, &expiredCount, sizeof(uint64_t))) == sizeof(uint64_t))
                timerWatcher->timerCB(timerWatcher, expiredCount, events);
      // should close fd (I am not sure this will ever happen)
      if (readSize < 0 && errno == EAGAIN)
        rtn = true;
    }
    if (!rtn)
            timerWatcher->timerCB(timerWatcher, 0, events);
  }
  return rtn;
}

static inline bool swEdgeLoopSignalProcess(swEdgeSignal *signalWatcher, uint32_t events)
{
  bool rtn = false;
  if (signalWatcher)
  {
    if (events & EPOLLIN)
    {
      swEdgeWatcher *watcher = (swEdgeWatcher *)signalWatcher;
      int readSize = 0;
      struct signalfd_siginfo signalInfo = {0};
      while ((readSize = read(watcher->fd, &signalInfo, sizeof(struct signalfd_siginfo))) == sizeof(struct signalfd_siginfo))
      {
        if (sigismember(&(signalWatcher->mask), signalInfo.ssi_signo) == 1)
          signalWatcher->signalCB(signalWatcher, &signalInfo, events);
        else
          break;
      }
      if (readSize < 0 && errno == EAGAIN)
        rtn = true;
    }
    if (!rtn)
      signalWatcher->signalCB(signalWatcher, NULL, events);
  }
  return rtn;
}

static inline bool swEdgeLoopEventProcess(swEdgeEvent *eventWatcher, uint32_t events)
{
  bool rtn = false;
  if (eventWatcher)
  {
    if (events & EPOLLIN)
    {
      swEdgeWatcher *watcher = (swEdgeWatcher *)eventWatcher;
      int readSize = 0;
      eventfd_t value = 0;
      while ((readSize = read(watcher->fd, &value, sizeof(value))) == sizeof(value))
        eventWatcher->eventCB(eventWatcher, value, events);
      if (readSize < 0 && errno == EAGAIN)
        rtn = true;
    }
    if (!rtn)
      eventWatcher->eventCB(eventWatcher, 0, events);
  }
  return rtn;
}

static inline bool swEdgeLoopIOProcess(swEdgeIO *ioWatcher, uint32_t events)
{
  bool rtn = false;
  if (ioWatcher && ioWatcher->ioCB)
  {
    if (ioWatcher->pendingEvents)
    {
      // printf("'%s' (%d): delaying till pending is called, events 0x%x, pending events 0x%x\n",
      //        __func__, swEdgeIOFDGet(ioWatcher), events, ioWatcher->pendingEvents);
      ioWatcher->pendingEvents |= events;
    }
    else
    {
      // printf("'%s' (%d): calling ioCB with events 0x%x\n",
      //        __func__, swEdgeIOFDGet(ioWatcher), events);
      ioWatcher->ioCB(ioWatcher, events);
    }
    rtn = true;
  }
  return rtn;
}

static swEdgeWatcherProcess watcherProcess[swWatcherTypeMax] =
{
  [swWatcherTypeTimer]          = (swEdgeWatcherProcess)swEdgeLoopTimerProcess,
  [swWatcherTypePeriodicTimer]  = (swEdgeWatcherProcess)swEdgeLoopTimerProcess,
  [swWatcherTypeSignal]         = (swEdgeWatcherProcess)swEdgeLoopSignalProcess,
  [swWatcherTypeEvent]          = (swEdgeWatcherProcess)swEdgeLoopEventProcess,
  [swWatcherTypeIO]             = (swEdgeWatcherProcess)swEdgeLoopIOProcess,
};

swEdgeLoop *swEdgeLoopNew()
{
  swEdgeLoop *rtn = NULL;
  swEdgeLoop *newLoop = swMemoryCalloc(1, sizeof(swEdgeLoop));
  if (newLoop)
  {
    if (swStaticArrayInit(&(newLoop->epollEvents), sizeof(struct epoll_event), SW_EPOLLEVENTS_SIZE))
    {
      if (swStaticArrayInit(&(newLoop->pendingEvents[0]), sizeof(swEdgeIO *), SW_EPOLLEVENTS_SIZE) &&
          swStaticArrayInit(&(newLoop->pendingEvents[1]), sizeof(swEdgeIO *), SW_EPOLLEVENTS_SIZE))
      {
        if ((newLoop->fd = epoll_create1(EPOLL_CLOEXEC)) >= 0)
        {
          rtn = newLoop;
        }
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
    if (swStaticArraySize(loop->pendingEvents[0]))
      swStaticArrayClear(&(loop->pendingEvents[0]));
    if (swStaticArraySize(loop->pendingEvents[1]))
      swStaticArrayClear(&(loop->pendingEvents[1]));
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

bool swEdgeLoopWatcherModify(swEdgeLoop *loop, swEdgeWatcher *watcher)
{
  bool rtn = false;
  if (loop && watcher)
  {
    if (epoll_ctl(loop->fd, EPOLL_CTL_MOD, watcher->fd, &(watcher->event)) == 0)
      rtn = true;
  }
  return rtn;
}

bool swEdgeLoopPendingAdd(swEdgeLoop *loop, swEdgeWatcher *watcher, uint32_t events)
{
  bool rtn = false;
  if (loop && watcher && watcher->type == swWatcherTypeIO)
  {
    swEdgeIO *ioWatcher = (swEdgeIO *)watcher;
    if (swStaticArrayPush(loop->pendingEvents[loop->currentPending], watcher))
    {
      ioWatcher->pendingPosition = swStaticArrayCount(loop->pendingEvents[loop->currentPending]) - 1;
      ioWatcher->pendingEvents = events;
      rtn = true;
    }
  }
  return rtn;
}

bool swEdgeLoopPendingRemove(swEdgeLoop *loop, swEdgeWatcher *watcher)
{
  bool rtn = false;
  swEdgeIO *ioWatcher = (swEdgeIO *)watcher;
  if (loop && watcher && watcher->type == swWatcherTypeIO && ioWatcher->pendingEvents)
  {
    swEdgeIO *ioWatcherLast = NULL;
    if (swStaticArrayPop(loop->pendingEvents[loop->currentPending], ioWatcherLast))
    {
      if (ioWatcherLast != ioWatcher)
      {
        if (swStaticArraySet(loop->pendingEvents[loop->currentPending], ioWatcher->pendingPosition, ioWatcherLast))
        {
          ioWatcherLast->pendingPosition = ioWatcher->pendingPosition;
          rtn = true;
        }
      }
    }
  }
  return rtn;
}

void swEdgeLoopRun(swEdgeLoop *loop, bool once)
{
  if (loop)
  {
    loop->shutdown = false;
    int defaultTimeout = 1000;  // 1 second
    bool run = true;
    while (run)
    {
      uint32_t pendingCount = swStaticArrayCount(loop->pendingEvents[loop->currentPending]);
      int eventCount = epoll_wait(loop->fd, (struct epoll_event *)swStaticArrayData(loop->epollEvents), swStaticArraySize(loop->epollEvents), ((pendingCount)? 0 : defaultTimeout));
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
        run = false;

      if (pendingCount)
      {
        uint32_t lastPending = loop->currentPending;
        loop->currentPending++;
        for (uint32_t i = 0; i < pendingCount; i++)
        {
          swEdgeWatcher *watcher = NULL;
          if (swStaticArrayGet(loop->pendingEvents[lastPending], i, watcher) && (watcher->type == swWatcherTypeIO))
          {
            swEdgeIO *ioWatcher = (swEdgeIO *)watcher;
            uint32_t pendingEvents = ioWatcher->pendingEvents;
            ioWatcher->pendingEvents = 0;
            watcherProcess[watcher->type](watcher, pendingEvents);
          }
        }
        loop->pendingEvents[lastPending].count = 0;
      }
      if (once)
        run = false;
    }
  }
}

void swEdgeLoopBreak(swEdgeLoop *loop)
{
  if (loop)
    loop->shutdown = true;
}

