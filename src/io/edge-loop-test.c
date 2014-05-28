#include "edge-loop.h"
#include "edge-timer.h"
#include "edge-signal.h"

#include "unittest/unittest.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

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
  uint64_t *totalExpired = swEdgeWatcherDataGet(timer);
  if (totalExpired)
  {
    *totalExpired += expiredCount;
    swTestLogLine("Timer expired %llu time(s), total %llu times ...\n", expiredCount, *totalExpired);
    if (*totalExpired > 5)
      swEdgeLoopBreak(swEdgeWatcherLoopGet(timer));
  }
  else
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(swEdgeWatcherLoopGet(timer));
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
      swEdgeWatcherDataSet(&timer, &totalExpired);
      swEdgeLoopRun(loop);
      rtn = true;
      swEdgeTimerStop(&timer);
    }
    swEdgeTimerClose(&timer);
  }
  return rtn;
}

static uint64_t sigIntReceived = 0;
static uint64_t sigHupReceived = 0;
static uint64_t sigQuitReceived = 0;

void signalCallback(swEdgeSignal *signalWatcher, struct signalfd_siginfo *signalReceived)
{
  if (signalWatcher && signalReceived &&
      (signalReceived->ssi_signo == SIGINT || signalReceived->ssi_signo == SIGHUP || signalReceived->ssi_signo == SIGQUIT) &&
      signalReceived->ssi_pid == (uint32_t)getpid())
  {
    if (signalReceived->ssi_signo == SIGINT)
    {
      sigIntReceived++;
      swTestLogLine("Signal SIGINT received ...\n");
    }
    if (signalReceived->ssi_signo == SIGQUIT)
    {
      sigQuitReceived++;
      swTestLogLine("Signal SIGQUIT received ...\n");
    }
    if (signalReceived->ssi_signo == SIGHUP)
    {
      sigHupReceived++;
      swTestLogLine("Signal SIGHUP received ...\n");
    }
    if (sigIntReceived > 5 && sigQuitReceived > 5 && sigHupReceived > 5)
      swEdgeLoopBreak(swEdgeWatcherLoopGet(signalWatcher));
  }
  else
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(swEdgeWatcherLoopGet(signalWatcher));
  }
}

void sendSignalCallback(swEdgeTimer *timer, uint64_t expiredCount)
{
  bool success = false;
  if (timer)
  {
    pid_t processId = getpid();
    if (kill(processId, SIGINT) == 0 && kill(processId, SIGHUP) == 0 && kill(processId, SIGQUIT) == 0)
    {
      success = true;
      swTestLogLine("Signal SIGINT, SIGHUP, SIGQUIT sent...\n");
    }
  }
  if (!success)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(swEdgeWatcherLoopGet(timer));
  }
}

bool sendSignalTimerStart(swEdgeTimer *timer, swEdgeLoop *loop)
{
  bool rtn = false;
  if (timer && loop && swEdgeTimerInit(timer, sendSignalCallback))
  {
    if (swEdgeTimerStart(timer, loop, 200, 200, false))
      rtn = true;
    else
      swEdgeTimerClose(timer);
  }
  return rtn;
}

void sendSignalTimerStop(swEdgeTimer *timer)
{
  if (timer)
    swEdgeTimerClose(timer);
}

swTestDeclare(EdgeSignalTest, NULL, NULL, swTestRun)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(loop);
  bool rtn = false;
  swEdgeSignal signalWatcher = {.signalCB = NULL};
  if (swEdgeSignalInit(&signalWatcher, signalCallback))
  {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGQUIT);
    if (swEdgeSignalStart(&signalWatcher, loop, mask))
    {
      swEdgeTimer sendSignalTimer = {.timerCB = NULL};
      if (sendSignalTimerStart(&sendSignalTimer, loop))
      {
        swTestLogLine("Starting event loop ...\n");
        swEdgeLoopRun(loop);
        swTestLogLine("Stopping event loop ...\n");
        rtn = true;
        sendSignalTimerStop(&sendSignalTimer);
      }
      swEdgeSignalStop(&signalWatcher);
    }
    swEdgeSignalClose(&signalWatcher);
  }
  return rtn;
}

swTestSuiteStructDeclare(EdgeEventLoopTest, edgeLoopSetup, edgeLoopTeardown, swTestRun,
                         &EdgeTimerTest, &EdgeSignalTest);
