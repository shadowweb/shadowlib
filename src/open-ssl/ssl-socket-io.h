#ifndef SW_OPENSSL_SSLSOCKETIO_H
#define SW_OPENSSL_SSLSOCKETIO_H

#include "io/socket-io.h"
#include "open-ssl/ssl.h"

typedef struct swSSLSocketIO  swSSLSocketIO;

typedef void (*swSSLSocketIOReadReadyFunc)   (swSSLSocketIO *io);
typedef void (*swSSLSocketIOWriteReadyFunc)  (swSSLSocketIO *io);
typedef bool (*swSSLSocketIOReadTimeoutFunc) (swSSLSocketIO *io);
typedef bool (*swSSLSocketIOWriteTimeoutFunc)(swSSLSocketIO *io);
typedef void (*swSSLSocketIOErrorFunc)       (swSSLSocketIO *io, swSocketIOErrorType errorCode);
typedef void (*swSSLSocketIOCloseFunc)       (swSSLSocketIO *io);

struct swSSLSocketIO
{
  swSocketIO socketIO;
  swSSL *ssl;
  swStaticBuffer pendingRead;
  swStaticBuffer pendingWrite;
};

swSSLSocketIO *swSSLSocketIONew();
bool swSSLSocketIOInit   (swSSLSocketIO *io);
void swSSLSocketIODelete (swSSLSocketIO *io);
void swSSLSocketIOCleanup(swSSLSocketIO *io);

static inline void swSSLSocketIOReadTimeoutSet      (swSSLSocketIO *io, uint64_t timeout)                   { swSocketIOReadTimeoutSet     (io, timeout);                          }
static inline void swSSLSocketIOWriteTimeoutSet     (swSSLSocketIO *io, uint64_t timeout)                   { swSocketIOWriteTimeoutSet    (io, timeout);                          }
static inline void swSSLSocketIOReadReadyFuncSet    (swSSLSocketIO *io, swSSLSocketIOReadReadyFunc func)    { swSocketIOReadReadyFuncSet   (io, (swSocketIOReadReadyFunc)func);    }
static inline void swSSLSocketIOWriteReadyFuncSet   (swSSLSocketIO *io, swSSLSocketIOWriteReadyFunc func)   { swSocketIOWriteReadyFuncSet  (io, (swSocketIOWriteReadyFunc)func);   }
static inline void swSSLSocketIOReadTimeoutFuncSet  (swSSLSocketIO *io, swSSLSocketIOReadTimeoutFunc func)  { swSocketIOReadTimeoutFuncSet (io, (swSocketIOReadTimeoutFunc)func);  }
static inline void swSSLSocketIOWriteTimeoutFuncSet (swSSLSocketIO *io, swSSLSocketIOWriteTimeoutFunc func) { swSocketIOWriteTimeoutFuncSet(io, (swSocketIOWriteTimeoutFunc)func); }
static inline void swSSLSocketIOErrorFuncSet        (swSSLSocketIO *io, swSSLSocketIOErrorFunc func)        { swSocketIOErrorFuncSet       (io, (swSocketIOErrorFunc)func);        }
static inline void swSSLSocketIOCloseFuncSet        (swSSLSocketIO *io, swSSLSocketIOCloseFunc func)        { swSocketIOCloseFuncSet       (io, (swSocketIOCloseFunc)func);        }

static inline bool swSSLSocketIOStart(swSSLSocketIO *io, swEdgeLoop *loop)              { return swSocketIOStart((swSocketIO *)io, loop); }
static inline void swSSLSocketIOClose(swSSLSocketIO *io, swSocketIOErrorType errorCode) { swSocketIOClose((swSocketIO *)io, errorCode);   }

// TODO: we probably do not need those functions and can use the same through server and client directly
//      basically, there is no need to wrap it up twice
static inline void *swSSLSocketIODataGet(swSSLSocketIO *io)             { return swSocketIODataGet(io); }
static inline void  swSSLSocketIODataSet(swSSLSocketIO *io, void *data) { swSocketIODataSet(io, data);  }

swSocketReturnType swSSLSocketIORead (swSSLSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesRead);
swSocketReturnType swSSLSocketIOWrite(swSSLSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesWritten);

#endif // SW_OPENSSL_SSLSOCKETIO_H