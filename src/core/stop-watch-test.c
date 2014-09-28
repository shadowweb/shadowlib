#include "core/stop-watch.h"

#include "unittest/unittest.h"

static void testFunction(void)
{
  uint32_t j = 0;
  for (uint32_t i = 0; i < 100; i++)
    j++;
}

swTestDeclare(StopWatchBasicTest, NULL, NULL, swTestRun)
{
  swStopWatch watch = { 0 };
  uint32_t j = 0;
  swStopWatchStart(&watch);
  for (uint32_t i = 0; i < 100; i++)
    j++;
  swStopWatchStop(&watch);
  swTestLogLine("Stop watch ticks = %lu for %u iterations\n", watch.timeTicks, j);
  return true;
}

swTestDeclare(StopWatchMeasureTest, NULL, NULL, swTestRun)
{
  swStopWatch watch = { 0 };
  swStopWatchPrepare(&watch);
  swStopWatchMeasure(&watch, testFunction);
  swTestLogLine("Stop watch ticks = %lu\n", watch.timeTicks);
  return true;
}

swTestSuiteStructDeclare(StopWatchTest, NULL, NULL, swTestRun,
                         &StopWatchBasicTest, &StopWatchMeasureTest);

