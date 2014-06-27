#ifndef SW_IO_SOCKETIO_H
#define SW_IO_SOCKETIO_H

#include "socket.h"
#include "socket-address.h"
#include "edge-loop.h"
#include "edge-io.h"
#include "edge-timer.h"

#define SW_SOCKETIO_DEFAULT_TIMEOUT  60000 // 60s

typedef struct swSocketIO  swSocketIO;

typedef enum swSocketIOErrorType
{
  swSocketIOErrorNone,
  swSocketIOErrorReadTimeout,
  swSocketIOErrorWriteTimeout,
  swSocketIOErrorReadError,
  swSocketIOErrorWriteError,
  swSocketIOErrorSocketError,
  swSocketIOErrorSocketHangUp,
  swSocketIOErrorSocketClose,
  swSocketIOErrorOtherError,
  swSocketIOErrorConnectedCheckFailed,
  swSocketIOErrorConnectFailed,
  swSocketIOErrorConnectTimeout,
  swSocketIOErrorListenFailed,
  swSocketIOErrorAcceptFailed,
  swSocketIOErrorMax
} swSocketIOErrorType;

const char const *swSocketIOErrorTextGet(swSocketIOErrorType errorCode);

typedef void (*swSocketIOReadReadyFunc)   (swSocketIO *io);
typedef void (*swSocketIOWriteReadyFunc)  (swSocketIO *io);
typedef bool (*swSocketIOReadTimeoutFunc) (swSocketIO *io);
typedef bool (*swSocketIOWriteTimeoutFunc)(swSocketIO *io);
typedef void (*swSocketIOErrorFunc)       (swSocketIO *io, swSocketIOErrorType errorCode);
typedef void (*swSocketIOCloseFunc)       (swSocketIO *io);

struct swSocketIO
{
  swSocket sock;
  swEdgeIO ioEvent;
  swEdgeTimer readTimer;
  swEdgeTimer writeTimer;
  swEdgeLoop *loop;
  void *data;

  uint64_t readTimeout;
  uint64_t writeTimeout;

  swSocketIOReadReadyFunc readReadyFunc;
  swSocketIOWriteReadyFunc writeReadyFunc;
  swSocketIOReadTimeoutFunc readTimeoutFunc;
  swSocketIOWriteTimeoutFunc writeTimeoutFunc;
  swSocketIOErrorFunc errorFunc;
  swSocketIOCloseFunc closeFunc;

  unsigned int cleaning : 1;
  unsigned int deleting : 1;
};

swSocketIO *swSocketIONew();
bool swSocketIOInit   (swSocketIO *io);
void swSocketIODelete (swSocketIO *io);
void swSocketIOCleanup(swSocketIO *io);

#define swSocketIOReadTimeoutSet(c, t)       do { if ((c)) ((swSocketIO *)(c))->readTimeout = (t); } while(0)
#define swSocketIOWriteTimeoutSet(c, t)      do { if ((c)) ((swSocketIO *)(c))->writeTimeout = (t); } while(0)
#define swSocketIOReadReadyFuncSet(c, f)     do { if ((c)) ((swSocketIO *)(c))->readReadyFunc = (f); } while(0)
#define swSocketIOWriteReadyFuncSet(c, f)    do { if ((c)) ((swSocketIO *)(c))->writeReadyFunc = (f); } while(0)
#define swSocketIOReadTimeoutFuncSet(c, f)   do { if ((c)) ((swSocketIO *)(c))->readTimeoutFunc = (f); } while(0)
#define swSocketIOWriteTimeoutFuncSet(c, f)  do { if ((c)) ((swSocketIO *)(c))->writeTimeoutFunc = (f); } while(0)
#define swSocketIOErrorFuncSet(c, f)         do { if ((c)) ((swSocketIO *)(c))->errorFunc = (f); } while(0)
#define swSocketIOCloseFuncSet(c, f)         do { if ((c)) ((swSocketIO *)(c))->closeFunc = (f); } while(0)

bool swSocketIOStart(swSocketIO *io, swEdgeLoop *loop);
void swSocketIOClose(swSocketIO *io, swSocketIOErrorType errorCode);

static inline void *swSocketIODataGet(swSocketIO *io)
{
  if (io)
    return io->data;
  return NULL;
}

static inline void swSocketIODataSet(swSocketIO *io, void *data)
{
  if (io)
    io->data = data;
}

#define swSocketIODataGet(t)     swSocketIODataGet((swSocketIO *)(t))
#define swSocketIODataSet(t, d)  swSocketIODataSet((swSocketIO *)(t), (void *)(d))

swSocketReturnType swSocketIORead (swSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesRead);
swSocketReturnType swSocketIOWrite(swSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesWritten);

swSocketReturnType swSocketIOReadFrom (swSocketIO *io, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesRead);
swSocketReturnType swSocketIOWriteTo  (swSocketIO *io, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesWritten);

#endif // SW_IO_SOCKETIO_H