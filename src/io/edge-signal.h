#ifndef SW_IO_EDGESIGNAL_H
#define SW_IO_EDGESIGNAL_H

#include "edge-loop.h"
#include <sys/signalfd.h>

struct swEdgeSignal;

typedef void (*swEdgeSignalCallback)(struct swEdgeSignal *signalWatcher, struct signalfd_siginfo *signalReceived);

typedef struct swEdgeSignal
{
  swEdgeWatcher watcher;

  swEdgeSignalCallback signalCB;
  sigset_t mask;
  // unsigned int active : 1;
} swEdgeSignal;

bool swEdgeSignalInit(swEdgeSignal *signalWatcher, swEdgeSignalCallback cb);
bool swEdgeSignalStart(swEdgeSignal *signalWatcher, swEdgeLoop *loop, sigset_t mask);
bool swEdgeSignalProcess(swEdgeSignal *signalWatcher, uint32_t events);
void swEdgeSignalStop(swEdgeSignal *signalWatcher);
void swEdgeSignalClose(swEdgeSignal *signalWatcher);

#endif // SW_IO_EDGESIGNAL_H
