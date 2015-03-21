#include <arpa/inet.h>
#include <linux/udp.h>
#include <net/ethernet.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>
#include <stdio.h>
#include <sys/stat.h>

#include "protocol/protocol-parser.h"

static inline void swParseSetNextFrame(swProtocolParser *parser, uint32_t protocolId, uint32_t frameId, void *ptr, size_t size, swProtocolFrame **returnFrame)
{
  swProtocolFrame *frame = (swProtocolFrame *)swDynamicArrayGetNext(&(parser->items));
  frame->protocolId = protocolId;
  frame->frameId = frameId;
  frame->frameData = swStaticBufferSetWithLength(ptr, size);
  if (returnFrame)
    *returnFrame = frame;
}

static void swParseIPv6Options(swProtocolParser *parser, size_t parsePosition, size_t optionsLength)
{
  struct ip6_opt *option = NULL;
  size_t offset = 0;
  while (offset < optionsLength)
  {
    option = (struct ip6_opt *)(&parser->buffer.data[parsePosition + offset]);
    switch (option->ip6o_type)
    {
      case IP6OPT_PAD1:
        offset++;
        break;
      case IP6OPT_PADN:
        offset += sizeof(struct ip6_opt) + option->ip6o_len;
        break;
      default:
        swParseSetNextFrame(parser, swProtocolIPv6, swProtocolIPv6Option, option, (sizeof(struct ip6_opt) + option->ip6o_len), NULL);
        offset += (sizeof(struct ip6_opt) + option->ip6o_len);
        break;
    }
  }
}

static bool swParseIPv6ExtensionHeader(swProtocolParser *parser, uint32_t frameId, uint16_t *flags, uint8_t *next, bool parseOptions)
{
  bool rtn = false;
  uint16_t headerFlag = 1 << (frameId - swProtocolIPv6MainHeader);
  if (*flags < headerFlag && parser->bytesLeft >= sizeof(struct ip6_ext))
  {
    struct ip6_ext *header = (struct ip6_ext *)(&(parser->buffer.data[parser->parsePosition]));
    *next = header->ip6e_nxt;
    size_t parsePosition = parser->parsePosition;
    size_t extensionSize = (header->ip6e_len + 1) * 8;
    if (parser->bytesLeft >= extensionSize)
    {
      swParseSetNextFrame(parser, swProtocolIPv6, frameId, header, extensionSize, NULL);
      parser->parsePosition += extensionSize;
      if (parseOptions)
        swParseIPv6Options(parser, parsePosition + sizeof(struct ip6_ext), extensionSize);
      *flags |= headerFlag;
      rtn = true;
    }
  }
  return rtn;
}

static bool swParseIPv6FragmentHeader(swProtocolParser *parser, uint32_t frameId, uint16_t *flags, uint8_t *next)
{
  bool rtn = false;
  uint16_t headerFlag = 1 << (frameId - swProtocolIPv6MainHeader);
  size_t extensionSize = sizeof(struct ip6_frag);
  if (*flags < headerFlag && parser->bytesLeft >= extensionSize)
  {
    struct ip6_ext *header = (struct ip6_ext *)&(parser->buffer.data[parser->parsePosition]);
    *next = header->ip6e_nxt;
    swParseSetNextFrame(parser, swProtocolIPv6, frameId, header, extensionSize, NULL);
    parser->parsePosition += extensionSize;
    *flags |= headerFlag;
    rtn = true;
  }
  return rtn;
}

static bool swParseEthernetFrame(swProtocolParser *parser, swProtocolIndentifier *nextProtocol)
{
  printf("'%s': parsing ethernet frame, parser->buffer.len = %zu, parser->parsePosition = %zu\n",
         __func__, parser->buffer.len, parser->parsePosition);
  bool rtn = false;
  if (parser->buffer.len >= (parser->parsePosition + sizeof(struct ether_header)))
  {
    struct ether_header *header = (struct ether_header *)&(parser->buffer.data[parser->parsePosition]);
    switch (ntohs(header->ether_type))
    {
      case ETH_P_IP:
        *nextProtocol = swProtocolIPv4;
        break;
      case ETH_P_IPV6:
        *nextProtocol = swProtocolIPv6;
        break;
      default:
            parser->error = true;
        break;
    }
    if (!parser->error)
    {
      swParseSetNextFrame(parser, swProtocolEthernet, 0, header, sizeof(struct ether_header), NULL);
      parser->parsePosition += sizeof(struct ether_header);
      rtn = true;
    }
  }
  return rtn;
}

