#include "open-ssl/init.h"
#include "open-ssl/ssl-client.h"
#include "open-ssl/ssl-server.h"
#include "utils/file.h"
#include "core/memory.h"

#include "unittest/unittest.h"

#include <signal.h>
#include <unistd.h>

void edgeLoopSetup(swTestSuite *suite)
{
  signal(SIGPIPE, SIG_IGN);
  swTestLogLine("Creating loop ...\n");
  swEdgeLoop *loop = swEdgeLoopNew();
  ASSERT_NOT_NULL(loop);
  ASSERT_TRUE(swOpenSSLStart());
  swTestSuiteDataSet(suite, loop);
}

void edgeLoopTeardown(swTestSuite *suite)
{
  swTestLogLine("Deleting loop ...\n");
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(loop);
  swEdgeLoopDelete(loop);
  swOpenSSLStop();
}

#define BUFFER_SIZE   2048
#define EXPECT_BYTES  204800

static uint8_t serverReadBuffer[BUFFER_SIZE] = {0};
static uint8_t serverWriteBuffer[BUFFER_SIZE] = {0};
static uint8_t clientReadBuffer[BUFFER_SIZE] = {0};
static uint8_t clientWriteBuffer[BUFFER_SIZE] = {0};
static ssize_t serverBytesRead = 0;
static ssize_t serverBytesWritten = 0;
static ssize_t clientBytesRead = 0;
static ssize_t clientBytesWritten = 0;
static swSSLServer *serverConnection = NULL;

void onServerReadReady(swSSLServer *server)
{
  // swTestLogLine("Server: read ready, bytes read = %zd\n", serverBytesRead);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(serverReadBuffer);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swSSLServerRead(server, &buffer, &bytesRead);
    if (ret != swSocketReturnOK)
      break;
    serverBytesRead += bytesRead;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady && ret != swSocketReturnReadNotReady && ret!= swSocketReturnWriteNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(server->io.socketIO.loop);
  }
  else if (serverBytesRead > EXPECT_BYTES && clientBytesRead > EXPECT_BYTES)
    swEdgeLoopBreak(server->io.socketIO.loop);
}

void onServerWriteReady(swSSLServer *server)
{
  // swTestLogLine("Server: write ready, bytes written = %zd\n", serverBytesWritten);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(serverWriteBuffer);
  ssize_t bytesWritten = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swSSLServerWrite(server, &buffer, &bytesWritten);
    if (ret != swSocketReturnOK)
      break;
    serverBytesWritten += bytesWritten;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady && ret != swSocketReturnReadNotReady && ret!= swSocketReturnWriteNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(server->io.socketIO.loop);
  }
}

bool onServerReadTimeout(swSSLServer *server)
{
  swTestLogLine("Server: read timeout\n");
  return false;
}

bool onServerWriteTimeout(swSSLServer *server)
{
  swTestLogLine("Server: write timeout\n");
  return false;
}

void onServerError(swSSLServer *server, swSocketIOErrorType errorCode)
{
  swTestLogLine("Server: error \"%s\"\n", swSocketIOErrorTextGet(errorCode));
}

void onServerClose(swSSLServer *server)
{
  // swTestLogLine("Server: close\n");
  swSSLServerDelete(server);
  serverConnection = NULL;
}

bool onAccept(swSSLServerAcceptor *serverAcceptor)
{
  static uint32_t lifetimeServerConnectionsCount = 0;
  lifetimeServerConnectionsCount++;
  // swTestLogLine("Acceptor: accepting connection %u\n", lifetimeServerConnectionsCount);
  return true;
}

void onStop(swSSLServerAcceptor *serverAcceptor)
{
  // swTestLogLine("Acceptor: stopping\n");
}

void onError(swSSLServerAcceptor *serverAcceptor, swSocketIOErrorType errorCode)
{
  swTestLogLine("Acceptor: error \"%s\"\n", swSocketIOErrorTextGet(errorCode));
}

