#include "edge-loop.h"
#include "edge-timer.h"

#include "unittest/unittest.h"

void edgeLoopSetup(swTestSuite *suite)
{
  swTestLogLine("Creating loop ...\n");
  swEdgeLoop *loop = swEdgeLoopNew();
  ASSERT_NOT_NULL(loop);
  swTestSuiteDataSet(suite, loop);
}

void edgeLoopTeardown(swTestSuite *suite)
{
  swTestLogLine("Deleting loop ...\n");
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(loop);
  swEdgeLoopDelete(loop);
}

void timerCallback(swEdgeTimer *timer, uint64_t expiredCount)
{
  uint64_t *totalExpired = swEdgeTimerDataGet(timer);
  if (totalExpired)
  {
    *totalExpired += expiredCount;
    swTestLogLine("Timer expired %llu time(s), total %llu times ...\n", expiredCount, *totalExpired);
    if (*totalExpired > 5)
      swEdgeLoopBreak(swEdgeTimerLoopGet(timer));
  }
}

static uint64_t totalExpired = 0;

swTestDeclare(EdgeTimerTest, NULL, NULL, swTestRun)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(loop);
  bool rtn = false;
  swEdgeTimer timer = {.timerCB = NULL};
  if (swEdgeTimerInit(&timer, timerCallback))
  {
    if (swEdgeTimerStart(&timer, loop, 200, 200, false))
    {
      swEdgeTimerDataSet(&timer, &totalExpired);
      swEdgeLoopRun(loop);
      rtn = true;
      swEdgeTimerStop(&timer);
    }
    swEdgeTimerClose(&timer);
  }
  return rtn;
}

swTestSuiteStructDeclare(HashSetLinearTest, edgeLoopSetup, edgeLoopTeardown, swTestRun, &EdgeTimerTest);
