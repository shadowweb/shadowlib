#include "protocol/ip-v6.h"

#include <arpa/inet.h>
#include <netinet/ip6.h>

static swStaticString routerMulticastString = swStaticStringDefine("ff02::2");
static swSocketAddress routerMulticastIP = {.len = 0};

bool swIPv6InitWellKnownIPs()
{
  bool rtn = false;
  if (swIPv6FromString(&routerMulticastString, &routerMulticastIP))
    rtn = true;
  return rtn;
}

bool swIPv6FromString(swStaticString *ipString, swSocketAddress *ipAddress)
{
  bool rtn = false;
  if (ipString)
    rtn = swSocketAddressInitInet6(ipAddress, ipString->data, 0);
  return rtn;
}

bool swIPv6ToString(swSocketAddress *ipAddress, swDynamicString *ipString)
{
  bool rtn = false;
  if (ipAddress && swDynamicStringEnsureSize(ipString, INET6_ADDRSTRLEN))
  {
    if (inet_ntop(AF_INET6, &(ipAddress->inet6.sin6_addr), ipString->data, INET6_ADDRSTRLEN))
      rtn = true;
  }
  return rtn;
}

swSocketAddress *swIPv6GetRouterMulticastAddress()
{
  return &routerMulticastIP;
}

bool swIPv6BuildMainHeader(swProtocolBuilder *builder, swSocketAddress *source, swSocketAddress *target, swIPv6Flow flow, uint16_t payloadLength, uint8_t nextProtocol, uint8_t hopLimit)
{
  bool rtn = false;
  if (builder && swSocketAddressIsInet6(source) && swSocketAddressIsInet6(target))
  {
    swStaticBuffer *ip6Buffer = swProtocolBuilderAddFrame(builder, swProtocolIPv6, swProtocolIPv6MainHeader, sizeof(struct ip6_hdr));
    if (ip6Buffer)
    {
      struct ip6_hdr *ip6Header = (struct ip6_hdr *)ip6Buffer->data;
      if (ip6Header)
      {
        ip6Header->ip6_flow = htonl((uint32_t)((flow.version << 28) + (flow.trafficClass << 20) + flow.flowId));
        ip6Header->ip6_plen = htons(payloadLength);
        ip6Header->ip6_hlim = hopLimit;
        ip6Header->ip6_nxt = nextProtocol;
        ip6Header->ip6_src = source->inet6.sin6_addr;
        ip6Header->ip6_dst = target->inet6.sin6_addr;
        rtn = true;
      }
    }
  }
  return rtn;
}
