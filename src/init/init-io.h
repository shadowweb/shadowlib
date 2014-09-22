#ifndef SW_INIT_INITIO_H
#define SW_INIT_INITIO_H

#include "init/init.h"
#include "io/edge-loop.h"

swInitData *swInitIOEdgeLoopDataGet(swEdgeLoop **loopPtr);
swInitData *swInitIOEdgeSignalsDataGet(swEdgeLoop *loop, ...);

#endif // SW_INIT_INITIO_H
