#include "udp-client.h"

#include <core/memory.h>

static inline bool swUDPClientConnectProcess(swUDPClient *client, swSocketReturnType ret, swSocketIOErrorType *errorCode)
{
  bool rtn = false;
  if ( ret > swSocketReturnNone)
  {
    if (ret == swSocketReturnOK)
    {
      if (swSocketIOStart((swSocketIO *)client, client->loop))
      {
        rtn = true;
        if (client->connectedFunc)
          client->connectedFunc(client);
      }
      else
        *errorCode = swSocketIOErrorOtherError;
    }
    else
    {
      rtn = true;
      swSocketIOClose((swSocketIO *)client, swSocketIOErrorConnectFailed);
    }
  }
  else
    *errorCode = swSocketIOErrorConnectFailed;
  return rtn;
}


static void swUDPClientReconnectTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swEdgeTimerStop(timer);
  swUDPClient *client = swEdgeWatcherDataGet(timer);
  swSocket *sock = (swSocket *)client;
  if (sock && (sock->fd < 0))
  {
    bool rtn = false;
    swSocketIOErrorType errorCode = swSocketIOErrorNone;
    if ((expiredCount > 0) && (events & swEdgeEventRead))
    {
      swSocketReturnType ret = swSocketReconnect(sock);
      rtn = swUDPClientConnectProcess(client, ret, &errorCode);
    }
    if (!rtn)
    {
      client->reconnect = false;
      swSocketIOClose((swSocketIO *)client, errorCode);
    }
  }
}

static void swUDPClientConnectionCloseCallback(swSocketIO *io)
{
  swUDPClient *client = (swUDPClient *)io;
  if (client && !client->closing)
  {
    client->closing = true;
    if (client->closeFunc)
      client->closeFunc(client);
    if (client->reconnect)
    {
      swEdgeTimerStop(&(client->reconnectTimer));
      if (!swEdgeTimerStart(&(client->reconnectTimer), client->loop, client->reconnectTimeout, 0, false))
        swSocketIOClose((swSocketIO *)client, swSocketIOErrorOtherError);
    }
    client->closing = false;
  }
}

swUDPClient *swUDPClientNew()
{
  swUDPClient *rtn = swMemoryMalloc(sizeof(swUDPClient));
  if (rtn)
  {
    if (!swUDPClientInit(rtn))
    {
      swUDPClientDelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swUDPClientInit(swUDPClient *client)
{
  bool rtn = false;
  if (client)
  {
    memset(client, 0, sizeof(swUDPClient));
    if (swSocketIOInit((swSocketIO *)client))
    {
      if (swEdgeTimerInit(&(client->reconnectTimer), swUDPClientReconnectTimerCallback, false))
      {
        swEdgeWatcherDataSet(&(client->reconnectTimer), client);
        client->reconnectTimeout = SW_SOCKETIO_DEFAULT_TIMEOUT;
        client->reconnect = true;
        ((swSocketIO *)client)->closeFunc = swUDPClientConnectionCloseCallback;
        rtn = true;
      }
    }
  }
  return rtn;
}

void swUDPClientCleanup(swUDPClient *client)
{
  if (client && !client->cleaning)
  {
    client->cleaning = true;
    client->reconnect = false;
    client->loop = NULL;
    swEdgeTimerClose(&(client->reconnectTimer));
    swSocketIOCleanup((swSocketIO *)client);
    client->cleaning = false;
  }
}

void swUDPClientDelete(swUDPClient *client)
{
  if (client && !client->deleting)
  {
    client->deleting = true;
    swUDPClientCleanup(client);
    swMemoryFree(client);
  }
}

bool swUDPClientStart(swUDPClient *client, swSocketAddress *address, swEdgeLoop *loop, swSocketAddress *bindAddress)
{
  bool rtn = false;
  if (client && address && loop)
  {
    swSocket *sock = (swSocket *)client;
    if (swSocketInit(sock, address->storage.ss_family, SOCK_DGRAM))
    {
      swSocketIOErrorType errorCode = swSocketIOErrorNone;
      if (!bindAddress || swSocketBind(sock, bindAddress))
      {
        client->loop = loop;
        swSocketReturnType ret = swSocketConnect(sock, address);
        rtn = swUDPClientConnectProcess(client, ret, &errorCode);
      }
      else
        errorCode = swSocketIOErrorConnectFailed;
      if (!rtn)
      {
        client->reconnect = false;
        swSocketIOClose((swSocketIO *)client, errorCode);
      }
    }
  }
  return rtn;
}

void swUDPClientStop(swUDPClient *client)
{
  if (client && client->loop)
  {
    client->reconnect = false;
    swEdgeTimerStop(&(client->reconnectTimer));
    swSocketIOClose((swSocketIO *)client, swSocketIOErrorNone);

    client->loop = NULL;
    if (client->stopFunc)
      client->stopFunc(client);
  }
}

