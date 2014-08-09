#include "io/socket-io.h"

#include <core/memory.h>

static const char const *swSocketIOErrorText[swSocketIOErrorMax] =
{
  [swSocketIOErrorReadTimeout]           = "Read Timeout",
  [swSocketIOErrorWriteTimeout]          = "Write Timeout",
  [swSocketIOErrorReadError]             = "Read Error",
  [swSocketIOErrorWriteError]            = "Write Error",
  [swSocketIOErrorSocketError]           = "Socket Error",
  [swSocketIOErrorSocketHangUp]          = "Socket Hang Up",
  [swSocketIOErrorSocketClose]           = "Socket Close",
  [swSocketIOErrorOtherError]            = "Other Error",
  [swSocketIOErrorConnectedCheckFailed]  = "Connection Check Failed",
  [swSocketIOErrorConnectFailed]         = "Connect Failed",
  [swSocketIOErrorConnectTimeout]        = "Connect Timeout",
  [swSocketIOErrorListenFailed]          = "Listen Failed",
  [swSocketIOErrorAcceptFailed]          = "Accept Failed",
};

const char const *swSocketIOErrorTextGet(swSocketIOErrorType errorCode)
{
  if (errorCode > swSocketIOErrorNone && errorCode < swSocketIOErrorMax)
    return swSocketIOErrorText[errorCode];
  return NULL;
}

static inline void swSocketIOSocketCleanup(swSocketIO *io)
{
  swSocket *sock = (swSocket *)io;
  swSocketClose(sock);
}

void swSocketIOClose(swSocketIO *io, swSocketIOErrorType errorCode)
{
  if (io)
  {
    if (errorCode && io->errorFunc)
      io->errorFunc(io, errorCode);
    // cleanup IO watcher before closing the socket, otherwise epoll_ctl is not going to like
    // this file descriptor
    swEdgeIOStop(&(io->ioEvent));
    swEdgeTimerStop(&(io->readTimer));
    swEdgeTimerStop(&(io->writeTimer));
    io->socketCleanupFunc(io);
    if (io->closeFunc)
      io->closeFunc(io);
  }
}

static void swSocketIOReadTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swSocketIO *io = swEdgeWatcherDataGet(timer);
  if (io && (((swSocket*)io)->fd >= 0))
  {
    if ((expiredCount > 0) && (events & swEdgeEventRead))
    {
      bool rtn = false;
      if (io->readTimeoutFunc)
        rtn = io->readTimeoutFunc(io);
      if (!rtn)
        swSocketIOClose(io, swSocketIOErrorReadTimeout);
    }
  }
}

static void swSocketIOWriteTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swSocketIO *io = swEdgeWatcherDataGet(timer);
  if (io && (((swSocket*)io)->fd >= 0))
  {
    if (expiredCount > 0 && (events & swEdgeEventRead))
    {
      bool rtn = false;
      if (io->writeTimeoutFunc)
        rtn = io->writeTimeoutFunc(io);
      if (!rtn)
        swSocketIOClose(io, swSocketIOErrorWriteTimeout);
    }
  }
}

static void swSocketIOIOEventCallback(swEdgeIO *ioWatcher, uint32_t events)
{
  swSocketIO *io = swEdgeWatcherDataGet(ioWatcher);
  if (io && (((swSocket*)io)->fd >= 0))
  {
    if (!(events & (swEdgeEventError | swEdgeEventHungUp)))
    {
      if (events & swEdgeEventRead)
      {
        swEdgeTimerStop(&(io->readTimer));
        if (io->readReadyFunc)
          io->readReadyFunc(io);
      }
      if (events & swEdgeEventWrite)
      {
        swEdgeTimerStop(&(io->writeTimer));
        if (io->writeReadyFunc)
          io->writeReadyFunc(io);
      }
    }
    else
      swSocketIOClose(io, ((events & swEdgeEventError) ? swSocketIOErrorSocketError : swSocketIOErrorSocketHangUp));
  }
}

