#include "edge-async.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>

bool swEdgeAsyncInit(swEdgeAsync *asyncWatcher, swEdgeAsyncCallback cb)
{
  bool rtn = false;
  if (asyncWatcher && cb)
  {
    memset(asyncWatcher, 0, sizeof(swEdgeAsync));
    swEdgeWatcher *watcher = (swEdgeWatcher *)asyncWatcher;
    watcher->event.events = (EPOLLIN | EPOLLRDHUP | EPOLLET);
    watcher->event.data.ptr = asyncWatcher;
    watcher->type = swWatcherTypeAsync;
    asyncWatcher->eventCB = cb;
    rtn = true;
  }
  return rtn;
}

bool swEdgeAsyncStart(swEdgeAsync *asyncWatcher, swEdgeLoop *loop)
{
  bool rtn = false;
  if (asyncWatcher && loop)
  {
    swEdgeWatcher *watcher = (swEdgeWatcher *)asyncWatcher;
    if ((watcher->fd = eventfd(0, (EFD_NONBLOCK | EFD_CLOEXEC))) >= 0)
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
  }
  return rtn;
}

bool swEdgeAsyncSend(swEdgeAsync *asyncWatcher)
{
  bool rtn = false;
  swEdgeWatcher *watcher = (swEdgeWatcher *)asyncWatcher;
  if (asyncWatcher && watcher->loop)
  {
    eventfd_t value = 1;
    if (eventfd_write(watcher->fd, value) == 0)
      rtn = true;
  }
  return rtn;
}

void swEdgeAsyncStop(swEdgeAsync *asyncWatcher)
{
  swEdgeWatcher *watcher = (swEdgeWatcher *)asyncWatcher;
  if (asyncWatcher && watcher->loop)
  {
    swEdgeLoopWatcherRemove(watcher->loop, watcher);
    watcher->loop = NULL;
    close(watcher->fd);
    watcher->fd = -1;
  }
}

void swEdgeAsyncClose(swEdgeAsync *asyncWatcher)
{
  swEdgeAsyncStop(asyncWatcher);
}

