#include "command-line/option-category.h"
#include "io/tcp-client.h"
#include "tools/traffic-generator/traffic-client.h"
#include "tools/traffic-generator/traffic-connection.h"

#include <limits.h>
#include <stdlib.h>

static swStaticArray ipAddresses          = swStaticArrayDefineEmpty;
static swStaticArray ports                = swStaticArrayDefineEmpty;
static swStaticArray connectionsPerPorts  = swStaticArrayDefineEmpty;
static swStaticArray sendIntervals        = swStaticArrayDefineEmpty;

static int64_t readTimeout       = 0;
static int64_t writeTimeout      = 0;
static int64_t connectTimeout    = 0;
static int64_t reconnectInterval = 0;

swOptionCategoryModuleDeclare(swTrafficClientOptions, "Traffic Generator Client Options",
  swOptionDeclareArray("connect-ip",            "List of IP addresses that client should connect to",
    "IP",   &ipAddresses,         0, swOptionValueTypeString,  swOptionArrayTypeSimple, false),
  swOptionDeclareArray("connect-port",          "List of ports that client should connect to for each IP",
    "PORT", &ports,               0, swOptionValueTypeInt,     swOptionArrayTypeSimple, false),
  swOptionDeclareArray("connections-per-port",  "Number of client connections for each port",
    NULL,   &connectionsPerPorts, 0, swOptionValueTypeInt,     swOptionArrayTypeSimple, false),
  swOptionDeclareArray("client-send-interval",  "Send interval in milli seconds for each port",
    NULL,   &sendIntervals,       0, swOptionValueTypeInt,     swOptionArrayTypeSimple, false),

  swOptionDeclareScalar("client-read-timeout",        "Client connection read timeout",
    NULL,   &readTimeout,         swOptionValueTypeInt, false),
  swOptionDeclareScalar("client-write-timeout",       "Client connection write timeout",
    NULL,   &writeTimeout,        swOptionValueTypeInt, false),
  swOptionDeclareScalar("client-connect-timeout",     "Client connection connect timeout",
    NULL,   &connectTimeout,      swOptionValueTypeInt, false),
  swOptionDeclareScalar("client-reconnect-interval",  "Client connection write timeout",
    NULL,   &reconnectInterval,   swOptionValueTypeInt, false)
);

static swDynamicArray *clientConnectionsData = NULL;
static uint32_t minMessageSize = 0;
static uint32_t maxMessageSize = 0;

static bool swTrafficClientValidate()
{
  bool rtn = false;
  if (ipAddresses.count == ports.count && ports.count == connectionsPerPorts.count && connectionsPerPorts.count == sendIntervals.count &&
      ipAddresses.count <= UINT_MAX &&
      readTimeout >= 0 && writeTimeout >= 0 && connectTimeout >= 0 && reconnectInterval >= 0)
  {
    uint32_t i = 0;
    swStaticString *verifyIpAddresses = (swStaticString *)ipAddresses.data;
    int64_t *verifyPorts = (int64_t *)ports.data;
    int64_t *verifyConnectionsPerPort = (int64_t *)connectionsPerPorts.data;
    int64_t *verifySendIntervals = (int64_t *)sendIntervals.data;

    while (i < ipAddresses.count)
    {
      if (verifyIpAddresses[i].len && verifyPorts[i] > 0 && verifyPorts[i] <= USHRT_MAX &&
          verifyConnectionsPerPort[i] > 0 && verifyConnectionsPerPort[i] <= UINT_MAX &&
          verifySendIntervals[i] > 0)
        i++;
      else
        break;
    }
    if (i == ipAddresses.count)
      rtn = true;
  }
  return rtn;
}

static void onClientConnected(swTCPClient *client)
{
  printf("'%s': connected\n", __func__);
  swTrafficConnectionData *clientData = (swTrafficConnectionData *)swTCPClientDataGet(client);
  if (swEdgeTimerStart(&(clientData->sendTimer), client->loop, clientData->sendInterval, clientData->sendInterval, false))
    clientData->retrySend = true;
  else
    swTCPClientClose(client);
}

static void swTrafficClientStorageDelete(swDynamicArray *connStorage)
{
  if (connStorage)
  {
    swDynamicArray *connStorageData = (swDynamicArray *)(connStorage->data);
    swDynamicArray *connStorageDataLast = connStorageData + connStorage->size;
    while (connStorageData < connStorageDataLast)
    {
      // TODO: per array element cleanup might be needed here
      swDynamicArrayRelease(connStorageData);
      connStorageData++;
    }
    swDynamicArrayDelete(connStorage);
  }
}