bool onConnectionSetup(swSSLServerAcceptor *serverAcceptor, swSSLServer *server)
{
  bool rtn = false;
  // swTestLogLine("Server: new connection setup\n");
  if (server)
  {
    serverConnection = server;
    swSSLServerReadTimeoutSet     (server, 1000);
    swSSLServerWriteTimeoutSet    (server, 1000);
    swSSLServerReadReadyFuncSet   (server, onServerReadReady);
    swSSLServerWriteReadyFuncSet  (server, onServerWriteReady);
    swSSLServerReadTimeoutFuncSet (server, onServerReadTimeout);
    swSSLServerWriteTimeoutFuncSet(server, onServerWriteTimeout);
    swSSLServerErrorFuncSet       (server, onServerError);
    swSSLServerCloseFuncSet       (server, onServerClose);
    rtn = true;
  }
  ASSERT_TRUE(rtn);
  return rtn;
}

void onClientConnected(swSSLClient *client)
{
  // swTestLogLine("Client: connected\n");
}

void onClientClose(swSSLClient *client)
{
  // swTestLogLine("Client: close\n");
}

void onClientStop(swSSLClient *client)
{
  // swTestLogLine("Client: stop\n");
}

void onClientReadReady(swSSLClient *client)
{
  // swTestLogLine("Client: read ready, bytes read = %zd\n", clientBytesRead);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(clientReadBuffer);
  ssize_t bytesRead = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swSSLClientRead(client, &buffer, &bytesRead);
    if (ret != swSocketReturnOK)
      break;
    clientBytesRead += bytesRead;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady && ret != swSocketReturnReadNotReady && ret!= swSocketReturnWriteNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(client->loop);
  }
  else if (serverBytesRead > EXPECT_BYTES && clientBytesRead > EXPECT_BYTES)
    swEdgeLoopBreak(client->loop);
}

void onClientWriteReady(swSSLClient *client)
{
  // swTestLogLine("Client: write ready, bytes written = %zd\n", clientBytesWritten);
  swSocketReturnType ret = swSocketReturnNone;
  swStaticBuffer buffer = swStaticBufferDefine(clientWriteBuffer);
  ssize_t bytesWritten = 0;
  for (uint32_t i = 0; i < 10; i++)
  {
    ret = swSSLClientWrite(client, &buffer, &bytesWritten);
    if (ret != swSocketReturnOK)
      break;
    clientBytesWritten += bytesWritten;
  }
  if (ret != swSocketReturnOK && ret != swSocketReturnNotReady && ret != swSocketReturnReadNotReady && ret!= swSocketReturnWriteNotReady)
  {
    ASSERT_FAIL();
    swEdgeLoopBreak(client->loop);
  }
}

bool onClientReadTimeout(swSSLClient *client)
{
  swTestLogLine("Client: read timeout\n");
  return false;
}

bool onClientWriteTimeout(swSSLClient *client)
{
  swTestLogLine("Client: write timeout\n");
  return false;
}

void onClientError(swSSLClient *client, swSocketIOErrorType errorCode)
{
  swTestLogLine("Client: error \"%s\"\n", swSocketIOErrorTextGet(errorCode));
}

swSSLContext *createServerContext()
{
  swSSLContext *rtn = NULL;
  swSSLContext *serverSSLContext = swSSLContextNew(swSSLMethodTLSV12, swSSLMethodModeServer);
  if (serverSSLContext)
  {
    swStaticString certificateData = swStaticStringDefineEmpty;
    if ((certificateData.len = swFileRead("data/rsa-2048-cert.pem", &(certificateData.data))))
    {
      swSSLCertificate *certificate = swSSLCertificateNewFromPEM(&certificateData);
      if (certificate)
      {
        if (swSSLContextSetCertificate(serverSSLContext, certificate))
        {
          swStaticString keyData = swStaticStringDefineEmpty;
          if ((keyData.len = swFileRead("data/rsa-2048-key.pem", &(keyData.data))))
          {
            swSSLKey *key = swSSLKeyNewFromPEM(&keyData);
            if (key)
            {
              if (swSSLContextSetKey(serverSSLContext, key))
              {
                rtn = serverSSLContext;
              }
              swSSLKeyDelete(key);
            }
            swMemoryFree(keyData.data);
          }
        }
        swSSLCertificateDelete(certificate);
      }
      swMemoryFree(certificateData.data);
    }
    if (!rtn)
      swSSLContextDelete(serverSSLContext);
  }
  return rtn;
}

