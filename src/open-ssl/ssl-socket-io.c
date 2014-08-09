#include "open-ssl/ssl-socket-io-new.h"

#include "core/memory.h"

static void swSSLSocketIOSocketCleanup(swSSLSocketIO *io)
{
  swSocket *sock = (swSocket *)io;
  if (sock->fd >= 0)
    swSSLShutdown(io->ssl);
  if (io->pendingRead.len)
    io->pendingRead = swStaticBufferSetEmpty;
  if (io->pendingWrite.len)
    io->pendingWrite = swStaticBufferSetEmpty;

  swSocketClose(sock);

  if (io->ssl)
  {
    swSSLDelete(io->ssl);
    io->ssl = NULL;
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
    if (swSocketIOInit((swSocketIO *)io))
    {
      io->socketIO.socketCleanupFunc = (swSocketIOSocketCleanupFunc)swSSLSocketIOSocketCleanup;
      rtn = true;
    }
  }
  return rtn;
}

void swSSLSocketIOCleanup(swSSLSocketIO *io)
{
  if (io && !io->socketIO.cleaning)
  {
    swSocketIOCleanup((swSocketIO *)io);
    if (io->ssl)
    {
      swSSLDelete(io->ssl);
      io->ssl = NULL;
    }
    if (io->pendingRead.len)
      io->pendingRead = swStaticBufferSetEmpty;
    if (io->pendingWrite.len)
      io->pendingWrite = swStaticBufferSetEmpty;
  }
}

void swSSLSocketIODelete(swSSLSocketIO *io)
{
  if (io && !io->socketIO.deleting)
  {
    io->socketIO.deleting = true;
    swSSLSocketIOCleanup(io);
    swMemoryFree(io);
  }
}

swSocketReturnType swSSLSocketIORead(swSSLSocketIO *io, swStaticBuffer *buffer, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (io)
  {
    swSocketIO *socketIO = (swSocketIO *)io;
    if (!(io->pendingRead.len) || swStaticBufferSame(&(io->pendingRead), buffer))
    {
      if ((rtn = swSSLRead(io->ssl, buffer, bytesRead)) == swSocketReturnOK)
      {
        swEdgeWatcherPendingSet((swEdgeWatcher *)&(socketIO->ioEvent), swEdgeEventRead);
        if (io->pendingRead.len)
          io->pendingRead = swStaticBufferSetEmpty;
      }
      else if (rtn != swSocketReturnNotReady)
      {
        if (rtn == swSocketReturnReadNotReady)
        {
          io->pendingRead = *buffer;
          if (!swEdgeTimerStart(&(socketIO->readTimer), socketIO->loop, socketIO->readTimeout, socketIO->readTimeout, false))
            swSocketIOClose(socketIO, swSocketIOErrorOtherError);
        }
        else if (rtn == swSocketReturnWriteNotReady)
        {
          io->pendingRead = *buffer;
          if (!swEdgeTimerStart(&(socketIO->writeTimer), socketIO->loop, socketIO->writeTimeout, socketIO->writeTimeout, false))
            swSocketIOClose(socketIO, swSocketIOErrorOtherError);
        }
        else
          swSocketIOClose(socketIO, ((rtn == swSocketReturnClose)? swSocketIOErrorSocketClose : swSocketIOErrorSocketError));
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
    swSocketIO *socketIO = (swSocketIO *)io;
    if (!(io->pendingWrite.len) || swStaticBufferSame(&(io->pendingWrite), buffer))
    {
      if ((rtn = swSSLWrite(io->ssl, buffer, bytesWritten)) == swSocketReturnOK)
      {
        swEdgeWatcherPendingSet((swEdgeWatcher *)&(socketIO->ioEvent), swEdgeEventWrite);
        if (io->pendingWrite.len)
          io->pendingWrite = swStaticBufferSetEmpty;
      }
      else if (rtn != swSocketReturnNotReady)
      {
        if (rtn == swSocketReturnWriteNotReady)
        {
          io->pendingWrite = *buffer;
          if (!swEdgeTimerStart(&(socketIO->writeTimer), socketIO->loop, socketIO->writeTimeout, socketIO->writeTimeout, false))
            swSocketIOClose(socketIO, swSocketIOErrorOtherError);
        }
        else if (rtn == swSocketReturnReadNotReady)
        {
          io->pendingWrite = *buffer;
          if (!swEdgeTimerStart(&(socketIO->readTimer), socketIO->loop, socketIO->readTimeout, socketIO->readTimeout, false))
            swSocketIOClose(socketIO, swSocketIOErrorOtherError);
        }
        else
          swSocketIOClose(socketIO, ((rtn == swSocketReturnClose)? swSocketIOErrorSocketClose : swSocketIOErrorSocketError));
      }
    }
    else
      rtn = swSocketReturnInvalidBuffer;
  }
  return rtn;
}