static swDynamicArray *swTrafficClietStorageNew(uint32_t portCount, int64_t *connPerPort)
{
  swDynamicArray *rtn = NULL;
  if (portCount && connPerPort)
  {
    swDynamicArray *connStorage = swDynamicArrayNew(sizeof(swDynamicArray), portCount);
    if (connStorage)
    {
      swDynamicArray *connStorageData = (swDynamicArray *)(connStorage->data);
      swDynamicArray *connStorageDataLast = connStorageData + portCount;
      int64_t *connPerPortPtr = connPerPort;
      while (connStorageData < connStorageDataLast)
      {
        if (swDynamicArrayInit(connStorageData, sizeof(swTrafficConnectionData), (uint32_t)(*connPerPortPtr++)))
          connStorageData++;
        else
          break;
      }
      if (connStorageData == connStorageDataLast)
      {
        connStorage->count = portCount;
        rtn = connStorage;
      }
      else
        swTrafficClientStorageDelete(connStorage);
    }
  }
  return rtn;
}

static swDynamicArray *swTrafficClientStorageGet(swDynamicArray *connStorage, uint32_t portPosition)
{
  swDynamicArray *rtn = NULL;
  if (connStorage && (portPosition < connStorage->count))
    rtn = (swDynamicArray *)(connStorage->data) + portPosition;
  return rtn;
}

static void onClientClose(swTCPClient *client)
{
  printf("'%s': close\n", __func__);
  swTrafficConnectionData *clientData = (swTrafficConnectionData *)swTCPClientDataGet(client);
  if (clientData)
  {
    swEdgeTimerStop(&(clientData->sendTimer));
    printf ("'%s': bytesSent = %lu, bytesReceived = %lu\n", __func__, clientData->bytesSent, clientData->bytesReceived);
  }
}

static void onClientStop(swTCPClient *client)
{
  printf("'%s': stop\n", __func__);
}

static void onClientReadReady(swTCPClient *client)
{
  // printf ("'%s': read ready entered\n", __func__);
  swSocketReturnType ret = swSocketReturnNone;
  swTrafficConnectionData *clientData = (swTrafficConnectionData *)swTCPClientDataGet(client);
  swStaticBuffer buffer = swStaticBufferDefineWithLength(clientData->receiveBuffer.data, clientData->receiveBuffer.size);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPClientRead(client, &buffer, &bytesRead);
    if (ret != swSocketReturnOK)
      break;
    clientData->bytesReceived += bytesRead;
  }
}

static void onClientWriteReady(swTCPClient *client)
{
  // printf ("'%s': write ready entered\n", __func__);
  swTrafficConnectionData *clientData = (swTrafficConnectionData *)swTCPClientDataGet(client);
  if (clientData->retrySend)
  {
    swSocketReturnType ret = swSocketReturnNone;
    swStaticBuffer buffer = swStaticBufferDefineWithLength(clientData->sendBuffer.data, minMessageSize + rand()%(maxMessageSize - minMessageSize + 1));
    ssize_t bytesWritten = 0;
    ret = swTCPClientWrite(client, &buffer, &bytesWritten);
    if (ret == swSocketReturnOK)
    {
      clientData->bytesSent += bytesWritten;
      clientData->retrySend = false;
    }
  }
}

static bool onClientReadTimeout(swTCPClient *client)
{
  printf ("'%s': read timeout\n", __func__);
  return false;
}

static bool onClientWriteTimeout(swTCPClient *client)
{
  printf ("'%s': write timeout\n", __func__);
  return false;
}

static void onClientError(swTCPClient *client, swSocketIOErrorType errorCode)
{
  printf("'%s': error \"%s\"\n", __func__, swSocketIOErrorTextGet(errorCode));
}

static void onSendTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  // printf ("'%s': sender callback entered\n", __func__);
  if (timer)
  {
    swTrafficConnectionData *clientData = swEdgeWatcherDataGet(timer);
    swTCPClient *client = (swTCPClient *)(clientData->connection);
    swStaticBuffer buffer = swStaticBufferDefineWithLength(clientData->sendBuffer.data, minMessageSize + rand()%(maxMessageSize - minMessageSize + 1));
    ssize_t bytesWritten = 0;
    swSocketReturnType ret = swSocketReturnNone;
    ret = swTCPClientWrite(client, &buffer, &bytesWritten);
    if (ret == swSocketReturnOK)
      clientData->bytesSent += bytesWritten;
    else if (ret == swSocketReturnNotReady)
      clientData->retrySend = true;
  }
}

