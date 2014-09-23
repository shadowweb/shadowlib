#include "io/edge-loop.h"
#include "io/edge-timer.h"
#include "io/edge-signal.h"
#include "io/edge-async.h"
#include "io/edge-io.h"
#include "core/memory.h"

#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define SW_EPOLLEVENTS_SIZE 64

static const char const *swWatcherTypeText[swWatcherTypeMax] =
{
  [swWatcherTypeTimer]          = "Timer",
  [swWatcherTypePeriodicTimer]  = "PeriodicTimer",
  [swWatcherTypeSignal]         = "Signal",
  [swWatcherTypeAsync]          = "Async",
  [swWatcherTypeIO]             = "IO",
};

const char const *swWatcherTypeTextGet(swWatcherType watcherType)
{
  if (watcherType > swWatcherTypeNone && watcherType < swWatcherTypeMax)
    return swWatcherTypeText[watcherType];
  return NULL;
}

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

static inline bool swEdgeLoopAsyncProcess(swEdgeAsync *asyncWatcher, uint32_t events)
{
  bool rtn = false;
  if (asyncWatcher)
  {
    if (events & EPOLLIN)
    {
      swEdgeWatcher *watcher = (swEdgeWatcher *)asyncWatcher;
      int readSize = 0;
      eventfd_t value = 0;
      while ((readSize = read(watcher->fd, &value, sizeof(value))) == sizeof(value))
        asyncWatcher->eventCB(asyncWatcher, value, events);
      if (readSize < 0 && errno == EAGAIN)
        rtn = true;
    }
    if (!rtn)
      asyncWatcher->eventCB(asyncWatcher, 0, events);
  }
  return rtn;
}

static inline bool swEdgeLoopIOProcess(swEdgeIO *ioWatcher, uint32_t events)
{
  bool rtn = false;
  if (ioWatcher && ioWatcher->ioCB)
  {
    // printf("'%s' (%d): calling ioCB with events 0x%x\n",
    //        __func__, swEdgeIOFDGet(ioWatcher), events);
    ioWatcher->ioCB(ioWatcher, events);
    rtn = true;
  }
  return rtn;
}

static swEdgeWatcherProcess watcherProcess[swWatcherTypeMax] =
{
  [swWatcherTypeTimer]          = (swEdgeWatcherProcess)swEdgeLoopTimerProcess,
  [swWatcherTypePeriodicTimer]  = (swEdgeWatcherProcess)swEdgeLoopTimerProcess,
  [swWatcherTypeSignal]         = (swEdgeWatcherProcess)swEdgeLoopSignalProcess,
  [swWatcherTypeAsync]          = (swEdgeWatcherProcess)swEdgeLoopAsyncProcess,
  [swWatcherTypeIO]             = (swEdgeWatcherProcess)swEdgeLoopIOProcess,
};

