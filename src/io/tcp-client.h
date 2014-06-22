#ifndef SW_IO_TCPCLIENT_H
#define SW_IO_TCPCLIENT_H

#include "tcp-connection.h"

typedef struct swTCPClient  swTCPClient;

typedef void (*swTCPClientConnectedFunc)(swTCPClient *client);
typedef void (*swTCPClientCloseFunc)(swTCPClient *client);
typedef void (*swTCPClientStopFunc)(swTCPClient *client);

struct swTCPClient
{
  swTCPConnection conn;
  swEdgeTimer connectTimer;
  swEdgeTimer reconnectTimer;
  swEdgeIO connectEvent;
  swEdgeLoop *loop;

  uint64_t connectTimeout;
  uint64_t reconnectTimeout;

  swTCPClientConnectedFunc connectedFunc;
  swTCPClientCloseFunc closeFunc;
  swTCPClientStopFunc stopFunc;

  unsigned int reconnect : 1;
  unsigned int connecting : 1;
  unsigned int cleaning : 1;
  unsigned int deleting : 1;
  unsigned int closing : 1;
};

swTCPClient *swTCPClientNew();
bool swTCPClientInit(swTCPClient *client);
void swTCPClientCleanup(swTCPClient *client);
void swTCPClientDelete(swTCPClient *client);

#define swTCPClientConnectTimeoutSet(c, t)    do { if ((c)) (c)->connectTimeout = (t); } while(0)
#define swTCPClientReconnectTimeoutSet(c, t)  do { if ((c)) (c)->reconnectTimeout = (t); } while(0)
#define swTCPClientConnectedFuncSet(c, f)     do { if ((c)) (c)->connectedFunc = (f); } while(0)
#define swTCPClientCloseFuncSet(c, f)         do { if ((c)) (c)->closeFunc = (f); } while(0)
#define swTCPClientStopFuncSet(c, f)          do { if ((c)) (c)->stopFunc = (f); } while(0)

bool swTCPClientStart(swTCPClient *client, swSocketAddress *address, swEdgeLoop *loop, swSocketAddress *bindAddress);
void swTCPClientStop(swTCPClient *client);
bool swTCPClientClose(swTCPClient *client);

#define swTCPClientRead(c, b, br)   swTCPConnectionRead((swTCPConnecton *)c, b, br)
#define swTCPClientWrite(c, b, bw)  swTCPConnectionWrite((swTCPConnecton *)c, b, bw)

#endif // SW_IO_TCPCLIENT_H
