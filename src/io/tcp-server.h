#ifndef SW_IO_TCPSERVER_H
#define SW_IO_TCPSERVER_H

#include "tcp-connection.h"

typedef struct swTCPServer  swTCPServer;

typedef bool (*swTCPServerAcceptFunc)(swTCPServer *server);
typedef void (*swTCPServerStopFunc)(swTCPServer *server);
typedef void (*swTCPServerErrorFunc)(swTCPServer *server, swTCPConnectionErrorType errorCode);
typedef bool (*swTCPServerConnectionSetupFunc)(swTCPServer *server, swTCPConnection *conn);

struct swTCPServer
{
  swSocket socket;
  swEdgeIO acceptEvent;
  swEdgeLoop *loop;
  void *data;

  swTCPServerAcceptFunc acceptFunc;
  swTCPServerStopFunc stopFunc;
  swTCPServerErrorFunc errorFunc;
  swTCPServerConnectionSetupFunc setupFunc;
};

swTCPServer *swTCPServerNew();
bool swTCPServerInit(swTCPServer *server);
void swTCPServerCleanup(swTCPServer *server);
void swTCPServerDelete(swTCPServer *server);

bool swTCPServerStart(swTCPServer *server, swEdgeLoop *loop, swSocketAddress *address);
void swTCPServerStop(swTCPServer *server);

#define swTCPServerAcceptFuncSet(s, f)      do { if ((s)) (s)->acceptFunc = (f); } while(0)
#define swTCPServerStopFuncSet(s, f)        do { if ((s)) (s)->stopFunc = (f); } while(0)
#define swTCPServerErrorFuncSet(s, f)       do { if ((s)) (s)->errorFunc = (f); } while(0)
#define swTCPServerSetupFuncSet(s, f)       do { if ((s)) (s)->setupFunc = (f); } while(0)

static inline void *swTCPServerDataGet(swTCPServer *server)
{
  if (server)
    return server->data;
  return NULL;
}

static inline void swTCPServerDataSet(swTCPServer *server, void *data)
{
  if (server)
    server->data = data;
}

#endif // SW_IO_TCPSERVER_H
