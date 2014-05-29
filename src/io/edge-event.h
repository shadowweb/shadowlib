#ifndef SW_IO_EDGEEVENT_H
#define SW_IO_EDGEEVENT_H

#include "edge-loop.h"
#include <sys/eventfd.h>

struct swEdgeEvent;

typedef void (*swEdgeEventCallback)(struct swEdgeEvent *eventWatcher, eventfd_t eventCount);

typedef struct swEdgeEvent
{
  swEdgeWatcher watcher;

  swEdgeEventCallback eventCB;
} swEdgeEvent;

bool swEdgeEventInit(swEdgeEvent *eventWatcher, swEdgeEventCallback cb);
bool swEdgeEventStart(swEdgeEvent *eventWatcher, swEdgeLoop *loop);
bool swEdgeEventProcess(swEdgeEvent *eventWatcher, uint32_t events);
bool swEdgeEventSend(swEdgeEvent *eventWatcher);
void swEdgeEventStop(swEdgeEvent *eventWatcher);
void swEdgeEventClose(swEdgeEvent *eventWatcher);

#endif // SW_IO_EDGEEVENT_H
