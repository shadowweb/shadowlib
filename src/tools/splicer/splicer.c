#include "tools/splicer/splicer-state.h"

#include "command-line/option-category.h"
#include "command-line/command-line.h"
#include "init/init.h"
#include "init/init-command-line.h"
#include "init/init-cpu-timer.h"
#include "init/init-io.h"
#include "init/init-log-manager.h"
#include "init/init-thread-manager.h"
#include "log/log-manager.h"
#include "storage/dynamic-buffer.h"

#include <limits.h>
#include <signal.h>
#include <time.h>

swLoggerDeclareWithLevel(splicerLogger, "Splicer", swLogLevelInfo);

static swStaticString ipAddress  = swStaticStringDefineEmpty;

static int64_t port   = 0;
static int64_t bufferSize   = 0;

static swStaticArray defaultIPAddress = swStaticArrayDefine((swStaticString[]){swStaticStringDefine("127.0.0.1")}, swStaticString);

swStaticArray defaultPort       = swStaticArrayDefine(((int64_t[]){6666}),  int64_t);
swStaticArray defaultBufferSize = swStaticArrayDefine(((int64_t[]){256}),   int64_t);

swOptionCategoryMainDeclare(swSplicerMainOptions, "Splicer Main Options",
  swOptionDeclareScalarWithDefault("ip-address|ip", "IP address for listening/connecting",  "IP",   &defaultIPAddress,
    &ipAddress,   swOptionValueTypeString,  false),
  swOptionDeclareScalarWithDefault("port|p",        "Port for listening/connecting",        "PORT", &defaultPort,
    &port,        swOptionValueTypeInt,     false),
  swOptionDeclareScalarWithDefault("buffer-size|b", "Initial buffer size",                  NULL,   &defaultBufferSize,
    &bufferSize,  swOptionValueTypeInt,     false)
);

static void *splicerArrayData[1] = {NULL};

static swSplicerState *splicerState = NULL;

static void swSplicerStop()
{
  if (splicerState)
  {
    swSplicerStateDelete(splicerState);
    splicerState = NULL;
  }
}

static bool swSplicerStart()
{
  bool rtn = false;
  swEdgeLoop **loopPtr = (swEdgeLoop **)splicerArrayData[0];
  if (loopPtr && *loopPtr)
  {
    if (bufferSize > 0 && port > 0 && port <= USHRT_MAX && ipAddress.len)
    {
      swSocketAddress address = { 0 };
      if (swSocketAddressInitInet(&address, ipAddress.data, port))
      {
        if ((splicerState = swSplicerStateNew(*loopPtr, &address, (size_t)bufferSize)))
          rtn = true;
      }
    }
  }
  return rtn;
}

static swInitData splicerData = {.startFunc = swSplicerStart, .stopFunc = swSplicerStop, .name = "Splicer"};

swInitData *swSpliceerDataGet(swEdgeLoop **loopPtr)
{
  splicerArrayData[0] = loopPtr;
  return &splicerData;
}

int main (int argc, char *argv[])
{
  int rtn = EXIT_FAILURE;
  swEdgeLoop *loop = NULL;
  swThreadManager *threadManager = NULL;
  srand(time(NULL));
  uint64_t cpuTimerInterval = 1000;
  swInitData *initData[] =
  {
    swInitCommandLineDataGet(&argc, argv, "Splice Tester", NULL),
    swInitIOEdgeLoopDataGet(&loop),
    swInitIOEdgeSignalsDataGet(&loop, SIGINT, SIGHUP, SIGQUIT, SIGTERM, SIGUSR1, SIGUSR2, 0),
    swInitThreadManagerDataGet(&loop, &threadManager),
    swInitLogManagerDataGet(&threadManager),
    swInitCPUTimerGet(&loop, &cpuTimerInterval),
    swSpliceerDataGet(&loop),
    NULL
  };

  if (swInitStart (initData))
  {
    // printf ("IP address = %.*s, Port = %ld, Buffer Size = %ld\n", (int)(ipAddress.len), ipAddress.data, port, bufferSize);
    swEdgeLoopRun(loop, false);
    rtn = EXIT_SUCCESS;
    swInitStop(initData);
  }

  return rtn;
}
