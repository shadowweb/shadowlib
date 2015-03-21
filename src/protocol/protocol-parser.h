#ifndef SW_PROTOCOL_PROTOCOLPARSER_H
#define SW_PROTOCOL_PROTOCOLPARSER_H

#include <stdint.h>
#include <stddef.h>

#include "collections/dynamic-array.h"
#include "protocol/protocol-common.h"

typedef struct swProtocolParser
{
  swDynamicArray items;
  swStaticBuffer buffer;
  size_t parsePosition;
  size_t bytesLeft;
  unsigned int done : 1;
  unsigned int error : 1;
} swProtocolParser;

bool swProtocolParserInit     (swProtocolParser *parser);
void swProtocolParserRelease  (swProtocolParser *parser);
bool swProtocolParserParse    (swProtocolParser *parser, swStaticBuffer *buffer);

static inline void swProtocolParserPrint (swProtocolParser *parser)
{
  if (parser)
    swProtocolFrameArrayPrint((swStaticArray *)&(parser->items));
}

#endif  // SW_PROTOCOL_PROTOCOLPARSER_H
