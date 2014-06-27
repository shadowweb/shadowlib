#include "tcp-client.h"
#include "tcp-server.h"

#include "unittest/unittest.h"

#include <signal.h>
#include <unistd.h>

void edgeLoopSetup(swTestSuite *suite)
{
  signal(SIGPIPE, SIG_IGN);
  swTestLogLine("Creating loop ...\n");
  swEdgeLoop *loop = swEdgeLoopNew();
  ASSERT_NOT_NULL(loop);
  swTestSuiteDataSet(suite, loop);
}

void edgeLoopTeardown(swTestSuite *suite)
{
  swTestLogLine("Deleting loop ...\n");
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(loop);
  swEdgeLoopDelete(loop);
}

#define BUFFER_SIZE   2048
#define EXPECT_BYTES  204800

static char serverReadBuffer[BUFFER_SIZE] = {0};
static char serverWriteBuffer[BUFFER_SIZE] = {0};
static char clientReadBuffer[BUFFER_SIZE] = {0};
static char clientWriteBuffer[BUFFER_SIZE] = {0};
static ssize_t serverBytesRead = 0;
static ssize_t serverBytesWritten = 0;
static ssize_t clientBytesRead = 0;
static ssize_t clientBytesWritten = 0;
static swTCPServer *serverConnection = NULL;

void onServerReadReady(swTCPServer *server)
{
  // swTestLogLine("Server: read ready, bytes read = %zd\n", serverBytesRead);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(serverReadBuffer);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPServerRead(server, &buffer, &bytesRead);
    if (ret != swSocketReturnOK)
      break;
    serverBytesRead += bytesRead;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(server->io.loop);
  }
  else if (serverBytesRead > EXPECT_BYTES && clientBytesRead > EXPECT_BYTES)
    swEdgeLoopBreak(server->io.loop);
}

void onServerWriteReady(swTCPServer *server)
{
  // swTestLogLine("Server: write ready, bytes written = %zd\n", serverBytesWritten);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(serverWriteBuffer);
  ssize_t bytesWritten = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPServerWrite(server, &buffer, &bytesWritten);
    if (ret != swSocketReturnOK)
      break;
    serverBytesWritten += bytesWritten;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(server->io.loop);
  }
}

bool onServerReadTimeout(swTCPServer *server)
{
  swTestLogLine("Server: read timeout\n");
  return false;
}

bool onServerWriteTimeout(swTCPServer *server)
{
  swTestLogLine("Server: write timeout\n");
  return false;
}

void onServerError(swTCPServer *server, swSocketIOErrorType errorCode)
{
  swTestLogLine("Server: error \"%s\"\n", swSocketIOErrorTextGet(errorCode));
}

void onServerClose(swTCPServer *server)
{
  // swTestLogLine("Server: close\n");
  swTCPServerDelete(server);
  serverConnection = NULL;
}

bool onAccept(swTCPServerAcceptor *serverAcceptor)
{
  static uint32_t lifetimeServerConnectionsCount = 0;
  lifetimeServerConnectionsCount++;
  // swTestLogLine("Acceptor: accepting connection %u\n", lifetimeServerConnectionsCount);
  return true;
}

void onStop(swTCPServerAcceptor *serverAcceptor)
{
  // swTestLogLine("Acceptor: stopping\n");
}

void onError(swTCPServerAcceptor *serverAcceptor, swSocketIOErrorType errorCode)
{
  swTestLogLine("Acceptor: error \"%s\"\n", swSocketIOErrorTextGet(errorCode));
}

bool onConnectionSetup(swTCPServerAcceptor *serverAcceptor, swTCPServer *server)
{
  bool rtn = false;
  // swTestLogLine("Server: new connection setup\n");
  if (server)
  {
    serverConnection = server;
    swTCPServerReadTimeoutSet     (server, 1000);
    swTCPServerWriteTimeoutSet    (server, 1000);
    swTCPServerReadReadyFuncSet   (server, onServerReadReady);
    swTCPServerWriteReadyFuncSet  (server, onServerWriteReady);
    swTCPServerReadTimeoutFuncSet (server, onServerReadTimeout);
    swTCPServerWriteTimeoutFuncSet(server, onServerWriteTimeout);
    swTCPServerErrorFuncSet       (server, onServerError);
    swTCPServerCloseFuncSet       (server, onServerClose);
    rtn = true;
  }
  ASSERT_TRUE(rtn);
  return rtn;
}

void onClientConnected(swTCPClient *client)
{
  // swTestLogLine("Client: connected\n");
}

void onClientClose(swTCPClient *client)
{
  // swTestLogLine("Client: close\n");
}

void onClientStop(swTCPClient *client)
{
  // swTestLogLine("Client: stop\n");
}

