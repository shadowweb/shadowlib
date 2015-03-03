#include "init/init-interface.h"

#include "protocol/ethernet.h"

static swInitData networkInterfacesData = {.startFunc = swInterfaceInfoInit, .stopFunc = swInterfaceInfoRelease, .name = "Network Interfaces"};

swInitData *swInitInterfaceDataGet()
{
  return &networkInterfacesData;
}
