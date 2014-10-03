#ifndef SW_INIT_INITCPUTIMER_H
#define SW_INIT_INITCPUTIMER_H

#include "init/init.h"
#include "io/edge-loop.h"

swInitData *swInitCPUTimerGet(swEdgeLoop **loop, uint64_t *timerInterval);

#endif // SW_INIT_INITCPUTIMER_H
