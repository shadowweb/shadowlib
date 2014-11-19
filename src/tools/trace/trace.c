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

static size_t fileSizeIncrement = 4096;
static off_t currentFileOffset = 0;
static char *memoryPtr = NULL;
static char *memoryCurrentPtr = NULL;
static char *memoryEndPtr = NULL;
static int fd = -1;
static bool traceEnabled = false;

extern const char *__progname;

void __attribute__ ((no_instrument_function)) traceFileUnmap()
{
  if ((fd > -1) && memoryPtr)
  {
    msync (memoryPtr, fileSizeIncrement, MS_SYNC);
    munmap ((void *)memoryPtr, fileSizeIncrement);
    memoryPtr = memoryCurrentPtr = memoryEndPtr = NULL;
    traceEnabled = false;
  }
}

void __attribute__ ((no_instrument_function)) traceFileMap()
{
  if ((fd > -1) && !memoryPtr)
  {
    if (ftruncate(fd, (currentFileOffset + fileSizeIncrement)) == 0 )
    {
      if ((memoryPtr = (char *)mmap(NULL, fileSizeIncrement, (PROT_WRITE), MAP_SHARED, fd, currentFileOffset)) != MAP_FAILED)
      {
        memoryCurrentPtr = memoryPtr;
        memoryEndPtr = memoryCurrentPtr + fileSizeIncrement;
        currentFileOffset += fileSizeIncrement;
        traceEnabled = true;
      }
      else
        fprintf(stderr, "Failed mmap with error %s\n", strerror(errno));
    }
    else
    {
      close(fd);
      fd = -1;
    }
  }
}

void __attribute__ ((constructor,no_instrument_function)) traceBegin (void)
{
  // fprintf(stderr, "%s: enter\n", __func__);
  // create binary file and map it to memory
  fileSizeIncrement = (size_t)getpagesize() * 1024;
  struct stat traceStat = { .st_ino = 0 };
  if (stat("./TRACE", &traceStat) == 0)
  {
    char tracePath[PATH_MAX];
    snprintf(tracePath, PATH_MAX, "./%s.TRACE", __progname);
    fd = open(tracePath, (O_CREAT | O_RDWR | O_TRUNC), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if (fd > -1)
      traceFileMap();
  }
  // fprintf(stderr, "%s: exit\n", __func__);
}

void __attribute__ ((destructor,no_instrument_function)) traceEnd (void)
{
  // fprintf(stderr, "%s: enter\n", __func__);
  // do msync and unmap and close file, truncate file to the correct file size
  if (fd > -1)
  {
    off_t currentFileSize = currentFileOffset - (off_t)(memoryEndPtr - memoryCurrentPtr);
    traceFileUnmap();
    if (ftruncate(fd, currentFileSize) < 0)
      fprintf (stderr, "failed to trancate trace file to the right size\n");
    close(fd);
    fd = -1;
  }
  // fprintf(stderr, "%s: exit\n", __func__);
}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_enter (void *func, void *caller)
{
  // write thread id  and func pointer into the next place in the mapped memory
  // when reached the end, sync, unmap, truncate file to the new size, and map to the new region in the file
  if (traceEnabled && memoryPtr)
  {
    if (!func)
      fprintf (stderr, "Enter with NULL function pointer\n");
    long int threadId = (long int)syscall(SYS_gettid);
    *(long int *)memoryCurrentPtr = threadId;
    memoryCurrentPtr += sizeof(threadId);
    unsigned long pointer = (unsigned long)(func);
    *(unsigned long *)memoryCurrentPtr = pointer;
    memoryCurrentPtr += sizeof(pointer);
    if (memoryCurrentPtr == memoryEndPtr)
    {
      traceFileUnmap();
      traceFileMap();
    }
  }
}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_exit (void *func, void *caller)
{
  //  put 1 in the first bit of func and write thread id and func pointer into the next place in the mapped memory
  // when reached the end, sync, unmap, truncate file to the new size, and map to the new region in the file
  if (traceEnabled && memoryPtr)
  {
    if (!func)
      fprintf (stderr, "Exit with NULL function pointer\n");
    long int threadId = (long int)syscall(SYS_gettid);
    *(long int *)memoryCurrentPtr = threadId;
    memoryCurrentPtr += sizeof(threadId);
    unsigned long pointer = (unsigned long)(func);
    pointer |= 0x8000000000000000UL;
    *(unsigned long *)memoryCurrentPtr = pointer;
    memoryCurrentPtr += sizeof(pointer);
    if (memoryCurrentPtr == memoryEndPtr)
    {
      traceFileUnmap();
      traceFileMap();
    }
  }
}
