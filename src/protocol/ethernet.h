#ifndef SW_PROTOCOL_ETHERNET_H
#define SW_PROTOCOL_ETHERNET_H

#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <netinet/ether.h>

#include "io/socket.h"
#include "protocol/protocol-builder.h"
#include "storage/static-string.h"

typedef struct ether_addr swEthernetAddress;

bool swEthernetAddressGetLocal(swSocket *sock, swStaticString *interfaceName, swEthernetAddress *address);
swEthernetAddress *swEthernetAddressGetMulticast();
swSocketAddress *swEthernetGetMulticastSocketAddress();

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
  int                     index;
} swInterfaceInfo;

bool swInterfaceInfoInit();
void swInterfaceInfoRelease();
bool swInterfaceInfoRefreshAll();
uint32_t swInterfaceInfoCount();
bool swInterfaceInfoGetAddress(const swStaticString *name, swSocketAddress *addr);
bool swInterfaceInfoGetStats(const swStaticString *name, swInterfaceAddressStats *stats);
bool swInterfaceInfoGetIndex(const swStaticString *name, int *index);
bool swInterfaceInfoGetStatsWithRefresh(const swStaticString *name, swInterfaceAddressStats *stats);
void swInterfaceInfoPrint();

bool swEthernetNameToAddress(const swStaticString *interfaceName, const swEthernetAddress *ethernetAddress, swSocketAddress *address, int domain, int protocol);
bool swEthernetInitWellKnownAddresses();
bool swEthernetSetAddress(swSocketAddress *address, const swStaticBuffer *buffer);

bool swEtherBuildFrame(swProtocolBuilder *builder, swEthernetAddress *to, swEthernetAddress *from, uint16_t protocol);

#endif  // SW_PROTOCOL_ETHERNET_H
