#include "open-ssl/ssl-client.h"

#include <core/memory.h>

static void swSSLClientConnectTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swEdgeTimerStop(timer);
  swSSLClient *client = swEdgeWatcherDataGet(timer);
  if (client && (((swSocket*)client)->fd >= 0))
  {
    client->connecting = false;
    swEdgeIOStop(&(client->connectEvent));
    swSSLSocketIOClose((swSSLSocketIO *)client, swSSLSocketIOErrorConnectTimeout);
  }
}

static inline bool swSSLClientConnectProcess(swSSLClient *client, swSocketReturnType ret, swSSLSocketIOErrorType *errorCode)
{
  bool rtn = false;
  if ( ret > swSocketReturnNone)
  {
    if (ret == swSocketReturnOK)
    {
      swSocket *sock = (swSocket *)client;
      if ((client->io.ssl = swSSLNewFromFD(client->context, sock->fd, true)))
      {
        if (swSSLSocketIOStart((swSSLSocketIO *)client, client->loop))
        {
          rtn = true;
          if (client->connectedFunc)
            client->connectedFunc(client);
        }
      }
      if (!rtn)
        *errorCode = swSSLSocketIOErrorOtherError;
    }
    else if (ret == swSocketReturnInProgress)
    {
      client->connecting = true;
      rtn = swEdgeIOStart(&(client->connectEvent), client->loop, ((swSocket *)client)->fd, swEdgeEventWrite) &&
        swEdgeTimerStart(&(client->connectTimer), client->loop, client->connectTimeout, 0, false);
      if (!rtn)
        *errorCode = swSSLSocketIOErrorOtherError;
    }
    else
    {
      rtn = true;
      swSSLSocketIOClose((swSSLSocketIO *)client, swSSLSocketIOErrorConnectFailed);
    }
  }
  else
    *errorCode = swSSLSocketIOErrorConnectFailed;
  return rtn;
}

static void swSSLClientReconnectTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swEdgeTimerStop(timer);
  swSSLClient *client = swEdgeWatcherDataGet(timer);
  swSocket *sock = (swSocket *)client;
  if (sock && (sock->fd < 0))
  {
    bool rtn = false;
    swSSLSocketIOErrorType errorCode = swSSLSocketIOErrorNone;
    if ((expiredCount > 0) && (events & swEdgeEventRead))
    {
      swSocketReturnType ret = swSocketReconnect(sock);
      rtn = swSSLClientConnectProcess(client, ret, &errorCode);
    }
    if (!rtn)
    {
      client->reconnect = false;
      swSSLSocketIOClose((swSSLSocketIO *)client, errorCode);
    }
  }
}

static void swSSLClientConnectEventCallback(swEdgeIO *ioWatcher, uint32_t events)
{
  swEdgeIOStop(ioWatcher);
  swSSLClient *client = swEdgeWatcherDataGet(ioWatcher);
  if (client)
  {
    swEdgeTimerStop(&(client->connectTimer));
    client->connecting = false;
    bool rtn = false;
    swSSLSocketIOErrorType errorCode = swSSLSocketIOErrorNone;
    if ((events & swEdgeEventWrite) && !(events & (swEdgeEventError | swEdgeEventHungUp)))
    {
      swSocket *sock = (swSocket *)client;
      if (swSocketIsConnected(sock, NULL))
      {
        swSocket *sock = (swSocket *)client;
        if ((client->io.ssl = swSSLNewFromFD(client->context, sock->fd, true)))
        {
          if (swSSLSocketIOStart((swSSLSocketIO *)client, client->loop))
          {
            rtn = true;
            if (client->connectedFunc)
              client->connectedFunc(client);
          }
        }
        if (!rtn)
          errorCode = swSSLSocketIOErrorOtherError;
      }
      else
        errorCode = swSSLSocketIOErrorConnectedCheckFailed;
    }
    else
      errorCode = ((events & swEdgeEventError) ? swSSLSocketIOErrorSocketError : swSSLSocketIOErrorSocketHangUp);
    if (!rtn)
      swSSLSocketIOClose((swSSLSocketIO *)client, errorCode);
  }
}

