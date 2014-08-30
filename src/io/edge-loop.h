#ifndef SW_IO_EDGELOOP_H
#define SW_IO_EDGELOOP_H

#include <stdlib.h>
#include <stdbool.h>
#include <sys/epoll.h>

#include "collections/fast-array.h"

typedef enum swWatcherType
{
  swWatcherTypeNone = 0,
  swWatcherTypeTimer,           // timerfd monotonic
  swWatcherTypePeriodicTimer,   // timerfd real time
  swWatcherTypeSignal,          // signalfd
  // swWatcherTypeFile,            // inotify
  swWatcherTypeAsync,           // eventfd
  swWatcherTypeIO,              // socket
  swWatcherTypeMax
} swWatcherType;

const char const *swWatcherTypeTextGet(swWatcherType watcherType);

typedef enum swEdgeEvents
{
  swEdgeEventRead = EPOLLIN,
  swEdgeEventWrite = EPOLLOUT,
  swEdgeEventError = EPOLLERR,
  swEdgeEventHungUp = EPOLLHUP
} swEdgeEvents;


typedef struct swEdgeLoop
{
  swFastArray epollEvents;
  swFastArray pendingEvents[2];
  int fd;
  unsigned int currentPending : 1;
  unsigned int shutdown : 1;
} swEdgeLoop;

#define SWEDGELOOP_PENDINGBITS 31
#define SWEDGELOOP_PENDINGMAX  0x7FFFFFF

typedef struct swEdgeWatcher
{
  struct epoll_event event;
  swEdgeLoop *loop;
  void *data;

  unsigned pendingArray: 1;
  unsigned pendingPosition: SWEDGELOOP_PENDINGBITS;
  uint32_t pendingEvents;
  swWatcherType type;
  int fd;
} swEdgeWatcher;

swEdgeLoop *swEdgeLoopNew();
void swEdgeLoopDelete(swEdgeLoop *loop);
void swEdgeLoopRun(swEdgeLoop *loop, bool once);
void swEdgeLoopBreak(swEdgeLoop *loop);
bool swEdgeLoopWatcherAdd(swEdgeLoop *loop, swEdgeWatcher *watcher);
bool swEdgeLoopWatcherRemove(swEdgeLoop *loop, swEdgeWatcher *watcher);
bool swEdgeLoopWatcherModify(swEdgeLoop *loop, swEdgeWatcher *watcher);
bool swEdgeLoopPendingSet(swEdgeLoop *loop, swEdgeWatcher *watcher, uint32_t events);

static inline void *swEdgeWatcherDataGet(swEdgeWatcher *watcher)
{
  if (watcher)
    return watcher->data;
  return NULL;
}

static inline void swEdgeWatcherDataSet(swEdgeWatcher *watcher, void *data)
{
  if (watcher)
    watcher->data = data;
}

static inline swEdgeLoop *swEdgeWatcherLoopGet(swEdgeWatcher *watcher)
{
  if (watcher)
    return watcher->loop;
  return NULL;
}

bool swEdgeWatcherPendingSet(swEdgeWatcher *watcher, uint32_t events);

#define swEdgeWatcherDataGet(t)     swEdgeWatcherDataGet((swEdgeWatcher *)(t))
#define swEdgeWatcherDataSet(t, d)  swEdgeWatcherDataSet((swEdgeWatcher *)(t), (void *)(d))
#define swEdgeWatcherLoopGet(t)     swEdgeWatcherLoopGet((swEdgeWatcher *)(t))

// void swEdgeLoopPendingPrint(swEdgeLoop *loop);

#endif // SW_IO_EDGELOOP_H