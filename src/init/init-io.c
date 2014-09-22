#include "init/init-io.h"
#include "io/edge-signal.h"

#include <signal.h>
#include <stdarg.h>
#include <string.h>

static void *edgeLoopArrayData[1] = {NULL};

static bool swInitIOEdgeLoopStart()
{
  bool rtn = false;
  swEdgeLoop **loopPtr = edgeLoopArrayData[0];
  if (loopPtr)
  {
    if ((*loopPtr = swEdgeLoopNew()))
      rtn = true;
  }
  return rtn;
}

static void swInitIOEdgeLoopStop()
{
  swEdgeLoop **loopPtr = edgeLoopArrayData[0];
  if (loopPtr && *loopPtr)
  {
    swEdgeLoopDelete(*loopPtr);
    edgeLoopArrayData[0] = NULL;
  }
}

static swInitData edgeLoopData = {.startFunc = swInitIOEdgeLoopStart, .stopFunc = swInitIOEdgeLoopStop, .name = "Edge Loop"};

swInitData *swInitIOEdgeLoopDataGet(swEdgeLoop **loopPtr)
{
  edgeLoopArrayData[0] = loopPtr;
  return &edgeLoopData;
}

static sigset_t mask = {{ 0 }};
static swEdgeSignal signalWatcher = {.signalCB = NULL};
static void *edgeSignalsArrayData[1] = {NULL};

static void signalCallback(swEdgeSignal *signalWatcher, struct signalfd_siginfo *signalReceived, uint32_t events)
{
  swEdgeLoop *loop = edgeSignalsArrayData[0];
  if (loop)
    swEdgeLoopBreak(loop);
}

static bool swInitIOEdgeSignalsStart()
{
  bool rtn = false;
  swEdgeLoop *loop = edgeLoopArrayData[0];
  if (loop)
  {
    if (swEdgeSignalInit(&signalWatcher, signalCallback))
    {
      if (swEdgeSignalStart(&signalWatcher, loop, mask))
      {
        signal(SIGPIPE, SIG_IGN);
        rtn = true;
      }
      else
        swEdgeSignalClose(&signalWatcher);
    }
  }
  return rtn;
}

static void swInitIOEdgeSignalsStop()
{
  swEdgeSignalStop(&signalWatcher);
  swEdgeSignalClose(&signalWatcher);
  memset(&signalWatcher, 0, sizeof(swEdgeSignal));
  edgeLoopArrayData[0] = NULL;
}

static swInitData edgeSignalsData = {.startFunc = swInitIOEdgeSignalsStart, .stopFunc = swInitIOEdgeSignalsStop, .name = "Signals"};

swInitData *swInitIOEdgeSignalsDataGet(swEdgeLoop *loop, ...)
{
  edgeSignalsArrayData[0] = loop;

  sigemptyset(&mask);
  va_list argPtr;
  va_start(argPtr, loop);
  while (1)
  {
    int signal = va_arg(argPtr, int);
    if (signal)
      sigaddset(&mask, signal);
    else
      break;
  }
  va_end(argPtr);

  return &edgeSignalsData;
}