static void swSSLClientConnectionCloseCallback(swSSLSocketIO *io)
{
  swSSLClient *client = (swSSLClient *)io;
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
        swSSLSocketIOClose((swSSLSocketIO *)client, swSSLSocketIOErrorOtherError);
    }
    client->closing = false;
  }
}

swSSLClient *swSSLClientNew(swSSLContext *context)
{
  swSSLClient *rtn = swMemoryMalloc(sizeof(swSSLClient));
  if ((rtn= swMemoryMalloc(sizeof(swSSLClient))))
  {
    if (!swSSLClientInit(rtn, context))
    {
      swSSLClientDelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swSSLClientInit(swSSLClient *client, swSSLContext *context)
{
  bool rtn = false;
  if (client && context)
  {
    memset(client, 0, sizeof(swSSLClient));
    if (swSSLSocketIOInit((swSSLSocketIO *)client))
    {
      if (swEdgeTimerInit(&(client->connectTimer), swSSLClientConnectTimerCallback, false))
      {
        swEdgeWatcherDataSet(&(client->connectTimer), client);
        if (swEdgeTimerInit(&(client->reconnectTimer), swSSLClientReconnectTimerCallback, false))
        {
          swEdgeWatcherDataSet(&(client->reconnectTimer), client);
          if (swEdgeIOInit(&(client->connectEvent), swSSLClientConnectEventCallback))
          {
            swEdgeWatcherDataSet(&(client->connectEvent), client);
            client->connectTimeout = client->reconnectTimeout = SW_SSLSOCKETIO_DEFAULT_TIMEOUT;
            client->reconnect = true;
            ((swSSLSocketIO *)client)->closeFunc = swSSLClientConnectionCloseCallback;
            client->context = context;
            rtn = true;
          }
        }
      }
    }
  }
  return rtn;
}

void swSSLClientCleanup(swSSLClient *client)
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

void swSSLClientDelete(swSSLClient *client)
{
  if (client && !client->deleting)
  {
    client->deleting = true;
    swSSLClientCleanup(client);
    swMemoryFree(client);
  }
}

bool swSSLClientStart(swSSLClient *client, swSocketAddress *address, swEdgeLoop *loop, swSocketAddress *bindAddress)
{
  bool rtn = false;
  if (client && address && loop)
  {
    swSocket *sock = (swSocket *)client;
    if (swSocketInit(sock, address->storage.ss_family, SOCK_STREAM))
    {
      swSSLSocketIOErrorType errorCode = swSSLSocketIOErrorNone;
      if (!bindAddress || swSocketBind(sock, bindAddress))
      {
        client->loop = loop;
        swSocketReturnType ret = swSocketConnect(sock, address);
        rtn = swSSLClientConnectProcess(client, ret, &errorCode);
      }
      else
        errorCode = swSSLSocketIOErrorConnectFailed;
      if (!rtn)
      {
        client->reconnect = false;
        swSSLSocketIOClose((swSSLSocketIO *)client, errorCode);
      }
    }
  }
  return rtn;
}

void swSSClientStop(swSSLClient *client)
{
  if (client && client->loop)
  {
    client->reconnect = false;
    client->connecting = false;
    swEdgeIOStop(&(client->connectEvent));
    swEdgeTimerStop(&(client->connectTimer));
    swEdgeTimerStop(&(client->reconnectTimer));
    swSSLSocketIOClose((swSSLSocketIO *)client, swSSLSocketIOErrorNone);

    client->loop = NULL;
    if (client->stopFunc)
      client->stopFunc(client);
  }
}

bool swSSLClientClose(swSSLClient *client)
{
  bool rtn = false;
  if (client && client->loop)
  {
    if (((swSocket *)client)->fd >= 0)
      swSSLSocketIOClose((swSSLSocketIO *)client, swSSLSocketIOErrorNone);
    rtn = true;
  }
  return rtn;
}
