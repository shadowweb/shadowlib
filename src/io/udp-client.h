#ifndef SW_IO_UDPCLIENT_H
#define SW_IO_UDPCLIENT_H

#include "tcp-connection.h"

// TODO: rename swTCPConnnnection into swSocketIO
// TODO: fix inheritance in swTCPClient and swTCPServer

typedef struct swUDPClient  swUDPClient;

typedef void (*swUDPClientConnectedFunc)(swUDPClient *client);
typedef void (*swUDPClientCloseFunc)(swUDPClient *client);
typedef void (*swUDPClientStopFunc)(swUDPClient *client);

typedef void (*swUDPClientReadReadyFunc)    (swUDPClient *client);
typedef void (*swUDPClientWriteReadyFunc)   (swUDPClient *client);
typedef bool (*swUDPClientReadTimeoutFunc)  (swUDPClient *client);
typedef bool (*swUDPClientWriteTimeoutFunc) (swUDPClient *client);
typedef void (*swUDPClientErrorFunc)        (swUDPClient *client, swTCPConnectionErrorType errorCode);

struct swUDPClient
{
  swTCPConnection conn;
  swEdgeTimer reconnectTimer;
  swEdgeLoop *loop;

  uint64_t reconnectTimeout;

  swUDPClientConnectedFunc connectedFunc;
  swUDPClientCloseFunc closeFunc;
  swUDPClientStopFunc stopFunc;

  unsigned int reconnect : 1;
  unsigned int cleaning : 1;
  unsigned int deleting : 1;
  unsigned int closing : 1;
};

swUDPClient *swUDPClientNew();
bool swUDPClientInit(swUDPClient *client);
void swUDPClientCleanup(swUDPClient *client);
void swUDPClientDelete(swUDPClient *client);

#define swUDPClientReconnectTimeoutSet(c, t)  do { if ((c)) (c)->reconnectTimeout = (t); } while(0)
#define swUDPClientConnectedFuncSet(c, f)     do { if ((c)) (c)->connectedFunc = (f); } while(0)
#define swUDPClientCloseFuncSet(c, f)         do { if ((c)) (c)->closeFunc = (f); } while(0)
#define swUDPClientStopFuncSet(c, f)          do { if ((c)) (c)->stopFunc = (f); } while(0)

bool swUDPClientStart(swUDPClient *client, swSocketAddress *address, swEdgeLoop *loop, swSocketAddress *bindAddress);
void swUDPClientStop(swUDPClient *client);

static inline swSocketReturnType swUDPClientRead    (swUDPClient *client, swStaticBuffer *buffer, ssize_t *bytesRead)                               { return swTCPConnectionRead    ((swTCPConnection *)client, buffer, bytesRead);             }
static inline swSocketReturnType swUDPClientWrite   (swUDPClient *client, swStaticBuffer *buffer, ssize_t *bytesWritten)                            { return swTCPConnectionWrite   ((swTCPConnection *)client, buffer, bytesWritten);          }
static inline swSocketReturnType swUDPClientReadFrom(swUDPClient *client, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesRead)     { return swTCPConnectionReadFrom((swTCPConnection *)client, buffer, address, bytesRead);    }
static inline swSocketReturnType swUDPClientWriteTo (swUDPClient *client, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesWritten)  { return swTCPConnectionWriteTo ((swTCPConnection *)client, buffer, address, bytesWritten); }

static inline void *swUDPClientDataGet(swUDPClient *client)             { return swTCPConnectionDataGet((swTCPConnection *)(client)); }
static inline void  swUDPClientDataSet(swUDPClient *client, void *data) { swTCPConnectionDataSet((swTCPConnection *)(client), data);  }

static inline void swUDPClientReadTimeoutSet      (swUDPClient *client, uint64_t timeout)                 { swTCPConnectionReadTimeoutSet     (client, timeout);                               }
static inline void swUDPClientWriteTimeoutSet     (swUDPClient *client, uint64_t timeout)                 { swTCPConnectionWriteTimeoutSet    (client, timeout);                               }
static inline void swUDPClientReadReadyFuncSet    (swUDPClient *client, swUDPClientReadReadyFunc func)    { swTCPConnectionReadReadyFuncSet   (client, (swTCPConnectionReadReadyFunc)func);    }
static inline void swUDPClientWriteReadyFuncSet   (swUDPClient *client, swUDPClientWriteReadyFunc func)   { swTCPConnectionWriteReadyFuncSet  (client, (swTCPConnectionWriteReadyFunc)func);   }
static inline void swUDPClientReadTimeoutFuncSet  (swUDPClient *client, swUDPClientReadTimeoutFunc func)  { swTCPConnectionReadTimeoutFuncSet (client, (swTCPConnectionReadTimeoutFunc)func);  }
static inline void swUDPClientWriteTimeoutFuncSet (swUDPClient *client, swUDPClientWriteTimeoutFunc func) { swTCPConnectionWriteTimeoutFuncSet(client, (swTCPConnectionWriteTimeoutFunc)func); }
static inline void swUDPClientErrorFuncSet        (swUDPClient *client, swUDPClientErrorFunc func)        { swTCPConnectionErrorFuncSet       (client, (swTCPConnectionErrorFunc)func);        }

#endif // SW_IO_UDPCLIENT_H
