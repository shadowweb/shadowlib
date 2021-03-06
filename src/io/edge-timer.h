#ifndef SW_IO_EDGETIMER_H
#define SW_IO_EDGETIMER_H

#include "edge-loop.h"
#include <sys/timerfd.h>

struct swEdgeTimer;

typedef void (*swEdgeTimerCallback)(struct swEdgeTimer *timer, uint64_t expiredCount, uint32_t events);

typedef struct swEdgeTimer
{
  swEdgeWatcher watcher;

  swEdgeTimerCallback timerCB;
  struct itimerspec timerSpec;
} swEdgeTimer;

bool swEdgeTimerInit(swEdgeTimer *timer, swEdgeTimerCallback cb, bool realTime);
// offset and interval is expressed in msec
bool swEdgeTimerStart(swEdgeTimer *timer, swEdgeLoop *loop, uint64_t offset, uint64_t interval, bool absolute);
void swEdgeTimerStop(swEdgeTimer *timer);
void swEdgeTimerClose(swEdgeTimer *timer);

#endif // SW_IO_EDGETIMER_H
