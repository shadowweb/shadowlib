#ifndef SW_IO_TCPCLIENT_H
#define SW_IO_TCPCLIENT_H

#include "socket-io.h"

typedef struct swTCPClient  swTCPClient;

typedef void (*swTCPClientConnectedFunc)(swTCPClient *client);
typedef void (*swTCPClientCloseFunc)(swTCPClient *client);
typedef void (*swTCPClientStopFunc)(swTCPClient *client);

typedef void (*swTCPClientReadReadyFunc)    (swTCPClient *client);
typedef void (*swTCPClientWriteReadyFunc)   (swTCPClient *client);
typedef bool (*swTCPClientReadTimeoutFunc)  (swTCPClient *client);
typedef bool (*swTCPClientWriteTimeoutFunc) (swTCPClient *client);
typedef void (*swTCPClientErrorFunc)        (swTCPClient *client, swSocketIOErrorType errorCode);

struct swTCPClient
{
  swSocketIO io;
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

static inline swSocketReturnType swTCPClientRead  (swTCPClient *client, swStaticBuffer *buffer, ssize_t *bytesRead)     { return swSocketIORead    ((swSocketIO *)client, buffer, bytesRead);    }
static inline swSocketReturnType swTCPClientWrite (swTCPClient *client, swStaticBuffer *buffer, ssize_t *bytesWritten)  { return swSocketIOWrite   ((swSocketIO *)client, buffer, bytesWritten); }

static inline void *swTCPClientDataGet(swTCPClient *client)             { return swSocketIODataGet(client); }
static inline void  swTCPClientDataSet(swTCPClient *client, void *data) { swSocketIODataSet(client, data);  }

static inline void swTCPClientReadTimeoutSet      (swTCPClient *client, uint64_t timeout)                 { swSocketIOReadTimeoutSet     (client, timeout);                          }
static inline void swTCPClientWriteTimeoutSet     (swTCPClient *client, uint64_t timeout)                 { swSocketIOWriteTimeoutSet    (client, timeout);                          }
static inline void swTCPClientReadReadyFuncSet    (swTCPClient *client, swTCPClientReadReadyFunc func)    { swSocketIOReadReadyFuncSet   (client, (swSocketIOReadReadyFunc)func);    }
static inline void swTCPClientWriteReadyFuncSet   (swTCPClient *client, swTCPClientWriteReadyFunc func)   { swSocketIOWriteReadyFuncSet  (client, (swSocketIOWriteReadyFunc)func);   }
static inline void swTCPClientReadTimeoutFuncSet  (swTCPClient *client, swTCPClientReadTimeoutFunc func)  { swSocketIOReadTimeoutFuncSet (client, (swSocketIOReadTimeoutFunc)func);  }
static inline void swTCPClientWriteTimeoutFuncSet (swTCPClient *client, swTCPClientWriteTimeoutFunc func) { swSocketIOWriteTimeoutFuncSet(client, (swSocketIOWriteTimeoutFunc)func); }
static inline void swTCPClientErrorFuncSet        (swTCPClient *client, swTCPClientErrorFunc func)        { swSocketIOErrorFuncSet       (client, (swSocketIOErrorFunc)func);        }

#endif // SW_IO_TCPCLIENT_H
