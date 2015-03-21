#include "socket-address.h"

#include <storage/static-string.h>

#include <arpa/inet.h>

bool swSocketAddressInitUnix(swSocketAddress *address, char *path)
{
  bool rtn = false;
  swStaticString pathString = swStaticStringDefineFromCstr(path);
  if (address && pathString.data && (pathString.len < (sizeof(struct sockaddr_un) - sizeof(sa_family_t))))
  {
    struct sockaddr_un *unixDomain = &(address->unixDomain);
        unixDomain->sun_family = AF_UNIX;
    memcpy(unixDomain->sun_path, pathString.data, (pathString.len + 1));
    address->len = sizeof(struct sockaddr_un);
    rtn = true;
  }
  return rtn;
}

bool swSocketAddressInitInet(swSocketAddress *address, char *ip, uint16_t port)
{
  bool rtn = false;
  if (address && ip)
  {
    struct sockaddr_in *inet = &(address->inet);
    inet->sin_family = AF_INET;
    inet->sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &(inet->sin_addr)) > 0)
    {
      address->len = sizeof(struct sockaddr_in);
      rtn = true;
    }
  }
  return rtn;
}

bool swSocketAddressInitInet6(swSocketAddress *address, char *ip6, uint16_t port)
{
  bool rtn = false;
  if (address && ip6)
  {
    struct sockaddr_in6 *inet6 = &(address->inet6);
        inet6->sin6_family = AF_INET6;
        inet6->sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip6, &(inet6->sin6_addr)) > 0)
    {
      address->len = sizeof(struct sockaddr_in6);
      rtn = true;
    }
  }
  return rtn;
}

bool swSocketAddressInitLinkLayer(swSocketAddress *address, uint16_t protocol, int index)
{
  bool rtn = false;
  if (address)
  {
    address->linkLayer.sll_family = AF_PACKET;
    address->linkLayer.sll_protocol = htons(protocol);
    address->linkLayer.sll_ifindex = index;
    address->len = sizeof(struct sockaddr_ll);
    rtn = true;
  }
  return rtn;
}

swDynamicString *swSocketAddressToString(swSocketAddress *address)
{
  swDynamicString *rtn = NULL;
  if (address)
  {
    switch (address->addr.sa_family)
    {
      case AF_INET:
      {
        char addressString[INET_ADDRSTRLEN];
        if (inet_ntop(address->inet.sin_family, &(address->inet.sin_addr), addressString, INET_ADDRSTRLEN))
          rtn = swDynamicStringNewFromFormat("AF_INET: address %s, port %u", addressString, htons(address->inet.sin_port));
        break;
      }
      case AF_INET6:
      {
        char addressString[INET6_ADDRSTRLEN];
        if (inet_ntop(address->inet6.sin6_family, &(address->inet6.sin6_addr), addressString, INET6_ADDRSTRLEN))
          rtn = swDynamicStringNewFromFormat("AF_INET6: address %s, port %u", addressString, htons(address->inet6.sin6_port));
        break;
      }
      case AF_UNIX:
      {
        rtn = swDynamicStringNewFromFormat("AF_UNIX: address %s", address->unixDomain.sun_path);
        break;
      }
      case AF_PACKET:
      {
        rtn = swDynamicStringNewFromFormat("AF_PACKET: address protocol %u, index %d, hatype %u, packet type %u, halen = %u, %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
            address->linkLayer.sll_protocol, address->linkLayer.sll_ifindex, address->linkLayer.sll_hatype,
            address->linkLayer.sll_pkttype, address->linkLayer.sll_halen,
            address->linkLayer.sll_addr[0], address->linkLayer.sll_addr[1], address->linkLayer.sll_addr[2], address->linkLayer.sll_addr[3],
            address->linkLayer.sll_addr[4], address->linkLayer.sll_addr[5], address->linkLayer.sll_addr[6], address->linkLayer.sll_addr[7]);
        break;
      }
      default:
        break;
    }
  }
  return rtn;
}
