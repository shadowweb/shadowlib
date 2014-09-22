#ifndef SW_TOOLS_TRAFFICGENERATOR_TRAFFICSERVER_H
#define SW_TOOLS_TRAFFICGENERATOR_TRAFFICSERVER_H

#include "collections/dynamic-array.h"
#include "init/init.h"
#include "io/edge-loop.h"
#include "io/tcp-server.h"

typedef struct swTrafficAcceptorData
{
  swTCPServerAcceptor *acceptor;
  swDynamicArray serverConnections;
  uint64_t sendInterval;
  uint32_t portPosition;
  uint32_t firstFree;
} swTrafficAcceptorData;

swInitData *swTrafficServerDataGet(swEdgeLoop *loop, int64_t *minMessageSize, int64_t *maxMessageSize);

#endif  // SW_TOOLS_TRAFFICGENERATOR_TRAFFICSERVER_H
