#ifndef SW_INIT_INITCOMMANDLINE_H
#define SW_INIT_INITCOMMANDLINE_H

#include "storage/dynamic-string.h"
#include "init/init.h"

swInitData *swInitCommandLineDataGet(int *argc, char **argv, const char *title, const char *usageMessage);

#endif // SW_INIT_INITCOMMANDLINE_H