static bool swParseIPv6Frame(swProtocolParser *parser, swProtocolIndentifier *nextProtocol)
{
  printf("'%s': parsing IPv6 frame\n", __func__);
  bool rtn = false;
  if (parser->buffer.len >= (parser->parsePosition + sizeof(struct ip6_hdr)))
  {
    uint16_t flags = 0;
    struct ip6_hdr *header = (struct ip6_hdr *)&(parser->buffer.data[parser->parsePosition]);
    uint8_t next = header->ip6_nxt;
    parser->bytesLeft = ntohs(header->ip6_plen);
    if ((parser->parsePosition + sizeof(struct ip6_hdr) + parser->bytesLeft) <= parser->buffer.len)
    {
      swProtocolFrame *frame = NULL;
      swParseSetNextFrame(parser, swProtocolIPv6, swProtocolIPv6MainHeader, header, sizeof(struct ip6_hdr), &frame);
      parser->parsePosition += sizeof(struct ip6_hdr);
      flags |= 1 << (frame->frameId - 1);
      bool done = false;
      while (parser->bytesLeft && !done && !parser->error)
      {
        switch (next)
        {
          case IPPROTO_HOPOPTS:
            if (!swParseIPv6ExtensionHeader(parser, swProtocolIPv6HopByHopHeader, &flags, &next, true))
            {
              printf("'%s': failed to parse hop by hop extension header\n", __func__);
              parser->error = true;
            }
            break;
          case IPPROTO_DSTOPTS:
            if (!swParseIPv6ExtensionHeader(parser, swProtocolIPv6DestinationHeader, &flags, &next, true) &&
                !swParseIPv6ExtensionHeader(parser, swProtocolIPv6FinalDestinationHeader, &flags, &next, true))
              parser->error = true;
            break;
          case IPPROTO_ROUTING:
            if (!swParseIPv6ExtensionHeader(parser, swProtocolIPv6RoutingHeader, &flags, &next, false))
              parser->error = true;
            break;
          case IPPROTO_FRAGMENT:
            if (!swParseIPv6FragmentHeader(parser, swProtocolIPv6FragmentHeader, &flags, &next))
              parser->error = true;
            break;
          case IPPROTO_NONE:
            if (parser->bytesLeft)
            {
              swParseSetNextFrame(parser, swProtocolIPv6, swProtocolIPv6NoNextHeaderPayload, &(parser->buffer.data[parser->parsePosition]), parser->bytesLeft, NULL);
              parser->parsePosition += parser->bytesLeft;
              parser->bytesLeft = 0;
            }
            parser->error = true;
            break;
          case IPPROTO_ICMPV6:
            *nextProtocol = swProtocolICMPv6;
            done = true;
            break;
          case IPPROTO_ICMP:
            *nextProtocol = swProtocolICMPv4;
            done = true;
            break;
          case IPPROTO_TCP:
            *nextProtocol = swProtocolTCP;
            done = true;
            break;
          case IPPROTO_UDP:
            *nextProtocol = swProtocolUDP;
            done = true;
            break;
          default:
            parser->error = true;
            break;
        }
      }
    }
    else
      parser->error = true;
    if (!parser->error)
      rtn = true;
  }
  return rtn;
}

static void swParseICMPv6Options(swProtocolParser *parser, size_t parsePosition, size_t optionsLength)
{
  struct nd_opt_hdr *option = NULL;
  size_t offset = 0;
  while (offset < optionsLength)
  {
    option = (struct nd_opt_hdr *)&(parser->buffer.data[parsePosition + offset]);
    swParseSetNextFrame(parser, swProtocolICMPv6, swProtocolICMPv6Option, option, option->nd_opt_len * 8, NULL);
    offset += option->nd_opt_len * 8;
  }
}

