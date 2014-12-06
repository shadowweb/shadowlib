#include "tools/splicer/splicer-state.h"

#include "core/memory.h"
#include "log/log-manager.h"

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

swLoggerDeclareWithLevel(stateLogger, "SplicerState", swLogLevelInfo);

#define SW_SPLICER_DEFAULT_TIMEOUT              60
#define SW_SPLICER_DEFAULT_CONNECT_TIMEOUT      10
#define SW_SPLICER_DEFAULT_RECONNECT_INTERVAL   5

static bool onAccept(swTCPServerAcceptor *serverAcceptor)
{
  SW_LOG_INFO(&stateLogger, "acceptor accepting connection");
  return true;
}

static void onStop(swTCPServerAcceptor *serverAcceptor)
{
  SW_LOG_INFO(&stateLogger, "accepptor stopping");
}

static void onError(swTCPServerAcceptor *serverAcceptor, swSocketIOErrorType errorCode)
{
  SW_LOG_ERROR(&stateLogger, "acceptor error \"%s\"", swSocketIOErrorTextGet(errorCode));
}

static void onServerReadReady(swTCPServer *server)
{
  SW_LOG_TRACE(&stateLogger, "server read ready entered");
  swSplicerState *splicerState = (swSplicerState *)swTCPServerDataGet(server);
  if (splicerState)
  {
    swSocketReturnType ret = swSocketReturnNone;
    ssize_t bytesRead = 0;
    ret = swSocketIOReadSplice((swSocketIO *)server, splicerState->pipeToClient, PIPE_BUF, &bytesRead);
    if (ret == swSocketReturnOK)
      splicerState->serverBytesReceived += bytesRead;
  }
}

static void onServerWriteReady(swTCPServer *server)
{
  SW_LOG_TRACE(&stateLogger, "server write ready entered");
  swSplicerState *splicerState = (swSplicerState *)swTCPServerDataGet(server);
  if (splicerState)
  {
    swSocketReturnType ret = swSocketReturnNone;
    ssize_t bytesWritten = 0;
    if (splicerState->serverBufferWritten)
    {
      ret = swSocketIOWriteSplice((swSocketIO *)server, splicerState->pipeToServer, PIPE_BUF, &bytesWritten);
      if (ret == swSocketReturnOK)
        splicerState->serverBytesSent += bytesWritten;
    }
    else
    {
      ret = swTCPServerWrite(server, (swStaticBuffer *)(&(splicerState->buffer)), &bytesWritten);
      if (ret == swSocketReturnOK)
      {
        splicerState->serverBytesSent += bytesWritten;
        splicerState->serverBufferWritten = true;
      }
    }
  }
}

static bool onServerReadTimeout(swTCPServer *server)
{
  SW_LOG_INFO(&stateLogger, "server read timeout");
  return false;
}

static bool onServerWriteTimeout(swTCPServer *server)
{
  SW_LOG_INFO(&stateLogger, "server write timeout");
  return false;
}

static void onServerError(swTCPServer *server, swSocketIOErrorType errorCode)
{
  SW_LOG_ERROR(&stateLogger, "server error \"%s\"", swSocketIOErrorTextGet(errorCode));
}

static void onServerClose(swTCPServer *server)
{
  SW_LOG_INFO(&stateLogger, "server close");
  swSplicerState *splicerState = (swSplicerState *)swTCPServerDataGet(server);
  if (splicerState)
  {
    SW_LOG_INFO(&stateLogger, "server bytesSent = %lu, bytesReceived = %lu", splicerState->serverBytesSent, splicerState->serverBytesReceived);
    splicerState->serverBufferWritten = false;
    splicerState->server = NULL;
  }
  swTCPServerDelete(server);
}

