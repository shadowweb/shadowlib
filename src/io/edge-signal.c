#include "edge-signal.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

bool swEdgeSignalInit(swEdgeSignal *signalWatcher, swEdgeSignalCallback cb)
{
  bool rtn = false;
  if (signalWatcher && cb)
  {
    memset(signalWatcher, 0, sizeof(swEdgeSignal));
    swEdgeWatcher *watcher = (swEdgeWatcher *)signalWatcher;
    watcher->event.events = (EPOLLIN | EPOLLRDHUP | EPOLLET);
    watcher->event.data.ptr = signalWatcher;
    watcher->type = swWatcherTypeSignal;
    signalWatcher->signalCB = cb;
    rtn = true;
  }
  return rtn;
}

bool swEdgeSignalStart(swEdgeSignal *signalWatcher, swEdgeLoop *loop, sigset_t mask)
{
  bool rtn = false;
  if (signalWatcher && loop)
  {
    signalWatcher->mask = mask;
    swEdgeWatcher *watcher = (swEdgeWatcher *)signalWatcher;
    if (sigprocmask(SIG_BLOCK, &(signalWatcher->mask), NULL) == 0)
    {
      if ((watcher->fd = signalfd(-1, &(signalWatcher->mask), (SFD_NONBLOCK | SFD_CLOEXEC))) >= 0)
      {
        if (swEdgeLoopWatcherAdd(loop, watcher))
        {
          watcher->loop = loop;
          rtn = true;
        }
        else
        {
          close(watcher->fd);
          watcher->fd = -1;
        }
      }
      if (!rtn)
        sigprocmask(SIG_UNBLOCK, &(signalWatcher->mask), NULL);
    }
  }
  return rtn;
}

void swEdgeSignalStop(swEdgeSignal *signalWatcher)
{
  swEdgeWatcher *watcher = (swEdgeWatcher *)signalWatcher;
  if (signalWatcher && watcher->loop)
  {
    swEdgeLoopWatcherRemove(watcher->loop, watcher);
    watcher->loop = NULL;
    close(watcher->fd);
    watcher->fd = -1;
    sigprocmask(SIG_UNBLOCK, &(signalWatcher->mask), NULL);
  }
}

void swEdgeSignalClose(swEdgeSignal *signalWatcher)
{
  swEdgeSignalStop(signalWatcher);
}
