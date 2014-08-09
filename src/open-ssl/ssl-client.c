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
    swSSLSocketIOClose((swSSLSocketIO *)client, swSocketIOErrorConnectTimeout);
  }
}

static inline bool swSSLClientConnectProcess(swSSLClient *client, swSocketReturnType ret, swSocketIOErrorType *errorCode)
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
      swSSLSocketIOClose((swSSLSocketIO *)client, swSocketIOErrorConnectFailed);
    }
  }
  else
    *errorCode = swSocketIOErrorConnectFailed;
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
    swSocketIOErrorType errorCode = swSocketIOErrorNone;
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
    swSocketIOErrorType errorCode = swSocketIOErrorNone;
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
          errorCode = swSocketIOErrorOtherError;
      }
      else
        errorCode = swSocketIOErrorConnectedCheckFailed;
    }
    else
      errorCode = ((events & swEdgeEventError) ? swSocketIOErrorSocketError : swSocketIOErrorSocketHangUp);
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
        swSSLSocketIOClose((swSSLSocketIO *)client, swSocketIOErrorOtherError);
    }
    client->closing = false;
  }
}

swSSLClient *swSSLClientNew(swSSLContext *context)
{
  swSSLClient *rtn = NULL;
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
            client->connectTimeout = client->reconnectTimeout = SW_SOCKETIO_DEFAULT_TIMEOUT;
            client->reconnect = true;
            ((swSocketIO *)client)->closeFunc = (swSocketIOCloseFunc)swSSLClientConnectionCloseCallback;
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
      swSocketIOErrorType errorCode = swSocketIOErrorNone;
      if (!bindAddress || swSocketBind(sock, bindAddress))
      {
        client->loop = loop;
        swSocketReturnType ret = swSocketConnect(sock, address);
        rtn = swSSLClientConnectProcess(client, ret, &errorCode);
      }
      else
        errorCode = swSocketIOErrorConnectFailed;
      if (!rtn)
      {
        client->reconnect = false;
        swSSLSocketIOClose((swSSLSocketIO *)client, errorCode);
      }
    }
  }
  return rtn;
}

void swSSLClientStop(swSSLClient *client)
{
  if (client && client->loop)
  {
    client->reconnect = false;
    client->connecting = false;
    swEdgeIOStop(&(client->connectEvent));
    swEdgeTimerStop(&(client->connectTimer));
    swEdgeTimerStop(&(client->reconnectTimer));
    swSSLSocketIOClose((swSSLSocketIO *)client, swSocketIOErrorNone);

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
      swSSLSocketIOClose((swSSLSocketIO *)client, swSocketIOErrorNone);
    rtn = true;
  }
  return rtn;
}
