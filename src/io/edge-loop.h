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

  /*
  char events[MAX_EVENTS];
  uint8_t currentEvent;
  */
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

/*
#define swEdgeWatcherDataGet(w)     ((w)? ((swEdgeWatcher *)(w))->data : NULL)
#define swEdgeWatcherDataSet(w, d)  do { if ((w) != NULL) ((swEdgeWatcher *)(w))->data = (d); } while(0)
#define swEdgeWatcherLoopGet(w)     ((w)? ((swEdgeWatcher *)(w))->loop : NULL)
*/

#endif // SW_IO_EDGELOOP_H