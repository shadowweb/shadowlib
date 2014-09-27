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
  if (connData && connection && timerCB && /* sendInterval && */ bufferSize)
  {
    memset(connData, 0, sizeof(*connData));
    if (swDynamicBufferInit(&(connData->sendBuffer), bufferSize) && swDynamicBufferInit(&(connData->receiveBuffer), SW_TRAFFIC_RECEIVE_BUFFER_SIZE))
    {
      memset(connData->sendBuffer.data, 0, bufferSize);
      memset(connData->receiveBuffer.data, 0, SW_TRAFFIC_RECEIVE_BUFFER_SIZE);
      if (!sendInterval || swEdgeTimerInit(&(connData->sendTimer), timerCB, true))
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
    if (connData->sendInterval)
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

void swTrafficConnectionDataSend(swTrafficConnectionData *connData, swSocketIO *connection, uint32_t minMessageSize)
{
  if (connData && connection && minMessageSize)
  {
    swSocketReturnType ret = swSocketReturnNone;
    ssize_t bytesWritten = 0;
    uint32_t iterationCount = (connData->sendInterval)? 1 : 100;
    // printf ("'%s': trying to send data %u time(s)\n", __func__, iterationCount);
    uint32_t maxMessageSize = connData->sendBuffer.size;
    swStaticBuffer buffer = *(swStaticBuffer *)(&(connData->sendBuffer));
    for (uint32_t i = 0; i < iterationCount; i++)
    {
      buffer.len = minMessageSize + rand()%(maxMessageSize - minMessageSize + 1);
      if ((ret = swSocketIOWrite(connection, &buffer, &bytesWritten)) == swSocketReturnOK)
      {
        connData->bytesSent += bytesWritten;
        connData->retrySend = false;
      }
      else
      {
        if (ret == swSocketReturnNotReady)
          connData->retrySend = true;
        break;
      }
    }
  }
}

