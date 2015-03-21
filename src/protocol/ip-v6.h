#ifndef SW_PROTOCOL_IPV6_H
#define SW_PROTOCOL_IPV6_H

#include "io/socket-address.h"
#include "protocol/protocol-builder.h"
#include "storage/dynamic-string.h"
#include "storage/static-string.h"

bool swIPv6InitWellKnownIPs();
bool swIPv6FromString(swStaticString *ipString, swSocketAddress *ipAddress);
bool swIPv6ToString(swSocketAddress *ipAddress, swDynamicString *ipString);

swSocketAddress *swIPv6GetRouterMulticastAddress();

typedef struct swIPv6Flow
{
  unsigned int version      : 4;
  unsigned int trafficClass : 8;
  unsigned int flowId       : 20;
} swIPv6Flow;

bool swIPv6BuildMainHeader(swProtocolBuilder *builder, swSocketAddress *source, swSocketAddress *target, swIPv6Flow flow, uint16_t payloadLength, uint8_t nextProtocol, uint8_t hopLimit);

#endif  // SW_PROTOCOL_IPV6_H
