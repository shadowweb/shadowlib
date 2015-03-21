#include "init/init-interface.h"

#include "protocol/ethernet.h"
#include "protocol/ip-v6.h"

static swInitData ethernetData = {.startFunc = swEthernetInitWellKnownAddresses, .stopFunc = NULL, .name = "Well Known Ethernet Addresses"};

swInitData *swInitEthernetDataGet()
{
  return &ethernetData;
}

static swInitData networkInterfacesData = {.startFunc = swInterfaceInfoInit, .stopFunc = swInterfaceInfoRelease, .name = "Network Interfaces"};

swInitData *swInitInterfaceDataGet()
{
  return &networkInterfacesData;
}

static swInitData ipData = {.startFunc = swIPv6InitWellKnownIPs, .stopFunc = NULL, .name = "Well Known IPs"};

swInitData *swInitIPDataGet()
{
  return &ipData;
}