static void swParseICMPv6DiscoveryMessage(swProtocolParser *parser, void *ptr, size_t headerSize)
{
  swParseSetNextFrame(parser, swProtocolICMPv6, swProtocolICMPv6InfoMessage, ptr, parser->bytesLeft, NULL);
  swParseICMPv6Options(parser, parser->parsePosition + headerSize, parser->bytesLeft - headerSize);
  parser->parsePosition += parser->bytesLeft;
  parser->bytesLeft = 0;
}

static bool swParseICMPv6Frame(swProtocolParser *parser, swProtocolIndentifier *nextProtocol)
{
  printf("'%s': parsing ICMPv6 frame\n", __func__);
  bool rtn = false;
  if (parser->bytesLeft >= sizeof(struct icmp6_hdr) && parser->buffer.len >= (parser->parsePosition + parser->bytesLeft))
  {
    struct icmp6_hdr *header = (struct icmp6_hdr *)&(parser->buffer.data[parser->parsePosition]);
    switch (header->icmp6_type)
    {
      case ICMP6_DST_UNREACH:
      case ICMP6_PACKET_TOO_BIG:
      case ICMP6_TIME_EXCEEDED:
      case ICMP6_PARAM_PROB:
      {
        swParseSetNextFrame(parser, swProtocolICMPv6, swProtocolICMPv6ErrorMessage, header, parser->bytesLeft, NULL);
        parser->parsePosition += parser->bytesLeft;
        parser->bytesLeft = 0;
        break;
      }
      case ICMP6_ECHO_REQUEST:
      case ICMP6_ECHO_REPLY:
      case MLD_LISTENER_QUERY:
      case MLD_LISTENER_REPORT:
      case MLD_LISTENER_REDUCTION:
      {
        swParseSetNextFrame(parser, swProtocolICMPv6, swProtocolICMPv6InfoMessage, header, parser->bytesLeft, NULL);
        parser->parsePosition += parser->bytesLeft;
        parser->bytesLeft = 0;
        break;
      }
      case ND_ROUTER_SOLICIT:
        swParseICMPv6DiscoveryMessage(parser, header, sizeof(struct nd_router_solicit));
        break;
      case ND_ROUTER_ADVERT:
        swParseICMPv6DiscoveryMessage(parser, header, sizeof(struct nd_router_advert));
        break;
      case ND_NEIGHBOR_SOLICIT:
        swParseICMPv6DiscoveryMessage(parser, header, sizeof(struct nd_neighbor_solicit));
        break;
      case ND_NEIGHBOR_ADVERT:
        swParseICMPv6DiscoveryMessage(parser, header, sizeof(struct nd_neighbor_advert));
        break;
      case ND_REDIRECT:
        swParseICMPv6DiscoveryMessage(parser, header, sizeof(struct nd_redirect));
        break;
      case ICMP6_ROUTER_RENUMBERING:
        swParseICMPv6DiscoveryMessage(parser, header, sizeof(struct icmp6_router_renum));
        break;
      default:
      {
        uint32_t frameId = swProtocolICMPv6UnknownErrorMessage;
        if (header->icmp6_type & ICMP6_INFOMSG_MASK)
          frameId = swProtocolICMPv6UnknownInfoMessage;
        swParseSetNextFrame(parser, swProtocolICMPv6, frameId, header, parser->bytesLeft, NULL);
        parser->parsePosition += parser->bytesLeft;
        parser->bytesLeft = 0;
        break;
      }
    }
    if (!parser->bytesLeft)
    {
      parser->done = true;
      rtn = true;
    }
  }
  return rtn;
}

static bool swParseUDPFrame(swProtocolParser *parser, swProtocolIndentifier *nextProtocol)
{
  printf("'%s': parsing UDP frame: bytesLeft = %zu, parsePosition = %zu, buffer length = %zu\n",
         __func__, parser->bytesLeft, parser->parsePosition, parser->buffer.len);
  bool rtn = false;
  if (parser->bytesLeft >= sizeof(struct udphdr) && parser->buffer.len >= (parser->parsePosition + parser->bytesLeft))
  {
    struct udphdr *header = (struct udphdr *)&(parser->buffer.data[parser->parsePosition]);
    swParseSetNextFrame(parser, swProtocolUDP, 0, header, sizeof(struct udphdr), NULL);
    parser->parsePosition += sizeof(struct udphdr);
    if (parser->bytesLeft == ntohs(header->len))
    {
      parser->bytesLeft -= sizeof(struct udphdr);
      if (parser->bytesLeft)
        *nextProtocol = swProtocolUnknown;
      rtn = true;
    }
    else
      printf ("bytesLeft != UDP header length\n");
  }
  return rtn;
}

