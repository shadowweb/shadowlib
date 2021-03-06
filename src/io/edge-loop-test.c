#include "io/edge-loop.h"
#include "io/edge-timer.h"
#include "io/edge-signal.h"
#include "io/edge-async.h"
#include "io/edge-io.h"

#include "unittest/unittest.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

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

void timerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  uint64_t *totalExpired = swEdgeWatcherDataGet(timer);
  if (totalExpired && expiredCount && (events & swEdgeEventRead))
  {
    *totalExpired += expiredCount;
    swTestLogLine("Timer expired %llu time(s), total %llu times, events 0x%08x ...\n", expiredCount, *totalExpired, events);
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
  if (swEdgeTimerInit(&timer, timerCallback, false))
  {
    if (swEdgeTimerStart(&timer, loop, 200, 200, false))
    {
      swEdgeWatcherDataSet(&timer, &totalExpired);
      swEdgeLoopRun(loop, false);
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

void signalCallback(swEdgeSignal *signalWatcher, struct signalfd_siginfo *signalReceived, uint32_t events)
{
  if (signalWatcher && signalReceived && (events & swEdgeEventRead) &&
      (signalReceived->ssi_signo == SIGINT || signalReceived->ssi_signo == SIGHUP || signalReceived->ssi_signo == SIGQUIT) &&
      signalReceived->ssi_pid == (uint32_t)getpid())
  {
    if (signalReceived->ssi_signo == SIGINT)
    {
      sigIntReceived++;
      swTestLogLine("Signal SIGINT received, events 0x%08x ...\n", events);
    }
    if (signalReceived->ssi_signo == SIGQUIT)
    {
      sigQuitReceived++;
      swTestLogLine("Signal SIGQUIT received, events 0x%08x ...\n", events);
    }
    if (signalReceived->ssi_signo == SIGHUP)
    {
      sigHupReceived++;
      swTestLogLine("Signal SIGHUP received, events 0x%08x ...\n", events);
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

void sendSignalCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  bool success = false;
  if (timer && expiredCount && (events & swEdgeEventRead))
  {
    pid_t processId = getpid();
    if (kill(processId, SIGINT) == 0 && kill(processId, SIGHUP) == 0 && kill(processId, SIGQUIT) == 0)
    {
      success = true;
      swTestLogLine("Signal SIGINT, SIGHUP, SIGQUIT sent, events 0x%08x...\n", events);
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
  if (timer && loop && swEdgeTimerInit(timer, sendSignalCallback, true))
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
        swEdgeLoopRun(loop, false);
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

static uint64_t eventsReceived = 0;

void eventCallback(swEdgeAsync *asyncWatcher, eventfd_t eventCount, uint32_t events)
{
  if (asyncWatcher && eventCount && (events & swEdgeEventRead))
  {
    swTestLogLine("Event occured %u time(s), events 0x%08x ...\n", eventCount, events);
    eventsReceived += eventCount;
    if (eventsReceived > 5)
      swEdgeLoopBreak(swEdgeWatcherLoopGet(asyncWatcher));
  }
  else
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(swEdgeWatcherLoopGet(asyncWatcher));
  }
}

void sendEventCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  bool success = false;
  if (timer && expiredCount && (events & swEdgeEventRead))
  {
    swEdgeAsync *asyncWatcher = swEdgeWatcherDataGet(timer);
    if (asyncWatcher)
      success = swEdgeAsyncSend(asyncWatcher) && swEdgeAsyncSend(asyncWatcher) && swEdgeAsyncSend(asyncWatcher);
  }
  if (!success)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(swEdgeWatcherLoopGet(timer));
  }
}

bool sendEventTimerStart(swEdgeTimer *timer, swEdgeLoop *loop, swEdgeAsync *async)
{
  bool rtn = false;
  if (timer && loop && swEdgeTimerInit(timer, sendEventCallback, true))
  {
    swEdgeWatcherDataSet(timer, async);
    if (swEdgeTimerStart(timer, loop, 200, 200, false))
      rtn = true;
    else
      swEdgeTimerClose(timer);
  }
  return rtn;
}

void sendEventTimerStop(swEdgeTimer *timer)
{
  if (timer)
    swEdgeTimerClose(timer);
}

swTestDeclare(EdgeAsyncTest, NULL, NULL, swTestRun)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(loop);
  bool rtn = false;
  swEdgeAsync asyncWatcher = {.eventCB = NULL};
  if (swEdgeAsyncInit(&asyncWatcher, eventCallback))
  {
    if (swEdgeAsyncStart(&asyncWatcher, loop))
    {
      swEdgeTimer sendEventTimer = {.timerCB = NULL};
      if (sendEventTimerStart(&sendEventTimer, loop, &asyncWatcher))
      {
        swTestLogLine("Starting event loop ...\n");
        swEdgeLoopRun(loop, false);
        swTestLogLine("Stopping event loop ...\n");
        rtn = true;
        sendEventTimerStop(&sendEventTimer);
      }
      swEdgeAsyncStop(&asyncWatcher);
    }
    swEdgeAsyncClose(&asyncWatcher);
  }
  return rtn;
}

typedef struct swIOTestData
{
  uint32_t dataSent;
  uint32_t dataReceived;
  unsigned int writeReady : 1;
} swIOTestData;

static swIOTestData ioTestData[2] = { { 0 } };

#define MAX_DATA    1048576

void ioCallback(swEdgeIO *ioWatcher, uint32_t events)
{
  swIOTestData *testData = swEdgeWatcherDataGet(ioWatcher);
  if (testData)
  {
    char buff[1024] = {0};
    int fd = swEdgeIOFDGet(ioWatcher);
    // printf ("fd %d received events 0x%X\n", fd, events);
    if (events & swEdgeEventRead)
    {
      int bytesRead = 0;
      uint64_t i = 0;
      for (; i < 10; i++)
      {
        if ((bytesRead = read(fd, buff, 1024)) > 0)
          testData->dataReceived += bytesRead;
        else
          break;
      }
      // printf ("fd %d read %d bytes\n", fd, testData->dataReceived);
      if (i == 10)
        ASSERT_TRUE(swEdgeWatcherPendingSet((swEdgeWatcher *)ioWatcher, events));
      else
      {
        if (!(bytesRead < 0 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))))
        {
          // done
          ASSERT_FAIL();
          shutdown(fd, SHUT_RD);
          swEdgeIOEventUnSet(ioWatcher, swEdgeEventRead);
        }
      }
    }
    if (events & swEdgeEventWrite)
      testData->writeReady = 1;
    if (testData->writeReady)
    {
      int bytesWritten = 0;
      uint64_t i = 0;
      for (; i < 10; i++)
      {
        if ((bytesWritten = write(fd, buff, 1024)) > 0)
          testData->dataSent += bytesWritten;
        else
          break;
      }
      // printf ("fd %d wrote %d bytes\n", fd, testData->dataSent);
      if (i == 10)
        ASSERT_TRUE(swEdgeWatcherPendingSet((swEdgeWatcher *)ioWatcher, events));
      else
      {
        if (bytesWritten < 0 && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
          testData->writeReady = 0;
        else
        {
          // done
          ASSERT_FAIL();
          shutdown(fd, SHUT_WR);
          swEdgeIOEventUnSet(ioWatcher, swEdgeEventWrite);
        }
      }
    }

    if (events & ~(swEdgeEventRead | swEdgeEventWrite))
      ASSERT_FAIL();
    if (ioTestData[0].dataSent > MAX_DATA && ioTestData[0].dataReceived > MAX_DATA &&
        ioTestData[1].dataSent > MAX_DATA && ioTestData[1].dataReceived > MAX_DATA )
      swEdgeLoopBreak(swEdgeWatcherLoopGet(ioWatcher));
  }
}

bool runIOTest(swTestSuite *suite, swTest *test, int type)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(loop);
  bool rtn = false;
  int fd[2];
  if (socketpair(AF_UNIX, type, 0, fd) == 0)
  {
    memset(ioTestData, 0, sizeof(ioTestData));
    swTestLogLine("created soceket pair %d and %d\n", fd[0], fd[1]);
    swEdgeIO ioEvents[2] = {{{ .loop = NULL }, .ioCB = NULL}};
    if (swEdgeIOInit(&ioEvents[0], ioCallback) && swEdgeIOInit(&ioEvents[1], ioCallback))
    {
      signal(SIGPIPE, SIG_IGN);
      swEdgeWatcherDataSet(&ioEvents[0], &ioTestData[0]);
      swEdgeWatcherDataSet(&ioEvents[1], &ioTestData[1]);
      if (swEdgeIOStart(&ioEvents[0], loop, fd[0], (swEdgeEventRead | swEdgeEventWrite)) &&
          swEdgeIOStart(&ioEvents[1], loop, fd[1], (swEdgeEventRead | swEdgeEventWrite)) )
      {
        swEdgeLoopRun(loop, false);
        swTestLogLine("fd[0] = %d (r: %u, w: %u); fd[1] = %d  (r: %u, w: %u)\n",
          fd[0], ioTestData[0].dataSent, ioTestData[0].dataReceived,
          fd[1], ioTestData[1].dataSent, ioTestData[1].dataReceived);
        rtn = true;
      }
      swEdgeIOClose(&ioEvents[0]);
      swEdgeIOClose(&ioEvents[1]);
    }
    close (fd[0]);
    close (fd[1]);
  }
  return rtn;
}

swTestDeclare(EdgeIOTCPTest, NULL, NULL, swTestRun)
{
  return runIOTest(suite, test, SOCK_STREAM);
}

swTestDeclare(EdgeIOUDPTest, NULL, NULL, swTestRun)
{
  return runIOTest(suite, test, SOCK_DGRAM);
}

swTestSuiteStructDeclare(EdgeEventLoopTest, edgeLoopSetup, edgeLoopTeardown, swTestRun,
                         &EdgeTimerTest, &EdgeSignalTest, &EdgeAsyncTest, &EdgeIOTCPTest, &EdgeIOUDPTest);
