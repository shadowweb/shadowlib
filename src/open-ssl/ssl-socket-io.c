#include "open-ssl/ssl-socket-io.h"

#include <core/memory.h>

static const char const *swSSLSocketIOErrorText[swSSLSocketIOErrorMax] =
{
  [swSSLSocketIOErrorReadTimeout]           = "Read Timeout",
  [swSSLSocketIOErrorWriteTimeout]          = "Write Timeout",
  [swSSLSocketIOErrorReadError]             = "Read Error",
  [swSSLSocketIOErrorWriteError]            = "Write Error",
  [swSSLSocketIOErrorSocketError]           = "Socket Error",
  [swSSLSocketIOErrorSocketHangUp]          = "Socket Hang Up",
  [swSSLSocketIOErrorSocketClose]           = "Socket Close",
  [swSSLSocketIOErrorOtherError]            = "Other Error",
  [swSSLSocketIOErrorConnectedCheckFailed]  = "Connection Check Failed",
  [swSSLSocketIOErrorConnectFailed]         = "Connect Failed",
  [swSSLSocketIOErrorConnectTimeout]        = "Connect Timeout",
  [swSSLSocketIOErrorListenFailed]          = "Listen Failed",
  [swSSLSocketIOErrorAcceptFailed]          = "Accept Failed",
};

const char const *swSSLSocketIOErrorTextGet(swSSLSocketIOErrorType errorCode)
{
  if (errorCode > swSSLSocketIOErrorNone && errorCode < swSSLSocketIOErrorMax)
    return swSSLSocketIOErrorText[errorCode];
  return NULL;
}

void swSSLSocketIOClose(swSSLSocketIO *io, swSSLSocketIOErrorType errorCode)
{
  if (io)
  {
    if (errorCode && io->errorFunc)
      io->errorFunc(io, errorCode);
    // cleanup IO watcher before closing the socket, otherwise epoll_ctl is not going to like
    // this file descriptor
    swEdgeIOStop(&(io->ioEvent));
    swSocket *sock = (swSocket *)io;
    if (sock->fd >= 0)
      swSSLShutdown(io->ssl);
    if (io->pendingRead.len)
      io->pendingRead = swStaticBufferSetEmpty;
    if (io->pendingWrite.len)
      io->pendingWrite = swStaticBufferSetEmpty;

    swSocketClose(sock);
    swEdgeTimerStop(&(io->readTimer));
    swEdgeTimerStop(&(io->writeTimer));
    if (io->ssl)
    {
      swSSLDelete(io->ssl);
      io->ssl = NULL;
    }
    if (io->closeFunc)
      io->closeFunc(io);
  }
}

static void swSSLSocketIOReadTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swSSLSocketIO *io = swEdgeWatcherDataGet(timer);
  if (io && (((swSocket*)io)->fd >= 0))
  {
    if ((expiredCount > 0) && (events & swEdgeEventRead))
    {
      bool rtn = false;
      if (io->readTimeoutFunc)
        rtn = io->readTimeoutFunc(io);
      if (!rtn)
        swSSLSocketIOClose(io, swSSLSocketIOErrorReadTimeout);
    }
  }
}

static void swSSLSocketIOWriteTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  swSSLSocketIO *io = swEdgeWatcherDataGet(timer);
  if (io && (((swSocket*)io)->fd >= 0))
  {
    if (expiredCount > 0 && (events & swEdgeEventRead))
    {
      bool rtn = false;
      if (io->writeTimeoutFunc)
        rtn = io->writeTimeoutFunc(io);
      if (!rtn)
        swSSLSocketIOClose(io, swSSLSocketIOErrorWriteTimeout);
    }
  }
}

static void swSSLSocketIOIOEventCallback(swEdgeIO *ioWatcher, uint32_t events)
{
  swSSLSocketIO *io = swEdgeWatcherDataGet(ioWatcher);
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
      swSSLSocketIOClose(io, ((events & swEdgeEventError) ? swSSLSocketIOErrorSocketError : swSSLSocketIOErrorSocketHangUp));
  }
}

swSSLSocketIO *swSSLSocketIONew()
{
  swSSLSocketIO *rtn = swMemoryMalloc(sizeof(swSSLSocketIO));
  if (rtn)
  {
    if (!swSSLSocketIOInit(rtn))
    {
      swSSLSocketIODelete(rtn);
      rtn = NULL;
    }
  }
  return rtn;
}

