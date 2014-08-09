#ifndef SW_IO_TCPSERVER_H
#define SW_IO_TCPSERVER_H

#include "socket-io.h"

typedef struct swTCPServerAcceptor  swTCPServerAcceptor;
typedef struct swTCPServer  swTCPServer;

typedef bool (*swTCPServerAcceptorAcceptFunc)       (swTCPServerAcceptor *serverAcceptor);
typedef void (*swTCPServerAcceptorStopFunc)         (swTCPServerAcceptor *serverAcceptor);
typedef void (*swTCPServerAcceptorErrorFunc)        (swTCPServerAcceptor *serverAcceptor, swSocketIOErrorType errorCode);
typedef bool (*swTCPServerAcceptorServerSetupFunc)  (swTCPServerAcceptor *serverAcceptor, swTCPServer *server);

struct swTCPServerAcceptor
{
  swSocket socket;
  swEdgeIO acceptEvent;
  swEdgeLoop *loop;
  void *data;

  swTCPServerAcceptorAcceptFunc       acceptFunc;
  swTCPServerAcceptorStopFunc         stopFunc;
  swTCPServerAcceptorErrorFunc        errorFunc;
  swTCPServerAcceptorServerSetupFunc  setupFunc;
};

swTCPServerAcceptor *swTCPServerAcceptorNew();
bool swTCPServerAcceptorInit    (swTCPServerAcceptor *serverAcceptor);
void swTCPServerAcceptorCleanup (swTCPServerAcceptor *serverAcceptor);
void swTCPServerAcceptorDelete  (swTCPServerAcceptor *serverAcceptor);

bool swTCPServerAcceptorStart (swTCPServerAcceptor *serverAcceptor, swEdgeLoop *loop, swSocketAddress *address);
void swTCPServerAcceptorStop  (swTCPServerAcceptor *serverAcceptor);

#define swTCPServerAcceptorAcceptFuncSet(s, f)      do { if ((s)) (s)->acceptFunc = (f); } while(0)
#define swTCPServerAcceptorStopFuncSet(s, f)        do { if ((s)) (s)->stopFunc = (f); } while(0)
#define swTCPServerAcceptorErrorFuncSet(s, f)       do { if ((s)) (s)->errorFunc = (f); } while(0)
#define swTCPServerAcceptorSetupFuncSet(s, f)       do { if ((s)) (s)->setupFunc = (f); } while(0)

static inline void *swTCPServerAcceptorDataGet(swTCPServerAcceptor *serverAcceptor)
{
  if (serverAcceptor)
    return serverAcceptor->data;
  return NULL;
}

static inline void swTCPServerAcceptorDataSet(swTCPServerAcceptor *serverAcceptor, void *data)
{
  if (serverAcceptor)
    serverAcceptor->data = data;
}

typedef void (*swTCPServerReadReadyFunc)    (swTCPServer *server);
typedef void (*swTCPServerWriteReadyFunc)   (swTCPServer *server);
typedef bool (*swTCPServerReadTimeoutFunc)  (swTCPServer *server);
typedef bool (*swTCPServerWriteTimeoutFunc) (swTCPServer *server);
typedef void (*swTCPServerErrorFunc)        (swTCPServer *server, swSocketIOErrorType errorCode);
typedef void (*swTCPServerCloseFunc)        (swTCPServer *server);

struct swTCPServer
{
  swSocketIO io;
};

static inline swTCPServer *swTCPServerNew()                 { return (swTCPServer *)swSocketIONew();        }
static inline bool swTCPServerInit    (swTCPServer *server) { return swSocketIOInit((swSocketIO *)server);  }
static inline void swTCPServerCleanup (swTCPServer *server) { swSocketIOCleanup((swSocketIO *)server);      }
static inline void swTCPServerDelete  (swTCPServer *server) { swSocketIODelete((swSocketIO *)server);       }

static inline bool swTCPServerStart (swTCPServer *server, swEdgeLoop *loop) { return swSocketIOStart((swSocketIO *)server, loop); }
static inline void swTCPServerStop  (swTCPServer *server) { swSocketIOClose((swSocketIO *)server, swSocketIOErrorNone); }

static inline swSocketReturnType swTCPServerRead  (swTCPServer *server, swStaticBuffer *buffer, ssize_t *bytesRead)     { return swSocketIORead  ((swSocketIO *)server, buffer, bytesRead);    }
static inline swSocketReturnType swTCPServerWrite (swTCPServer *server, swStaticBuffer *buffer, ssize_t *bytesWritten)  { return swSocketIOWrite ((swSocketIO *)server, buffer, bytesWritten); }

static inline void *swTCPServerDataGet(swTCPServer *server)            { return swSocketIODataGet(server); }
static inline void swTCPServerDataSet(swTCPServer *server, void *data) { swSocketIODataSet(server, data);  }

static inline void swTCPServerReadTimeoutSet      (swTCPServer *server, uint64_t timeout)                 { swSocketIOReadTimeoutSet     (server, timeout);                          }
static inline void swTCPServerWriteTimeoutSet     (swTCPServer *server, uint64_t timeout)                 { swSocketIOWriteTimeoutSet    (server, timeout);                          }
static inline void swTCPServerReadReadyFuncSet    (swTCPServer *server, swTCPServerReadReadyFunc func)    { swSocketIOReadReadyFuncSet   (server, (swSocketIOReadReadyFunc)func);    }
static inline void swTCPServerWriteReadyFuncSet   (swTCPServer *server, swTCPServerWriteReadyFunc func)   { swSocketIOWriteReadyFuncSet  (server, (swSocketIOWriteReadyFunc)func);   }
static inline void swTCPServerReadTimeoutFuncSet  (swTCPServer *server, swTCPServerReadTimeoutFunc func)  { swSocketIOReadTimeoutFuncSet (server, (swSocketIOReadTimeoutFunc)func);  }
static inline void swTCPServerWriteTimeoutFuncSet (swTCPServer *server, swTCPServerWriteTimeoutFunc func) { swSocketIOWriteTimeoutFuncSet(server, (swSocketIOWriteTimeoutFunc)func); }
static inline void swTCPServerErrorFuncSet        (swTCPServer *server, swTCPServerErrorFunc func)        { swSocketIOErrorFuncSet       (server, (swSocketIOErrorFunc)func);        }
static inline void swTCPServerCloseFuncSet        (swTCPServer *server, swTCPServerCloseFunc func)        { swSocketIOCloseFuncSet       (server, (swSocketIOCloseFunc)func);        }

#endif // SW_IO_TCPSERVER_H
