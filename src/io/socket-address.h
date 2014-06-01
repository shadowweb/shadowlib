#ifndef SW_IO_SOCKETADDRESS_H
#define SW_IO_SOCKETADDRESS_H

#include <stdbool.h>
 #include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
// #include <arpa/inet.h>

typedef struct swSocketAddress
{
  socklen_t len;
  union
  {
    struct sockaddr_storage  storage;
    struct sockaddr_un       unixDomain;
    struct sockaddr_in       inet;
    struct sockaddr_in6      inet6;
  };
} swSocketAddress;

bool swSocketAddressInitUnix(swSocketAddress *address, char *path);
bool swSocketAddressInitInet(swSocketAddress *address, char *ip, uint16_t port);
bool swSocketAddressInitInet6(swSocketAddress *address, char *ip6, uint16_t port);

#endif // SW_IO_SOCKETADDRESS_H
