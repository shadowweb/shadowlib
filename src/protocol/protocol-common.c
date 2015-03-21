#include "protocol/protocol-common.h"

#include <arpa/inet.h>
#include <linux/udp.h>
#include <net/ethernet.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>
#include <stdio.h>

static inline void swPrintEtherAddress(const char *prefix, uint8_t addr[ETH_ALEN])
{
  printf ("%s: %02x:%02x:%02x:%02x:%02x:%02x\n", prefix, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

static void swPrintEthernetFrame(swProtocolFrame *frame)
{
  if (frame)
  {
    printf("ETHERNET frame %p:\n", (void *)(frame->frameData.data));
    struct ether_header *header = (struct ether_header *)(frame->frameData.data);
    swPrintEtherAddress("\tether_dhost", header->ether_dhost);
    swPrintEtherAddress("\tether_shost", header->ether_shost);
    printf ("\tether_type: %04x\n", ntohs(header->ether_type));
  }
}

static void swPrintIPv6Address(const char *prefix, struct in6_addr *addr)
{
  char addressString[INET6_ADDRSTRLEN];
  if (inet_ntop(AF_INET6, addr, addressString, INET6_ADDRSTRLEN))
    printf ("%s: %s\n", prefix, addressString);
}

static void __attribute__((unused)) swPrintIPv4Address(const char *prefix, struct in_addr *addr)
{
  char addressString[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, addr, addressString, INET6_ADDRSTRLEN))
    printf ("%s: %s\n", prefix, addressString);
}

static void swPrintIPv6Frame(swProtocolFrame *frame)
{
  if (frame)
  {
    switch (frame->frameId)
    {
      case swProtocolIPv6MainHeader:
      {
        printf ("IPv6 MAIN HEADER frame %p:\n", (void *)(frame->frameData.data));
        struct ip6_hdr *header = (struct ip6_hdr *)frame->frameData.data;
        uint32_t flowData = ntohl(header->ip6_flow);
        printf ("\tversion: %u\n", flowData >> 28 );
        printf ("\ttc: %u\n", (flowData >> 20) & 0xFF );
        printf ("\tflow id: %u\n", flowData & 0xFFFFF );
        printf ("\tpacket length: %u\n", ntohs(header->ip6_plen));
        printf ("\tnext: %u\n", header->ip6_nxt);
        printf ("\thop limit: %u\n", header->ip6_hlim);
        swPrintIPv6Address("\tsource IP", &header->ip6_src);
        swPrintIPv6Address("\tdestination IP", &header->ip6_dst);
        break;
      }
      // TODO: print other supported headers too
      case swProtocolIPv6HopByHopHeader:
      case swProtocolIPv6DestinationHeader:
      case swProtocolIPv6RoutingHeader:
      case swProtocolIPv6FragmentHeader:
      case swProtocolIPv6FinalDestinationHeader:
      case swProtocolIPv6NoNextHeaderPayload:
      default:
        printf ("Invalid IPv6 header type\n");
        break;
    }
  }
}

static void swPrintICMPv6Frame(swProtocolFrame *frame)
{
  if (frame)
  {
    switch (frame->frameId)
    {
      case swProtocolICMPv6InfoMessage:
      {
        printf ("ICMPv6 HEADER frame %p:\n", (void *)(frame->frameData.data));
        struct icmp6_hdr *header = (struct icmp6_hdr *)(frame->frameData.data);
        printf ("\ttype: %u\n", header->icmp6_type);
        printf ("\tcode: %u\n", header->icmp6_code);
        printf ("\tcheck sum: %04x\n", header->icmp6_cksum);
        printf ("\tdata: %08x\n", header->icmp6_data32[0]);
        switch (header->icmp6_type)
        {
          case ND_ROUTER_ADVERT:
          {
            struct nd_router_advert *typeHeader = (struct nd_router_advert *)header;
            printf ("\treachable time: %u\n", ntohl(typeHeader-> nd_ra_reachable));
            printf ("\tretransmit timer: %u\n", ntohl(typeHeader->nd_ra_retransmit));
            break;
          }
          case ND_NEIGHBOR_SOLICIT:
          {
            struct nd_neighbor_solicit *typeHeader = (struct nd_neighbor_solicit *)header;
            swPrintIPv6Address("\ttarget IP", &typeHeader->nd_ns_target);
            break;
          }
          case ND_NEIGHBOR_ADVERT:
          {
            struct nd_neighbor_advert *typeHeader = (struct nd_neighbor_advert *)header;
            swPrintIPv6Address("\ttarget IP", &typeHeader->nd_na_target);
            break;
          }
          case ND_REDIRECT:
          {
            struct nd_redirect *typeHeader = (struct nd_redirect *)header;
            swPrintIPv6Address("\ttarget IP", &typeHeader->nd_rd_target);
            swPrintIPv6Address("\tdestinaton IP", &typeHeader->nd_rd_dst);
            break;
          }
          default:
            break;
        }
        break;
      }
      case swProtocolICMPv6Option:
      {
        printf ("ICMPv6 OPTION frame %p:\n", (void *)(frame->frameData.data));
        struct nd_opt_hdr *header = (struct nd_opt_hdr *)frame->frameData.data;
        printf ("\ttype: %u\n", header->nd_opt_type);
        printf ("\tlength: %u\n", header->nd_opt_len);
        switch (header->nd_opt_type)
        {
          case ND_OPT_SOURCE_LINKADDR:
          {
            // uint8_t addr[ETH_ALEN]
            uint8_t *addr = (uint8_t *)frame->frameData.data + sizeof(struct nd_opt_hdr);
            swPrintEtherAddress("\tsource", addr);
            printf ("\n");
            break;
          }
          case ND_OPT_TARGET_LINKADDR:
          {
            uint8_t *addr = (uint8_t *)frame->frameData.data + sizeof(struct nd_opt_hdr);
            swPrintEtherAddress("\ttarget", addr);
            printf ("\n");
            break;
          }
          case ND_OPT_PREFIX_INFORMATION:
          {
            struct nd_opt_prefix_info *typeHeader = (struct nd_opt_prefix_info *)header;
            printf ("\tprefix length: %u\n", typeHeader->nd_opt_pi_prefix_len);
            printf ("\tflags reserved: %u\n", typeHeader->nd_opt_pi_flags_reserved);
            printf ("\tvalid time: %u\n", ntohl(typeHeader->nd_opt_pi_valid_time));
            printf ("\tpreferred time: %u\n", ntohl(typeHeader->nd_opt_pi_prefix_len));
            printf ("\treserved2: %u\n", ntohl(typeHeader->nd_opt_pi_reserved2));
            swPrintIPv6Address("\tprefix", &typeHeader->nd_opt_pi_prefix);
            break;
          }
          case ND_OPT_REDIRECTED_HEADER:
          {
            struct nd_opt_rd_hdr *typeHeader = (struct nd_opt_rd_hdr *)header;
            printf ("\treserved1: %u\n", ntohs(typeHeader->nd_opt_rh_reserved1));
            printf ("\treserved2: %u\n", ntohl(typeHeader->nd_opt_rh_reserved2));
            // followed by IP header and data as much as can fit
            break;
          }
          case ND_OPT_MTU:
          {
            struct nd_opt_mtu *typeHeader = (struct nd_opt_mtu *)header;
            printf ("\treserved: %u\n", ntohs(typeHeader->nd_opt_mtu_reserved));
            printf ("\tmtu: %u\n", ntohl(typeHeader->nd_opt_mtu_mtu));
            // followed by IP header and data as much as can fit
            break;
          }
          // TODO: print other supported options too
          default:
            break;
        }
        break;
      }
      // TODO: print other supported headers too
      case swProtocolICMPv6ErrorMessage:
      case swProtocolICMPv6UnknownErrorMessage:
      case swProtocolICMPv6UnknownInfoMessage:
      default:
        printf ("Invalid ICMPv6 header type\n");
        break;
    }
  }
}

static void swPrintUDPFrame(swProtocolFrame *frame)
{
  if (frame)
  {
    printf("UDP frame %p:\n", (void *)(frame->frameData.data));
    struct udphdr *header = (struct udphdr *)frame->frameData.data;
    printf ("\tsource: %u\n", ntohs(header->source));
    printf ("\tdestination: %u\n", ntohs(header->dest));
    printf ("\tlength: %u\n", ntohs(header->len));
    printf ("\tcheck sum: %04x\n", header->check);
    // followed by data
  }
}

static void swPrintUnknownFrame(swProtocolFrame *frame)
{
  if (frame)
  {
    printf ("UNKNOWN frame %p\n", (void *)(frame->frameData.data));
    uint8_t *data = (uint8_t *)(frame->frameData.data);
    uint32_t i = 0;
    while (i + 15 < frame->frameData.len)
    {
      printf ("\t%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
        data[i],      data[i + 1],  data[i + 2],  data[i + 3],
        data[i + 4],  data[i + 5],  data[i + 6],  data[i + 7],
        data[i + 8],  data[i + 9],  data[i + 10], data[i + 11],
        data[i + 12], data[i + 13], data[i + 14], data[i + 15]);
      i += 16;
    }
    printf("\t");
    while (i + 1 < frame->frameData.len)
    {
      printf ("%02x%02x ", data[i], data[i + 1]);
      i += 2;
    }
    if (i < frame->frameData.len)
      printf ("%02x ", data[i]);
    printf ("\n");
  }
}

typedef void (*swProtocolPrintFrameFunction)(swProtocolFrame *frame);

swProtocolPrintFrameFunction swProtocolPrintMap[swProtocolMax + 1] =
{
  NULL,                   // swProtocolNone
  swPrintEthernetFrame,   // swProtocolEthernet
  NULL,                   // swProtocolIPv4   -- swParseIPv4Frame
  swPrintIPv6Frame,       // swProtocolIPv6
  NULL,                   // swProtocolICMPv4 -- swParseICMPv4Frame
  swPrintICMPv6Frame,     // swProtocolICMPv6
  NULL,                   // swProtocolTCP    -- swParseTCPFrame
  swPrintUDPFrame,        // swProtocolUDP
  swPrintUnknownFrame,    // swProtocolUnknown
  NULL                    // swProtocolMax
};

// TODO: figure out why I have memory errors here
void swProtocolFrameArrayPrint(const swStaticArray *frames)
{
  if (frames)
  {
    printf ("-----------------------------\n");
    swProtocolFrame *frame = (swProtocolFrame *)swStaticArrayData(*frames);
    swProtocolFrame *endFrame = frame + swStaticArrayCount(*frames);
    while (frame < endFrame)
    {
      if (swProtocolPrintMap[frame->protocolId])
        swProtocolPrintMap[frame->protocolId](frame);
      else
        printf ("no print functiion for frame: protocolId %u, frameId %u\n", frame->protocolId, frame->frameId);
      frame++;
    }
    printf ("-----------------------------\n");
  }
}
