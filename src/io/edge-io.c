#include "edge-io.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

bool swEdgeIOInit(swEdgeIO *ioWatcher, swEdgeIOCallback cb)
{
  bool rtn = false;
  if (ioWatcher && cb)
  {
    memset(ioWatcher, 0, sizeof(swEdgeIO));
    swEdgeWatcher *watcher = (swEdgeWatcher *)ioWatcher;
    // watcher->event.events = (EPOLLIN | EPOLLRDHUP | EPOLLET);
    watcher->event.data.ptr = ioWatcher;
    watcher->type = swWatcherTypeIO;
        ioWatcher->ioCB = cb;
    rtn = true;
  }
  return rtn;
}

bool swEdgeIOStart(swEdgeIO *ioWatcher, swEdgeLoop *loop, int fd, uint32_t events)
{
  bool rtn = false;
  if (ioWatcher && loop && (fd >= 0) && (events & (swEdgeEventRead | swEdgeEventWrite)))
  {
    swEdgeWatcher *watcher = (swEdgeWatcher *)ioWatcher;
    watcher->fd = fd;
    watcher->event.events = EPOLLET;
    if (events & swEdgeEventRead)
      watcher->event.events |= (EPOLLIN | EPOLLRDHUP);
    if (events & swEdgeEventWrite)
      watcher->event.events |= EPOLLOUT;
    if (swEdgeLoopWatcherAdd(loop, watcher))
    {
      watcher->loop = loop;
      rtn = true;
    }
  }
  return rtn;
}

bool swEdgeIOEventSet(swEdgeIO *ioWatcher, uint32_t events)
{
  bool rtn = false;
  swEdgeWatcher *watcher = (swEdgeWatcher *)ioWatcher;
  if (watcher && (watcher->loop) && (events & (swEdgeEventRead | swEdgeEventWrite)))
  {
    uint32_t oldEvents = watcher->event.events;
    watcher->event.events |= EPOLLET;
    if (events & swEdgeEventRead)
      watcher->event.events |= (EPOLLIN | EPOLLRDHUP);
    if (events & swEdgeEventWrite)
      watcher->event.events |= EPOLLOUT;
    rtn = swEdgeLoopWatcherModify(watcher->loop, watcher);
    if (!rtn)
      watcher->event.events = oldEvents;
  }
  return rtn;
}

bool swEdgeIOEventUnSet(swEdgeIO *ioWatcher, uint32_t events)
{
  bool rtn = false;
  swEdgeWatcher *watcher = (swEdgeWatcher *)ioWatcher;
  if (watcher && (watcher->loop) && ((events & swEdgeEventRead) || (events & swEdgeEventWrite)))
  {
    uint32_t oldEvents = watcher->event.events;
    watcher->event.events |= EPOLLET;
    if (events & swEdgeEventRead)
      watcher->event.events &= ~(EPOLLIN | EPOLLRDHUP);
    if (events & swEdgeEventWrite)
      watcher->event.events &= ~EPOLLOUT;
    rtn = swEdgeLoopWatcherModify(watcher->loop, watcher);
    if (!rtn)
      watcher->event.events = oldEvents;
  }
  return rtn;
}

bool swEdgeIOPendingSet(swEdgeIO *ioWatcher, uint32_t events)
{
  bool rtn = false;
  swEdgeWatcher *watcher = (swEdgeWatcher *)ioWatcher;
  if (ioWatcher)
  {
    if (ioWatcher->pendingEvents)
    {
      ioWatcher->pendingEvents |= events;
      rtn = true;
    }
    else
      rtn = swEdgeLoopPendingAdd(swEdgeWatcherLoopGet(ioWatcher), watcher, events);
  }
  return rtn;
}

void swEdgeIOStop(swEdgeIO *ioWatcher)
{
  swEdgeWatcher *watcher = (swEdgeWatcher *)ioWatcher;
  if (ioWatcher && watcher->loop)
  {
    if (ioWatcher->pendingEvents)
    {
      swEdgeLoopPendingRemove(watcher->loop, watcher);
      ioWatcher->pendingEvents = 0;
    }
    swEdgeLoopWatcherRemove(watcher->loop, watcher);
    watcher->loop = NULL;
    watcher->fd = -1;
  }
}

void swEdgeIOClose(swEdgeIO *ioWatcher)
{
  swEdgeIOStop(ioWatcher);
}
