#include "socket.h"

#include <core/memory.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static const char const *swSocketReturnTypeText[swSocketReturnMax] =
{
  [swSocketReturnNone]            = "None",
  [swSocketReturnOK]              = "OK",
  [swSocketReturnInProgress]      = "InProgress",
  [swSocketReturnNotReady]        = "NotReady",
  [swSocketReturnWriteNotReady]   = "WriteNotReady",
  [swSocketReturnReadNotReady]    = "ReadNotReady",
  [swSocketReturnInvalidBuffer]   = "InvalidBuffer",
  [swSocketReturnClose]           = "Close",
  [swSocketReturnError]           = "Error",
};


const char const *swSocketReturnTypeTextGet(swSocketReturnType returnCode)
{
  if (returnCode >= swSocketReturnNone && returnCode < swSocketReturnMax)
    return swSocketReturnTypeText[returnCode];
  return NULL;
}

swSocket *swSocketNew(int domain, int type)
{
  swSocket *rtn = swMemoryMalloc(sizeof(swSocket));
  if (rtn && !swSocketInit(rtn, domain, type))
  {
    swMemoryFree(rtn);
    rtn = NULL;
  }
  return rtn;
}

swSocket *swSocketNewWithProtocol(int domain, int type, int protocol)
{
  swSocket *rtn = swMemoryMalloc(sizeof(swSocket));
  if (rtn && !swSocketInitWithProtocol(rtn, domain, type, protocol))
  {
    swMemoryFree(rtn);
    rtn = NULL;
  }
  return rtn;
}

swSocket *swSocketNewFromAddress(swSocketAddress *address, int type)
{
  swSocket *rtn = swMemoryMalloc(sizeof(swSocket));
  if (rtn && !swSocketInitFromAddress(rtn, address, type))
  {
    swMemoryFree(rtn);
    rtn = NULL;
  }
  return rtn;
}

swSocket *swSocketNewFromFD(int fd)
{
  swSocket *rtn = swMemoryMalloc(sizeof(swSocket));
  if (rtn && !swSocketInitFromFD(rtn, fd))
  {
    swMemoryFree(rtn);
    rtn = NULL;
  }
  return rtn;
}

bool swSocketInit(swSocket *sock, int domain, int type)
{
  bool rtn = false;
  if (sock && domain > 0 && type > 0)
  {
    memset(sock, 0, sizeof(swSocket));
    sock->fd = -1;
    if ((sock->fd = socket(domain, (type | SOCK_NONBLOCK | SOCK_CLOEXEC), 0)) >= 0)
    {
      sock->type = (type | SOCK_NONBLOCK | SOCK_CLOEXEC);
      rtn = true;
    }
  }
  return rtn;
}

bool swSocketInitWithProtocol(swSocket *sock, int domain, int type, int protocol)
{
  bool rtn = false;
  if (sock && domain > 0 && type > 0)
  {
    memset(sock, 0, sizeof(swSocket));
    sock->fd = -1;
    if ((sock->fd = socket(domain, (type | SOCK_NONBLOCK | SOCK_CLOEXEC), protocol)) >= 0)
    {
      sock->type = (type | SOCK_NONBLOCK | SOCK_CLOEXEC);
      rtn = true;
    }
  }
  return rtn;
}

bool swSocketInitFromAddress(swSocket *sock, swSocketAddress *address, int type)
{
  bool rtn = false;
  if (sock && address && type > 0)
  {
    memset(sock, 0, sizeof(swSocket));
    sock->fd = -1;
    if ((sock->fd = socket(address->storage.ss_family, (type | SOCK_NONBLOCK | SOCK_CLOEXEC), 0)) >= 0)
    {
      if (bind(sock->fd, &(address->addr), address->len) == 0)
      {
        sock->type = (type | SOCK_NONBLOCK | SOCK_CLOEXEC);
        sock->localAddress = *address;
        sock->localSet = true;
        sock->bound = true;
        rtn = true;
      }
      else
      {
        close(sock->fd);
        sock->fd = -1;
      }
    }
  }
  return rtn;
}