static bool onConnectionSetup(swTCPServerAcceptor *serverAcceptor, swTCPServer *server)
{
  bool rtn = false;
  SW_LOG_INFO(&stateLogger, "new connection setup for %p", (void *)server);
  if (serverAcceptor)
  {
    swSplicerState *splicerState = (swSplicerState *)swTCPServerAcceptorDataGet(serverAcceptor);
    if (splicerState && !splicerState->server)
    {
      if (server)
      {
        swTCPServerReadTimeoutSet     (server, SW_SPLICER_DEFAULT_TIMEOUT);
        swTCPServerWriteTimeoutSet    (server, SW_SPLICER_DEFAULT_TIMEOUT);
        swTCPServerReadReadyFuncSet   (server, onServerReadReady);
        swTCPServerWriteReadyFuncSet  (server, onServerWriteReady);
        swTCPServerReadTimeoutFuncSet (server, onServerReadTimeout);
        swTCPServerWriteTimeoutFuncSet(server, onServerWriteTimeout);
        swTCPServerErrorFuncSet       (server, onServerError);
        swTCPServerCloseFuncSet       (server, onServerClose);

        swTCPServerDataSet(server, splicerState);
        splicerState->server = server;
        rtn = true;
      }
    }
  }
  return rtn;
}

static void onClientConnected(swTCPClient *client)
{
  SW_LOG_INFO(&stateLogger, "client connected");
  // swSplicerState *splicerState = (swSplicerState *)swTCPClientDataGet(client);
}

static void onClientClose(swTCPClient *client)
{
  SW_LOG_INFO(&stateLogger, "close");
  swSplicerState *splicerState = (swSplicerState *)swTCPClientDataGet(client);
  if (splicerState)
  {
    SW_LOG_INFO(&stateLogger, "client bytesSent = %lu, bytesReceived = %lu", splicerState->clientBytesSent, splicerState->clientBytesReceived);
    splicerState->clientBufferWritten = false;
  }
}

static void onClientStop(swTCPClient *client)
{
  SW_LOG_INFO(&stateLogger, "client stop");
  // swSplicerState *splicerState = (swSplicerState *)swTCPClientDataGet(client);
}

static void onClientReadReady(swTCPClient *client)
{
  SW_LOG_TRACE(&stateLogger, "client read ready entered");
  swSplicerState *splicerState = (swSplicerState *)swTCPClientDataGet(client);
  if (splicerState)
  {
    swSocketReturnType ret = swSocketReturnNone;
    ssize_t bytesRead = 0;
    ret = swSocketIOReadSplice((swSocketIO *)client, splicerState->pipeToServer, PIPE_BUF, &bytesRead);
    if (ret == swSocketReturnOK)
      splicerState->clientBytesReceived += bytesRead;
  }
}

static void onClientWriteReady(swTCPClient *client)
{
  SW_LOG_TRACE(&stateLogger, "client write ready entered");
  swSplicerState *splicerState = (swSplicerState *)swTCPClientDataGet(client);
  if (splicerState)
  {
    swSocketReturnType ret = swSocketReturnNone;
    ssize_t bytesWritten = 0;
    if (splicerState->clientBufferWritten)
    {
      ret = swSocketIOWriteSplice((swSocketIO *)client, splicerState->pipeToClient, PIPE_BUF, &bytesWritten);
      if (ret == swSocketReturnOK)
        splicerState->clientBytesSent += bytesWritten;
    }
    else
    {
      ret = swTCPClientWrite(client, (swStaticBuffer *)(&(splicerState->buffer)), &bytesWritten);
      if (ret == swSocketReturnOK)
      {
        splicerState->clientBytesSent += bytesWritten;
        splicerState->clientBufferWritten = true;
      }
    }
  }
}

static bool onClientReadTimeout(swTCPClient *client)
{
  SW_LOG_INFO(&stateLogger, "client read timeout");
  return false;
}

static bool onClientWriteTimeout(swTCPClient *client)
{
  SW_LOG_INFO(&stateLogger, "client write timeout");
  return false;
}

static void onClientError(swTCPClient *client, swSocketIOErrorType errorCode)
{
  SW_LOG_ERROR(&stateLogger, "client error \"%s\"", swSocketIOErrorTextGet(errorCode));
}

