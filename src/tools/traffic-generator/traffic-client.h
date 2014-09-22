#ifndef SW_TOOLS_TRAFFICGENERATOR_TRAFFICCLIENT_H
#define SW_TOOLS_TRAFFICGENERATOR_TRAFFICCLIENT_H

#include "init/init.h"
#include "io/edge-loop.h"

swInitData *swTrafficClientDataGet(swEdgeLoop *loop, int64_t *minMessageSize, int64_t *maxMessageSize);

#endif  // SW_TOOLS_TRAFFICGENERATOR_TRAFFICCLIENT_H