bool swSocketInitFromFD(swSocket *sock, int fd)
{
  bool rtn = false;
  if (sock && fd >= 0)
  {
    memset(sock, 0, sizeof(swSocket));
    sock->fd = -1;
    socklen_t length = sizeof( int );
    if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &(sock->type), &length ) == 0)
    {
      int flags = fcntl(fd, F_GETFL, 0);
      if (flags >= 0 && (fcntl (fd, F_SETFL, (flags | O_NONBLOCK)) == 0))
      {
        flags = fcntl (fd, F_GETFD);
        if ((flags >= 0) && fcntl (fd, F_SETFD, (flags | FD_CLOEXEC)) == 0)
        {
          sock->fd = fd;
          sock->accepted = true;
          rtn = true;
        }
      }
    }
  }
  return rtn;
}

bool swSocketBind(swSocket *sock, swSocketAddress *address)
{
  bool rtn = false;
  if (sock && address)
  {
    if (!sock->remoteAddress.storage.ss_family ||
        (sock->remoteAddress.storage.ss_family == address->storage.ss_family))
    {
      if (bind(sock->fd, &(address->addr), address->len) == 0)
      {
        sock->localAddress = *address;
        sock->localSet = true;
        sock->bound = true;
        rtn = true;
      }
    }
  }
  return rtn;
}

swSocketReturnType swSocketListen(swSocket *sock, swSocketAddress *address)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && address)
  {
    int reuse = 1;
    if (!setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)))
    {
      if (bind(sock->fd, &(address->addr), address->len) == 0)
      {
        sock->localAddress = *address;
        sock->localSet = true;
        sock->bound = true;
        if (!listen(sock->fd, SOMAXCONN))
          rtn = swSocketReturnOK;
      }
    }
    if (!rtn)
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketAccept(swSocket *sock, swSocket *acceptedSock)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && acceptedSock && (sock->fd >= 0))
  {
    acceptedSock->localAddress = acceptedSock->remoteAddress = sock->localAddress;
    acceptedSock->localSet = true;
    acceptedSock->remoteAddress.len = sizeof(acceptedSock->remoteAddress.storage);
    acceptedSock->fd = accept4(sock->fd, &(acceptedSock->remoteAddress.addr), &(acceptedSock->remoteAddress.len), (SOCK_NONBLOCK | SOCK_CLOEXEC));
    if (acceptedSock->fd >= 0)
    {
      // TODO: port is going to be different from the acceptor port, fix it to get correct address
      acceptedSock->remoteSet = true;
      acceptedSock->accepted = true;
      rtn = swSocketReturnOK;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketConnect(swSocket *sock, swSocketAddress *address)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && address)
  {
    if (!sock->localAddress.storage.ss_family ||
        (sock->localAddress.storage.ss_family == address->storage.ss_family))
    {
      sock->remoteAddress = *address;
      sock->remoteSet = true;
      int ret = -1;
      if ((ret = connect(sock->fd, &(address->addr), address->len)) == 0)
      {
        sock->connected = true;
        rtn = swSocketReturnOK;
      }
      else if (errno == EINPROGRESS)
        rtn = swSocketReturnInProgress;
      else
        rtn = swSocketReturnError;
    }
  }
  return rtn;
}

swSocketReturnType swSocketReconnect(swSocket *sock)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && (sock->fd == -1) && sock->remoteAddress.storage.ss_family)
  {
    if ((sock->fd = socket(sock->remoteAddress.storage.ss_family, sock->type, 0)) >= 0)
    {
      int ret = -1;
      if (sock->bound)
        ret = bind(sock->fd, &(sock->localAddress.addr), sock->localAddress.len);
      if (!sock->bound || (ret == 0))
      {
        if ((ret = connect(sock->fd, &(sock->remoteAddress.addr), sock->remoteAddress.len)) == 0)
        {
          sock->connected = true;
          rtn = swSocketReturnOK;
        }
        else if (errno == EINPROGRESS)
          rtn = swSocketReturnInProgress;
        else
          rtn = swSocketReturnError;
      }
      if (rtn == swSocketReturnNone)
        swSocketClose(sock);
    }
  }
  return rtn;
}

void swSocketClose(swSocket *sock)
{
  if (sock)
  {
    if (sock->fd > -1)
      close(sock->fd);
    sock->fd = -1;
    sock->connected = false;
    sock->accepted = false;
  }
}

void swSocketDelete(swSocket *sock)
{
  if (sock)
  {
    if (sock->fd >= 0)
      close(sock->fd);
    swMemoryFree(sock);
  }
}

bool swSocketIsConnected(swSocket *sock, int *returnError)
{
  bool rtn = false;
  if (sock)
  {
    int error = 0;
    socklen_t errorSize = sizeof(error);
    if (getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errorSize) == 0)
    {
      if (error == 0)
      {
        sock->connected = true;
        rtn = true;
      }
    }
    else
      error = errno;
    if (returnError)
      *returnError = error;
  }
  return rtn;
}

