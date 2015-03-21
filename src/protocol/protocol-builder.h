#ifndef SW_PROTOCOL_PROTOCOLBUILDER_H
#define SW_PROTOCOL_PROTOCOLBUILDER_H

#include "collections/dynamic-array.h"
#include "protocol/protocol-common.h"

typedef struct swProtocolBuilder
{
  swDynamicArray items;
  swStaticBuffer buffer;
  size_t buildPosition;
} swProtocolBuilder;

bool swProtocolBuilderInit(swProtocolBuilder *builder, swStaticBuffer *buffer);
void swProtocolBuilderRelease(swProtocolBuilder *builder);

// the general approach is to set a frame in the state items stack and return static buffer pointer
// then cast the data of the static buffer to apropriate data structure and populate it
swStaticBuffer *swProtocolBuilderAddFrame(swProtocolBuilder *builder, uint32_t protocolId, uint32_t frameId, size_t size);

static inline void swProtocolBuilderPrint (swProtocolBuilder *builder)
{
  if (builder)
    swProtocolFrameArrayPrint((swStaticArray *)&(builder->items));
}

static inline void swProtocolBuilderReset(swProtocolBuilder *builder)
{
  if (builder)
  {
    builder->buildPosition = 0;
    swDynamicArrayClear(&(builder->items));
  }
}

#endif  // SW_PROTOCOL_PROTOCOLBUILDER_H
