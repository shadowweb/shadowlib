#include "log/log-manager.h"
#include "storage/dynamic-string.h"

#include <sys/syscall.h>
#include <unistd.h>

static bool swLogStdoutFormatterFormat(swLogFormatter *formatter, size_t sizeNeeded, uint8_t *buffer, swLogLevel level, const char *file, const char *function, int line, const char *loggerName, const char *format, va_list argList)
{
  bool rtn = false;
  if (formatter)
  {
    long int tid = syscall(SYS_gettid);
    if (file)
    {
      char timeBuffer[64];
      swDynamicString timeString = {.len = 0, .data = timeBuffer, .size = (sizeof(timeBuffer) - 1)};
      if (swDynamicStringAppendTime(&timeString))
      {
        fprintf(stdout, "%s%s [%ld] [%s] [%s:%d] [%s] [%s]%s ",
                swLogLevelColorGet(level), swLogLevelTextGet(level), tid, timeBuffer, file, line, function, loggerName, swLogLevelColorGet(swLogLevelNone));
      }
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
