#include "open-ssl/tcp-client.h"

#include <core/memory.h>

static void swTCPClientConnectTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swEdgeTimerStop(timer);
  swTCPClient *client = swEdgeWatcherDataGet(timer);
  if (client && (((swSocket*)client)->fd >= 0))
  {
    client->connecting = false;
    swEdgeIOStop(&(client->connectEvent));
    swSocketIOClose((swSocketIO *)client, swSocketIOErrorConnectTimeout);
  }
}

static inline bool swTCPClientConnectProcess(swTCPClient *client, swSocketReturnType ret, swSocketIOErrorType *errorCode)
{
  bool rtn = false;
  if (ret > swSocketReturnNone)
  {
    if (ret == swSocketReturnOK)
    {
      swSocket *sock = (swSocket *)client;
      if ((client->io.ssl = swSSLNewFromFD(client->context, sock->fd, true)))
      {
        if (swSocketIOStart((swSocketIO *)client, client->loop))
        {
          rtn = true;
          if (client->connectedFunc)
            client->connectedFunc(client);
        }
      }
      if (!rtn)
        *errorCode = swSocketIOErrorOtherError;
    }
    else if (ret == swSocketReturnInProgress)
    {
      client->connecting = true;
      rtn = swEdgeIOStart(&(client->connectEvent), client->loop, ((swSocket *)client)->fd, swEdgeEventWrite) &&
        swEdgeTimerStart(&(client->connectTimer), client->loop, client->connectTimeout, 0, false);
      if (!rtn)
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

static void swTCPClientReconnectTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swEdgeTimerStop(timer);
  swTCPClient *client = swEdgeWatcherDataGet(timer);
  swSocket *sock = (swSocket *)client;
  if (sock && (sock->fd < 0))
  {
    bool rtn = false;
    swSocketIOErrorType errorCode = swSocketIOErrorNone;
    if ((expiredCount > 0) && (events & swEdgeEventRead))
    {
      swSocketReturnType ret = swSocketReconnect(sock);
      rtn = swTCPClientConnectProcess(client, ret, &errorCode);
    }
    if (!rtn)
    {
      client->reconnect = false;
      swSocketIOClose((swSocketIO *)client, errorCode);
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
    swSocketIOErrorType errorCode = swSocketIOErrorNone;
    if ((events & swEdgeEventWrite) && !(events & (swEdgeEventError | swEdgeEventHungUp)))
    {
      swSocket *sock = (swSocket *)client;
      if (swSocketIsConnected(sock, NULL))
      {
        swSocket *sock = (swSocket *)client;
        if ((client->io.ssl = swSSLNewFromFD(client->context, sock->fd, true)))
        {
          if (swSocketIOStart((swSocketIO *)client, client->loop))
          {
            rtn = true;
            if (client->connectedFunc)
              client->connectedFunc(client);
          }
        }
        if (!rtn)
          errorCode = swSocketIOErrorOtherError;
      }
      else
        errorCode = swSocketIOErrorConnectedCheckFailed;
    }
    else
      errorCode = ((events & swEdgeEventError) ? swSocketIOErrorSocketError : swSocketIOErrorSocketHangUp);
    if (!rtn)
      swSocketIOClose((swSocketIO *)client, errorCode);
  }
}

static void swTCPClientConnectionCloseCallback(swSocketIO *io)
{
  swTCPClient *client = (swTCPClient *)io;
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
        swSocketIOClose((swSocketIO *)client, swSocketIOErrorOtherError);
    }
    client->closing = false;
  }
}

swTCPClient *swTCPClientNew(swSSLContext *context)
{
  swTCPClient *rtn = swMemoryMalloc(sizeof(swTCPClient));
  if (rtn)
  {
    if (!swTCPClientInit(rtn, context))
    {
      swTCPClientDelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swTCPClientInit(swTCPClient *client, swSSLContext *context)
{
  bool rtn = false;
  if (client && context)
  {
    memset(client, 0, sizeof(swTPClient));
    if (swSSLSocketIOInit((swSSLSocketIO *)client))
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
            client->connectTimeout = client->reconnectTimeout = SW_SOCKETIO_DEFAULT_TIMEOUT;
            client->reconnect = true;
            ((swSocketIO *)client)->closeFunc = swTCPClientConnectionCloseCallback;
            client->context = context;
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
    swSSLSocketIOCleanup((swSSLSocketIO *)client);
    client->context = NULL;
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
      swSocketIOErrorType errorCode = swSocketIOErrorNone;
      if (!bindAddress || swSocketBind(sock, bindAddress))
      {
        client->loop = loop;
        swSocketReturnType ret = swSocketConnect(sock, address);
        rtn = swTCPClientConnectProcess(client, ret, &errorCode);
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

void swTCPClientStop(swTCPClient *client)
{
  if (client && client->loop)
  {
    client->reconnect = false;
    client->connecting = false;
    swEdgeIOStop(&(client->connectEvent));
    swEdgeTimerStop(&(client->connectTimer));
    swEdgeTimerStop(&(client->reconnectTimer));
    swSocketIOClose((swSocketIO *)client, swSocketIOErrorNone);

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
      swSocketIOClose((swSocketIO *)client, swSocketIOErrorNone);
    rtn = true;
  }
  return rtn;
}
