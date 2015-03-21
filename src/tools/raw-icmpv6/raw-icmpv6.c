#include <stdlib.h>
#include <signal.h>

#include "init/init.h"
#include "init/init-command-line.h"
#include "init/init-io.h"
#include "init/init-log-manager.h"
#include "init/init-thread-manager.h"
#include "init/init-interface.h"
#include "tools/raw-icmpv6/icmpv6-communicator.h"

int main (int argc, char *argv[])
{
  int rtn = EXIT_FAILURE;
  swEdgeLoop *loop = NULL;
  swThreadManager *threadManager = NULL;
  srand(time(NULL));

  swInitData *initData[] =
  {
    swInitCommandLineDataGet(&argc, argv, "ICMPv6 message sender/receiver", NULL),
    swInitIOEdgeLoopDataGet(&loop),
    swInitIOEdgeSignalsDataGet(&loop, SIGINT, SIGHUP, SIGQUIT, SIGTERM, SIGUSR1, SIGUSR2, 0),
    swInitThreadManagerDataGet(&loop, &threadManager),
    swInitLogManagerDataGet(&threadManager),
    swInitEthernetDataGet(),
    swInitInterfaceDataGet(),
    swInitIPDataGet(),
    swICMPv6CommunicatorDataGet(&loop),
    NULL
  };

  if (swInitStart (initData))
  {
    swEdgeLoopRun(loop, false);
    rtn = EXIT_SUCCESS;
    swInitStop(initData);
  }

  return rtn;
};
