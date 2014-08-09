#ifndef SW_OPENSSL_SSLCLIENT_H
#define SW_OPENSSL_SSLCLIENT_H

#include "open-ssl/ssl-socket-io.h"

typedef struct swSSLClient  swSSLClient;

typedef void (*swSSLClientConnectedFunc)(swSSLClient *client);
typedef void (*swSSLClientCloseFunc)(swSSLClient *client);
typedef void (*swSSLClientStopFunc)(swSSLClient *client);

typedef void (*swSSLClientReadReadyFunc)    (swSSLClient *client);
typedef void (*swSSLClientWriteReadyFunc)   (swSSLClient *client);
typedef bool (*swSSLClientReadTimeoutFunc)  (swSSLClient *client);
typedef bool (*swSSLClientWriteTimeoutFunc) (swSSLClient *client);
typedef void (*swSSLClientErrorFunc)        (swSSLClient *client, swSocketIOErrorType errorCode);

struct swSSLClient
{
  swSSLSocketIO io;
  swEdgeTimer connectTimer;
  swEdgeTimer reconnectTimer;
  swEdgeIO connectEvent;
  swEdgeLoop *loop;
  swSSLContext *context;

  uint64_t connectTimeout;
  uint64_t reconnectTimeout;

  swSSLClientConnectedFunc connectedFunc;
  swSSLClientCloseFunc closeFunc;
  swSSLClientStopFunc stopFunc;

  unsigned int reconnect : 1;
  unsigned int connecting : 1;
  unsigned int cleaning : 1;
  unsigned int deleting : 1;
  unsigned int closing : 1;
};

swSSLClient *swSSLClientNew(swSSLContext *context);
bool swSSLClientInit(swSSLClient *client, swSSLContext *context);
void swSSLClientCleanup(swSSLClient *client);
void swSSLClientDelete(swSSLClient *client);

#define swSSLClientConnectTimeoutSet(c, t)    do { if ((c)) (c)->connectTimeout = (t); } while(0)
#define swSSLClientReconnectTimeoutSet(c, t)  do { if ((c)) (c)->reconnectTimeout = (t); } while(0)
#define swSSLClientConnectedFuncSet(c, f)     do { if ((c)) (c)->connectedFunc = (f); } while(0)
#define swSSLClientCloseFuncSet(c, f)         do { if ((c)) (c)->closeFunc = (f); } while(0)
#define swSSLClientStopFuncSet(c, f)          do { if ((c)) (c)->stopFunc = (f); } while(0)

bool swSSLClientStart(swSSLClient *client, swSocketAddress *address, swEdgeLoop *loop, swSocketAddress *bindAddress);
void swSSLClientStop(swSSLClient *client);
bool swSSLClientClose(swSSLClient *client);

static inline swSocketReturnType swSSLClientRead  (swSSLClient *client, swStaticBuffer *buffer, ssize_t *bytesRead)     { return swSSLSocketIORead    ((swSSLSocketIO *)client, buffer, bytesRead);    }
static inline swSocketReturnType swSSLClientWrite (swSSLClient *client, swStaticBuffer *buffer, ssize_t *bytesWritten)  { return swSSLSocketIOWrite   ((swSSLSocketIO *)client, buffer, bytesWritten); }

static inline void *swSSLClientDataGet(swSSLClient *client)             { return swSocketIODataGet(client); }
static inline void  swSSLClientDataSet(swSSLClient *client, void *data) { swSocketIODataSet(client, data);  }

static inline void swSSLClientReadTimeoutSet      (swSSLClient *client, uint64_t timeout)                 { swSSLSocketIOReadTimeoutSet     ((swSSLSocketIO *)client, timeout);                             }
static inline void swSSLClientWriteTimeoutSet     (swSSLClient *client, uint64_t timeout)                 { swSSLSocketIOWriteTimeoutSet    ((swSSLSocketIO *)client, timeout);                             }
static inline void swSSLClientReadReadyFuncSet    (swSSLClient *client, swSSLClientReadReadyFunc func)    { swSSLSocketIOReadReadyFuncSet   ((swSSLSocketIO *)client, (swSSLSocketIOReadReadyFunc)func);    }
static inline void swSSLClientWriteReadyFuncSet   (swSSLClient *client, swSSLClientWriteReadyFunc func)   { swSSLSocketIOWriteReadyFuncSet  ((swSSLSocketIO *)client, (swSSLSocketIOWriteReadyFunc)func);   }
static inline void swSSLClientReadTimeoutFuncSet  (swSSLClient *client, swSSLClientReadTimeoutFunc func)  { swSSLSocketIOReadTimeoutFuncSet ((swSSLSocketIO *)client, (swSSLSocketIOReadTimeoutFunc)func);  }
static inline void swSSLClientWriteTimeoutFuncSet (swSSLClient *client, swSSLClientWriteTimeoutFunc func) { swSSLSocketIOWriteTimeoutFuncSet((swSSLSocketIO *)client, (swSSLSocketIOWriteTimeoutFunc)func); }
static inline void swSSLClientErrorFuncSet        (swSSLClient *client, swSSLClientErrorFunc func)        { swSSLSocketIOErrorFuncSet       ((swSSLSocketIO *)client, (swSSLSocketIOErrorFunc)func);        }

#endif // SW_OPENSSL_SSLCLIENT_H
