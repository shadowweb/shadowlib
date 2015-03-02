#ifndef SW_PROTOCOL_ETHERNET_H
#define SW_PROTOCOL_ETHERNET_H

#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <netinet/ether.h>

#include "io/socket.h"
#include "storage/static-string.h"

typedef struct ether_addr swEthernetAddress;

bool swEthernetAddressGetLocal(swSocket *sock, swStaticString *interfaceName, swEthernetAddress *address);
swEthernetAddress *swEthernetAddressGetMulticast();

typedef struct ifaddrs          swInterfaceAddress;
typedef struct rtnl_link_stats  swInterfaceAddressStats;

typedef struct swInterfaceAddressInfo
{
  swSocketAddress address;
  swSocketAddress mask;
  union {
    swSocketAddress broadcast;
    swSocketAddress destination;
  };
  unsigned int flags;
} swInterfaceAddressInfo;

typedef struct swInterfaceInfo
{
  swStaticString          nameString;
  char                    name[IF_NAMESIZE];
  swInterfaceAddressInfo  ipv4Info;
  swInterfaceAddressInfo  ipv6Info;
  swInterfaceAddressInfo  linkLayerInfo;
  swInterfaceAddressStats stats;
  unsigned int            index;
} swInterfaceInfo;

bool swInterfaceInfoInit();
void swInterfaceInfoRelease();
bool swInterfaceInfoRefreshAll();
uint32_t swInterfaceInfoCount();
bool swInterfaceInfoGetAddress(const swStaticString *name, swSocketAddress *addr);
bool swInterfaceInfoGetStats(const swStaticString *name, swInterfaceAddressStats *stats);
bool swInterfaceInfoGetStatsWithRefresh(const swStaticString *name, swInterfaceAddressStats *stats);
void swInterfaceInfoPrint();

bool swEthernetNameToAddress(const swStaticString *interfaceName, const swEthernetAddress *ethernetAddress, swSocketAddress *address, int domain, int protocol);

#endif  // SW_PROTOCOL_ETHERNET_H
