#include "edge-timer.h"

#include <core/time.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>

bool swEdgeTimerInit(swEdgeTimer *timer, swEdgeTimerCallback cb)
{
  bool rtn = false;
  if (timer && cb)
  {
    memset(timer, 0, sizeof(swEdgeTimer));
    swEdgeWatcher *watcher = (swEdgeWatcher *)timer;
    watcher->event.events = (EPOLLIN | EPOLLRDHUP | EPOLLET);
    watcher->event.data.ptr = timer;
    watcher->type = swWatcherTypeTimer;
    timer->timerCB = cb;
    if ((watcher->fd = timerfd_create(CLOCK_MONOTONIC, (TFD_NONBLOCK | TFD_CLOEXEC))) >= 0)
      rtn = true;
  }
  return rtn;
}

bool swEdgeTimerStart(swEdgeTimer *timer, swEdgeLoop *loop, uint64_t offset, uint64_t interval, bool absolute)
{
  bool rtn = false;
  if (timer && loop)
  {
    timer->timerSpec.it_value.tv_sec      = swTimeNSecToSec(offset);
    timer->timerSpec.it_value.tv_nsec     = swTimeMSecToNSec( swTimeNSecToSecRem(offset) );
    timer->timerSpec.it_interval.tv_sec   = swTimeNSecToSec(interval);
    timer->timerSpec.it_interval.tv_nsec  = swTimeMSecToNSec( swTimeNSecToSecRem(interval) );
    swEdgeWatcher *watcher = (swEdgeWatcher *)timer;
    if (timerfd_settime(watcher->fd, ((absolute)? TFD_TIMER_ABSTIME : 0), &(timer->timerSpec), NULL) == 0)
    {
      if (swEdgeLoopWatcherAdd(loop, watcher))
      {
        watcher->loop = loop;
        rtn = true;
      }
      else
      {
        struct itimerspec timerSpec = {.it_value = {0, 0}};
        timerfd_settime(watcher->fd, 0, &timerSpec, NULL);
      }
    }
  }
  return rtn;
}

bool swEdgeTimerProcess(swEdgeTimer *timer, uint32_t events)
{
  bool rtn = false;
  if (timer)
  {
    swEdgeWatcher *watcher = (swEdgeWatcher *)timer;
    int readSize = 0;
    uint64_t expiredCount = 0;
    if ((readSize = read(watcher->fd, &expiredCount, sizeof(uint64_t))) == sizeof(uint64_t))
    // while ((readSize = read(watcher->fd, &expiredCount, sizeof(uint64_t))) == sizeof(uint64_t))
    {
      timer->timerCB(timer, expiredCount);
    }
    // should close fd (I am not sure this will ever happen)
    if (readSize < 0 && errno == EAGAIN)
      rtn = true;
    else if (readSize == 0)
      swEdgeTimerClose(timer);
  }
  return rtn;
}

void swEdgeTimerStop(swEdgeTimer *timer)
{
  swEdgeWatcher *watcher = (swEdgeWatcher *)timer;
  if (timer && watcher->loop)
  {
    swEdgeLoopWatcherRemove(watcher->loop, watcher);
    watcher->loop = NULL;
    struct itimerspec timerSpec = {.it_value = {0, 0}};
    timerfd_settime(watcher->fd, 0, &timerSpec, NULL);
  }
}

void swEdgeTimerClose(swEdgeTimer *timer)
{
  swEdgeWatcher *watcher = (swEdgeWatcher *)timer;
  if (timer && watcher->fd >= 0)
  {
    swEdgeTimerStop(timer);
    close(watcher->fd);
    watcher->fd = -1;
  }
}
