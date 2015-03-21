#include "udp-client.h"
#include "udp-server.h"

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

static uint8_t serverReadBuffer[BUFFER_SIZE]   = {0};
static uint8_t serverWriteBuffer[BUFFER_SIZE]  = {0};
static uint8_t clientReadBuffer[BUFFER_SIZE]   = {0};
static uint8_t clientWriteBuffer[BUFFER_SIZE]  = {0};
static ssize_t serverBytesRead              = 0;
static ssize_t serverBytesWritten           = 0;
static ssize_t clientBytesRead              = 0;
static ssize_t clientBytesWritten           = 0;
static swSocketAddress serverReceiveAddress = {0};

void onServerReadReady(swUDPServer *server)
{
  // swTestLogLine("Server: read ready, bytes read = %zd\n", serverBytesRead);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(serverReadBuffer);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swUDPServerReadFrom(server, &buffer, &serverReceiveAddress, &bytesRead);
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

void onServerWriteReady(swUDPServer *server)
{
  // swTestLogLine("Server: write ready, bytes written = %zd\n", serverBytesWritten);
  if (serverReceiveAddress.len)
  {
    swSocketReturnType ret = swSocketReturnNone;
    swStaticBuffer buffer = swStaticBufferDefine(serverWriteBuffer);
    ssize_t bytesWritten = 0;
    for (uint32_t i = 0; i < 10; i++)
    {
      ret = swUDPServerWriteTo(server, &buffer, &serverReceiveAddress, &bytesWritten);
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
  else if (server)
  {
    swEdgeWatcherPendingSet((swEdgeWatcher *)&(server->io.ioEvent), swEdgeEventWrite);
  }
}

bool onServerReadTimeout(swUDPServer *server)
{
  swTestLogLine("Server: read timeout\n");
  return false;
}

bool onServerWriteTimeout(swUDPServer *server)
{
  swTestLogLine("Server: write timeout\n");
  return false;
}

void onServerError(swUDPServer *server, swSocketIOErrorType errorCode)
{
  swTestLogLine("Server: error \"%s\"\n", swSocketIOErrorTextGet(errorCode));
}

void onServerClose(swUDPServer *server)
{
  // swTestLogLine("Server: close\n");
}

void onClientConnected(swUDPClient *client)
{
  // swTestLogLine("Client: connected\n");
}

void onClientClose(swUDPClient *client)
{
  // swTestLogLine("Client: close\n");
}

void onClientStop(swUDPClient *client)
{
  // swTestLogLine("Client: stop\n");
}

void onClientReadReady(swUDPClient *client)
{
  // swTestLogLine("Client: read ready, bytes read = %zd\n", clientBytesRead);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(clientReadBuffer);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swUDPClientRead(client, &buffer, &bytesRead);
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

void onClientWriteReady(swUDPClient *client)
{
  // swTestLogLine("Client: write ready, bytes written = %zd\n", clientBytesWritten);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(clientWriteBuffer);
  ssize_t bytesWritten = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swUDPClientWrite(client, &buffer, &bytesWritten);
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

bool onClientReadTimeout(swUDPClient *client)
{
  swTestLogLine("Client: read timeout\n");
  return false;
}

bool onClientWriteTimeout(swUDPClient *client)
{
  swTestLogLine("Client: write timeout\n");
  return false;
}

void onClientError(swUDPClient *client, swSocketIOErrorType errorCode)
{
  swTestLogLine("Client: error \"%s\"\n", swSocketIOErrorTextGet(errorCode));
}

bool runClientServerTest(swSocketAddress *address,  swEdgeLoop *loop, swSocketAddress *bindAddress)
{
  ASSERT_NOT_NULL(loop);
  ASSERT_NOT_NULL(address);
  bool rtn = false;
  swUDPServer *server = swUDPServerNew();
  if (server)
  {
    // set callbacks
    swUDPServerReadTimeoutSet(server, 1000);
    swUDPServerWriteTimeoutSet(server, 1000);
    swUDPServerReadReadyFuncSet(server, onServerReadReady);
    swUDPServerWriteReadyFuncSet(server, onServerWriteReady);
    swUDPServerReadTimeoutFuncSet(server, onServerReadTimeout);
    swUDPServerWriteTimeoutFuncSet(server, onServerWriteTimeout);
    swUDPServerErrorFuncSet(server, onServerError);
    swUDPServerCloseFuncSet(server, onServerClose);

    if (swUDPServerStart(server, loop, address))
    {
      swUDPClient *client = swUDPClientNew();
      if (client)
      {
        // set callbacks and timeouts
        swUDPClientReconnectTimeoutSet(client, 1000);
        swUDPClientConnectedFuncSet(client, onClientConnected);
        swUDPClientCloseFuncSet(client, onClientClose);
        swUDPClientStopFuncSet(client, onClientStop);

        swUDPClientReadTimeoutSet(client, 1000);
        swUDPClientWriteTimeoutSet(client, 1000);
        swUDPClientReadReadyFuncSet(client, onClientReadReady);
        swUDPClientWriteReadyFuncSet(client, onClientWriteReady);
        swUDPClientReadTimeoutFuncSet(client, onClientReadTimeout);
        swUDPClientWriteTimeoutFuncSet(client, onClientWriteTimeout);
        swUDPClientErrorFuncSet(client, onClientError);

        if (swUDPClientStart(client, address, loop, bindAddress))
        {
          swEdgeLoopRun(loop, false);
          // swTestLogLine("server read = %zd, server written = %zd\n", serverBytesRead, serverBytesWritten);
          // swTestLogLine("client read = %zd, client written = %zd\n", clientBytesRead, clientBytesWritten);
          rtn = true;
          serverBytesRead = serverBytesWritten = clientBytesRead = clientBytesWritten = 0;
          memset(&serverReceiveAddress, 0, sizeof(serverReceiveAddress));
          swUDPClientStop(client);
        }
        swUDPClientDelete(client);
      }
      swUDPServerStop(server);
    }
    swUDPServerDelete(server);
  }
  return rtn;
}

swTestDeclare(UDPClientServerOverIP4Test, NULL, NULL, swTestRun)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  swSocketAddress address = { 0 };
  if (swSocketAddressInitInet(&address, "127.0.0.1", 10000))
    return runClientServerTest(&address, loop, NULL);
  return false;
}

swTestDeclare(UDPClientServerOverIP6Test, NULL, NULL, swTestRun)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  swSocketAddress address = { 0 };
  if (swSocketAddressInitInet6(&address, "::1", 10000))
    return runClientServerTest(&address, loop, NULL);
  return false;
}

swTestDeclare(UDPClientServerOverUnixTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  swSocketAddress address = { 0 };
  swSocketAddress bindAddress = { 0 };
  char *serverFilename = "/tmp/udp_server_socket";
  char *clientFilename = "/tmp/udp_client_socket";
  if (swSocketAddressInitUnix(&address, serverFilename) && swSocketAddressInitUnix(&bindAddress, clientFilename))
    rtn = runClientServerTest(&address, loop, &bindAddress);
  unlink(serverFilename);
  unlink(clientFilename);
  return rtn;
}

swTestSuiteStructDeclare(UDPClientServerTestSuite, edgeLoopSetup, edgeLoopTeardown, swTestRun,
                         &UDPClientServerOverIP4Test, &UDPClientServerOverIP6Test, &UDPClientServerOverUnixTest);
