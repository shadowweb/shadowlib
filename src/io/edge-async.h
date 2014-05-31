#ifndef SW_IO_EDGEASYNC_H
#define SW_IO_EDGEASYNC_H

#include "edge-loop.h"
#include <sys/eventfd.h>

struct swEdgeAsync;

typedef void (*swEdgeAsyncCallback)(struct swEdgeAsync *asyncWatcher, eventfd_t eventCount, uint32_t events);

typedef struct swEdgeAsync
{
  swEdgeWatcher watcher;

  swEdgeAsyncCallback eventCB;
} swEdgeAsync;

bool swEdgeAsyncInit(swEdgeAsync *asyncWatcher, swEdgeAsyncCallback cb);
bool swEdgeAsyncStart(swEdgeAsync *asyncWatcher, swEdgeLoop *loop);
bool swEdgeAsyncSend(swEdgeAsync *asyncWatcher);
void swEdgeAsyncStop(swEdgeAsync *asyncWatcher);
void swEdgeAsyncClose(swEdgeAsync *asyncWatcher);

#endif // SW_IO_EDGEASYNC_H
