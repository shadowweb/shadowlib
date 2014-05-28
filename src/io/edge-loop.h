#ifndef SW_IO_EDGELOOP_H
#define SW_IO_EDGELOOP_H

#include <stdlib.h>
#include <stdbool.h>
#include <sys/epoll.h>

#include <collections/array.h>

// TODO: continue with the edge loop

#define MAX_EVENTS 8

typedef enum swWatcherType
{
  swWatcherTypeTimer,           // timerfd monotonic
  swWatcherTypePeriodicTimer,   // timerfd real time
  swWatcherTypeSignal,          // signalfd
  swWatcherTypeFile,            // inotify
  swWatcherTypeEvent,           // eventfd
  swWatcherTypeIO,              // socket
  swWatcherTypeMax
} swWatcherType;

typedef struct swEdgeLoop
{
  // swStaticArray timers;
  swStaticArray epollEvents;
  int fd;
  unsigned int shutdown : 1;
} swEdgeLoop;

// typedef void (*swEdgeWatcherCallback)(swEdgeWatcher *watcher, uint32_t events);

typedef struct swEdgeWatcher
{
  struct epoll_event event;
  // swEdgeWatcherCallback *callback;
  swEdgeLoop *loop;
  void *data;
  swWatcherType type;
  int fd;
} swEdgeWatcher;

swEdgeLoop *swEdgeLoopNew();
void swEdgeLoopDelete(swEdgeLoop *loop);
void swEdgeLoopRun(swEdgeLoop *loop);
void swEdgeLoopRunOnce(swEdgeLoop *loop);
void swEdgeLoopBreak(swEdgeLoop *loop);
bool swEdgeLoopWatcherAdd(swEdgeLoop *loop, swEdgeWatcher *watcher);
bool swEdgeLoopWatcherRemove(swEdgeLoop *loop, swEdgeWatcher *watcher);

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