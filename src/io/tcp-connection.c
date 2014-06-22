#include "tcp-connection.h"

#include <core/memory.h>

static const char const *swTCPConnectionErrorText[swTCPConnectionErrorMax] =
{
  [swTCPConnectionErrorReadTimeout]           = "Read Timeout",
  [swTCPConnectionErrorWriteTimeout]          = "Write Timeout",
  [swTCPConnectionErrorReadError]             = "Read Error",
  [swTCPConnectionErrorWriteError]            = "Write Error",
  [swTCPConnectionErrorSocketError]           = "Socket Error",
  [swTCPConnectionErrorSocketHangUp]          = "Socket Hang Up",
  [swTCPConnectionErrorSocketClose]           = "Socket Close",
  [swTCPConnectionErrorOtherError]            = "Other Error",
  [swTCPConnectionErrorConnectedCheckFailed]  = "Connection Check Failed",
  [swTCPConnectionErrorConnectFailed]         = "Connect Failed",
  [swTCPConnectionErrorConnectTimeout]        = "Connect Timeout",
  [swTCPConnectionErrorListenFailed]          = "Listen Failed",
  [swTCPConnectionErrorAcceptFailed]          = "Accept Failed",
};

const char const *swTCPConnectionErrorTextGet(swTCPConnectionErrorType errorCode)
{
  if (errorCode > swTCPConnectionErrorNone && errorCode < swTCPConnectionErrorMax)
  {
    return swTCPConnectionErrorText[errorCode];
  }
  return NULL;
}

void swTCPConnectionClose(swTCPConnection *conn, swTCPConnectionErrorType errorCode)
{
  if (conn)
  {
    if (errorCode && conn->errorFunc)
      conn->errorFunc(conn, errorCode);
    // cleanup IO watcher before closing the socket, otherwise epoll_ctl is not going to like
    // this file descriptor
    swEdgeIOStop(&(conn->ioEvent));
    swSocket *sock = (swSocket *)conn;
    swSocketClose(sock);
    swEdgeTimerStop(&(conn->readTimer));
    swEdgeTimerStop(&(conn->writeTimer));
    if (conn->closeFunc)
      conn->closeFunc(conn);
  }
}

static void swTCPConnectionReadTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swTCPConnection *conn = swEdgeWatcherDataGet(timer);
  if (conn && (((swSocket*)conn)->fd >= 0))
  {
    if ((expiredCount > 0) && (events & swEdgeEventRead))
    {
      bool rtn = false;
      if (conn->readTimeoutFunc)
        rtn = conn->readTimeoutFunc(conn);
      if (!rtn)
        swTCPConnectionClose(conn, swTCPConnectionErrorReadTimeout);
    }
  }
}

static void swTCPConnectionWriteTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swTCPConnection *conn = swEdgeWatcherDataGet(timer);
  if (conn && (((swSocket*)conn)->fd >= 0))
  {
    if (expiredCount > 0 && (events & swEdgeEventRead))
    {
      bool rtn = false;
      if (conn->writeTimeoutFunc)
        rtn = conn->writeTimeoutFunc(conn);
      if (!rtn)
        swTCPConnectionClose(conn, swTCPConnectionErrorWriteTimeout);
    }
  }
}

static void swTCPConnectionIOEventCallback(swEdgeIO *ioWatcher, uint32_t events)
{
  swTCPConnection *conn = swEdgeWatcherDataGet(ioWatcher);
  if (conn && (((swSocket*)conn)->fd >= 0))
  {
    if (!(events & (swEdgeEventError | swEdgeEventHungUp)))
    {
      if (events & swEdgeEventRead)
      {
        swEdgeTimerStop(&(conn->readTimer));
        if (conn->readReadyFunc)
          conn->readReadyFunc(conn);
      }
      if (events & swEdgeEventWrite)
      {
        swEdgeTimerStop(&(conn->writeTimer));
        if (conn->writeReadyFunc)
          conn->writeReadyFunc(conn);
      }
    }
    else
      swTCPConnectionClose(conn, ((events & swEdgeEventError) ? swTCPConnectionErrorSocketError : swTCPConnectionErrorSocketHangUp));
  }
}