void onClientReadReady(swTCPClient *client)
{
  // swTestLogLine("Client: read ready, bytes read = %zd\n", clientBytesRead);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(clientReadBuffer);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPClientRead(client, &buffer, &bytesRead);
    if (ret != swSocketReturnOK)
      break;
    clientBytesRead += bytesRead;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(client->loop);
  }
  else if (serverBytesRead > EXPECT_BYTES && clientBytesRead > EXPECT_BYTES)
    swEdgeLoopBreak(client->loop);
}

void onClientWriteReady(swTCPClient *client)
{
  // swTestLogLine("Client: write ready, bytes written = %zd\n", clientBytesWritten);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(clientWriteBuffer);
  ssize_t bytesWritten = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPClientWrite(client, &buffer, &bytesWritten);
    if (ret != swSocketReturnOK)
      break;
    clientBytesWritten += bytesWritten;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(client->loop);
  }
}

bool onClientReadTimeout(swTCPClient *client)
{
  swTestLogLine("Client: read timeout\n");
  return false;
}

bool onClientWriteTimeout(swTCPClient *client)
{
  swTestLogLine("Client: write timeout\n");
  return false;
}

void onClientError(swTCPClient *client, swSocketIOErrorType errorCode)
{
  swTestLogLine("Client: error \"%s\"\n", swSocketIOErrorTextGet(errorCode));
}

bool runClientServerTest(swSocketAddress *address,  swEdgeLoop *loop)
{
  ASSERT_NOT_NULL(loop);
  ASSERT_NOT_NULL(address);
  bool rtn = false;
  swTCPServerAcceptor *serverAcceptor = swTCPServerAcceptorNew();
  if (serverAcceptor)
  {
    // set callbacks
    swTCPServerAcceptorAcceptFuncSet  (serverAcceptor, onAccept);
    swTCPServerAcceptorStopFuncSet    (serverAcceptor, onStop);
    swTCPServerAcceptorErrorFuncSet   (serverAcceptor, onError);
    swTCPServerAcceptorSetupFuncSet   (serverAcceptor, onConnectionSetup);

    if (swTCPServerAcceptorStart(serverAcceptor, loop, address))
    {
      swTCPClient *client = swTCPClientNew();
      if (client)
      {
        // set callbacks and timeouts
        swTCPClientConnectTimeoutSet(client, 1000);
        swTCPClientReconnectTimeoutSet(client, 1000);
        swTCPClientConnectedFuncSet(client, onClientConnected);
        swTCPClientCloseFuncSet(client, onClientClose);
        swTCPClientStopFuncSet(client, onClientStop);

        swTCPClientReadTimeoutSet(client, 1000);
        swTCPClientWriteTimeoutSet(client, 1000);
        swTCPClientReadReadyFuncSet(client, onClientReadReady);
        swTCPClientWriteReadyFuncSet(client, onClientWriteReady);
        swTCPClientReadTimeoutFuncSet(client, onClientReadTimeout);
        swTCPClientWriteTimeoutFuncSet(client, onClientWriteTimeout);
        swTCPClientErrorFuncSet(client, onClientError);

        if (swTCPClientStart(client, address, loop, NULL))
        {
          swEdgeLoopRun(loop, false);
          // swTestLogLine("server read = %zd, server written = %zd\n", serverBytesRead, serverBytesWritten);
          // swTestLogLine("client read = %zd, client written = %zd\n", clientBytesRead, clientBytesWritten);
          rtn = true;
          serverBytesRead = serverBytesWritten = clientBytesRead = clientBytesWritten = 0;
          if (serverConnection)
          {
            swTCPServerDelete(serverConnection);
            serverConnection = NULL;
          }
          swTCPClientStop(client);
        }
        swTCPClientDelete(client);
      }
      swTCPServerAcceptorStop(serverAcceptor);
    }
    swTCPServerAcceptorDelete(serverAcceptor);
  }
  return rtn;
}

swTestDeclare(TCPClientServerOverIP4Test, NULL, NULL, swTestRun)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  swSocketAddress address = { 0 };
  if (swSocketAddressInitInet(&address, "127.0.0.1", 10000))
    return runClientServerTest(&address, loop);
  return false;
}

swTestDeclare(TCPClientServerOverIP6Test, NULL, NULL, swTestRun)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  swSocketAddress address = { 0 };
  if (swSocketAddressInitInet6(&address, "::1", 10000))
    return runClientServerTest(&address, loop);
  return false;
}

swTestDeclare(TCPClientServerOverUnixTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  swSocketAddress address = { 0 };
  char *filename = "/tmp/test_socket";
  if (swSocketAddressInitUnix(&address, filename))
    rtn = runClientServerTest(&address, loop);
  unlink(filename);
  return rtn;
}

swTestSuiteStructDeclare(TCPClientServerTestSuite, edgeLoopSetup, edgeLoopTeardown, swTestRun,
                         &TCPClientServerOverIP4Test, &TCPClientServerOverIP6Test , &TCPClientServerOverUnixTest);