void swSplicerStateDelete(swSplicerState *splicerState)
{
  if (splicerState)
  {
    if (splicerState->client)
    {
      swTCPClientStop(splicerState->client);
      swTCPClientDelete(splicerState->client);
    }
    if (splicerState->server)
      swTCPServerStop(splicerState->server);
    if (splicerState->acceptor)
    {
      swTCPServerAcceptorStop(splicerState->acceptor);
      swTCPServerAcceptorDelete(splicerState->acceptor);
    }
    swDynamicBufferRelease(&(splicerState->buffer));
    if (splicerState->pipeToServer[0] >= 0)
      close(splicerState->pipeToServer[0]);
    if (splicerState->pipeToServer[1] >= 0)
      close(splicerState->pipeToServer[1]);
    if (splicerState->pipeToClient[0] >= 0)
      close(splicerState->pipeToClient[0]);
    if (splicerState->pipeToClient[1] >= 0)
      close(splicerState->pipeToClient[1]);
    swMemoryFree(splicerState);
  }
}

swSplicerState *swSplicerStateNew(swEdgeLoop *loop, swSocketAddress *address, size_t bufferSize)
{
  swSplicerState *rtn = NULL;
  if (loop && address && bufferSize)
  {
    swSplicerState *splicerState = swMemoryCalloc(1, sizeof(*rtn));
    if (splicerState)
    {
      if (swDynamicBufferInit(&(splicerState->buffer), (size_t)bufferSize))
      {
        // set all the memory in the buffer to make valgrind happy
        memset(splicerState->buffer.data, 0, bufferSize);
        splicerState->buffer.len = bufferSize;
        splicerState->acceptor = swTCPServerAcceptorNew();
        if (splicerState->acceptor)
        {
          // set callbacks
          swTCPServerAcceptorAcceptFuncSet  (splicerState->acceptor, onAccept);
          swTCPServerAcceptorStopFuncSet    (splicerState->acceptor, onStop);
          swTCPServerAcceptorErrorFuncSet   (splicerState->acceptor, onError);
          swTCPServerAcceptorSetupFuncSet   (splicerState->acceptor, onConnectionSetup);

          if (swTCPServerAcceptorStart(splicerState->acceptor, loop, address))
          {
            swTCPServerAcceptorDataSet(splicerState->acceptor, splicerState);
            splicerState->client = swTCPClientNew();
            if (splicerState->client)
            {
              // set callbacks and timeouts
              swTCPClientConnectTimeoutSet    (splicerState->client, SW_SPLICER_DEFAULT_CONNECT_TIMEOUT);
              swTCPClientReconnectTimeoutSet  (splicerState->client, SW_SPLICER_DEFAULT_RECONNECT_INTERVAL);
              swTCPClientConnectedFuncSet     (splicerState->client, onClientConnected);
              swTCPClientCloseFuncSet         (splicerState->client, onClientClose);
              swTCPClientStopFuncSet          (splicerState->client, onClientStop);

              swTCPClientReadTimeoutSet       (splicerState->client, SW_SPLICER_DEFAULT_TIMEOUT);
              swTCPClientWriteTimeoutSet      (splicerState->client, SW_SPLICER_DEFAULT_TIMEOUT);
              swTCPClientReadReadyFuncSet     (splicerState->client, onClientReadReady);
              swTCPClientWriteReadyFuncSet    (splicerState->client, onClientWriteReady);
              swTCPClientReadTimeoutFuncSet   (splicerState->client, onClientReadTimeout);
              swTCPClientWriteTimeoutFuncSet  (splicerState->client, onClientWriteTimeout);
              swTCPClientErrorFuncSet         (splicerState->client, onClientError);

              if (swTCPClientStart(splicerState->client, address, loop, NULL))
              {
                swTCPClientDataSet(splicerState->client, splicerState);
                if(pipe2(splicerState->pipeToServer, O_CLOEXEC | O_NONBLOCK) == 0)
                {
                  if(pipe2(splicerState->pipeToClient, O_CLOEXEC | O_NONBLOCK) == 0)
                    rtn = splicerState;
                  else
                    splicerState->pipeToClient[0] = splicerState->pipeToClient[1] = -1;
                }
                else
                  splicerState->pipeToServer[0] = splicerState->pipeToServer[1] = -1;
              }
            }
          }
        }
      }
      if (!rtn)
        swSplicerStateDelete(splicerState);
    }
  }
  return rtn;
}
