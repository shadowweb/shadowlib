#ifndef SW_OPENSSL_SSLSOCKETIO_H
#define SW_OPENSSL_SSLSOCKETIO_H

#include "io/socket.h"
#include "io/socket-address.h"
#include "io/edge-loop.h"
#include "io/edge-io.h"
#include "io/edge-timer.h"
#include "open-ssl/ssl.h"

#define SW_SSLSOCKETIO_DEFAULT_TIMEOUT  60000 // 60s

typedef struct swSSLSocketIO  swSSLSocketIO;

typedef enum swSSLSocketIOErrorType
{
  swSSLSocketIOErrorNone,
  swSSLSocketIOErrorReadTimeout,
  swSSLSocketIOErrorWriteTimeout,
  swSSLSocketIOErrorReadError,
  swSSLSocketIOErrorWriteError,
  swSSLSocketIOErrorSocketError,
  swSSLSocketIOErrorSocketHangUp,
  swSSLSocketIOErrorSocketClose,
  swSSLSocketIOErrorOtherError,
  swSSLSocketIOErrorConnectedCheckFailed,
  swSSLSocketIOErrorConnectFailed,
  swSSLSocketIOErrorConnectTimeout,
  swSSLSocketIOErrorListenFailed,
  swSSLSocketIOErrorAcceptFailed,
  swSSLSocketIOErrorMax
} swSSLSocketIOErrorType;

const char const *swSSLSocketIOErrorTextGet(swSSLSocketIOErrorType errorCode);

typedef void (*swSSLSocketIOReadReadyFunc)   (swSSLSocketIO *io);
typedef void (*swSSLSocketIOWriteReadyFunc)  (swSSLSocketIO *io);
typedef bool (*swSSLSocketIOReadTimeoutFunc) (swSSLSocketIO *io);
typedef bool (*swSSLSocketIOWriteTimeoutFunc)(swSSLSocketIO *io);
typedef void (*swSSLSocketIOErrorFunc)       (swSSLSocketIO *io, swSSLSocketIOErrorType errorCode);
typedef void (*swSSLSocketIOCloseFunc)       (swSSLSocketIO *io);

struct swSSLSocketIO
{
  swSocket sock;
  swEdgeIO ioEvent;
  swEdgeTimer readTimer;
  swEdgeTimer writeTimer;
  swEdgeLoop *loop;
  void *data;
  swSSL *ssl;
  swStaticBuffer pendingRead;
  swStaticBuffer pendingWrite;

  uint64_t readTimeout;
  uint64_t writeTimeout;

  swSSLSocketIOReadReadyFunc readReadyFunc;
  swSSLSocketIOWriteReadyFunc writeReadyFunc;
  swSSLSocketIOReadTimeoutFunc readTimeoutFunc;
  swSSLSocketIOWriteTimeoutFunc writeTimeoutFunc;
  swSSLSocketIOErrorFunc errorFunc;
  swSSLSocketIOCloseFunc closeFunc;

  unsigned int cleaning : 1;
  unsigned int deleting : 1;
};

swSSLSocketIO *swSSLSocketIONew();
bool swSSLSocketIOInit   (swSSLSocketIO *io);
void swSSLSocketIODelete (swSSLSocketIO *io);
void swSSLSocketIOCleanup(swSSLSocketIO *io);

#define swSSLSocketIOReadTimeoutSet(c, t)       do { if ((c)) ((swSSLSocketIO *)(c))->readTimeout = (t); } while(0)
#define swSSLSocketIOWriteTimeoutSet(c, t)      do { if ((c)) ((swSSLSocketIO *)(c))->writeTimeout = (t); } while(0)
#define swSSLSocketIOReadReadyFuncSet(c, f)     do { if ((c)) ((swSSLSocketIO *)(c))->readReadyFunc = (f); } while(0)
#define swSSLSocketIOWriteReadyFuncSet(c, f)    do { if ((c)) ((swSSLSocketIO *)(c))->writeReadyFunc = (f); } while(0)
#define swSSLSocketIOReadTimeoutFuncSet(c, f)   do { if ((c)) ((swSSLSocketIO *)(c))->readTimeoutFunc = (f); } while(0)
#define swSSLSocketIOWriteTimeoutFuncSet(c, f)  do { if ((c)) ((swSSLSocketIO *)(c))->writeTimeoutFunc = (f); } while(0)
#define swSSLSocketIOErrorFuncSet(c, f)         do { if ((c)) ((swSSLSocketIO *)(c))->errorFunc = (f); } while(0)
#define swSSLSocketIOCloseFuncSet(c, f)         do { if ((c)) ((swSSLSocketIO *)(c))->closeFunc = (f); } while(0)

bool swSSLSocketIOStart(swSSLSocketIO *io, swEdgeLoop *loop);
void swSSLSocketIOClose(swSSLSocketIO *io, swSSLSocketIOErrorType errorCode);

static inline void *swSSLSocketIODataGet(swSSLSocketIO *io)
{
  if (io)
    return io->data;
  return NULL;
}

static inline void swSSLSocketIODataSet(swSSLSocketIO *io, void *data)
{
  if (io)
    io->data = data;
}

#define swSSLSocketIODataGet(t)     swSSLSocketIODataGet((swSSLSocketIO *)(t))
#define swSSLSocketIODataSet(t, d)  swSSLSocketIODataSet((swSSLSocketIO *)(t), (void *)(d))

swSocketReturnType swSSLSocketIORead (swSSLSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesRead);
swSocketReturnType swSSLSocketIOWrite(swSSLSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesWritten);

// swSocketReturnType swSSLSocketIOReadFrom (swSSLSocketIO *io, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesRead);
// swSocketReturnType swSSLSocketIOWriteTo  (swSSLSocketIO *io, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesWritten);

#endif // SW_OPENSSL_SSLSOCKETIO_H