static bool swParseUnknownFrame(swProtocolParser *parser, swProtocolIndentifier *nextProtocol)
{
  printf("'%s': parsing Unknown frame\n", __func__);
  bool rtn = false;
  if (parser->buffer.len >= (parser->parsePosition + parser->bytesLeft))
  {
    swParseSetNextFrame(parser, swProtocolUnknown, 0, &(parser->buffer.data[parser->parsePosition]), parser->bytesLeft, NULL);
    parser->parsePosition += parser->bytesLeft;
    parser->bytesLeft = 0;
    parser->done = true;
    rtn = true;
  }
  return rtn;
}

typedef bool (*swProtocolParseFunction)(swProtocolParser *parser, swProtocolIndentifier *nextProtocol);

static swProtocolParseFunction swProtocolMap[swProtocolMax + 1] =
{
  NULL,                   // swProtocolNone
  swParseEthernetFrame,   // swProtocolEthernet
  NULL,                   // swProtocolIPv4   -- swParseIPv4Frame
  swParseIPv6Frame,       // swProtocolIPv6
  NULL,                   // swProtocolICMPv4 -- swParseICMPv4Frame
  swParseICMPv6Frame,     // swProtocolICMPv6
  NULL,                   // swProtocolTCP    -- swParseTCPFrame
  swParseUDPFrame,        // swProtocolUDP
  swParseUnknownFrame,    // swProtocolUnknown
  NULL                    // swProtocolMax
};
/*
{
  [swProtocolNone]      = NULL,
  [swProtocolEthernet]  = swParseEthernetFrame,
  // [swProtocolIPv4]      = swParseIPv4Frame,
  [swProtocolIPv6]      = swParseIPv6Frame,
  // [swProtocolICMPv4]    = swParseICMPv4Frame,
  [swProtocolICMPv6]    = swParseICMPv6Frame,
  // [swProtocolTCP]       = swParseTCPFrame,
  // [swProtocolUDP]       = swParseUDPFrame,
  [swProtocolMax]       = NULL
};
*/

bool swProtocolParserParse(swProtocolParser *parser, swStaticBuffer *buffer)
{
  bool rtn = false;
  if (parser && buffer)
  {
    parser->buffer = *buffer;
    parser->parsePosition = 0;
    parser->done = false;
    parser->error = false;
    swDynamicArrayClear(&(parser->items));
    swProtocolIndentifier protocolId = swProtocolEthernet;
    swProtocolIndentifier nextProtocolId = swProtocolNone;
    while(protocolId && (!parser->done || !parser->error) && protocolId < swProtocolMax)
    {
      nextProtocolId = swProtocolNone;
      if (swProtocolMap[protocolId])
      {
        if ((swProtocolMap[protocolId])(parser, &nextProtocolId))
        {
          protocolId = nextProtocolId;
          continue;
        }
      }
      else
      {
        swParseSetNextFrame(parser, swProtocolUnknown, 0, &(parser->buffer.data[parser->parsePosition]), parser->buffer.len - parser->parsePosition, NULL);
        parser->parsePosition = parser->buffer.len;
        parser->done = true;
      }
      break;
    }
    if (parser->done && !parser->error)
      rtn = true;
  }
  return rtn;
}

bool swProtocolParserInit(swProtocolParser *parser)
{
  bool rtn = false;
  if (parser)
  {
    parser->buffer = swStaticBufferSetEmpty;
    if (swDynamicArrayInit(&(parser->items), sizeof(swProtocolFrame), 16))
    {
      parser->parsePosition = 0;
      parser->bytesLeft = 0;
      parser->done = false;
      parser->error = false;
      rtn = true;
    }
  }
  return rtn;
}

void swProtocolParserRelease(swProtocolParser *parser)
{
  if (parser)
  {
    swDynamicArrayRelease(&(parser->items));
    parser->buffer = swStaticBufferSetEmpty;
  }
}
