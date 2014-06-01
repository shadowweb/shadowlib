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
    if (inet_pton(AF_INET, ip6, &(inet6->sin6_addr)) > 0)
    {
      address->len = sizeof(struct sockaddr_in6);
      rtn = true;
    }
  }
  return rtn;
}