swEdgeLoop *swEdgeLoopNew()
{
  swEdgeLoop *rtn = NULL;
  swEdgeLoop *newLoop = swMemoryCalloc(1, sizeof(swEdgeLoop));
  if (newLoop)
  {
    if (swFastArrayInit(&(newLoop->epollEvents), sizeof(struct epoll_event), SW_EPOLLEVENTS_SIZE))
    {
      if (swFastArrayInit(&(newLoop->pendingEvents[0]), sizeof(swEdgeWatcher *), SW_EPOLLEVENTS_SIZE) &&
          swFastArrayInit(&(newLoop->pendingEvents[1]), sizeof(swEdgeWatcher *), SW_EPOLLEVENTS_SIZE))
      {
        if ((newLoop->fd = epoll_create1(EPOLL_CLOEXEC)) >= 0)
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
    if (swFastArraySize(loop->epollEvents))
      swFastArrayClear(&(loop->epollEvents));
    if (swFastArraySize(loop->pendingEvents[0]))
      swFastArrayClear(&(loop->pendingEvents[0]));
    if (swFastArraySize(loop->pendingEvents[1]))
      swFastArrayClear(&(loop->pendingEvents[1]));
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
    {
      if (!watcher->pendingEvents)
        rtn = true;
      else
      {
        if (swFastArrayCount(loop->pendingEvents[watcher->pendingArray]) > watcher->pendingPosition)
        {
          if (swFastArraySet(loop->pendingEvents[watcher->pendingArray], watcher->pendingPosition, NULL))
          {
            watcher->pendingEvents = 0;
            rtn = true;
          }
        }
      }
    }
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

bool swEdgeWatcherPendingSet(swEdgeWatcher *watcher, uint32_t events)
{
  bool rtn = false;
  swEdgeLoop *loop = swEdgeWatcherLoopGet(watcher);
  if (loop && watcher && watcher->type == swWatcherTypeIO)
  {
    if (watcher->pendingEvents)
    {
      watcher->pendingEvents |= events;
      rtn = true;
    }
    else
    {
      if (swFastArrayPush(loop->pendingEvents[loop->currentPending], watcher))
      {
        watcher->pendingPosition = swFastArrayCount(loop->pendingEvents[loop->currentPending]) - 1;
        watcher->pendingEvents = events;
        watcher->pendingArray = loop->currentPending;
        rtn = true;
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
      uint32_t pendingCount = swFastArrayCount(loop->pendingEvents[loop->currentPending]);
      int eventCount = epoll_wait(loop->fd, (struct epoll_event *)swFastArrayData(loop->epollEvents), swFastArraySize(loop->epollEvents), ((pendingCount)? 0 : defaultTimeout));
      if (eventCount >= 0)
      {
        // first pass: transfer all events to pending
        for (int i = 0; i < eventCount; i++)
        {
          swEdgeWatcher *watcher = ((struct epoll_event *)swFastArrayData(loop->epollEvents))[i].data.ptr;
          if (watcher->type > swWatcherTypeNone && watcher->type < swWatcherTypeMax)
          {
            if(!watcher->pendingEvents)
            {
              if (swFastArrayPush(loop->pendingEvents[loop->currentPending], watcher))
              {
                watcher->pendingArray = loop->currentPending;
                watcher->pendingPosition = swFastArrayCount(loop->pendingEvents[loop->currentPending]) - 1;
                watcher->pendingEvents = ((struct epoll_event *)swFastArrayData(loop->epollEvents))[i].events;
              }
            }
            else
              watcher->pendingEvents |= ((struct epoll_event *)swFastArrayData(loop->epollEvents))[i].events;
          }
        }
        // resize array if needed
        if ((uint32_t)eventCount == swFastArraySize(loop->epollEvents) && !swFastArrayResize(&(loop->epollEvents), swFastArraySize(loop->epollEvents)))
          run = false;
      }
      else
        run = false;

      // second pass: run all pending events
      pendingCount = swFastArrayCount(loop->pendingEvents[loop->currentPending]);
      if (pendingCount)
      {
        uint32_t lastPending = loop->currentPending;
        loop->currentPending++;
        for (uint32_t i = 0; i < pendingCount; i++)
        {
          swEdgeWatcher *watcher = NULL;
          if (swFastArrayGet(loop->pendingEvents[lastPending], i, watcher) && watcher && watcherProcess[watcher->type])
          {
            uint32_t pendingEvents = watcher->pendingEvents;
            watcher->pendingEvents = 0;
            watcherProcess[watcher->type](watcher, pendingEvents);
          }
        }
        loop->pendingEvents[lastPending].count = 0;
      }
      if (loop->shutdown)
        run = !loop->shutdown;
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

/*
void swEdgeLoopPendingPrint(swEdgeLoop *loop)
{
  if (loop)
  {
    printf("currentPending = %u\n", loop->currentPending);
    for (uint8_t i = 0; i < 2; i++)
    {
      printf("Pending %u: ", i);
      for(uint32_t j = 0; j < loop->pendingEvents[i].count; j++)
      {
        swEdgeWatcher *watcher = NULL;
        if (swFastArrayGet(loop->pendingEvents[i], j, watcher))
        {
          if (watcher)
            printf("%p (%s, %u, %u, 0x%x), ", (void *)watcher, swWatcherTypeTextGet(watcher->type), watcher->pendingArray, watcher->pendingPosition, watcher->pendingEvents);
          else
            printf("%p (%s, %u, %u, 0x%x), ", (void *)watcher, "", 0, 0, 0);
        }
        else
          printf("%p, ", (void *)NULL);
      }
      printf("\n");
    }
  }
}
*/

