#include "log/log-manager.h"
#include "storage/dynamic-string.h"

#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

static bool swLogBufferPreformatterFormat(swLogFormatter *formatter, size_t *sizeNeeded, swLogLevel level, const char *file, const char *function, int line, const char *loggerName, const char *format, va_list argList)
{
  bool rtn = false;
  if (formatter && sizeNeeded)
  {
    int prefixSize = 0;
    long int tid = syscall(SYS_gettid);
    if (file)
    {
      prefixSize = snprintf(NULL, 0, "%s [%ld] [] [%s:%d] [%s] [%s] ", swLogLevelTextGet(level), tid, file, line, function, loggerName);
      if (prefixSize)
        prefixSize += SW_TIME_STRING_SIZE;
    }
    else
      prefixSize = snprintf(NULL, 0, "%s [%ld]\t", swLogLevelTextGet(level), tid);
    if (prefixSize)
    {
      int bodySize = vsnprintf(NULL, 0, format, argList);
      if (bodySize)
      {
        *sizeNeeded = prefixSize + bodySize;
        rtn = true;
      }
    }
  }
  return rtn;
}

static bool swLogBufferFormatterFormat(swLogFormatter *formatter, size_t sizeNeeded, uint8_t *buffer, swLogLevel level, const char *file, const char *function, int line, const char *loggerName, const char *format, va_list argList)
{
  bool rtn = false;
  if (formatter && sizeNeeded && buffer)
  {
    int sizePrinted = 0;
    long int tid = syscall(SYS_gettid);
    if (file)
    {
      char timeBuffer[64];
      swDynamicString timeString = {.len = 0, .data = timeBuffer, .size = (sizeof(timeBuffer) - 1)};
      if (swDynamicStringAppendTime(&timeString))
        sizePrinted = snprintf((char *)buffer, sizeNeeded, "%s [%ld] [%s] [%s:%d] [%s] [%s] ", swLogLevelTextGet(level), tid, timeBuffer, file, line, function, loggerName);
    }
    else
      sizePrinted = snprintf((char *)buffer, sizeNeeded, "%s [%ld]\t", swLogLevelTextGet(level), tid);
    if (sizePrinted)
    {
      sizePrinted += vsnprintf((char *)(&buffer[sizePrinted]), (sizeNeeded - sizePrinted), format, argList);
      if ((size_t)sizePrinted == sizeNeeded)
      {
        // the last character written by vsnprintf() will be '\0', replace it with '\n'
        buffer[sizeNeeded - 1] = '\n';
        rtn = true;
      }
    }
  }
  return rtn;
}

bool swLogBufferFormatterInit(swLogFormatter *formatter)
{
  bool rtn = false;
  if (formatter)
  {
    memset(formatter, 0, sizeof(*formatter));
    formatter->preformatFunc = swLogBufferPreformatterFormat;
    formatter->formatFunc = swLogBufferFormatterFormat;
    rtn = true;
  }
  return rtn;
}
