#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#define SW_MAX_THREADS  200

static pthread_key_t traceKey;
static bool traceEnabled = false;
extern const char *__progname;

typedef struct swTrace
{
  off_t currentFileOffset;
  uint64_t *memoryCurrentPtr;
  uint64_t *memoryEndPtr;
  int fd;
  bool traceEnabled;
  // fill up the cache line
  uint64_t unused[4];
  // a page worth of data
  uint64_t buffer[4096/sizeof(uint64_t)];
} swTrace;

static swTrace threadTraces[SW_MAX_THREADS] = { {0} };
static uint32_t firstFreeSlot = 0;
// if the number of threads gets greater than this number, stop tracing on new threads
static const uint32_t maxThreadTraces = SW_MAX_THREADS;

swTrace * __attribute__ ((no_instrument_function)) traceDataCreate()
{
  swTrace *traceData = NULL;
  uint32_t nextAvailableSlot = 0;
  uint32_t nextFreeSlot = 0;
  while (true)
  {
    nextAvailableSlot = firstFreeSlot;
    nextFreeSlot = nextAvailableSlot + 1;
    if (nextAvailableSlot < maxThreadTraces)
    {
      if (__sync_bool_compare_and_swap(&firstFreeSlot, nextAvailableSlot, nextFreeSlot))
      {
        traceData = &threadTraces[nextAvailableSlot];
        break;
      }
    }
    else
      break;
  }
  if (traceData)
  {
    traceData->currentFileOffset = 0;
    traceData->memoryCurrentPtr = traceData->buffer;
    traceData->memoryEndPtr = traceData->memoryCurrentPtr + sizeof(traceData->buffer)/sizeof(uint64_t);
    traceData->fd = -1;
    traceData->traceEnabled = true;
    pid_t threadId = (pid_t)syscall(SYS_gettid);
    char tracePath[PATH_MAX];
    snprintf(tracePath, PATH_MAX, "./%s.TRACE.%u", __progname, threadId);
    traceData->fd = open(tracePath, (O_CREAT | O_RDWR | O_TRUNC), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if (traceData->fd > -1)
      pthread_setspecific(traceKey, traceData);
    else
      traceData = NULL;
  }
  return traceData;
}

void __attribute__ ((no_instrument_function)) traceDataDelete (swTrace *traceData)
{
  if (traceData)
  {
    if (traceData->fd > -1)
    {
      write(traceData->fd, (void *)(traceData->buffer), (traceData->memoryCurrentPtr - traceData->buffer) * sizeof(uint64_t));
      close(traceData->fd);
      traceData->traceEnabled = false;
      traceData->fd = -1;
    }
  }
}

void __attribute__ ((constructor,no_instrument_function)) traceBegin (void)
{
  // fprintf(stderr, "%s: enter\n", __func__);
  // create binary file and map it to memory
  struct stat traceStat = { .st_ino = 0 };
  if (stat("./TRACE", &traceStat) == 0)
  {
    pthread_key_create(&traceKey, NULL);
    traceEnabled = true;
  }
  // fprintf(stderr, "%s: exit\n", __func__);
}

void __attribute__ ((destructor,no_instrument_function)) traceEnd (void)
{
  // fprintf(stderr, "%s: enter\n", __func__);
  // do msync and unmap and close file, truncate file to the correct file size
  if (traceEnabled)
  {
    for (uint32_t i = 0; i < firstFreeSlot; i++)
      traceDataDelete(&(threadTraces[i]));
    pthread_key_delete(traceKey);
    traceEnabled = false;
  }
}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_enter (void *func, void *caller)
{
  // write thread id  and func pointer into the next place in the mapped memory
  // when reached the end, sync, unmap, truncate file to the new size, and map to the new region in the file
  if (traceEnabled)
  {
    swTrace *traceData = (swTrace *)pthread_getspecific(traceKey);
    if (!traceData)
      traceData = traceDataCreate();
    if (traceData && traceData->traceEnabled)
    {
      if (!func)
        fprintf (stderr, "Enter with NULL function pointer\n");
      unsigned long pointer = (unsigned long)(func);
      *(unsigned long *)traceData->memoryCurrentPtr = pointer;
      traceData->memoryCurrentPtr++;
      if (traceData->memoryCurrentPtr == traceData->memoryEndPtr)
      {
        write(traceData->fd, (void *)(traceData->buffer), sizeof(traceData->buffer));
        traceData->memoryCurrentPtr = traceData->buffer;
      }
    }
  }
}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_exit (void *func, void *caller)
{
  //  put 1 in the first bit of func and write thread id and func pointer into the next place in the mapped memory
  // when reached the end, sync, unmap, truncate file to the new size, and map to the new region in the file
  if (traceEnabled)
  {
    swTrace *traceData = (swTrace *)pthread_getspecific(traceKey);
    if (traceData && traceData->traceEnabled)
    {
      if (!func)
        fprintf (stderr, "Exit with NULL function pointer\n");
      unsigned long pointer = (unsigned long)(func);
      pointer |= 0x8000000000000000UL;
      *(unsigned long *)traceData->memoryCurrentPtr = pointer;
      traceData->memoryCurrentPtr++;
      if (traceData->memoryCurrentPtr == traceData->memoryEndPtr)
      {
        write(traceData->fd, (void *)(traceData->buffer), sizeof(traceData->buffer));
        traceData->memoryCurrentPtr = traceData->buffer;
      }
    }
  }
}
