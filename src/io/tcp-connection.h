#ifndef SW_IO_TCPCONNECTION_H
#define SW_IO_TCPCONNECTION_H

#include "socket.h"
#include "socket-address.h"
#include "edge-loop.h"
#include "edge-io.h"
#include "edge-timer.h"

#define SW_TCPCONNECTION_DEFAULT_TIMEOUT  60000 // 60s

typedef struct swTCPConnection  swTCPConnection;

typedef enum swTCPConnectionErrorType
{
  swTCPConnectionErrorNone,
  swTCPConnectionErrorReadTimeout,
  swTCPConnectionErrorWriteTimeout,
  swTCPConnectionErrorReadError,
  swTCPConnectionErrorWriteError,
  swTCPConnectionErrorSocketError,
  swTCPConnectionErrorSocketHangUp,
  swTCPConnectionErrorSocketClose,
  swTCPConnectionErrorOtherError,
  swTCPConnectionErrorConnectedCheckFailed,
  swTCPConnectionErrorConnectFailed,
  swTCPConnectionErrorConnectTimeout,
  swTCPConnectionErrorListenFailed,
  swTCPConnectionErrorAcceptFailed,
  swTCPConnectionErrorMax
} swTCPConnectionErrorType;

const char const *swTCPConnectionErrorTextGet(swTCPConnectionErrorType errorCode);

typedef void (*swTCPConnectionReadReadyFunc)(swTCPConnection *conn);
typedef void (*swTCPConnectionWriteReadyFunc)(swTCPConnection *conn);
typedef bool (*swTCPConnectionReadTimeoutFunc)(swTCPConnection *conn);
typedef bool (*swTCPConnectionWriteTimeoutFunc)(swTCPConnection *conn);
typedef void (*swTCPConnectionErrorFunc)(swTCPConnection *conn, swTCPConnectionErrorType errorCode);
typedef void (*swTCPConnectionCloseFunc)(swTCPConnection *conn);

struct swTCPConnection
{
  swSocket sock;
  swEdgeIO ioEvent;
  swEdgeTimer readTimer;
  swEdgeTimer writeTimer;
  swEdgeLoop *loop;
  void *data;

  uint64_t readTimeout;
  uint64_t writeTimeout;

  swTCPConnectionReadReadyFunc readReadyFunc;
  swTCPConnectionWriteReadyFunc writeReadyFunc;
  swTCPConnectionReadTimeoutFunc readTimeoutFunc;
  swTCPConnectionWriteTimeoutFunc writeTimeoutFunc;
  swTCPConnectionErrorFunc errorFunc;
  swTCPConnectionCloseFunc closeFunc;

  unsigned int cleaning : 1;
  unsigned int deleting : 1;
};

swTCPConnection *swTCPConnectionNew();
bool swTCPConnectionInit(swTCPConnection *conn);
void swTCPConnectionDelete(swTCPConnection *conn);
void swTCPConnectionCleanup(swTCPConnection *conn);

#define swTCPConnectionReadTimeoutSet(c, t)       do { if ((c)) ((swTCPConnection *)(c))->readTimeout = (t); } while(0)
#define swTCPConnectionWriteTimeoutSet(c, t)      do { if ((c)) ((swTCPConnection *)(c))->writeTimeout = (t); } while(0)
#define swTCPConnectionReadReadyFuncSet(c, f)     do { if ((c)) ((swTCPConnection *)(c))->readReadyFunc = (f); } while(0)
#define swTCPConnectionWriteReadyFuncSet(c, f)    do { if ((c)) ((swTCPConnection *)(c))->writeReadyFunc = (f); } while(0)
#define swTCPConnectionReadTimeoutFuncSet(c, f)   do { if ((c)) ((swTCPConnection *)(c))->readTimeoutFunc = (f); } while(0)
#define swTCPConnectionWriteTimeoutFuncSet(c, f)  do { if ((c)) ((swTCPConnection *)(c))->writeTimeoutFunc = (f); } while(0)
#define swTCPConnectionErrorFuncSet(c, f)         do { if ((c)) ((swTCPConnection *)(c))->errorFunc = (f); } while(0)
#define swTCPConnectionCloseFuncSet(c, f)         do { if ((c)) ((swTCPConnection *)(c))->closeFunc = (f); } while(0)

bool swTCPConnectionStart(swTCPConnection *conn, swEdgeLoop *loop);
void swTCPConnectionClose(swTCPConnection *conn, swTCPConnectionErrorType errorCode);

static inline void *swTCPConnetionDataGet(swTCPConnection *conn)
{
  if (conn)
    return conn->data;
  return NULL;
}

static inline void swTCPConnetionDataSet(swTCPConnection *conn, void *data)
{
  if (conn)
    conn->data = data;
}

#define swTCPConnectionDataGet(t)     swTCPConnectionDataGet((swTCPConnection *)(t))
#define swTCPConnectionDataSet(t, d)  swTCPConnectionDataSet((swTCPConnection *)(t), (void *)(d))

swSocketReturnType swTCPConnectionRead(swTCPConnection *conn, swStaticBuffer *buffer, ssize_t *bytesRead);
swSocketReturnType swTCPConnectionWrite(swTCPConnection *conn, swStaticBuffer *buffer, ssize_t *bytesWritten);

#endif // SW_IO_TCPCONNECTION_H