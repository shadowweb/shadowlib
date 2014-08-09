#ifndef SW_OPENSSL_SSLSERVER_H
#define SW_OPENSSL_SSLSERVER_H

#include "open-ssl/ssl-socket-io.h"
#include "open-ssl/ssl-context.h"
#include "open-ssl/ssl.h"

typedef struct swSSLServerAcceptor  swSSLServerAcceptor;
typedef struct swSSLServer  swSSLServer;

typedef bool (*swSSLServerAcceptorAcceptFunc)       (swSSLServerAcceptor *serverAcceptor);
typedef void (*swSSLServerAcceptorStopFunc)         (swSSLServerAcceptor *serverAcceptor);
typedef void (*swSSLServerAcceptorErrorFunc)        (swSSLServerAcceptor *serverAcceptor, swSocketIOErrorType errorCode);
typedef bool (*swSSLServerAcceptorServerSetupFunc)  (swSSLServerAcceptor *serverAcceptor, swSSLServer *server);

struct swSSLServerAcceptor
{
  swSocket socket;
  swEdgeIO acceptEvent;
  swEdgeLoop *loop;
  swSSLContext *context;
  void *data;

  swSSLServerAcceptorAcceptFunc       acceptFunc;
  swSSLServerAcceptorStopFunc         stopFunc;
  swSSLServerAcceptorErrorFunc        errorFunc;
  swSSLServerAcceptorServerSetupFunc  setupFunc;
};

swSSLServerAcceptor *swSSLServerAcceptorNew(swSSLContext *context);
bool swSSLServerAcceptorInit    (swSSLServerAcceptor *serverAcceptor, swSSLContext *context);
void swSSLServerAcceptorCleanup (swSSLServerAcceptor *serverAcceptor);
void swSSLServerAcceptorDelete  (swSSLServerAcceptor *serverAcceptor);

bool swSSLServerAcceptorStart (swSSLServerAcceptor *serverAcceptor, swEdgeLoop *loop, swSocketAddress *address);
void swSSLServerAcceptorStop  (swSSLServerAcceptor *serverAcceptor);

#define swSSLServerAcceptorAcceptFuncSet(s, f)      do { if ((s)) (s)->acceptFunc = (f); } while(0)
#define swSSLServerAcceptorStopFuncSet(s, f)        do { if ((s)) (s)->stopFunc = (f); } while(0)
#define swSSLServerAcceptorErrorFuncSet(s, f)       do { if ((s)) (s)->errorFunc = (f); } while(0)
#define swSSLServerAcceptorSetupFuncSet(s, f)       do { if ((s)) (s)->setupFunc = (f); } while(0)

static inline void *swSSLServerAcceptorDataGet(swSSLServerAcceptor *serverAcceptor)
{
  if (serverAcceptor)
    return serverAcceptor->data;
  return NULL;
}

static inline void swSSLServerAcceptorDataSet(swSSLServerAcceptor *serverAcceptor, void *data)
{
  if (serverAcceptor)
    serverAcceptor->data = data;
}

typedef void (*swSSLServerReadReadyFunc)    (swSSLServer *server);
typedef void (*swSSLServerWriteReadyFunc)   (swSSLServer *server);
typedef bool (*swSSLServerReadTimeoutFunc)  (swSSLServer *server);
typedef bool (*swSSLServerWriteTimeoutFunc) (swSSLServer *server);
typedef void (*swSSLServerErrorFunc)        (swSSLServer *server, swSocketIOErrorType errorCode);
typedef void (*swSSLServerCloseFunc)        (swSSLServer *server);

struct swSSLServer
{
  swSSLSocketIO io;
};

static inline swSSLServer *swSSLServerNew()                 { return (swSSLServer *)swSSLSocketIONew();           }
static inline bool swSSLServerInit    (swSSLServer *server) { return swSSLSocketIOInit((swSSLSocketIO *)server);  }
static inline void swSSLServerCleanup (swSSLServer *server) { swSSLSocketIOCleanup((swSSLSocketIO *)server);      }
static inline void swSSLServerDelete  (swSSLServer *server) { swSSLSocketIODelete((swSSLSocketIO *)server);       }

static inline bool swSSLServerStart (swSSLServer *server, swEdgeLoop *loop) { return swSSLSocketIOStart((swSSLSocketIO *)server, loop);             }
static inline void swSSLServerStop  (swSSLServer *server)                   { swSSLSocketIOClose((swSSLSocketIO *)server, swSocketIOErrorNone);  }

static inline swSocketReturnType swSSLServerRead  (swSSLServer *server, swStaticBuffer *buffer, ssize_t *bytesRead)     { return swSSLSocketIORead  ((swSSLSocketIO *)server, buffer, bytesRead);    }
static inline swSocketReturnType swSSLServerWrite (swSSLServer *server, swStaticBuffer *buffer, ssize_t *bytesWritten)  { return swSSLSocketIOWrite ((swSSLSocketIO *)server, buffer, bytesWritten); }

static inline void *swSSLServerDataGet(swSSLServer *server)             { return swSocketIODataGet(server); }
static inline void  swSSLServerDataSet(swSSLServer *server, void *data) { swSocketIODataSet(server, data);  }

static inline void swSSLServerReadTimeoutSet      (swSSLServer *server, uint64_t timeout)                 { swSSLSocketIOReadTimeoutSet     ((swSSLSocketIO *)server, timeout);                             }
static inline void swSSLServerWriteTimeoutSet     (swSSLServer *server, uint64_t timeout)                 { swSSLSocketIOWriteTimeoutSet    ((swSSLSocketIO *)server, timeout);                             }
static inline void swSSLServerReadReadyFuncSet    (swSSLServer *server, swSSLServerReadReadyFunc func)    { swSSLSocketIOReadReadyFuncSet   ((swSSLSocketIO *)server, (swSSLSocketIOReadReadyFunc)func);    }
static inline void swSSLServerWriteReadyFuncSet   (swSSLServer *server, swSSLServerWriteReadyFunc func)   { swSSLSocketIOWriteReadyFuncSet  ((swSSLSocketIO *)server, (swSSLSocketIOWriteReadyFunc)func);   }
static inline void swSSLServerReadTimeoutFuncSet  (swSSLServer *server, swSSLServerReadTimeoutFunc func)  { swSSLSocketIOReadTimeoutFuncSet ((swSSLSocketIO *)server, (swSSLSocketIOReadTimeoutFunc)func);  }
static inline void swSSLServerWriteTimeoutFuncSet (swSSLServer *server, swSSLServerWriteTimeoutFunc func) { swSSLSocketIOWriteTimeoutFuncSet((swSSLSocketIO *)server, (swSSLSocketIOWriteTimeoutFunc)func); }
static inline void swSSLServerErrorFuncSet        (swSSLServer *server, swSSLServerErrorFunc func)        { swSSLSocketIOErrorFuncSet       ((swSSLSocketIO *)server, (swSSLSocketIOErrorFunc)func);        }
static inline void swSSLServerCloseFuncSet        (swSSLServer *server, swSSLServerCloseFunc func)        { swSSLSocketIOCloseFuncSet       ((swSSLSocketIO *)server, (swSSLSocketIOCloseFunc)func);        }

#endif // SW_OPENSSL_SSLSERVER_H
