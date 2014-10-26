#ifndef SW_THREAD_FUTEX_H
#define SW_THREAD_FUTEX_H

#include <errno.h>
#include <limits.h>
#include <linux/futex.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

static inline bool swFutexWait( void *futex, int value)
{
  bool rtn = false;
  if (futex)
  {
    int ret = syscall(SYS_futex, futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, value, NULL, NULL, 0);
    if (ret == 0 || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      rtn = true;
  }
  return rtn;
}

static inline bool swFutexWakeupOne(void *futex)
{
  bool rtn = false;
  if (futex)
  {
    int ret = syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, NULL, NULL, 0);
    if (ret == 0 || ret == 1)
      rtn = true;
  }
  return rtn;
}

static inline bool swFutexWakeupAll(void *futex)
{
  bool rtn = false;
  if (futex)
  {
    int ret = syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, INT_MAX, NULL, NULL, 0);
    if (ret >= 0)
      rtn = true;
  }
  return rtn;
}

static inline bool swFutexWaitWithTimeout( void *futex, int value, struct timespec *waitInterval)
{
  bool rtn = false;
  if (futex)
  {
    int ret = syscall(SYS_futex, futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, value, waitInterval, NULL, 0);
    if (ret == 0 || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ETIMEDOUT)
      rtn = true;
  }
  return rtn;
}

#endif
