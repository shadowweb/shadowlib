#include "edge-event.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>

bool swEdgeEventInit(swEdgeEvent *eventWatcher, swEdgeEventCallback cb)
{
  bool rtn = false;
  if (eventWatcher && cb)
  {
    memset(eventWatcher, 0, sizeof(swEdgeEvent));
    swEdgeWatcher *watcher = (swEdgeWatcher *)eventWatcher;
    watcher->event.events = (EPOLLIN | EPOLLRDHUP | EPOLLET);
    watcher->event.data.ptr = eventWatcher;
    watcher->type = swWatcherTypeEvent;
    eventWatcher->eventCB = cb;
    rtn = true;
  }
  return rtn;
}

bool swEdgeEventStart(swEdgeEvent *eventWatcher, swEdgeLoop *loop)
{
  bool rtn = false;
  if (eventWatcher && loop)
  {
    swEdgeWatcher *watcher = (swEdgeWatcher *)eventWatcher;
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

bool swEdgeEventSend(swEdgeEvent *eventWatcher)
{
  bool rtn = false;
  swEdgeWatcher *watcher = (swEdgeWatcher *)eventWatcher;
  if (eventWatcher && watcher->loop)
  {
    eventfd_t value = 1;
    if (eventfd_write(watcher->fd, value) == 0)
      rtn = true;
  }
  return rtn;
}

void swEdgeEventStop(swEdgeEvent *eventWatcher)
{
  swEdgeWatcher *watcher = (swEdgeWatcher *)eventWatcher;
  if (eventWatcher && watcher->loop)
  {
    swEdgeLoopWatcherRemove(watcher->loop, watcher);
    watcher->loop = NULL;
    close(watcher->fd);
    watcher->fd = -1;
  }
}

void swEdgeEventClose(swEdgeEvent *eventWatcher)
{
  swEdgeEventStop(eventWatcher);
}

