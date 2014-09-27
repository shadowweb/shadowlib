#ifndef SW_TOOLS_TRAFFICGENERATOR_TRAFFICCONNECTION_H
#define SW_TOOLS_TRAFFICGENERATOR_TRAFFICCONNECTION_H

#include "collections/dynamic-array.h"
#include "io/edge-timer.h"
#include "io/socket-io.h"
#include "storage/dynamic-buffer.h"

typedef struct swTrafficConnectionData
{
  swSocketIO *connection;
  swEdgeTimer sendTimer;
  swDynamicBuffer sendBuffer;
  swDynamicBuffer receiveBuffer;
  uint64_t sendInterval;
  uint64_t bytesSent;
  uint64_t bytesReceived;
  uint32_t connectedCount;
  uint32_t disconnectedCount;
  uint32_t portPosition;
  uint32_t arrayPosition;
  unsigned int retrySend : 1;
} swTrafficConnectionData;

swTrafficConnectionData *swTrafficConnectionDataNew(swSocketIO *connection, swEdgeTimerCallback timerCB, uint64_t sendInterval, uint32_t bufferSize);
bool swTrafficConnectionDataInit(swTrafficConnectionData *connData, swSocketIO *connection, swEdgeTimerCallback timerCB, uint64_t sendInterval, uint32_t bufferSize);
void swTrafficConnectionDataDelete(swTrafficConnectionData *connData);
void swTrafficConnectionDataRelease(swTrafficConnectionData *connData);
void swTrafficConnectionDataSend(swTrafficConnectionData *connData, swSocketIO *connection, uint32_t minMessageSize);

#endif  // SW_TOOLS_TRAFFICGENERATOR_TRAFFICCONNECTION_H
