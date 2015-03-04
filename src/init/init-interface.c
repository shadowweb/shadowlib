#include "init/init-interface.h"

#include "protocol/ethernet.h"
#include "protocol/ip-v6.h"

static swInitData networkInterfacesData = {.startFunc = swInterfaceInfoInit, .stopFunc = swInterfaceInfoRelease, .name = "Network Interfaces"};

swInitData *swInitInterfaceDataGet()
{
  return &networkInterfacesData;
}

static swInitData ipData = {.startFunc = swInterfaceInfoInit, .stopFunc = NULL, .name = "Well Known IPs"};

swInitData *swInitIPDataGet()
{
  return &ipData;
}