bool swSSLSocketIOInit(swSSLSocketIO *io)
{
  bool rtn = false;
  if (io)
  {
    memset(io, 0, sizeof(swSSLSocketIO));
    if (swEdgeTimerInit(&(io->readTimer), swSSLSocketIOReadTimerCallback, false))
    {
      swEdgeWatcherDataSet(&(io->readTimer), io);
      if (swEdgeTimerInit(&(io->writeTimer), swSSLSocketIOWriteTimerCallback, false))
      {
        swEdgeWatcherDataSet(&(io->writeTimer), io);
        if (swEdgeIOInit(&(io->ioEvent), swSSLSocketIOIOEventCallback))
        {
          swEdgeWatcherDataSet(&(io->ioEvent), io);
          io->readTimeout = io->writeTimeout = SW_SSLSOCKETIO_DEFAULT_TIMEOUT;
          rtn = true;
        }
      }
    }
  }
  return rtn;
}

void swSSLSocketIOCleanup(swSSLSocketIO *io)
{
  if (io && !io->cleaning)
  {
    io->cleaning = true;
    if (((swSocket*)io)->fd >= 0)
      swSSLSocketIOClose(io, swSSLSocketIOErrorNone);
    swEdgeIOClose(&(io->ioEvent));
    swEdgeTimerClose(&(io->writeTimer));
    swEdgeTimerClose(&(io->readTimer));
    if (io->ssl)
    {
      swSSLDelete(io->ssl);
      io->ssl = NULL;
    }
    io->cleaning = false;
  }
}

void swSSLSocketIODelete(swSSLSocketIO *io)
{
  if (io && !io->deleting)
  {
    io->deleting = true;
    swSSLSocketIOCleanup(io);
    swMemoryFree(io);
  }
}

bool swSSLSocketIOStart(swSSLSocketIO *io, swEdgeLoop *loop)
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

swSocketReturnType swSSLSocketIORead(swSSLSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (io)
  {
    if (!(io->pendingRead.len) || swStaticBufferSame(&(io->pendingRead), buffer))
    {
      if ((rtn = swSSLRead(io->ssl, buffer, bytesRead)) == swSocketReturnOK)
      {
        swEdgeWatcherPendingSet((swEdgeWatcher *)&(io->ioEvent), swEdgeEventRead);
        if (io->pendingRead.len)
          io->pendingRead = swStaticBufferSetEmpty;
      }
      else if (rtn != swSocketReturnNotReady)
      {
        if (rtn == swSocketReturnReadNotReady)
        {
          io->pendingRead = *buffer;
          if (!swEdgeTimerStart(&(io->readTimer), io->loop, io->readTimeout, io->readTimeout, false))
            swSSLSocketIOClose(io, swSSLSocketIOErrorOtherError);
        }
        else if (rtn == swSocketReturnWriteNotReady)
        {
          io->pendingRead = *buffer;
          if (!swEdgeTimerStart(&(io->writeTimer), io->loop, io->writeTimeout, io->writeTimeout, false))
            swSSLSocketIOClose(io, swSSLSocketIOErrorOtherError);
        }
        else
          swSSLSocketIOClose(io, ((rtn == swSocketReturnClose)? swSSLSocketIOErrorSocketClose : swSSLSocketIOErrorSocketError));
      }
    }
    else
      rtn = swSocketReturnInvalidBuffer;
  }
  return rtn;
}

swSocketReturnType swSSLSocketIOWrite(swSSLSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesWritten)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (io)
  {
    if (!(io->pendingWrite.len) || swStaticBufferSame(&(io->pendingWrite), buffer))
    {
      if ((rtn = swSSLWrite(io->ssl, buffer, bytesWritten)) == swSocketReturnOK)
      {
        swEdgeWatcherPendingSet((swEdgeWatcher *)&(io->ioEvent), swEdgeEventWrite);
        if (io->pendingWrite.len)
          io->pendingWrite = swStaticBufferSetEmpty;
      }
      else if (rtn != swSocketReturnNotReady)
      {
        if (rtn == swSocketReturnWriteNotReady)
        {
          io->pendingWrite = *buffer;
          if (!swEdgeTimerStart(&(io->writeTimer), io->loop, io->writeTimeout, io->writeTimeout, false))
            swSSLSocketIOClose(io, swSSLSocketIOErrorOtherError);
        }
        else if (rtn == swSocketReturnReadNotReady)
        {
          io->pendingWrite = *buffer;
          if (!swEdgeTimerStart(&(io->readTimer), io->loop, io->readTimeout, io->readTimeout, false))
            swSSLSocketIOClose(io, swSSLSocketIOErrorOtherError);
        }
        else
          swSSLSocketIOClose(io, ((rtn == swSocketReturnClose)? swSSLSocketIOErrorSocketClose : swSSLSocketIOErrorSocketError));
      }
    }
    else
      rtn = swSocketReturnInvalidBuffer;
  }
  return rtn;
}
