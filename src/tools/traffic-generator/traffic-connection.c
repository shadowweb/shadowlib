#include "tools/traffic-generator/traffic-connection.h"

#include "core/memory.h"

#define SW_TRAFFIC_RECEIVE_BUFFER_SIZE    (4 * 1024 * 1024)

swTrafficConnectionData *swTrafficConnectionDataNew(swSocketIO *connection, swEdgeTimerCallback timerCB, uint64_t sendInterval, uint32_t bufferSize)
{
  swTrafficConnectionData *rtn = swMemoryMalloc(sizeof(*rtn));
  if (rtn)
  {
    if (!swTrafficConnectionDataInit(rtn, connection, timerCB, sendInterval, bufferSize))
    {
      swMemoryFree(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swTrafficConnectionDataInit(swTrafficConnectionData *connData, swSocketIO *connection, swEdgeTimerCallback timerCB, uint64_t sendInterval, uint32_t bufferSize)
{
  bool rtn = false;
  if (connData && connection && timerCB && sendInterval && bufferSize)
  {
    memset(connData, 0, sizeof(*connData));
    if (swDynamicBufferInit(&(connData->sendBuffer), bufferSize) && swDynamicBufferInit(&(connData->receiveBuffer), SW_TRAFFIC_RECEIVE_BUFFER_SIZE))
    {
      memset(connData->sendBuffer.data, 0, bufferSize);
      memset(connData->receiveBuffer.data, 0, SW_TRAFFIC_RECEIVE_BUFFER_SIZE);
      if (swEdgeTimerInit(&(connData->sendTimer), timerCB, true))
      {
        swEdgeWatcherDataSet(&(connData->sendTimer), connData);
        connData->sendInterval = sendInterval;
        connData->connection = connection;
        rtn = true;
      }
    }
    if (!rtn)
      swTrafficConnectionDataRelease(connData);
  }
  return rtn;
}

void swTrafficConnectionDataRelease(swTrafficConnectionData *connData)
{
  if (connData)
  {
    if (connData->sendBuffer.size)
      swDynamicBufferRelease(&(connData->sendBuffer));
    if (connData->receiveBuffer.size)
      swDynamicBufferRelease(&(connData->receiveBuffer));
    swEdgeTimerClose(&(connData->sendTimer));
  }
}

void swTrafficConnectionDataDelete(swTrafficConnectionData *connData)
{
  if (connData)
  {
    swTrafficConnectionDataRelease(connData);
    swMemoryFree(connData);
  }
}
