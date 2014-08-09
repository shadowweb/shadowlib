#include "open-ssl/ssl-server.h"

#include <core/memory.h>

// TODO: consider limiting the number of accepts on the same pass, if we do limit, set pending on the loop
static void swSSLServerAcceptorAcceptEventCallback(swEdgeIO *ioWatcher, uint32_t events)
{
  swSSLServerAcceptor *serverAcceptor = swEdgeWatcherDataGet(ioWatcher);
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
            swSSL *ssl = swSSLNewFromFD(serverAcceptor->context, acceptedSock.fd, false);
            if (ssl)
            {
              swSSLServer *server = swSSLServerNew();
              if (server)
              {
                server->io.socketIO.sock = acceptedSock;
                server->io.ssl = ssl;
                if (!(!serverAcceptor->setupFunc || serverAcceptor->setupFunc(serverAcceptor, server)) || !swSSLServerStart(server, serverAcceptor->loop))
                  swSSLServerDelete(server);
              }
              else
                swSSLDelete(ssl);
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
      swSSLServerAcceptorStop(serverAcceptor);
    }
  }
}

swSSLServerAcceptor *swSSLServerAcceptorNew(swSSLContext *context)
{
  swSSLServerAcceptor *rtn = NULL;
  if (context)
  {
    if ((rtn = swMemoryMalloc(sizeof(swSSLServerAcceptor))))
    {
      if (!swSSLServerAcceptorInit(rtn, context))
      {
        swSSLServerAcceptorDelete(rtn);
        rtn = NULL;
      }
    }
  }
  return rtn;
}

bool swSSLServerAcceptorInit(swSSLServerAcceptor *serverAcceptor, swSSLContext *context)
{
  bool rtn = false;
  if (serverAcceptor && context)
  {
    memset(serverAcceptor, 0, sizeof(swSSLServerAcceptor));
    if (swEdgeIOInit(&(serverAcceptor->acceptEvent), swSSLServerAcceptorAcceptEventCallback))
    {
      serverAcceptor->context = context;
      swEdgeWatcherDataSet(&(serverAcceptor->acceptEvent), serverAcceptor);
      rtn = true;
    }
  }
  return rtn;
}

void swSSLServerAcceptorCleanup(swSSLServerAcceptor *serverAcceptor)
{
  if (serverAcceptor)
  {
    serverAcceptor->loop = NULL;
    serverAcceptor->context = NULL;
    swEdgeIOClose(&(serverAcceptor->acceptEvent));
  }
}

void swSSLServerAcceptorDelete(swSSLServerAcceptor *serverAcceptor)
{
  if (serverAcceptor)
  {
    swSSLServerAcceptorCleanup(serverAcceptor);
    swMemoryFree(serverAcceptor);
  }
}

bool swSSLServerAcceptorStart(swSSLServerAcceptor *serverAcceptor, swEdgeLoop *loop, swSocketAddress *address)
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

void swSSLServerAcceptorStop(swSSLServerAcceptor *serverAcceptor)
{
  if (serverAcceptor)
  {
    swEdgeIOStop(&(serverAcceptor->acceptEvent));
    swSocketClose((swSocket *)serverAcceptor);
    if (serverAcceptor->stopFunc)
      serverAcceptor->stopFunc(serverAcceptor);
  }
}
