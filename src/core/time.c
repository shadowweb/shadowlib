#include "core/time.h"

uint64_t swTimeMeasure(clockid_t clockId, void (*func)())
{
  struct timespec timeValueStart = {0};
  struct timespec timeValueStop = {0};
  if (!clock_gettime(clockId, &timeValueStart))
  {
    func();
    if (!clock_gettime(clockId, &timeValueStop))
      return (swTimeSecToNSec(timeValueStop.tv_sec) + timeValueStop.tv_nsec) - (swTimeSecToNSec(timeValueStart.tv_sec) + timeValueStart.tv_nsec);
  }
  return 0;
}

uint64_t swTimeGet(clockid_t clockId)
{
  struct timespec timeValue = {0};
  if (!clock_gettime(clockId, &timeValue))
    return swTimeSecToNSec(timeValue.tv_sec) + timeValue.tv_nsec;
  return 0;
}