swSocketReturnType swSocketRead(swSocket *sock, swStaticBuffer *buffer, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && buffer && buffer->len && buffer->data && (sock->fd >= 0))
  {
    ssize_t ret = read(sock->fd, buffer->data, buffer->len);
    if (ret > 0)
    {
      rtn = swSocketReturnOK;
      if (bytesRead)
        *bytesRead = ret;
    }
    else if (ret == 0)
      rtn = swSocketReturnClose;
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketWrite(swSocket *sock, swStaticBuffer *buffer, ssize_t *bytesWritten)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && buffer && buffer->len && buffer->data && (sock->fd >= 0))
  {
    ssize_t ret = write(sock->fd, buffer->data, buffer->len);
    if (ret >= 0)
    {
      rtn = swSocketReturnOK;
      if (bytesWritten)
        *bytesWritten = ret;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketReadSplice(swSocket *sock, int pipefd[2], size_t len, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && len && (sock->fd >= 0) && pipefd[1] >= 0)
  {
    ssize_t ret = splice(sock->fd, NULL, pipefd[1], NULL, len, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
    if (ret > 0)
    {
      rtn = swSocketReturnOK;
      if (bytesRead)
        *bytesRead = ret;
    }
    else if (ret == 0)
      rtn = swSocketReturnClose;
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketWriteSplice(swSocket *sock, int pipefd[2], size_t len, ssize_t *bytesWritten)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && len && (sock->fd >= 0) && pipefd[0] >= 0)
  {
    ssize_t ret = splice(pipefd[0], NULL, sock->fd, NULL, len, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
    if (ret >= 0)
    {
      rtn = swSocketReturnOK;
      if (bytesWritten)
        *bytesWritten = ret;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketSend(swSocket *sock, swStaticBuffer *buffer, ssize_t *bytesWritten)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && buffer && buffer->len && buffer->data && (sock->fd >= 0))
  {
    ssize_t ret = send(sock->fd, buffer->data, buffer->len, 0);
    if (ret >= 0)
    {
      rtn = swSocketReturnOK;
      if (bytesWritten)
        *bytesWritten = ret;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketSendTo(swSocket *sock, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesWritten)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && buffer && buffer->len && buffer->data && (sock->fd >= 0))
  {
    ssize_t ret = sendto(sock->fd, buffer->data, buffer->len, 0, (address? &(address->addr) : NULL), (address? address->len : 0));
    if (ret >= 0)
    {
      rtn = swSocketReturnOK;
      if (bytesWritten)
        *bytesWritten = ret;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketSendMsg(swSocket *sock, struct msghdr *msg)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && msg && (sock->fd >= 0))
  {
    ssize_t ret = sendmsg(sock->fd, msg, 0);
    if (ret >= 0)
      rtn = swSocketReturnOK;
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketReceive(swSocket *sock, swStaticBuffer *buffer, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && buffer && buffer->len && buffer->data && (sock->fd >= 0))
  {
    ssize_t ret = recv(sock->fd, buffer->data, buffer->len, 0);
    if (ret > 0)
    {
      rtn = swSocketReturnOK;
      if (bytesRead)
        *bytesRead = ret;
    }
    else if (ret == 0)
      rtn = swSocketReturnClose;
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketReceiveFrom(swSocket *sock, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && buffer && buffer->len && buffer->data && (sock->fd >= 0))
  {
    ssize_t ret = recvfrom(sock->fd, buffer->data, buffer->len, 0, (address? &(address->addr) : NULL), (address? &(address->len) : NULL));
    if (ret > 0)
    {
      rtn = swSocketReturnOK;
      if (bytesRead)
        *bytesRead = ret;
    }
    else if (ret == 0)
      rtn = swSocketReturnClose;
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}

swSocketReturnType swSocketReceiveMsg(swSocket *sock, struct msghdr *msg)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (sock && msg && (sock->fd >= 0))
  {
    ssize_t ret = recvmsg(sock->fd, msg, 0);
    if (ret == sizeof(msg))
      rtn = swSocketReturnOK;
    else if (ret == 0)
      rtn = swSocketReturnClose;
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      rtn = swSocketReturnNotReady;
    else
      rtn = swSocketReturnError;
  }
  return rtn;
}
