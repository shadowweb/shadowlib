#include "tcp-client.h"

#include <core/memory.h>

static void swTCPClientConnectTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swEdgeTimerStop(timer);
  swTCPClient *client = swEdgeWatcherDataGet(timer);
  if (client && (((swSocket*)client)->fd >= 0))
  {
    client->connecting = false;
    swEdgeIOStop(&(client->connectEvent));
    swTCPConnectionClose((swTCPConnection *)client, swTCPConnectionErrorConnectTimeout);
  }
}

static inline bool swTCPClientConnectProcess(swTCPClient *client, swSocketReturnType ret, swTCPConnectionErrorType *errorCode)
{
  bool rtn = false;
  if ( ret > swSocketReturnNone)
  {
    if (ret == swSocketReturnOK)
    {
      if (swTCPConnectionStart((swTCPConnection *)client, client->loop))
      {
        rtn = true;
        if (client->connectedFunc)
          client->connectedFunc(client);
      }
      else
        *errorCode = swTCPConnectionErrorOtherError;
    }
    else if (ret == swSocketReturnInProgress)
    {
      client->connecting = true;
      rtn = swEdgeIOStart(&(client->connectEvent), client->loop, ((swSocket *)client)->fd, swEdgeEventWrite) &&
        swEdgeTimerStart(&(client->connectTimer), client->loop, client->connectTimeout, 0, false);
      if (!rtn)
        *errorCode = swTCPConnectionErrorOtherError;
    }
    else
    {
      rtn = true;
      swTCPConnectionClose((swTCPConnection *)client, swTCPConnectionErrorConnectFailed);
    }
  }
  else
    *errorCode = swTCPConnectionErrorConnectFailed;
  return rtn;
}

static void swTCPClientReconnectTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swEdgeTimerStop(timer);
  swTCPClient *client = swEdgeWatcherDataGet(timer);
  swSocket *sock = (swSocket *)client;
  if (sock && (sock->fd >= 0))
  {
    bool rtn = false;
    swTCPConnectionErrorType errorCode = swTCPConnectionErrorNone;
    if ((expiredCount > 0) && (events & swEdgeEventRead))
    {
      swSocketReturnType ret = swSocketReconnect(sock);
      rtn = swTCPClientConnectProcess(client, ret, &errorCode);
    }
    if (!rtn)
    {
      client->reconnect = false;
      swTCPConnectionClose((swTCPConnection *)client, errorCode);
    }
  }
}

static void swTCPClientConnectEventCallback(swEdgeIO *ioWatcher, uint32_t events)
{
  swEdgeIOStop(ioWatcher);
  swTCPClient *client = swEdgeWatcherDataGet(ioWatcher);
  if (client)
  {
    swEdgeTimerStop(&(client->connectTimer));
    client->connecting = false;
    bool rtn = false;
    swTCPConnectionErrorType errorCode = swTCPConnectionErrorNone;
    if ((events & swEdgeEventWrite) && !(events & (swEdgeEventError | swEdgeEventHungUp)))
    {
      swSocket *sock = (swSocket *)client;
      if (swSocketIsConnected(sock, NULL))
      {
        if (swTCPConnectionStart((swTCPConnection *)client, client->loop))
        {
          rtn = true;
          if (client->connectedFunc)
            client->connectedFunc(client);
        }
        else
          errorCode = swTCPConnectionErrorOtherError;
      }
      else
        errorCode = swTCPConnectionErrorConnectedCheckFailed;
    }
    else
      errorCode = ((events & swEdgeEventError) ? swTCPConnectionErrorSocketError : swTCPConnectionErrorSocketHangUp);
    if (!rtn)
      swTCPConnectionClose((swTCPConnection *)client, errorCode);
  }
}

static void swTCPClientConnectionCloseCallback(swTCPConnection *conn)
{
  swTCPClient *client = (swTCPClient *)conn;
  if (client && !client->closing)
  {
    client->closing = true;
    if (client->connecting)
    {
      swEdgeTimerStop(&(client->connectTimer));
      swEdgeIOStop(&(client->connectEvent));
      client->connecting = false;
    }
    if (client->closeFunc)
      client->closeFunc(client);
    if (client->reconnect)
    {
      swEdgeTimerStop(&(client->reconnectTimer));
      if (!swEdgeTimerStart(&(client->reconnectTimer), client->loop, client->reconnectTimeout, 0, false))
        swTCPConnectionClose((swTCPConnection *)client, swTCPConnectionErrorOtherError);
    }
    client->closing = false;
  }
}

