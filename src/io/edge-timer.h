#ifndef SW_IO_EDGETIMER_H
#define SW_IO_EDGETIMER_H

#include "edge-loop.h"
#include <sys/timerfd.h>

struct swEdgeTimer;

typedef void (*swEdgeTimerCallback)(struct swEdgeTimer *timer, uint64_t expiredCount);

typedef struct swEdgeTimer
{
  swEdgeWatcher watcher;

  swEdgeTimerCallback timerCB;
  struct itimerspec timerSpec;
  unsigned int active : 1;
} swEdgeTimer;

bool swEdgeTimerInit(swEdgeTimer *timer, swEdgeTimerCallback cb);
// offset and interval is expressed in msec
bool swEdgeTimerStart(swEdgeTimer *timer, swEdgeLoop *loop, uint64_t offset, uint64_t interval, bool absolute);
bool swEdgeTimerProcess(swEdgeTimer *timer, uint32_t events);
void swEdgeTimerStop(swEdgeTimer *timer);
void swEdgeTimerClose(swEdgeTimer *timer);

#define swEdgeTimerDataGet(t)     swEdgeWatcherDataGet((swEdgeWatcher *)(t))
#define swEdgeTimerDataSet(t, d)  swEdgeWatcherDataSet((swEdgeWatcher *)(t), (void *)(d))
#define swEdgeTimerLoopGet(t)     swEdgeWatcherLoopGet((swEdgeWatcher *)(t))

#endif // SW_IO_EDGETIMER_H
