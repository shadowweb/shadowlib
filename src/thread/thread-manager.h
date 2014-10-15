#ifndef SW_THREAD_MANAGER_H
#define SW_THREAD_MANAGER_H

#include "collections/sparse-array.h"
#include "io/edge-async.h"
#include "io/edge-timer.h"

// #define _GNU_SOURCE // TODO: find out why defining it did not work
#define __USE_GNU
#include <pthread.h>
#undef __USE_GNU
// #undef _GNU_SOURCE

// this can only be used from the main thread
// create specific thread dedicated to performing specific task
// when this task is done, the thread terminates
// different from the task pool that keeps the threads around so
// that various tasks can be scheduled. when done, this tasks free up the thread
// so that it can be used for another task

// TODO: implement task pool that acquires the threads from the thread manager

// TODO: I want to use thread manager for logging: single producer vs single consumer case and
// work my way up to multiple producer vs single consumer; the intention is to utilize lock free
// concurrency in all the cases; I would also need it for testing of all these scenarios

// TODO: create unit test

typedef void *(*swThreadRunFunction)(void *arg);
typedef void (*swThreadStopFunction)(void *arg);
typedef void (*swThreadDoneFunction)(void *arg, void *returnValue);

typedef struct swThreadManager
{
  swSparseArray threadInfoArray;
  swEdgeLoop *loop;
  bool terminate : 1;
  uint64_t joinWaitInterval: 63;
} swThreadManager;

typedef struct swThreadInfo
{
  swEdgeAsync doneEvent;
  // not initialized and started till it is actually needed
  swEdgeTimer joinWaitTimer;
  pthread_t threadId;
  swThreadRunFunction runFunc;
  swThreadStopFunction stopFunc;
  swThreadDoneFunction doneFunc;
  swThreadManager *manager;
  void *data;
  void *returnValue;
  uint32_t position;
} swThreadInfo;

swThreadManager *swThreadManagerNew(swEdgeLoop *loop);
bool swThreadManagerInit(swThreadManager *manager, swEdgeLoop *loop);
bool swThreadManagerStartThread(swThreadManager *manager, swThreadRunFunction runFunc, swThreadStopFunction stopFunc, swThreadDoneFunction doneFunc, void *data);
void swThreadManagerRelease(swThreadManager *manager);
void swThreadManagerDelete(swThreadManager *manager);

#endif // SW_THREAD_MANAGER_H