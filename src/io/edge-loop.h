#ifndef SW_IO_EDGELOOP_H
#define SW_IO_EDGELOOP_H

#include <stdlib.h>
#include <stdbool.h>
#include <sys/epoll.h>

#include <collections/array.h>

typedef enum swWatcherType
{
  swWatcherTypeTimer,           // timerfd monotonic
  swWatcherTypePeriodicTimer,   // timerfd real time
  swWatcherTypeSignal,          // signalfd
  // swWatcherTypeFile,            // inotify
  swWatcherTypeEvent,           // eventfd
  swWatcherTypeIO,              // socket
  swWatcherTypeMax
} swWatcherType;

typedef struct swEdgeLoop
{
  swStaticArray epollEvents;
  swStaticArray pendingEvents[2];
  int fd;
  unsigned int currentPending : 1;
  unsigned int shutdown : 1;
} swEdgeLoop;

typedef struct swEdgeWatcher
{
  struct epoll_event event;
  swEdgeLoop *loop;
  void *data;
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
bool swEdgeLoopPendingAdd(swEdgeLoop *loop, swEdgeWatcher *watcher, uint32_t events);
bool swEdgeLoopPendingRemove(swEdgeLoop *loop, swEdgeWatcher *watcher);

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

#define swEdgeWatcherDataGet(t)     swEdgeWatcherDataGet((swEdgeWatcher *)(t))
#define swEdgeWatcherDataSet(t, d)  swEdgeWatcherDataSet((swEdgeWatcher *)(t), (void *)(d))
#define swEdgeWatcherLoopGet(t)     swEdgeWatcherLoopGet((swEdgeWatcher *)(t))

#endif // SW_IO_EDGELOOP_H