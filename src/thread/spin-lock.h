#ifndef SW_THREAD_SPINLOCK_H
#define SW_THREAD_SPINLOCK_H

#include <pthread.h>
#include <stdint.h>

typedef int swSpinLock;

static inline void swSpinLockInit(swSpinLock *lock)
{
  if (lock)
    lock = 0;
}

// WARNING: this spinlock might misbehave when two threads are compeating for it on the same CPU
static inline void swSpinLockLock(swSpinLock *lock)
{
  if (lock)
  {
    while (__sync_lock_test_and_set(lock, 1))
      while (*lock);
  }
}

static inline void swSpinLockUnlock(swSpinLock *lock)
{
  if (lock)
    __sync_lock_release(lock);
}

#endif  // SW_THREAD_SPINLOCK_H