bool runClientServerTest(swSocketAddress *address,  swEdgeLoop *loop)
{
  ASSERT_NOT_NULL(loop);
  ASSERT_NOT_NULL(address);
  bool rtn = false;
  swSSLContext *serverSSLContext = createServerContext();
  if (serverSSLContext)
  {
    swSSLServerAcceptor *serverAcceptor = swSSLServerAcceptorNew(serverSSLContext);
    if (serverAcceptor)
    {
      // set callbacks
      swSSLServerAcceptorAcceptFuncSet  (serverAcceptor, onAccept);
      swSSLServerAcceptorStopFuncSet    (serverAcceptor, onStop);
      swSSLServerAcceptorErrorFuncSet   (serverAcceptor, onError);
      swSSLServerAcceptorSetupFuncSet   (serverAcceptor, onConnectionSetup);

      if (swSSLServerAcceptorStart(serverAcceptor, loop, address))
      {
        swSSLContext *clientSSLContext = swSSLContextNew(swSSLMethodTLSV12, swSSLMethodModeClient);
        if (clientSSLContext)
        {
          swSSLClient *client = swSSLClientNew(clientSSLContext);
          if (client)
          {
            // set callbacks and timeouts
            swSSLClientConnectTimeoutSet(client, 1000);
            swSSLClientReconnectTimeoutSet(client, 1000);
            swSSLClientConnectedFuncSet(client, onClientConnected);
            swSSLClientCloseFuncSet(client, onClientClose);
            swSSLClientStopFuncSet(client, onClientStop);

            swSSLClientReadTimeoutSet(client, 1000);
            swSSLClientWriteTimeoutSet(client, 1000);
            swSSLClientReadReadyFuncSet(client, onClientReadReady);
            swSSLClientWriteReadyFuncSet(client, onClientWriteReady);
            swSSLClientReadTimeoutFuncSet(client, onClientReadTimeout);
            swSSLClientWriteTimeoutFuncSet(client, onClientWriteTimeout);
            swSSLClientErrorFuncSet(client, onClientError);

            if (swSSLClientStart(client, address, loop, NULL))
            {
              swEdgeLoopRun(loop, false);
              // swTestLogLine("server read = %zd, server written = %zd\n", serverBytesRead, serverBytesWritten);
              // swTestLogLine("client read = %zd, client written = %zd\n", clientBytesRead, clientBytesWritten);
              rtn = true;
              serverBytesRead = serverBytesWritten = clientBytesRead = clientBytesWritten = 0;
              if (serverConnection)
              {
                swSSLServerDelete(serverConnection);
                serverConnection = NULL;
              }
              swSSLClientStop(client);
            }
            swSSLClientDelete(client);
          }
          swSSLContextDelete(clientSSLContext);
        }
        swSSLServerAcceptorStop(serverAcceptor);
      }
      swSSLServerAcceptorDelete(serverAcceptor);
    }
    swSSLContextDelete(serverSSLContext);
  }
  return rtn;
}

swTestDeclare(SSLClientServerOverIP4Test, NULL, NULL, swTestRun)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  swSocketAddress address = { 0 };
  if (swSocketAddressInitInet(&address, "127.0.0.1", 10000))
    return runClientServerTest(&address, loop);
  return false;
}

swTestDeclare(SSLClientServerOverIP6Test, NULL, NULL, swTestRun)
{
  swEdgeLoop *loop = swTestSuiteDataGet(suite);
  swSocketAddress address = { 0 };
  if (swSocketAddressInitInet6(&address, "::1", 10000))
    return runClientServerTest(&address, loop);
  return false;
}

swTestDeclare(SSLClientServerOverUnixTest, NULL, NULL, swTestRun)
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

swTestSuiteStructDeclare(SSLClientServerTestSuite, edgeLoopSetup, edgeLoopTeardown, swTestRun,
                         &SSLClientServerOverIP4Test, &SSLClientServerOverIP6Test, &SSLClientServerOverUnixTest);
