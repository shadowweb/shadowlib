#ifndef SW_IO_SOCKET_H
#define SW_IO_SOCKET_H

#include <stdbool.h>
#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdio.h>

#include <storage/static-buffer.h>

#include "socket-address.h"

// TODO: consider error code for invalid input
typedef enum swSocketReturnType
{
  swSocketReturnNone = 0,
  swSocketReturnOK,
  swSocketReturnInProgress,
  swSocketReturnNotReady,
  swSocketReturnWriteNotReady,
  swSocketReturnReadNotReady,
  swSocketReturnInvalidBuffer,
  swSocketReturnClose,
  swSocketReturnError,
  swSocketReturnMax
} swSocketReturnType;

const char const *swSocketReturnTypeTextGet(swSocketReturnType returnCode);

typedef struct swSocket swSocket;

typedef swSocketReturnType (*swSocketReadFunc)(swSocket *sock, swStaticBuffer *buffer);
typedef swSocketReturnType (*swSocketWriteFunc)(swSocket *sock, swStaticBuffer *buffer);
typedef swSocketReturnType (*swSocketCloseFunc)(swSocket *sock);

struct swSocket
{
  swSocketAddress localAddress;
  swSocketAddress remoteAddress;
  int fd;
  int type;

  unsigned int bound : 1;
  unsigned int connected : 1;
  unsigned int accepted : 1;
  unsigned int encrypted : 1;
  unsigned int localSet : 1;
  unsigned int remoteSet : 1;
};

swSocket *swSocketNew(int domain, int type);
swSocket *swSocketNewFromAddress(swSocketAddress *address, int type);
swSocket *swSocketNewFromFD(int fd);

bool swSocketInit(swSocket *sock, int domain, int type);
bool swSocketInitFromAddress(swSocket *sock, swSocketAddress *address, int type);
bool swSocketInitFromFD(swSocket *sock, int fd);

bool swSocketBind(swSocket *sock, swSocketAddress *address);
swSocketReturnType swSocketListen(swSocket *sock, swSocketAddress *address);
swSocketReturnType swSocketAccept(swSocket *sock, swSocket *acceptedSock);
swSocketReturnType swSocketConnect(swSocket *sock, swSocketAddress *address);
swSocketReturnType swSocketReconnect(swSocket *sock);

// TODO: datagram sockets have as many receives as sends(unless CORK is used)
//        the way to see the how many bytes should be read is to use MSG_PEEK
//        flag in receive or ioctl(fd, FIONREAD, &msgSize)
//        investigate which one is faster
//        I do not have a good idea at the moment on how to fit it in
//        ideally, if we try to read from such a socket into the buffer that can't
//        contain all the data, we should not attempt to read it, but should instead
//        fail and notify the caller that a bigger buffer is needed and how big

swSocketReturnType swSocketRead(swSocket *sock, swStaticBuffer *buffer, ssize_t *bytesRead);
swSocketReturnType swSocketWrite(swSocket *sock, swStaticBuffer *buffer, ssize_t *bytesWritten);

swSocketReturnType swSocketSend(swSocket *sock, swStaticBuffer *buffer, ssize_t *bytesWritten);
swSocketReturnType swSocketSendTo(swSocket *sock, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesWritten);
swSocketReturnType swSocketSendMsg(swSocket *sock, struct msghdr *msg);

swSocketReturnType swSocketReceive(swSocket *sock, swStaticBuffer *buffer, ssize_t *bytesRead);
swSocketReturnType swSocketReceiveFrom(swSocket *sock, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesRead);
swSocketReturnType swSocketReceiveMsg(swSocket *sock, struct msghdr *msg);

bool swSocketIsConnected(swSocket *sock, int *returnError);

void swSocketClose(swSocket *sock);
void swSocketDelete(swSocket *sock);

#endif // SW_IO_SOCKET_H
