#ifndef SW_IO_UDPSERVER_H
#define SW_IO_UDPSERVER_H

#include "tcp-connection.h"

typedef struct swUDPServer  swUDPServer;

typedef void (*swUDPServerReadReadyFunc)    (swUDPServer *server);
typedef void (*swUDPServerWriteReadyFunc)   (swUDPServer *server);
typedef bool (*swUDPServerReadTimeoutFunc)  (swUDPServer *server);
typedef bool (*swUDPServerWriteTimeoutFunc) (swUDPServer *server);
typedef void (*swUDPServerErrorFunc)        (swUDPServer *server, swTCPConnectionErrorType errorCode);
typedef void (*swUDPServerCloseFunc)        (swUDPServer *server);

struct swUDPServer
{
  swTCPConnection conn;
};

static inline swUDPServer *swUDPServerNew()                 { return (swUDPServer *)swTCPConnectionNew();             }
static inline bool swUDPServerInit(swUDPServer *server)     { return swTCPConnectionInit((swTCPConnection *)server);  }
static inline void swUDPServerCleanup(swUDPServer *server)  { swTCPConnectionCleanup((swTCPConnection *)server);      }
static inline void swUDPServerDelete(swUDPServer *server)   { swTCPConnectionDelete((swTCPConnection *)server);       }

bool swUDPServerStart(swUDPServer *server, swEdgeLoop *loop, swSocketAddress *address);
static inline void swUDPServerStop(swUDPServer *server) { swTCPConnectionClose((swTCPConnection *)server, swTCPConnectionErrorNone); }

static inline swSocketReturnType swUDPServerRead(swUDPServer *server, swStaticBuffer *buffer, ssize_t *bytesRead)                                 { return swTCPConnectionRead    ((swTCPConnection *)server, buffer, bytesRead);             }
static inline swSocketReturnType swUDPServerWrite(swUDPServer *server, swStaticBuffer *buffer, ssize_t *bytesWritten)                             { return swTCPConnectionWrite   ((swTCPConnection *)server, buffer, bytesWritten);          }
static inline swSocketReturnType swUDPServerReadFrom(swUDPServer *server, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesRead)   { return swTCPConnectionReadFrom((swTCPConnection *)server, buffer, address, bytesRead);    }
static inline swSocketReturnType swUDPServerWriteTo(swUDPServer *server, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesWritten) { return swTCPConnectionWriteTo ((swTCPConnection *)server, buffer, address, bytesWritten); }

static inline void *swUDPServerDataGet(swUDPServer *server)            { return swTCPConnectionDataGet((swTCPConnection *)(server)); }
static inline void swUDPServerDataSet(swUDPServer *server, void *data) { swTCPConnectionDataSet((swTCPConnection *)(server), data);  }

static inline void swUDPServerReadTimeoutSet      (swUDPServer *server, uint64_t timeout)                 { swTCPConnectionReadTimeoutSet     (server, timeout);                               }
static inline void swUDPServerWriteTimeoutSet     (swUDPServer *server, uint64_t timeout)                 { swTCPConnectionWriteTimeoutSet    (server, timeout);                               }
static inline void swUDPServerReadReadyFuncSet    (swUDPServer *server, swUDPServerReadReadyFunc func)    { swTCPConnectionReadReadyFuncSet   (server, (swTCPConnectionReadReadyFunc)func);    }
static inline void swUDPServerWriteReadyFuncSet   (swUDPServer *server, swUDPServerWriteReadyFunc func)   { swTCPConnectionWriteReadyFuncSet  (server, (swTCPConnectionWriteReadyFunc)func);   }
static inline void swUDPServerReadTimeoutFuncSet  (swUDPServer *server, swUDPServerReadTimeoutFunc func)  { swTCPConnectionReadTimeoutFuncSet (server, (swTCPConnectionReadTimeoutFunc)func);  }
static inline void swUDPServerWriteTimeoutFuncSet (swUDPServer *server, swUDPServerWriteTimeoutFunc func) { swTCPConnectionWriteTimeoutFuncSet(server, (swTCPConnectionWriteTimeoutFunc)func); }
static inline void swUDPServerErrorFuncSet        (swUDPServer *server, swUDPServerErrorFunc func)        { swTCPConnectionErrorFuncSet       (server, (swTCPConnectionErrorFunc)func);        }
static inline void swUDPServerCloseFuncSet        (swUDPServer *server, swUDPServerCloseFunc func)        { swTCPConnectionCloseFuncSet       (server, (swTCPConnectionCloseFunc)func);        }

#endif // SW_IO_UDPSERVER_H
