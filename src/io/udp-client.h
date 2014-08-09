#ifndef SW_IO_UDPCLIENT_H
#define SW_IO_UDPCLIENT_H

#include "socket-io.h"

typedef struct swUDPClient  swUDPClient;

typedef void (*swUDPClientConnectedFunc)(swUDPClient *client);
typedef void (*swUDPClientCloseFunc)(swUDPClient *client);
typedef void (*swUDPClientStopFunc)(swUDPClient *client);

typedef void (*swUDPClientReadReadyFunc)    (swUDPClient *client);
typedef void (*swUDPClientWriteReadyFunc)   (swUDPClient *client);
typedef bool (*swUDPClientReadTimeoutFunc)  (swUDPClient *client);
typedef bool (*swUDPClientWriteTimeoutFunc) (swUDPClient *client);
typedef void (*swUDPClientErrorFunc)        (swUDPClient *client, swSocketIOErrorType errorCode);

struct swUDPClient
{
  swSocketIO io;
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

static inline swSocketReturnType swUDPClientRead    (swUDPClient *client, swStaticBuffer *buffer, ssize_t *bytesRead)                               { return swSocketIORead    ((swSocketIO *)client, buffer, bytesRead);             }
static inline swSocketReturnType swUDPClientWrite   (swUDPClient *client, swStaticBuffer *buffer, ssize_t *bytesWritten)                            { return swSocketIOWrite   ((swSocketIO *)client, buffer, bytesWritten);          }
static inline swSocketReturnType swUDPClientReadFrom(swUDPClient *client, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesRead)     { return swSocketIOReadFrom((swSocketIO *)client, buffer, address, bytesRead);    }
static inline swSocketReturnType swUDPClientWriteTo (swUDPClient *client, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesWritten)  { return swSocketIOWriteTo ((swSocketIO *)client, buffer, address, bytesWritten); }

static inline void *swUDPClientDataGet(swUDPClient *client)             { return swSocketIODataGet(client); }
static inline void  swUDPClientDataSet(swUDPClient *client, void *data) { swSocketIODataSet(client, data);  }

static inline void swUDPClientReadTimeoutSet      (swUDPClient *client, uint64_t timeout)                 { swSocketIOReadTimeoutSet     (client, timeout);                          }
static inline void swUDPClientWriteTimeoutSet     (swUDPClient *client, uint64_t timeout)                 { swSocketIOWriteTimeoutSet    (client, timeout);                          }
static inline void swUDPClientReadReadyFuncSet    (swUDPClient *client, swUDPClientReadReadyFunc func)    { swSocketIOReadReadyFuncSet   (client, (swSocketIOReadReadyFunc)func);    }
static inline void swUDPClientWriteReadyFuncSet   (swUDPClient *client, swUDPClientWriteReadyFunc func)   { swSocketIOWriteReadyFuncSet  (client, (swSocketIOWriteReadyFunc)func);   }
static inline void swUDPClientReadTimeoutFuncSet  (swUDPClient *client, swUDPClientReadTimeoutFunc func)  { swSocketIOReadTimeoutFuncSet (client, (swSocketIOReadTimeoutFunc)func);  }
static inline void swUDPClientWriteTimeoutFuncSet (swUDPClient *client, swUDPClientWriteTimeoutFunc func) { swSocketIOWriteTimeoutFuncSet(client, (swSocketIOWriteTimeoutFunc)func); }
static inline void swUDPClientErrorFuncSet        (swUDPClient *client, swUDPClientErrorFunc func)        { swSocketIOErrorFuncSet       (client, (swSocketIOErrorFunc)func);        }

#endif // SW_IO_UDPCLIENT_H
