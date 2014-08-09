#ifndef SW_IO_UDPSERVER_H
#define SW_IO_UDPSERVER_H

#include "socket-io.h"

typedef struct swUDPServer  swUDPServer;

typedef void (*swUDPServerReadReadyFunc)    (swUDPServer *server);
typedef void (*swUDPServerWriteReadyFunc)   (swUDPServer *server);
typedef bool (*swUDPServerReadTimeoutFunc)  (swUDPServer *server);
typedef bool (*swUDPServerWriteTimeoutFunc) (swUDPServer *server);
typedef void (*swUDPServerErrorFunc)        (swUDPServer *server, swSocketIOErrorType errorCode);
typedef void (*swUDPServerCloseFunc)        (swUDPServer *server);

struct swUDPServer
{
  swSocketIO io;
};

static inline swUDPServer *swUDPServerNew()                 { return (swUDPServer *)swSocketIONew();             }
static inline bool swUDPServerInit(swUDPServer *server)     { return swSocketIOInit((swSocketIO *)server);  }
static inline void swUDPServerCleanup(swUDPServer *server)  { swSocketIOCleanup((swSocketIO *)server);      }
static inline void swUDPServerDelete(swUDPServer *server)   { swSocketIODelete((swSocketIO *)server);       }

bool swUDPServerStart(swUDPServer *server, swEdgeLoop *loop, swSocketAddress *address);
static inline void swUDPServerStop(swUDPServer *server) { swSocketIOClose((swSocketIO *)server, swSocketIOErrorNone); }

static inline swSocketReturnType swUDPServerRead(swUDPServer *server, swStaticBuffer *buffer, ssize_t *bytesRead)                                 { return swSocketIORead    ((swSocketIO *)server, buffer, bytesRead);             }
static inline swSocketReturnType swUDPServerWrite(swUDPServer *server, swStaticBuffer *buffer, ssize_t *bytesWritten)                             { return swSocketIOWrite   ((swSocketIO *)server, buffer, bytesWritten);          }
static inline swSocketReturnType swUDPServerReadFrom(swUDPServer *server, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesRead)   { return swSocketIOReadFrom((swSocketIO *)server, buffer, address, bytesRead);    }
static inline swSocketReturnType swUDPServerWriteTo(swUDPServer *server, swStaticBuffer *buffer, swSocketAddress *address, ssize_t *bytesWritten) { return swSocketIOWriteTo ((swSocketIO *)server, buffer, address, bytesWritten); }

static inline void *swUDPServerDataGet(swUDPServer *server)            { return swSocketIODataGet(server); }
static inline void swUDPServerDataSet(swUDPServer *server, void *data) { swSocketIODataSet(server, data);  }

static inline void swUDPServerReadTimeoutSet      (swUDPServer *server, uint64_t timeout)                 { swSocketIOReadTimeoutSet     (server, timeout);                          }
static inline void swUDPServerWriteTimeoutSet     (swUDPServer *server, uint64_t timeout)                 { swSocketIOWriteTimeoutSet    (server, timeout);                          }
static inline void swUDPServerReadReadyFuncSet    (swUDPServer *server, swUDPServerReadReadyFunc func)    { swSocketIOReadReadyFuncSet   (server, (swSocketIOReadReadyFunc)func);    }
static inline void swUDPServerWriteReadyFuncSet   (swUDPServer *server, swUDPServerWriteReadyFunc func)   { swSocketIOWriteReadyFuncSet  (server, (swSocketIOWriteReadyFunc)func);   }
static inline void swUDPServerReadTimeoutFuncSet  (swUDPServer *server, swUDPServerReadTimeoutFunc func)  { swSocketIOReadTimeoutFuncSet (server, (swSocketIOReadTimeoutFunc)func);  }
static inline void swUDPServerWriteTimeoutFuncSet (swUDPServer *server, swUDPServerWriteTimeoutFunc func) { swSocketIOWriteTimeoutFuncSet(server, (swSocketIOWriteTimeoutFunc)func); }
static inline void swUDPServerErrorFuncSet        (swUDPServer *server, swUDPServerErrorFunc func)        { swSocketIOErrorFuncSet       (server, (swSocketIOErrorFunc)func);        }
static inline void swUDPServerCloseFuncSet        (swUDPServer *server, swUDPServerCloseFunc func)        { swSocketIOCloseFuncSet       (server, (swSocketIOCloseFunc)func);        }

#endif // SW_IO_UDPSERVER_H
