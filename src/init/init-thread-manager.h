#ifndef SW_INIT_INITTHREADMANAGER_H
#define SW_INIT_INITTHREADMANAGER_H

#include "init/init.h"
#include "thread/thread-manager.h"

swInitData *swInitThreadManagerDataGet(swEdgeLoop **loopPtr, swThreadManager **threadManagerPtr);

#endif // SW_INIT_INITTHREADMANAGER_H
