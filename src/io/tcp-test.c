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
static swTCPConnection *serverConnection = NULL;

void onServerReadReady(swTCPConnection *conn)
{
  // swTestLogLine("Server: read ready, bytes read = %zd\n", serverBytesRead);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(serverReadBuffer);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPConnectionRead(conn, &buffer, &bytesRead);
    if (ret != swSocketReturnOK)
      break;
    serverBytesRead += bytesRead;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(conn->loop);
  }
  else if (serverBytesRead > EXPECT_BYTES && clientBytesRead > EXPECT_BYTES)
    swEdgeLoopBreak(conn->loop);
}

void onServerWriteReady(swTCPConnection *conn)
{
  // swTestLogLine("Server: write ready, bytes written = %zd\n", serverBytesWritten);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(serverWriteBuffer);
  ssize_t bytesWritten = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPConnectionWrite(conn, &buffer, &bytesWritten);
    if (ret != swSocketReturnOK)
      break;
    serverBytesWritten += bytesWritten;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(conn->loop);
  }
}

bool onServerReadTimeout(swTCPConnection *conn)
{
  swTestLogLine("Server: read timeout\n");
  return false;
}

bool onServerWriteTimeout(swTCPConnection *conn)
{
  swTestLogLine("Server: write timeout\n");
  return false;
}

void onServerError(swTCPConnection *conn, swTCPConnectionErrorType errorCode)
{
  swTestLogLine("Server: error \"%s\"\n", swTCPConnectionErrorTextGet(errorCode));
}

void onServerClose(swTCPConnection *conn)
{
  // swTestLogLine("Server: close\n");
  swTCPConnectionDelete(conn);
  serverConnection = NULL;
}

bool onAccept(swTCPServer *server)
{
  static uint32_t lifetimeServerConnectionsCount = 0;
  lifetimeServerConnectionsCount++;
  // swTestLogLine("Acceptor: accepting connection %u\n", lifetimeServerConnectionsCount);
  return true;
}

void onStop(swTCPServer *server)
{
  // swTestLogLine("Acceptor: stopping\n");
}

void onError(swTCPServer *server, swTCPConnectionErrorType errorCode)
{
  swTestLogLine("Acceptor: error \"%s\"\n", swTCPConnectionErrorTextGet(errorCode));
}

bool onConnectionSetup(swTCPServer *server, swTCPConnection *conn)
{
  bool rtn = false;
  // swTestLogLine("Server: new connection setup\n");
  if (conn)
  {
    serverConnection = conn;
    swTCPConnectionReadTimeoutSet(conn, 1000);
    swTCPConnectionWriteTimeoutSet(conn, 1000);
    swTCPConnectionReadReadyFuncSet(conn, onServerReadReady);
    swTCPConnectionWriteReadyFuncSet(conn, onServerWriteReady);
    swTCPConnectionReadTimeoutFuncSet(conn, onServerReadTimeout);
    swTCPConnectionWriteTimeoutFuncSet(conn, onServerWriteTimeout);
    swTCPConnectionErrorFuncSet(conn, onServerError);
    swTCPConnectionCloseFuncSet(conn, onServerClose);
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

void onClientReadReady(swTCPConnection *conn)
{
  // swTestLogLine("Client: read ready, bytes read = %zd\n", clientBytesRead);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(clientReadBuffer);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPConnectionRead(conn, &buffer, &bytesRead);
    if (ret != swSocketReturnOK)
      break;
    clientBytesRead += bytesRead;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(conn->loop);
  }
  else if (serverBytesRead > EXPECT_BYTES && clientBytesRead > EXPECT_BYTES)
    swEdgeLoopBreak(conn->loop);
}

void onClientWriteReady(swTCPConnection *conn)
{
  // swTestLogLine("Client: write ready, bytes written = %zd\n", clientBytesWritten);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(clientWriteBuffer);
  ssize_t bytesWritten = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swTCPConnectionWrite(conn, &buffer, &bytesWritten);
    if (ret != swSocketReturnOK)
      break;
    clientBytesWritten += bytesWritten;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(conn->loop);
  }
}

bool onClientReadTimeout(swTCPConnection *conn)
{
  swTestLogLine("Client: read timeout\n");
  return false;
}

bool onClientWriteTimeout(swTCPConnection *conn)
{
  swTestLogLine("Client: write timeout\n");
  return false;
}

void onClientError(swTCPConnection *conn, swTCPConnectionErrorType errorCode)
{
  swTestLogLine("Client: error \"%s\"\n", swTCPConnectionErrorTextGet(errorCode));
}

bool runClientServerTest(swSocketAddress *address,  swEdgeLoop *loop)
{
  ASSERT_NOT_NULL(loop);
  ASSERT_NOT_NULL(address);
  bool rtn = false;
  swTCPServer *server = swTCPServerNew();
  if (server)
  {
    // set callbacks
    swTCPServerAcceptFuncSet(server, onAccept);
    swTCPServerStopFuncSet(server, onStop);
    swTCPServerErrorFuncSet(server, onError);
    swTCPServerSetupFuncSet(server, onConnectionSetup);

    if (swTCPServerStart(server, loop, address))
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

        swTCPConnectionReadTimeoutSet(client, 1000);
        swTCPConnectionWriteTimeoutSet(client, 1000);
        swTCPConnectionReadReadyFuncSet(client, onClientReadReady);
        swTCPConnectionWriteReadyFuncSet(client, onClientWriteReady);
        swTCPConnectionReadTimeoutFuncSet(client, onClientReadTimeout);
        swTCPConnectionWriteTimeoutFuncSet(client, onClientWriteTimeout);
        swTCPConnectionErrorFuncSet(client, onClientError);

        if (swTCPClientStart(client, address, loop, NULL))
        {
          swEdgeLoopRun(loop, false);
          // swTestLogLine("server read = %zd, server written = %zd\n", serverBytesRead, serverBytesWritten);
          // swTestLogLine("client read = %zd, client written = %zd\n", clientBytesRead, clientBytesWritten);
          rtn = true;
          serverBytesRead = serverBytesWritten = clientBytesRead = clientBytesWritten = 0;
          if (serverConnection)
          {
            swTCPConnectionDelete(serverConnection);
            serverConnection = NULL;
          }
          swTCPClientStop(client);
        }
        swTCPClientDelete(client);
      }
      swTCPServerStop(server);
    }
    swTCPServerDelete(server);
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
