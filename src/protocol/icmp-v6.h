#ifndef SW_PROTOCOL_ICMPV6_H
#define SW_PROTOCOL_ICMPV6_H

#include "io/socket-address.h"
#include "protocol/ethernet.h"
#include "protocol/protocol-builder.h"

bool swICMPv6BuildRouterSolicit(swProtocolBuilder *builder, swSocketAddress *localIPv6, swEthernetAddress *localMAC, bool raw);

// TODO: add other message types

#endif  // SW_PROTOCOL_ICMPV6_H
