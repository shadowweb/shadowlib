#include "command-line/option-category.h"
#include "io/tcp-client.h"
#include "tools/traffic-generator/traffic-server.h"
#include "tools/traffic-generator/traffic-connection.h"

#include <limits.h>
#include <stdlib.h>

static swStaticArray ipAddresses          = swStaticArrayDefineEmpty;
static swStaticArray ports                = swStaticArrayDefineEmpty;
static swStaticArray sendIntervals        = swStaticArrayDefineEmpty;

static int64_t readTimeout       = 0;
static int64_t writeTimeout      = 0;

swOptionCategoryModuleDeclare(swTrafficServerOptions, "Traffic Generator Server Options",

  swOptionDeclareArray("listen-ip",             "List of IP addresses that server should listen on",
    "IP",   &ipAddresses,         0, swOptionValueTypeString,  swOptionArrayTypeSimple, false),
  swOptionDeclareArray("listen-port",           "List of ports that server should listen on (one for each IP)",
    "port", &ports,               0, swOptionValueTypeInt,     swOptionArrayTypeSimple, false),
  swOptionDeclareArray("server-send-interval",  "Send interval in milli seconds for each port",
    NULL,   &sendIntervals,       0, swOptionValueTypeInt,     swOptionArrayTypeSimple, false),

  swOptionDeclareScalar("server-read-timeout",  "Server connection read timeout",
    NULL, &readTimeout,         swOptionValueTypeInt, false),
  swOptionDeclareScalar("server-write-timeout", "Server connection write timeout",
    NULL, &writeTimeout,        swOptionValueTypeInt, false)
);

static swDynamicArray *serverAcceptorsData = NULL;
static uint32_t minMessageSize = 0;
static uint32_t maxMessageSize = 0;

/*
static void swTrafficServerStoragePrint(swDynamicArray *serversStorage)
{
  printf ("serverStorage =\n{\n\t%p(size = %u, count = %u, data = %p)\n",
          (void *)serversStorage, serversStorage->size, serversStorage->count, serversStorage->data);
  swTrafficAcceptorData *acceptorData = (swTrafficAcceptorData *)(serversStorage->data);
  for (uint32_t i = 0; i < serversStorage->count; i++)
  {
    printf("\t\t%u: acceptor = %p, serverConnections = (s: %u, c: %u, d: %p), sendInterval = %lu, portPosition = %u, firstFree = %u\n",
           i, (void *)(acceptorData[i].acceptor), acceptorData[i].serverConnections.size, acceptorData[i].serverConnections.count, (void *)(acceptorData[i].serverConnections.data),
           acceptorData[i].sendInterval, acceptorData[i].portPosition, acceptorData[i].firstFree);
    swTrafficConnectionData **connections = (swTrafficConnectionData **)(acceptorData[i].serverConnections.data);
    for (uint32_t j = 0; j < acceptorData[i].serverConnections.count; j++)
    {
      printf ("\t\t\t%u: connectionData = %p, connection = %p\n", j, (void *)(connections[i]), (void *)(connections[i]->connection));
    }
  }
  printf ("}\n");
}
*/

static void swTrafficServerStorageDelete(swDynamicArray *serversStorage)
{
  if (serversStorage)
  {
    swTrafficAcceptorData *acceptorData = (swTrafficAcceptorData *)(serversStorage->data);
    swTrafficAcceptorData *acceptorDataLast = acceptorData + serversStorage->size;
    while (acceptorData < acceptorDataLast)
    {
      swDynamicArrayRelease(&(acceptorData->serverConnections));
      acceptorData++;
    }
    swDynamicArrayDelete(serversStorage);
  }
}

static swDynamicArray *swTrafficServerStorageNew(uint32_t portCount)
{
  swDynamicArray *rtn = NULL;
  if (portCount)
  {
    swDynamicArray *acceptorsStorage = swDynamicArrayNew(sizeof(swTrafficAcceptorData), portCount);
    if (acceptorsStorage)
    {
      swTrafficAcceptorData *acceptorData = (swTrafficAcceptorData *)acceptorsStorage->data;
      swTrafficAcceptorData *acceptorDataLast = acceptorData + portCount;
      while (acceptorData < acceptorDataLast)
      {
        if (swDynamicArrayInit(&(acceptorData->serverConnections), sizeof(swTrafficConnectionData *), 2))
          acceptorData++;
        else
          break;
      }
      if (acceptorData == acceptorDataLast)
      {
        acceptorsStorage->count = portCount;
        rtn = acceptorsStorage;
      }
      else
        swTrafficServerStorageDelete(acceptorsStorage);
    }
  }
  return rtn;
}