static bool swTrafficClientCreate(swEdgeLoop *loop, swTrafficConnectionData *connectionData, swStaticString *ip, uint16_t port, uint64_t sendInterval)
{
  bool rtn = false;
  if (loop && connectionData && ip && port)
  {
    swSocketAddress address = { 0 };
    if (swSocketAddressInitInet(&address, ip->data, port))
    {
      swTCPClient *client = swTCPClientNew();
      if (client)
      {
        // set callbacks and timeouts
        swTCPClientConnectTimeoutSet(client, connectTimeout);
        swTCPClientReconnectTimeoutSet(client, reconnectInterval);
        swTCPClientConnectedFuncSet(client, onClientConnected);
        swTCPClientCloseFuncSet(client, onClientClose);
        swTCPClientStopFuncSet(client, onClientStop);

        swTCPClientReadTimeoutSet(client, readTimeout);
        swTCPClientWriteTimeoutSet(client, writeTimeout);
        swTCPClientReadReadyFuncSet(client, onClientReadReady);
        swTCPClientWriteReadyFuncSet(client, onClientWriteReady);
        swTCPClientReadTimeoutFuncSet(client, onClientReadTimeout);
        swTCPClientWriteTimeoutFuncSet(client, onClientWriteTimeout);
        swTCPClientErrorFuncSet(client, onClientError);

        if (swTCPClientStart(client, &address, loop, NULL))
        {
          if (swTrafficConnectionDataInit(connectionData, (swSocketIO *)client, onSendTimerCallback, sendInterval, maxMessageSize))
          {
            swTCPClientDataSet(client, connectionData);
            rtn = true;
          }
        }
        else
          swTCPClientDelete(client);
      }
    }
  }
  return rtn;
}

static void swTrafficClientDestroy(swTCPClient *client)
{
  // printf ("'%s': entered\n", __func__);
  if (client)
  {
    swTCPClientStop(client);
    swTCPClientDelete(client);
  }
}

static void *trafficClientArrayData[3] = {NULL};

static void swTrafficClientStop()
{
  // printf ("'%s': entered\n", __func__);
  if (clientConnectionsData)
  {
    uint32_t portCount = clientConnectionsData->count;
    for (uint32_t i = 0; i < portCount; i++)
    {
      swDynamicArray *connDataArray = swTrafficClientStorageGet(clientConnectionsData, i);
      if (connDataArray)
      {
        swTrafficConnectionData *connData = (swTrafficConnectionData *)connDataArray->data;
        for (uint32_t j = 0; j < connDataArray->count; j++)
        {
          swTCPClient *client = (swTCPClient *)(connData[j].connection);
          swTrafficClientDestroy(client);
          swTrafficConnectionDataRelease(&(connData[j]));
        }
      }
    }
    swTrafficClientStorageDelete(clientConnectionsData);
    clientConnectionsData = NULL;
  }
}

static bool swTrafficClientStart()
{
  bool rtn = false;
  swEdgeLoop **loopPtr = (swEdgeLoop **)trafficClientArrayData[0];
  if (loopPtr && *loopPtr && trafficClientArrayData[1] && trafficClientArrayData[2])
  {
    minMessageSize = *((uint32_t *)(trafficClientArrayData[1]));
    maxMessageSize = *((uint32_t *)(trafficClientArrayData[2]));
    if (swTrafficClientValidate())
    {
      if (ipAddresses.count)
      {
        int64_t *connectionsPerPort = (int64_t *)connectionsPerPorts.data;
        if ((clientConnectionsData = swTrafficClietStorageNew(ipAddresses.count, connectionsPerPort)))
        {
          swStaticString *ipAddress = (swStaticString *)ipAddresses.data;
          int64_t *port = (int64_t *)ports.data;
          int64_t *sendInterval = (int64_t *)sendIntervals.data;

          uint32_t i = 0;
          while (i < ipAddresses.count)
          {
            swDynamicArray *storageDataArray = swTrafficClientStorageGet(clientConnectionsData, i);
            swTrafficConnectionData *connectionDataPtr = (swTrafficConnectionData *)(storageDataArray->data);
            int64_t j = 0;
            while (j < connectionsPerPort[i])
            {
              if (swTrafficClientCreate(*loopPtr, connectionDataPtr++, &(ipAddress[i]), port[i], sendInterval[i]))
                j++;
              else
                break;
            }
            if (j == connectionsPerPort[i])
            {
              storageDataArray->count = j;
              i++;
            }
            else
              break;
          }
          if (i == ipAddresses.count)
            rtn = true;
          else
            swTrafficClientStop();
        }
      }
      else
        rtn = true;
    }
  }
  return rtn;
}

static swInitData trafficClientData = {.startFunc = swTrafficClientStart, .stopFunc = swTrafficClientStop, .name = "Traffic Clients"};

swInitData *swTrafficClientDataGet(swEdgeLoop **loopPtr, int64_t *minMessageSize, int64_t *maxMessageSize)
{
  trafficClientArrayData[0] = loopPtr;
  trafficClientArrayData[1] = minMessageSize;
  trafficClientArrayData[2] = maxMessageSize;
  return &trafficClientData;
}