swTCPClient *swTCPClientNew()
{
  swTCPClient *rtn = swMemoryMalloc(sizeof(swTCPClient));
  if (rtn)
  {
    if (!swTCPClientInit(rtn))
    {
      swTCPClientDelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swTCPClientInit(swTCPClient *client)
{
  bool rtn = false;
  if (client)
  {
    memset(client, 0, sizeof(swTCPClient));
    if (swTCPConnectionInit((swTCPConnection *)client))
    {
      if (swEdgeTimerInit(&(client->connectTimer), swTCPClientConnectTimerCallback, false))
      {
        swEdgeWatcherDataSet(&(client->connectTimer), client);
        if (swEdgeTimerInit(&(client->reconnectTimer), swTCPClientReconnectTimerCallback, false))
        {
          swEdgeWatcherDataSet(&(client->reconnectTimer), client);
          if (swEdgeIOInit(&(client->connectEvent), swTCPClientConnectEventCallback))
          {
            swEdgeWatcherDataSet(&(client->connectEvent), client);
            client->connectTimeout = client->reconnectTimeout = SW_TCPCONNECTION_DEFAULT_TIMEOUT;
            client->reconnect = true;
            ((swTCPConnection *)client)->closeFunc = swTCPClientConnectionCloseCallback;
            rtn = true;
          }
        }
      }
    }
  }
  return rtn;
}

void swTCPClientCleanup(swTCPClient *client)
{
  if (client && !client->cleaning)
  {
    client->cleaning = true;
    client->reconnect = false;
    client->connecting = false;
    client->loop = NULL;
    swEdgeIOClose(&(client->connectEvent));
    swEdgeTimerClose(&(client->reconnectTimer));
    swEdgeTimerClose(&(client->connectTimer));
    swTCPConnectionCleanup((swTCPConnection *)client);
    client->cleaning = false;
  }
}

void swTCPClientDelete(swTCPClient *client)
{
  if (client && !client->deleting)
  {
    client->deleting = true;
    swTCPClientCleanup(client);
    swMemoryFree(client);
  }
}

bool swTCPClientStart(swTCPClient *client, swSocketAddress *address, swEdgeLoop *loop, swSocketAddress *bindAddress)
{
  bool rtn = false;
  if (client && address && loop)
  {
    swSocket *sock = (swSocket *)client;
    if (swSocketInit(sock, address->storage.ss_family, SOCK_STREAM))
    {
      swTCPConnectionErrorType errorCode = swTCPConnectionErrorNone;
      if (!bindAddress || swSocketBind(sock, bindAddress))
      {
        client->loop = loop;
        swSocketReturnType ret = swSocketConnect(sock, address);
        rtn = swTCPClientConnectProcess(client, ret, &errorCode);
      }
      else
        errorCode = swTCPConnectionErrorConnectFailed;
      if (!rtn)
      {
        client->reconnect = false;
        swTCPConnectionClose((swTCPConnection *)client, errorCode);
      }
    }
  }
  return rtn;
}

void swTCPClientStop(swTCPClient *client)
{
  if (client && client->loop)
  {
    client->reconnect = false;
    client->connecting = false;
    swEdgeIOStop(&(client->connectEvent));
    swEdgeTimerStop(&(client->connectTimer));
    swEdgeTimerStop(&(client->reconnectTimer));
    swTCPConnectionClose((swTCPConnection *)client, swTCPConnectionErrorNone);

    client->loop = NULL;
    if (client->stopFunc)
      client->stopFunc(client);
  }
}

bool swTCPClientClose(swTCPClient *client)
{
  bool rtn = false;
  if (client && client->loop)
  {
    if (((swSocket *)client)->fd >= 0)
      swTCPConnectionClose((swTCPConnection *)client, swTCPConnectionErrorNone);
    rtn = true;
  }
  return rtn;
}
