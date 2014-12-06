#ifndef SW_TOOLS_SPLICER_SPLICERSTATE_H
#define SW_TOOLS_SPLICER_SPLICERSTATE_H

#include "io/tcp-client.h"
#include "io/tcp-server.h"
#include "storage/dynamic-buffer.h"

typedef struct swSplicerState
{
  swTCPServerAcceptor *acceptor;
  swTCPServer *server;
  swTCPClient *client;
  swDynamicBuffer buffer;
  int pipeToServer[2];
  int pipeToClient[2];
  uint64_t serverBytesSent;
  uint64_t serverBytesReceived;
  uint64_t clientBytesSent;
  uint64_t clientBytesReceived;
  unsigned clientBufferWritten : 1;
  unsigned serverBufferWritten : 1;
} swSplicerState;

swSplicerState *swSplicerStateNew(swEdgeLoop *loop, swSocketAddress *address, size_t bufferSize);
void swSplicerStateDelete(swSplicerState *state);

#endif  // SW_TOOLS_SPLICER_SPLICERSTATE_H
