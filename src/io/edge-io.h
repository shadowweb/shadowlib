#ifndef SW_IO_EDGEIO_H
#define SW_IO_EDGEIO_H

#include "edge-loop.h"
#include <sys/eventfd.h>

struct swEdgeIO;

typedef void (*swEdgeIOCallback)(struct swEdgeIO *ioWatcher, uint32_t events);

typedef struct swEdgeIO
{
  swEdgeWatcher watcher;

  swEdgeIOCallback ioCB;
  uint32_t pendingEvents;
  uint32_t pendingPosition;
} swEdgeIO;

typedef enum swEdgeIOEvents
{
  swEdgeIOEventRead = EPOLLIN,
  swEdgeIOEventWrite = EPOLLOUT
} swEdgeIOEvents;

bool swEdgeIOInit(swEdgeIO *ioWatcher, swEdgeIOCallback cb);
bool swEdgeIOStart(swEdgeIO *ioWatcher, swEdgeLoop *loop, int fd, uint32_t events);
bool swEdgeIOEventSet(swEdgeIO *ioWatcher, uint32_t events);
bool swEdgeIOEventUnSet(swEdgeIO *ioWatcher, uint32_t events);
bool swEdgeIOPendingSet(swEdgeIO *ioWatcher, uint32_t events);
void swEdgeIOStop(swEdgeIO *ioWatcher);
void swEdgeIOClose(swEdgeIO *ioWatcher);

#define swEdgeIOFDGet(t)     ((swEdgeWatcher *)(t))->fd

#endif // SW_IO_EDGEIO_H
