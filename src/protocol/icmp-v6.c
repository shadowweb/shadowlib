#include "protocol/icmp-v6.h"

#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#include "protocol/checksum.h"
#include "protocol/ip-v6.h"

// TODO: /usr/include/net/if.h contains a list of flags that can be set for interface
// figure out how to determine active interface

static void swICMPv6CalculateChecksum(swProtocolBuilder *builder, bool raw)
{
  swProtocolFrame *frame = (swProtocolFrame *)(builder->items.data);
  if (raw)
    ++frame;
  struct ip6_hdr *ipHeader = (struct ip6_hdr *)(frame->frameData.data);
  // printf ("IPv6 Header: data = %p, len = %zu\n", (void *)(frame->frameData.data), frame->frameData.len);
  ++frame;
  struct icmp6_hdr *icmp6Header = (struct icmp6_hdr *)(frame->frameData.data);
  // printf ("ICMPv6 Header: data = %p, len = %zu\n", (void *)(frame->frameData.data), frame->frameData.len);
  swChecksum checkSum = swChecksumSetEmpty;
  // printf ("ipHeader = %p, ip6_src = %p, ip6_dst = %p, size = %zu\n",
  //         ipHeader, &(ipHeader->ip6_src), &(ipHeader->ip6_dst), sizeof(struct in6_addr)*2);
  swStaticBuffer buffer = swStaticBufferDefineWithLength(&(ipHeader->ip6_src), sizeof(struct in6_addr)*2);
  swChecksumAdd(&checkSum, &buffer);
  buffer = swStaticBufferSetWithLength(&(ipHeader->ip6_plen), sizeof(uint16_t));
  swChecksumAdd(&checkSum, &buffer);
  uint16_t nextProtocol = htons((uint16_t)(ipHeader->ip6_nxt));
  buffer = swStaticBufferSetWithLength(&nextProtocol, sizeof(uint16_t));
  swChecksumAdd(&checkSum, &buffer);
  buffer = swStaticBufferSetWithLength(icmp6Header, builder->buildPosition - sizeof(struct ip6_hdr) - ((raw)? sizeof(struct ether_header) : 0));
  swChecksumAdd(&checkSum, &buffer);
  swChecksumFinalize(&checkSum);
  icmp6Header->icmp6_cksum = swChecksumGet(checkSum);
}

bool swICMPv6BuildRouterSolicit(swProtocolBuilder *builder, swSocketAddress *localIPv6, swEthernetAddress *localMAC, bool raw)
{
  bool rtn = false;
  if (builder && localIPv6 && localMAC)
  {
    if (!raw || swEtherBuildFrame(builder, swEthernetAddressGetMulticast(), localMAC, ETH_P_IPV6))
    {
      swIPv6Flow flow = { .version = 6 };
      uint16_t payloadLength = sizeof(struct nd_router_solicit) + sizeof(struct nd_opt_hdr) + sizeof(swEthernetAddress);
      if (swIPv6BuildMainHeader(builder, localIPv6, swIPv6GetRouterMulticastAddress(), flow, payloadLength, IPPROTO_ICMPV6, 255))
      {
        swStaticBuffer *icmp6Buffer = swProtocolBuilderAddFrame(builder, swProtocolICMPv6, swProtocolICMPv6InfoMessage, sizeof(struct nd_router_solicit));
        if (icmp6Buffer)
        {
          struct nd_router_solicit *icmp6Header = (struct nd_router_solicit *)icmp6Buffer->data;
          icmp6Header->nd_rs_hdr.icmp6_type = ND_ROUTER_SOLICIT;
          icmp6Header->nd_rs_hdr.icmp6_code = 0;
          icmp6Header->nd_rs_hdr.icmp6_cksum = 0;
          icmp6Header->nd_rs_hdr.icmp6_data32[0] = 0;
          swStaticBuffer *icmp6OptionBuffer = swProtocolBuilderAddFrame(builder, swProtocolICMPv6, swProtocolICMPv6Option, sizeof(struct nd_opt_hdr) + sizeof(swEthernetAddress));
          if (icmp6OptionBuffer)
          {
            struct nd_opt_hdr *option = (struct nd_opt_hdr *)icmp6OptionBuffer->data;
            option->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
            option->nd_opt_len = 1;
            swEthernetAddress *addr = (swEthernetAddress *)(icmp6OptionBuffer->data + sizeof(struct nd_opt_hdr));
            *addr = *localMAC;
            swICMPv6CalculateChecksum(builder, raw);
            return true;
          }
        }
      }
    }
  }
  return rtn;
}

/*
    swSocketAddress localAddress = {0};
    swStaticString interfaceName = swStaticStringDefine("eth0");
    swInterfaceAddress interfaceList = {0};

    if (swInterfaceAddressNew(interfaceList) == 0)
    {
      if (swInterfaceAddressFind(interfaceList, &interfaceName, AF_INET6, &localAddress))
      {
      }
      swInterfaceAddressDelete(interfaceList);
    }
*/
