#include "tcp-server.h"

#include <core/memory.h>

// TODO: consider limiting the number of accepts on the same pass, if we do limit, set pending on the loop
static void swTCPServerAcceptEventCallback(swEdgeIO *ioWatcher, uint32_t events)
{
  swTCPServer *server = swEdgeWatcherDataGet(ioWatcher);
  if (server)
  {
    swSocket *sock = (swSocket *)server;
    swTCPConnectionErrorType errorCode = swTCPConnectionErrorNone;
    if ((events & swEdgeEventRead) && !(events & (swEdgeEventError | swEdgeEventHungUp)))
    {
      swSocketReturnType ret = swSocketReturnNone;
      while (true)
      {
        swSocket acceptedSock = {.type = SOCK_STREAM, .fd = -1};
        if ((ret = swSocketAccept(sock, &acceptedSock)) == swSocketReturnOK)
        {
          if (!server->acceptFunc || server->acceptFunc(server))
          {
            swTCPConnection *conn = swTCPConnectionNew();
            if (conn)
            {
              conn->sock = acceptedSock;
              if (!(!server->setupFunc || server->setupFunc(server, conn)) || !swTCPConnectionStart(conn, server->loop))
                swTCPConnectionDelete(conn);
            }
          }
          else
            swSocketClose(&acceptedSock);
        }
        else
          break;
      }
      if (ret != swSocketReturnNotReady)
        errorCode = swTCPConnectionErrorAcceptFailed;
    }
    else
      errorCode = ((events & swEdgeEventError) ? swTCPConnectionErrorSocketError : swTCPConnectionErrorSocketHangUp);
    if (errorCode)
    {
      if (server->errorFunc)
        server->errorFunc(server, errorCode);
      swTCPServerStop(server);
    }
  }
}

swTCPServer *swTCPServerNew()
{
  swTCPServer *rtn = swMemoryCalloc(1, sizeof(swTCPServer));
  if (rtn)
  {
    if (!swTCPServerInit(rtn))
    {
      swTCPServerDelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swTCPServerInit(swTCPServer *server)
{
  bool rtn = false;
  if (server)
  {
    if (swEdgeIOInit(&(server->acceptEvent), swTCPServerAcceptEventCallback))
    {
      swEdgeWatcherDataSet(&(server->acceptEvent), server);
      rtn = true;
    }
  }
  return rtn;
}

void swTCPServerCleanup(swTCPServer *server)
{
  if (server)
  {
    server->loop = NULL;
    swEdgeIOClose(&(server->acceptEvent));
  }
}

void swTCPServerDelete(swTCPServer *server)
{
  if (server)
  {
    swTCPServerCleanup(server);
    swMemoryFree(server);
  }
}

bool swTCPServerStart(swTCPServer *server, swEdgeLoop *loop, swSocketAddress *address)
{
  bool rtn = false;
  if(server && loop && address)
  {
    swSocket *sock = (swSocket *)server;
    if (swSocketInit(sock, address->storage.ss_family, SOCK_STREAM))
    {
      swTCPConnectionErrorType errorCode = swTCPConnectionErrorNone;
      if (swSocketListen(sock, address) == swSocketReturnOK)
      {
        if (swEdgeIOStart(&(server->acceptEvent), loop, sock->fd, swEdgeEventRead))
        {
          server->loop = loop;
          rtn = true;
        }
        else
          errorCode = swTCPConnectionErrorOtherError;
      }
      else
        errorCode = swTCPConnectionErrorListenFailed;
      if (!rtn)
      {
        if (errorCode && server->errorFunc)
          server->errorFunc(server, errorCode);
        swSocketClose((swSocket *)server);
      }
    }
  }
  return rtn;
}

void swTCPServerStop(swTCPServer *server)
{
  if (server)
  {
    swEdgeIOStop(&(server->acceptEvent));
    swSocketClose((swSocket *)server);
    if (server->stopFunc)
      server->stopFunc(server);
  }
}
