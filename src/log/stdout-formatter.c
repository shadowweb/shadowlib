#include "log/log-manager.h"
#include "storage/dynamic-string.h"

#include "core/time.h"

#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// TODO: move it to static or dynamic string
static void getTime(swDynamicString *timeStr)
{
  if (timeStr)
  {
    uint64_t timeInNs = swTimeGet(CLOCK_REALTIME);
    time_t timeInSec = swTimeNSecToSec(timeInNs);
    uint64_t millisec = swTimeNSecToMSec(swTimeNSecToSecRem(timeInNs));
    struct tm brokenDownTime = {0};
    if (gmtime_r(&timeInSec, &brokenDownTime))
    {
      timeStr->len = strftime(timeStr->data, timeStr->size, "%FT%T", &brokenDownTime);
    }
    int printed = sprintf(&(timeStr->data[timeStr->len]), ".%03lu", millisec);
    if (printed > 0)
      timeStr->len += printed;
  }
}

static bool swLogStdoutFormatterFormat(swLogFormatter *formatter, size_t sizeNeeded, uint8_t *buffer, swLogLevel level, const char *file, const char *function, int line, const char *format, va_list argList)
{
  bool rtn = false;
  if (formatter)
  {
    long int tid = syscall(SYS_gettid);
    if (file)
    {
      char timeBuffer[50];
      swDynamicString timeString = {.len = 0, .data = timeBuffer, .size = (sizeof(timeBuffer) - 1)};
      getTime(&timeString);
      fprintf(stdout, "%s%s [%ld] [%s] [%s:%d] [%s]%s ",
              swLogLevelColorGet(level), swLogLevelTextGet(level), tid, timeBuffer, file, line, function, swLogLevelColorGet(swLogLevelNone));
    }
    else
      fprintf(stdout, "%s%s [%ld]%s\t",
              swLogLevelColorGet(level), swLogLevelTextGet(level), tid, swLogLevelColorGet(swLogLevelNone));
    vfprintf(stdout, format, argList);
    rtn = true;
  }
  return rtn;
}

bool swLogStdoutFormatterInit(swLogFormatter *formatter)
{
  bool rtn = false;
  if (formatter)
  {
    setlinebuf(stdout);
    memset(formatter, 0, sizeof(*formatter));
    formatter->formatFunc = swLogStdoutFormatterFormat;
    rtn = true;
  }
  return rtn;
}
