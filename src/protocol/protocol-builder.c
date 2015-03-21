#include "protocol/protocol-builder.h"

#include <string.h>

bool swProtocolBuilderInit(swProtocolBuilder *builder, swStaticBuffer *buffer)
{
  bool rtn = false;
  if (builder && buffer)
  {
    if (swDynamicArrayInit(&(builder->items), sizeof(swProtocolFrame), 16))
    {
      builder->buildPosition = 0;
      builder->buffer = *buffer;
      rtn = true;
    }
  }
  return rtn;
}

void swProtocolBuilderRelease(swProtocolBuilder *builder)
{
  if (builder)
  {
    swDynamicArrayRelease(&(builder->items));
    builder->buffer = swStaticBufferSetEmpty;
    memset(builder, 0, sizeof(*builder));
  }
}

swStaticBuffer *swProtocolBuilderAddFrame(swProtocolBuilder *builder, uint32_t protocolId, uint32_t frameId, size_t size)
{
  swStaticBuffer *rtn = NULL;
  if (builder && builder->buffer.len >= (builder->buildPosition + size))
  {
    swProtocolFrame *frame = (swProtocolFrame *)swDynamicArrayGetNext(&(builder->items));
    if (frame)
    {
      frame->protocolId = protocolId;
      frame->frameId = frameId;
      frame->frameData = swStaticBufferSetWithLength(&(builder->buffer.data[builder->buildPosition]), size);
      builder->buildPosition += size;
      rtn = &(frame->frameData);
    }
  }
  return rtn;
}