swSocketIO *swSocketIONew()
{
  swSocketIO *rtn = swMemoryMalloc(sizeof(swSocketIO));
  if (rtn)
  {
    if (!swSocketIOInit(rtn))
    {
      swSocketIODelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swSocketIOInit(swSocketIO *io)
{
  bool rtn = false;
  if (io)
  {
    memset(io, 0, sizeof(swSocketIO));
    if (swEdgeTimerInit(&(io->readTimer), swSocketIOReadTimerCallback, false))
    {
      swEdgeWatcherDataSet(&(io->readTimer), io);
      if (swEdgeTimerInit(&(io->writeTimer), swSocketIOWriteTimerCallback, false))
      {
        swEdgeWatcherDataSet(&(io->writeTimer), io);
        if (swEdgeIOInit(&(io->ioEvent), swSocketIOIOEventCallback))
        {
          swEdgeWatcherDataSet(&(io->ioEvent), io);
          io->readTimeout = io->writeTimeout = SW_SOCKETIO_DEFAULT_TIMEOUT;
          io->socketCleanupFunc = swSocketIOSocketCleanup;
          rtn = true;
        }
      }
    }
  }
  return rtn;
}

void swSocketIOCleanup(swSocketIO *io)
{
  if (io && !io->cleaning)
  {
    io->cleaning = true;
    if (((swSocket*)io)->fd >= 0)
      swSocketIOClose(io, swSocketIOErrorNone);
    swEdgeIOClose(&(io->ioEvent));
    swEdgeTimerClose(&(io->writeTimer));
    swEdgeTimerClose(&(io->readTimer));
    io->cleaning = false;
  }
}

void swSocketIODelete(swSocketIO *io)
{
  if (io && !io->deleting)
  {
    io->deleting = true;
    swSocketIOCleanup(io);
    swMemoryFree(io);
  }
}

bool swSocketIOStart(swSocketIO *io, swEdgeLoop *loop)
{
  bool rtn = false;
  if (io && loop && (((swSocket *)io)->fd >= 0))
  {
    if (swEdgeIOStart(&(io->ioEvent), loop, ((swSocket *)io)->fd, (swEdgeEventRead | swEdgeEventWrite)))
    {
      io->loop = loop;
      rtn = true;
    }
  }
  return rtn;
}

swSocketReturnType swSocketIORead(swSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (io)
  {
    if ((rtn = swSocketReceive((swSocket *)io, buffer, bytesRead)) == swSocketReturnOK)
      swEdgeWatcherPendingSet((swEdgeWatcher *)&(io->ioEvent), swEdgeEventRead);
    else if (rtn == swSocketReturnNotReady)
    {
      if (!swEdgeTimerStart(&(io->readTimer), io->loop, io->readTimeout, io->readTimeout, false))
        swSocketIOClose(io, swSocketIOErrorOtherError);
    }
    else
      swSocketIOClose(io, ((rtn == swSocketReturnClose)? swSocketIOErrorSocketClose : swSocketIOErrorSocketError));
  }
  return rtn;
}

swSocketReturnType swSocketIOWrite(swSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesWritten)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (io)
  {
    if ((rtn = swSocketSend((swSocket *)io, buffer, bytesWritten)) == swSocketReturnOK)
      swEdgeWatcherPendingSet((swEdgeWatcher *)&(io->ioEvent), swEdgeEventWrite);
    else if (rtn == swSocketReturnNotReady)
    {
      if (!swEdgeTimerStart(&(io->writeTimer), io->loop, io->writeTimeout, io->writeTimeout, false))
        swSocketIOClose(io, swSocketIOErrorOtherError);
    }
    else
      swSocketIOClose(io, ((rtn == swSocketReturnClose ) ? swSocketIOErrorSocketClose : swSocketIOErrorSocketError));
  }
  return rtn;
}

swSocketReturnType swSocketIOReadFrom(swSocketIO *io, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (io)
  {
    if ((rtn = swSocketReceiveFrom((swSocket *)io, buffer, address, bytesRead)) == swSocketReturnOK)
      swEdgeWatcherPendingSet((swEdgeWatcher *)&(io->ioEvent), swEdgeEventRead);
    else if (rtn == swSocketReturnNotReady)
    {
      if (!swEdgeTimerStart(&(io->readTimer), io->loop, io->readTimeout, io->readTimeout, false))
        swSocketIOClose(io, swSocketIOErrorOtherError);
    }
    else
      swSocketIOClose(io, ((rtn == swSocketReturnClose)? swSocketIOErrorSocketClose : swSocketIOErrorSocketError));
  }
  return rtn;
}

swSocketReturnType swSocketIOWriteTo(swSocketIO *io, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesWritten)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (io)
  {
    if ((rtn = swSocketSendTo((swSocket *)io, buffer, address, bytesWritten)) == swSocketReturnOK)
      swEdgeWatcherPendingSet((swEdgeWatcher *)&(io->ioEvent), swEdgeEventWrite);
    else if (rtn == swSocketReturnNotReady)
    {
      if (!swEdgeTimerStart(&(io->writeTimer), io->loop, io->writeTimeout, io->writeTimeout, false))
        swSocketIOClose(io, swSocketIOErrorOtherError);
    }
    else
      swSocketIOClose(io, ((rtn == swSocketReturnClose ) ? swSocketIOErrorSocketClose : swSocketIOErrorSocketError));
  }
  return rtn;
}
