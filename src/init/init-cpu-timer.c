#include "init/init-cpu-timer.h"

#include "core/time.h"
#include "io/edge-timer.h"
#include "log/log-manager.h"

#include <string.h>

swLoggerDeclareWithLevel(cpuLogger, "CPUUtilization", swLogLevelInfo);

typedef struct swCPUTimerData
{
  swEdgeTimer timer;
  uint64_t lastMonotonicTimeStamp;
  uint64_t lastCPUTimeStamp;
} swCPUTimerData;

static swCPUTimerData timerData = { .timer = {.watcher = { .fd = -1 } } };

static void swCPUTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  if (timer)
  {
    swCPUTimerData *timerData = swEdgeWatcherDataGet(timer);
    uint64_t lastMonotonicTimeStamp = timerData->lastMonotonicTimeStamp;
    uint64_t lastCPUTimeStamp = timerData->lastCPUTimeStamp;
    timerData->lastMonotonicTimeStamp = swTimeGet(CLOCK_MONOTONIC_RAW);
    timerData->lastCPUTimeStamp = swTimeGet(CLOCK_PROCESS_CPUTIME_ID);
    double cpuUtilization = ((double)(timerData->lastCPUTimeStamp - lastCPUTimeStamp)) * 100 / ((double)(timerData->lastMonotonicTimeStamp - lastMonotonicTimeStamp));
    SW_LOG_INFO(&cpuLogger, "CPU Utilization: %.2f%%", cpuUtilization);
  }
}

static void *cpuTimerArrayData[2] = { NULL };

static bool swInitCPUTimerStart()
{
  bool rtn = false;
  swEdgeLoop **loop = (swEdgeLoop **)cpuTimerArrayData[0];
  uint64_t *timerInterval = (uint64_t *)cpuTimerArrayData[1];
  if (loop && *loop && timerInterval)
  {
    if (swEdgeTimerInit(&(timerData.timer), swCPUTimerCallback, false))
    {
      timerData.lastMonotonicTimeStamp = swTimeGet(CLOCK_MONOTONIC_RAW);
      timerData.lastCPUTimeStamp = swTimeGet(CLOCK_PROCESS_CPUTIME_ID);
      if (swEdgeTimerStart(&(timerData.timer), *loop, *timerInterval, *timerInterval, false))
      {
        swEdgeWatcherDataSet(&(timerData.timer), &timerData);
        rtn = true;
      }
      else
        swEdgeTimerClose(&(timerData.timer));
    }
  }
  return rtn;
}

static void swInitCPUTimerStop()
{
  if (timerData.timer.watcher.loop)
    swEdgeTimerStop(&(timerData.timer));
  if (timerData.timer.watcher.fd >= 0)
    swEdgeTimerClose(&(timerData.timer));
  memset(cpuTimerArrayData, 0, sizeof(cpuTimerArrayData));
}

static swInitData cpuTimerInitData = {.startFunc = swInitCPUTimerStart, .stopFunc = swInitCPUTimerStop, .name = "CPU Timer"};

swInitData *swInitCPUTimerGet(swEdgeLoop **loop, uint64_t *timerInterval)
{
  cpuTimerArrayData[0] = loop;
  cpuTimerArrayData[1] = timerInterval;
  return &cpuTimerInitData;
}