static swTrafficAcceptorData *swTrafficServerStorageGet(swDynamicArray *connStorage, uint32_t portPosition)
{
  swTrafficAcceptorData *rtn = NULL;
  if (connStorage && (portPosition < connStorage->count))
    rtn = (swTrafficAcceptorData *)(connStorage->data) + portPosition;
  return rtn;
}

static bool swTrafficServerValidate()
{
  bool rtn = false;
  if (ipAddresses.count == ports.count && ports.count == sendIntervals.count &&
      ipAddresses.count <= UINT_MAX &&
      readTimeout >= 0 && writeTimeout >= 0)
  {
    uint32_t i = 0;
    swStaticString *verifyIpAddresses = (swStaticString *)ipAddresses.data;
    int64_t *verifyPorts = (int64_t *)ports.data;
    int64_t *verifySendIntervals = (int64_t *)sendIntervals.data;

    while (i < ipAddresses.count)
    {
      if (verifyIpAddresses[i].len && verifyPorts[i] > 0 && verifyPorts[i] <= USHRT_MAX &&
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

static bool onAccept(swTCPServerAcceptor *serverAcceptor)
{
  printf("'%s': accepting connection\n", __func__);
  return true;
}

static void onStop(swTCPServerAcceptor *serverAcceptor)
{
  printf("'%s': stopping\n", __func__);
}

static void onError(swTCPServerAcceptor *serverAcceptor, swSocketIOErrorType errorCode)
{
  printf("'%s': error \"%s\"\n", __func__, swSocketIOErrorTextGet(errorCode));
}

static void onServerReadReady(swTCPServer *server)
{
  // printf ("'%s': read ready entered\n", __func__);
  swSocketReturnType ret = swSocketReturnNone;
  swTrafficConnectionData *serverData = (swTrafficConnectionData *)swTCPServerDataGet(server);
  swStaticBuffer buffer = swStaticBufferDefineWithLength(serverData->receiveBuffer.data, serverData->receiveBuffer.size);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPServerRead(server, &buffer, &bytesRead);
    if (ret != swSocketReturnOK)
      break;
    serverData->bytesReceived += bytesRead;
  }
}

static void onServerWriteReady(swTCPServer *server)
{
  // printf ("'%s': write ready entered\n", __func__);
  swTrafficConnectionData *serverData = (swTrafficConnectionData *)swTCPServerDataGet(server);
  if (serverData->retrySend)
  {
    swSocketReturnType ret = swSocketReturnNone;
    swStaticBuffer buffer = swStaticBufferDefineWithLength(serverData->sendBuffer.data, minMessageSize + rand()%(maxMessageSize - minMessageSize + 1));
    ssize_t bytesWritten = 0;
    ret = swTCPServerWrite(server, &buffer, &bytesWritten);
    if (ret == swSocketReturnOK)
    {
      serverData->bytesSent += bytesWritten;
      serverData->retrySend = false;
    }
  }
}

static bool onServerReadTimeout(swTCPServer *server)
{
  printf ("'%s': read timeout\n", __func__);
  return false;
}

static bool onServerWriteTimeout(swTCPServer *server)
{
  printf ("'%s': write timeout\n", __func__);
  return false;
}

static void onServerError(swTCPServer *server, swSocketIOErrorType errorCode)
{
  printf("'%s': error \"%s\"\n", __func__, swSocketIOErrorTextGet(errorCode));
}

static void onServerClose(swTCPServer *server)
{
  printf ("'%s': Server close\n", __func__);
  swTrafficConnectionData *serverData = swTCPServerDataGet(server);
  if (serverData)
  {
    printf ("'%s': bytesSent = %lu, bytesReceived = %lu\n", __func__, serverData->bytesSent, serverData->bytesReceived);
    swEdgeTimerStop(&(serverData->sendTimer));
    serverData->connection = NULL;
    swTrafficAcceptorData *acceptorData = swTrafficServerStorageGet(serverAcceptorsData, serverData->portPosition);
    if (acceptorData)
    {
      if ((acceptorData->firstFree > serverData->arrayPosition))
        acceptorData->firstFree = serverData->arrayPosition;
    }
  }
  swTCPServerDelete(server);
}

static void onSendTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  // printf ("'%s': entered\n", __func__);
  if (timer)
  {
    swTrafficConnectionData *serverData = swEdgeWatcherDataGet(timer);
    swTCPServer *server = (swTCPServer *)(serverData->connection);
    ssize_t bytesWritten = 0;
    swStaticBuffer buffer = swStaticBufferDefineWithLength(serverData->sendBuffer.data, minMessageSize + rand()%(maxMessageSize - minMessageSize + 1));
    swSocketReturnType ret = swSocketReturnNone;
    ret = swTCPServerWrite(server, &buffer, &bytesWritten);
    if (ret == swSocketReturnOK)
      serverData->bytesSent += bytesWritten;
    else if (ret == swSocketReturnNotReady)
      serverData->retrySend = true;
  }
}

static bool onConnectionSetup(swTCPServerAcceptor *serverAcceptor, swTCPServer *server)
{
  bool rtn = false;
  printf("'%s': new connection setup for %p\n", __func__, (void *)server);
  if (serverAcceptor)
  {
    swTrafficAcceptorData *acceptorData = swTCPServerAcceptorDataGet(serverAcceptor);
    if (acceptorData)
    {
      if (server)
      {
        swTCPServerReadTimeoutSet     (server, readTimeout);
        swTCPServerWriteTimeoutSet    (server, writeTimeout);
        swTCPServerReadReadyFuncSet   (server, onServerReadReady);
        swTCPServerWriteReadyFuncSet  (server, onServerWriteReady);
        swTCPServerReadTimeoutFuncSet (server, onServerReadTimeout);
        swTCPServerWriteTimeoutFuncSet(server, onServerWriteTimeout);
        swTCPServerErrorFuncSet       (server, onServerError);
        swTCPServerCloseFuncSet       (server, onServerClose);

        swTrafficConnectionData **connections = (swTrafficConnectionData **)(acceptorData->serverConnections.data);
        swTrafficConnectionData *serverData = NULL;
        if (acceptorData->firstFree < acceptorData->serverConnections.count)
        {
          // printf ("have free serverData spot %u\n", acceptorData->firstFree);
          serverData = connections[acceptorData->firstFree];
          serverData->connection = (swSocketIO *)server;
          acceptorData->firstFree++;
          while ((acceptorData->firstFree < acceptorData->serverConnections.count) && !(connections[acceptorData->firstFree]->connection))
            acceptorData->firstFree++;
        }
        else
        {
          // printf ("trying to create serverData\n");
          if ((serverData = swTrafficConnectionDataNew((swSocketIO *)server, onSendTimerCallback, acceptorData->sendInterval, maxMessageSize)))
          {
            if (swDynamicArrayPush(&(acceptorData->serverConnections), &serverData))
            {
              serverData->portPosition = acceptorData->portPosition;
              serverData->arrayPosition = acceptorData->firstFree;
              acceptorData->firstFree++;
            }
            else
            {
              swTrafficConnectionDataDelete(serverData);
              serverData = NULL;
            }
          }
        }
        if (serverData)
        {
          if (swEdgeTimerStart(&(serverData->sendTimer), serverAcceptor->loop, serverData->sendInterval, serverData->sendInterval, false))
          {
            serverData->retrySend = true;
            swTCPServerDataSet(server, serverData);
            rtn = true;
          }
          else
            serverData->connection = NULL;
        }
      }
    }
  }
  return rtn;
}

swTCPServerAcceptor *swTrafficServerNew(swEdgeLoop *loop, swTrafficAcceptorData *acceptorData, swStaticString *ip, uint16_t port, uint64_t sendInterval)
{
  swTCPServerAcceptor *rtn = NULL;
  if (loop && acceptorData && ip && port)
  {
    swSocketAddress address = { 0 };
    if (swSocketAddressInitInet(&address, ip->data, port))
    {
      swTCPServerAcceptor *serverAcceptor = swTCPServerAcceptorNew();
      if (serverAcceptor)
      {
        // set callbacks
        swTCPServerAcceptorAcceptFuncSet  (serverAcceptor, onAccept);
        swTCPServerAcceptorStopFuncSet    (serverAcceptor, onStop);
        swTCPServerAcceptorErrorFuncSet   (serverAcceptor, onError);
        swTCPServerAcceptorSetupFuncSet   (serverAcceptor, onConnectionSetup);

        if (swTCPServerAcceptorStart(serverAcceptor, loop, &address))
        {
          swTCPServerAcceptorDataSet(serverAcceptor, acceptorData);
          acceptorData->sendInterval = sendInterval;
          acceptorData->acceptor = serverAcceptor;
          acceptorData->firstFree = 0;
          rtn = serverAcceptor;
        }
        else
          swTCPServerAcceptorDelete(serverAcceptor);
      }
    }
  }
  return rtn;
}

static void swTrafficServerDestroy(swTCPServer *server)
{
  if (server)
    swTCPServerStop(server);
}

static void *trafficServerArrayData[3] = {NULL};

static void swTrafficServerStop()
{
  if (serverAcceptorsData)
  {
    uint32_t portCount = serverAcceptorsData->count;
    swTrafficAcceptorData *acceptorData = (swTrafficAcceptorData *)serverAcceptorsData->data;
    for (uint32_t i = 0; i < portCount; i++, acceptorData++)
    {
      if (acceptorData->acceptor)
      {
        swTrafficConnectionData **connData = (swTrafficConnectionData **)(acceptorData->serverConnections.data);
        uint32_t connCount = acceptorData->serverConnections.count;
        for (uint32_t j = 0; j < connCount; j++)
        {
          swTCPServer *server = (swTCPServer *)(connData[j]->connection);
          swTrafficServerDestroy(server);
          swTrafficConnectionDataDelete(connData[j]);
        }

        swTCPServerAcceptorStop(acceptorData->acceptor);
        swTCPServerAcceptorDelete(acceptorData->acceptor);
      }
    }
    swTrafficServerStorageDelete(serverAcceptorsData);
    serverAcceptorsData = NULL;
  }
}

static bool swTrafficServerStart()
{
  bool rtn = false;
  swEdgeLoop **loopPtr = (swEdgeLoop **)trafficServerArrayData[0];
  if (loopPtr && *loopPtr && trafficServerArrayData[1] && trafficServerArrayData[2])
  {
    minMessageSize = *((uint32_t *)(trafficServerArrayData[1]));
    maxMessageSize = *((uint32_t *)(trafficServerArrayData[2]));
    if (swTrafficServerValidate())
    {
      if (ipAddresses.count)
      {
        if ((serverAcceptorsData = swTrafficServerStorageNew(ipAddresses.count)))
        {
          swStaticString *ipAddress = (swStaticString *)ipAddresses.data;
          int64_t *port = (int64_t *)ports.data;
          int64_t *sendInterval = (int64_t *)sendIntervals.data;
          swTrafficAcceptorData *acceptorData = (swTrafficAcceptorData *)(serverAcceptorsData->data);

          uint32_t i = 0;
          while (i < ipAddresses.count)
          {
            if ((acceptorData->acceptor = swTrafficServerNew(*loopPtr, acceptorData, &(ipAddress[i]), port[i], sendInterval[i])))
            {
              acceptorData->portPosition = i;
              i++;
              acceptorData++;
            }
            else
              break;
          }
          if (i == ipAddresses.count)
            rtn = true;
          else
            swTrafficServerStop();
        }
      }
      else
        rtn = true;
    }
  }
  return rtn;
}

static swInitData trafficServerData = {.startFunc = swTrafficServerStart, .stopFunc = swTrafficServerStop, .name = "Traffic Servers"};

swInitData *swTrafficServerDataGet(swEdgeLoop **loopPtr, int64_t *minMessageSize, int64_t *maxMessageSize)
{
  trafficServerArrayData[0] = loopPtr;
  trafficServerArrayData[1] = minMessageSize;
  trafficServerArrayData[2] = maxMessageSize;
  return &trafficServerData;
}
