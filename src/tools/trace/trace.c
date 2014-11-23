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

static pthread_key_t traceKey;
static bool traceEnabled = false;
static size_t fileSizeIncrement = 4096;
extern const char *__progname;

typedef struct swTrace
{
  off_t currentFileOffset;
  char *memoryPtr;
  char *memoryCurrentPtr;
  char *memoryEndPtr;
  int fd;
  bool traceEnabled;
} swTrace;

void __attribute__ ((no_instrument_function)) traceFileUnmap(swTrace *traceData)
{
  if (traceData && (traceData->fd > -1) && traceData->memoryPtr)
  {
    msync (traceData->memoryPtr, fileSizeIncrement, MS_SYNC);
    munmap ((void *)(traceData->memoryPtr), fileSizeIncrement);
    traceData->memoryPtr = traceData->memoryCurrentPtr = traceData->memoryEndPtr = NULL;
    traceData->traceEnabled = false;
  }
}


void __attribute__ ((no_instrument_function)) traceFileMap(swTrace *traceData)
{
  if (traceData && (traceData->fd > -1) && !traceData->memoryPtr)
  {
    if (ftruncate(traceData->fd, (traceData->currentFileOffset + fileSizeIncrement)) == 0 )
    {
      if ((traceData->memoryPtr = (char *)mmap(NULL, fileSizeIncrement, (PROT_WRITE), MAP_SHARED, traceData->fd, traceData->currentFileOffset)) != MAP_FAILED)
      {
        traceData->memoryCurrentPtr = traceData->memoryPtr;
        traceData->memoryEndPtr = traceData->memoryCurrentPtr + fileSizeIncrement;
        traceData->currentFileOffset += fileSizeIncrement;
        traceData->traceEnabled = true;
      }
      else
        fprintf(stderr, "Failed mmap with error %s\n", strerror(errno));
    }
    else
    {
      close(traceData->fd);
      traceData->fd = -1;
    }
  }
}

swTrace * __attribute__ ((no_instrument_function)) traceDataCreate()
{
  swTrace *traceData = malloc(sizeof(swTrace));
  if (traceData)
  {
    traceData->currentFileOffset = 0;
    traceData->memoryPtr = traceData->memoryCurrentPtr = traceData->memoryEndPtr = NULL;
    traceData->fd = -1;
    traceData->traceEnabled = false;
    pid_t threadId = (pid_t)syscall(SYS_gettid);
    char tracePath[PATH_MAX];
    snprintf(tracePath, PATH_MAX, "./%s.TRACE.%u", __progname, threadId);
    traceData->fd = open(tracePath, (O_CREAT | O_RDWR | O_TRUNC), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if (traceData->fd > -1)
      pthread_setspecific(traceKey, traceData);
    else
    {
      free(traceData);
      traceData = NULL;
    }
  }
  return traceData;
}


void __attribute__ ((no_instrument_function)) traceDataDelete(void *data)
{
  swTrace *traceData = (swTrace *)data;
  if (traceData)
  {
    if (traceData->fd > -1)
    {
      off_t currentFileSize = traceData->currentFileOffset - (off_t)(traceData->memoryEndPtr - traceData->memoryCurrentPtr);
      traceFileUnmap(traceData);
      if (ftruncate(traceData->fd, currentFileSize) < 0)
        fprintf (stderr, "failed to trancate trace file to the right size\n");
      close(traceData->fd);
      traceData->fd = -1;
      free(traceData);
    }
  }
}

void __attribute__ ((constructor,no_instrument_function)) traceBegin (void)
{
  // fprintf(stderr, "%s: enter\n", __func__);
  // create binary file and map it to memory
  // WARNING: for the processes with a large number of threads this multiplier should be small to prevent
  //          the threads from chewing up too much memory for tracing
  fileSizeIncrement = (size_t)getpagesize() * 8;
  struct stat traceStat = { .st_ino = 0 };
  if (stat("./TRACE", &traceStat) == 0)
  {
    pthread_key_create(&traceKey, traceDataDelete);
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
    swTrace *traceData = (swTrace *)pthread_getspecific(traceKey);
    if (traceData)
    {
      pthread_setspecific(traceKey, NULL);
      traceDataDelete(traceData);
    }
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
    {
      traceData = traceDataCreate();
      traceFileMap(traceData);
    }
    if (traceData && traceData->traceEnabled && traceData->memoryPtr)
    {
      if (!func)
        fprintf (stderr, "Enter with NULL function pointer\n");
      unsigned long pointer = (unsigned long)(func);
      *(unsigned long *)traceData->memoryCurrentPtr = pointer;
      traceData->memoryCurrentPtr += sizeof(pointer);
      if (traceData->memoryCurrentPtr == traceData->memoryEndPtr)
      {
        traceFileUnmap(traceData);
        traceFileMap(traceData);
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
    if (traceData && traceData->traceEnabled && traceData->memoryPtr)
    {
      if (!func)
        fprintf (stderr, "Exit with NULL function pointer\n");
      unsigned long pointer = (unsigned long)(func);
      pointer |= 0x8000000000000000UL;
      *(unsigned long *)traceData->memoryCurrentPtr = pointer;
      traceData->memoryCurrentPtr += sizeof(pointer);
      if (traceData->memoryCurrentPtr == traceData->memoryEndPtr)
      {
        traceFileUnmap(traceData);
        traceFileMap(traceData);
      }
    }
  }
}
