#ifndef SW_PROTOCOL_PROTOCOLCOMMON_H
#define SW_PROTOCOL_PROTOCOLCOMMON_H

#include <stdint.h>

#include "collections/static-array.h"
#include "storage/static-buffer.h"

typedef enum swProtocolIndentifier
{
  swProtocolNone,
  swProtocolEthernet,
  swProtocolIPv4,
  swProtocolIPv6,
  swProtocolICMPv4,
  swProtocolICMPv6,
  swProtocolTCP,
  swProtocolUDP,
  swProtocolUnknown,
  swProtocolMax
} swProtocolIndentifier;

typedef enum swProtocolIPv6Identifier
{
  swProtocolIPv6None,
  swProtocolIPv6MainHeader,             // IPPROTO_IPV6 (46)
  swProtocolIPv6HopByHopHeader,         // IPPROTO_HOPOPTS (0)
  swProtocolIPv6DestinationHeader,      // IPPROTO_DSTOPTS (60)
  swProtocolIPv6RoutingHeader,          // IPPROTO_ROUTING (43)
  swProtocolIPv6FragmentHeader,         // IPPROTO_FRAGMENT (44)
  // swProtocolIPv6AuthenticationHeader,   // IPPROTO_AH (51)
  // swProtocolIPv6SecurityPayloadHeader,  // IPPROTO_ESP (50)
  swProtocolIPv6FinalDestinationHeader, // IPPROTO_DSTOPTS (60)
  // mobolity header IPPROTO_MH (135)
  swProtocolIPv6NoNextHeaderPayload,    // IPPROTO_NONE (59)
  swProtocolIPv6Option,
  swProtocolIPv6Max
} swProtocolIPv6Identifier;

typedef enum swProtocolICMPv6Identifier
{
  swProtocolICMPv6None,
  swProtocolICMPv6ErrorMessage,
  swProtocolICMPv6InfoMessage,
  swProtocolICMPv6UnknownErrorMessage,
  swProtocolICMPv6UnknownInfoMessage,
  swProtocolICMPv6Option,
  swProtocolICMPv6Max
} swProtocolICMPv6Identifier;

typedef struct swProtocolFrame
{
  uint32_t        protocolId;
  uint32_t        frameId;
  swStaticBuffer  frameData;
} swProtocolFrame;

void swProtocolFrameArrayPrint(const swStaticArray *frames);

#endif  // SW_PROTOCOL_PROTOCOLCOMMON_H
