#include "tcp-server.h"

#include <core/memory.h>

// TODO: consider limiting the number of accepts on the same pass, if we do limit, set pending on the loop
static void swTCPServerAcceptorAcceptEventCallback(swEdgeIO *ioWatcher, uint32_t events)
{
  swTCPServerAcceptor *serverAcceptor = swEdgeWatcherDataGet(ioWatcher);
  if (serverAcceptor)
  {
    swSocket *sock = (swSocket *)serverAcceptor;
    swSocketIOErrorType errorCode = swSocketIOErrorNone;
    if ((events & swEdgeEventRead) && !(events & (swEdgeEventError | swEdgeEventHungUp)))
    {
      swSocketReturnType ret = swSocketReturnNone;
      while (true)
      {
        swSocket acceptedSock = {.type = SOCK_STREAM, .fd = -1};
        if ((ret = swSocketAccept(sock, &acceptedSock)) == swSocketReturnOK)
        {
          if (!serverAcceptor->acceptFunc || serverAcceptor->acceptFunc(serverAcceptor))
          {
            swTCPServer *server = swTCPServerNew();
            if (server)
            {
              server->io.sock = acceptedSock;
              if (!(!serverAcceptor->setupFunc || serverAcceptor->setupFunc(serverAcceptor, server)) || !swTCPServerStart(server, serverAcceptor->loop))
                swTCPServerDelete(server);
            }
          }
          else
            swSocketClose(&acceptedSock);
        }
        else
          break;
      }
      if (ret != swSocketReturnNotReady)
        errorCode = swSocketIOErrorAcceptFailed;
    }
    else
      errorCode = ((events & swEdgeEventError) ? swSocketIOErrorSocketError : swSocketIOErrorSocketHangUp);
    if (errorCode)
    {
      if (serverAcceptor->errorFunc)
        serverAcceptor->errorFunc(serverAcceptor, errorCode);
      swTCPServerAcceptorStop(serverAcceptor);
    }
  }
}

swTCPServerAcceptor *swTCPServerAcceptorNew()
{
  swTCPServerAcceptor *rtn = swMemoryCalloc(1, sizeof(swTCPServerAcceptor));
  if (rtn)
  {
    if (!swTCPServerAcceptorInit(rtn))
    {
      swTCPServerAcceptorDelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swTCPServerAcceptorInit(swTCPServerAcceptor *serverAcceptor)
{
  bool rtn = false;
  if (serverAcceptor)
  {
    if (swEdgeIOInit(&(serverAcceptor->acceptEvent), swTCPServerAcceptorAcceptEventCallback))
    {
      swEdgeWatcherDataSet(&(serverAcceptor->acceptEvent), serverAcceptor);
      rtn = true;
    }
  }
  return rtn;
}

void swTCPServerAcceptorCleanup(swTCPServerAcceptor *serverAcceptor)
{
  if (serverAcceptor)
  {
    serverAcceptor->loop = NULL;
    swEdgeIOClose(&(serverAcceptor->acceptEvent));
  }
}

void swTCPServerAcceptorDelete(swTCPServerAcceptor *serverAcceptor)
{
  if (serverAcceptor)
  {
    swTCPServerAcceptorCleanup(serverAcceptor);
    swMemoryFree(serverAcceptor);
  }
}

bool swTCPServerAcceptorStart(swTCPServerAcceptor *serverAcceptor, swEdgeLoop *loop, swSocketAddress *address)
{
  bool rtn = false;
  if(serverAcceptor && loop && address)
  {
    swSocket *sock = (swSocket *)serverAcceptor;
    if (swSocketInit(sock, address->storage.ss_family, SOCK_STREAM))
    {
      swSocketIOErrorType errorCode = swSocketIOErrorNone;
      if (swSocketListen(sock, address) == swSocketReturnOK)
      {
        if (swEdgeIOStart(&(serverAcceptor->acceptEvent), loop, sock->fd, swEdgeEventRead))
        {
          serverAcceptor->loop = loop;
          rtn = true;
        }
        else
          errorCode = swSocketIOErrorOtherError;
      }
      else
        errorCode = swSocketIOErrorListenFailed;
      if (!rtn)
      {
        if (errorCode && serverAcceptor->errorFunc)
          serverAcceptor->errorFunc(serverAcceptor, errorCode);
        swSocketClose((swSocket *)serverAcceptor);
      }
    }
  }
  return rtn;
}

void swTCPServerAcceptorStop(swTCPServerAcceptor *serverAcceptor)
{
  if (serverAcceptor)
  {
    swEdgeIOStop(&(serverAcceptor->acceptEvent));
    swSocketClose((swSocket *)serverAcceptor);
    if (serverAcceptor->stopFunc)
      serverAcceptor->stopFunc(serverAcceptor);
  }
}