swTCPConnection *swTCPConnectionNew()
{
  swTCPConnection *rtn = swMemoryMalloc(sizeof(swTCPConnection));
  if (rtn)
  {
    if (!swTCPConnectionInit(rtn))
    {
      swTCPConnectionDelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swTCPConnectionInit(swTCPConnection *conn)
{
  bool rtn = false;
  if (conn)
  {
    memset(conn, 0, sizeof(swTCPConnection));
    if (swEdgeTimerInit(&(conn->readTimer), swTCPConnectionReadTimerCallback, false))
    {
      swEdgeWatcherDataSet(&(conn->readTimer), conn);
      if (swEdgeTimerInit(&(conn->writeTimer), swTCPConnectionWriteTimerCallback, false))
      {
        swEdgeWatcherDataSet(&(conn->writeTimer), conn);
        if (swEdgeIOInit(&(conn->ioEvent), swTCPConnectionIOEventCallback))
        {
          swEdgeWatcherDataSet(&(conn->ioEvent), conn);
          conn->readTimeout = conn->writeTimeout = SW_TCPCONNECTION_DEFAULT_TIMEOUT;
          rtn = true;
        }
      }
    }
  }
  return rtn;
}

void swTCPConnectionCleanup(swTCPConnection *conn)
{
  if (conn && !conn->cleaning)
  {
    conn->cleaning = true;
    if (((swSocket*)conn)->fd >= 0)
      swTCPConnectionClose(conn, swTCPConnectionErrorNone);
    swEdgeIOClose(&(conn->ioEvent));
    swEdgeTimerClose(&(conn->writeTimer));
    swEdgeTimerClose(&(conn->readTimer));
    conn->cleaning = false;
  }
}

void swTCPConnectionDelete(swTCPConnection *conn)
{
  if (conn && !conn->deleting)
  {
    conn->deleting = true;
    swTCPConnectionCleanup(conn);
    swMemoryFree(conn);
  }
}

bool swTCPConnectionStart(swTCPConnection *conn, swEdgeLoop *loop)
{
  bool rtn = false;
  if (conn && loop && (((swSocket *)conn)->fd >= 0))
  {
    if (swEdgeIOStart(&(conn->ioEvent), loop, ((swSocket *)conn)->fd, (swEdgeEventRead | swEdgeEventWrite)))
    {
      conn->loop = loop;
      rtn = true;
    }
  }
  return rtn;
}

swSocketReturnType swTCPConnectionRead(swTCPConnection *conn, swStaticBuffer *buffer, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (conn)
  {
    if ((rtn = swSocketReceive((swSocket *)conn, buffer, bytesRead)) == swSocketReturnOK)
      swEdgeWatcherPendingSet((swEdgeWatcher *)&(conn->ioEvent), swEdgeEventRead);
    else if (rtn == swSocketReturnNotReady)
    {
      if (!swEdgeTimerStart(&(conn->readTimer), conn->loop, conn->readTimeout, conn->readTimeout, false))
        swTCPConnectionClose(conn, swTCPConnectionErrorOtherError);
    }
    else
      swTCPConnectionClose(conn, ((rtn == swSocketReturnClose)? swTCPConnectionErrorSocketClose : swTCPConnectionErrorSocketError));
  }
  return rtn;
}

swSocketReturnType swTCPConnectionWrite(swTCPConnection *conn, swStaticBuffer *buffer, ssize_t *bytesWritten)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (conn)
  {
    if ((rtn = swSocketSend((swSocket *)conn, buffer, bytesWritten)) == swSocketReturnOK)
      swEdgeWatcherPendingSet((swEdgeWatcher *)&(conn->ioEvent), swEdgeEventWrite);
    else if (rtn == swSocketReturnNotReady)
    {
      if (!swEdgeTimerStart(&(conn->writeTimer), conn->loop, conn->writeTimeout, conn->writeTimeout, false))
        swTCPConnectionClose(conn, swTCPConnectionErrorOtherError);
    }
    else
      swTCPConnectionClose(conn, ((rtn == swSocketReturnClose ) ? swTCPConnectionErrorSocketClose : swTCPConnectionErrorSocketError));
  }
  return rtn;
}
