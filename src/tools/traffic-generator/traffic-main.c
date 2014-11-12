#include "tools/traffic-generator/traffic-client.h"
#include "tools/traffic-generator/traffic-server.h"
#include "tools/traffic-generator/traffic-connection.h"

#include "command-line/option-category.h"
#include "command-line/command-line.h"
#include "init/init.h"
#include "init/init-command-line.h"
#include "init/init-cpu-timer.h"
#include "init/init-io.h"
#include "init/init-log-manager.h"
#include "init/init-thread-manager.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

static int64_t minMessageSize       = 0;
static int64_t maxMessageSize       = 0;

swOptionCategoryMainDeclare(swTrafficMainOptions, "Traffic Generator Main Options",
  swOptionDeclareScalar("message-size-min", "Minimum message size",
    NULL,   &minMessageSize,  swOptionValueTypeInt, false),
  swOptionDeclareScalar("message-size-max", "Maximum message size",
    NULL,   &maxMessageSize,  swOptionValueTypeInt, false)
);

int main (int argc, char *argv[])
{
  int rtn = EXIT_FAILURE;
  swEdgeLoop *loop = NULL;
  swThreadManager *threadManager = NULL;
  srand(time(NULL));
  uint64_t cpuTimerInterval = 1000;
  swInitData *initData[] =
  {
    swInitCommandLineDataGet(&argc, argv, "Traffic Generator Program", NULL),
    swInitIOEdgeLoopDataGet(&loop),
    swInitIOEdgeSignalsDataGet(&loop, SIGINT, SIGHUP, SIGQUIT, SIGTERM, SIGUSR1, SIGUSR2, 0),
    swInitThreadManagerDataGet(&loop, &threadManager),
    swInitLogManagerDataGet(&threadManager),
    swTrafficClientDataGet(&loop, &minMessageSize, &maxMessageSize),
    swTrafficServerDataGet(&loop, &minMessageSize, &maxMessageSize),
    swInitCPUTimerGet(&loop, &cpuTimerInterval),
    NULL
  };

  if (swInitStart (initData))
  {
    swEdgeLoopRun(loop, false);
    rtn = EXIT_SUCCESS;
    swInitStop(initData);
  }

  // TODO: implement stats module
  /*
    // init stats reporting
  */

  return rtn;
}
