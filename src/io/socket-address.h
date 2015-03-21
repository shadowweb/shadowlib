#ifndef SW_IO_SOCKETADDRESS_H
#define SW_IO_SOCKETADDRESS_H

#include <netinet/in.h>
#include <linux/if_packet.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "storage/dynamic-string.h"

typedef struct swSocketAddress
{
  socklen_t len;
  union
  {
    struct sockaddr_storage  storage;
    struct sockaddr_un       unixDomain;
    struct sockaddr_in       inet;
    struct sockaddr_in6      inet6;
    struct sockaddr_ll       linkLayer;
    struct sockaddr          addr;
  };
} swSocketAddress;

bool swSocketAddressInitUnix(swSocketAddress *address, char *path);
bool swSocketAddressInitInet(swSocketAddress *address, char *ip, uint16_t port);
bool swSocketAddressInitInet6(swSocketAddress *address, char *ip6, uint16_t port);
bool swSocketAddressInitLinkLayer(swSocketAddress *address, uint16_t protocol, int index);

#define swSocketAddressSetUnixDomain(a) *(swSocketAddress[]){{.len = sizeof(struct sockaddr_un),  { .storage = *((struct sockaddr_storage *)&(a)) } }}
#define swSocketAddressSetInet(a)       *(swSocketAddress[]){{.len = sizeof(struct sockaddr_in),  { .storage = *((struct sockaddr_storage *)&(a)) } }}
#define swSocketAddressSetInet6(a)      *(swSocketAddress[]){{.len = sizeof(struct sockaddr_in6), { .storage = *((struct sockaddr_storage *)&(a)) } }}
#define swSocketAddressSetLinkLayer(a)  *(swSocketAddress[]){{.len = sizeof(struct sockaddr_ll),  { .storage = *((struct sockaddr_storage *)&(a)) } }}

#define swSocketAddressIsUnixDomain(a)  ((a) && ((a)->addr.sa_family == AF_UNIX))
#define swSocketAddressIsInet(a)        ((a) && ((a)->addr.sa_family == AF_INET))
#define swSocketAddressIsInet6(a)       ((a) && ((a)->addr.sa_family == AF_INET6))
#define swSocketAddressIsLinkLayer(a)   ((a) && ((a)->addr.sa_family == AF_PACKET))

swDynamicString *swSocketAddressToString(swSocketAddress *address);

#endif // SW_IO_SOCKETADDRESS_H
