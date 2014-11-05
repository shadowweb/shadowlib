#ifndef SW_LOG_LOGMANAGER_H
#define SW_LOG_LOGMANAGER_H

#include "collections/fast-array.h"
#include "collections/hash-map-linear.h"
#include "storage/static-string.h"
#include "thread/thread-manager.h"

#include <stdarg.h>
#include <stdio.h>

typedef enum swLogLevel
{
  swLogLevelNone = 0,
  swLogLevelFatal,
  swLogLevelError,
  swLogLevelWarning,
  swLogLevelInfo,
  swLogLevelDebug,
  swLogLevelTrace,
  swLogLevelMax
} swLogLevel;

const char *swLogLevelTextGet(swLogLevel level);
const char *swLogLevelColorGet(swLogLevel level);

//  log manager initializes with standard stdout log sink
//  other sinks can be added later on

//  each sink might have:
//    formatter object (not always needed and when it is not needed means that one of glibc library calls can be used)
//  sink types:
//    file stream sink (stdout, stderr)
//    file descriptor sink (backed by file)
//    file name sink (with or without rollover strategy)
//    non blocking socket (with connection to remote server, will probably need mpsc buffer sink for multithreading)
//    mpsc buffer sink into a file
//    syslog sink (although not sure why anyone might need it)

//  formatters:
//    1. as is (ignore everything) (optional flags to include log level, logger id, file, line, func)
//    2. same as above, but add timestamp (human readable, non human readable)
//  two methods:
//    1. preformat the string and get the number of bytes needed to print it
//    2. print the string into the provided buffer that gurantees given string length

typedef struct swLogSink swLogSink;

typedef bool (*swLogSinkAcquireFunction)(swLogSink *sink, size_t sizeNeeded, uint8_t **buffer);
typedef bool (*swLogSinkReleaseFunction)(swLogSink *sink, size_t sizeNeeded, uint8_t  *buffer);
typedef void (*swLogSinkClearFunction)  (swLogSink *sink);

struct swLogSink
{
  // acquire buffer function
  swLogSinkAcquireFunction acquireFunc;
  // release buffer function
  swLogSinkReleaseFunction releaseFunc;
  // clear function
  swLogSinkClearFunction clearFunc;
  // data
  void *data;
};

static inline void  swLogSinkDataSet(swLogSink *sink, void *data)
{
  if (sink)
    sink->data = data;
}

static inline void *swLogSinkDataGet(swLogSink *sink)
{
  if (sink)
    return sink->data;
  return NULL;
}

// TODO: need multithreaded unittest
bool swLogFileSinkInit(swLogSink *sink, swThreadManager *threadManager, size_t maxFileSize, uint32_t maxFileCount, swStaticString *baseFileName);

// bool swLogSinkInit(swLogSink *sink, swLogSinkAcquireFunction );

typedef struct swLogFormatter swLogFormatter;

// TODO: add logger name to formatting
typedef bool (*swLogFormatterPreformatFunction) (swLogFormatter *formatter, size_t *sizeNeeded,                   swLogLevel level, const char *file, const char *function, int line, const char *format, va_list argList);
typedef bool (*swLogFormatterFormatFunction)    (swLogFormatter *formatter, size_t  sizeNeeded, uint8_t  *buffer, swLogLevel level, const char *file, const char *function, int line, const char *format, va_list argList);
typedef void (*swLogFormatterClearFunction)     (swLogFormatter *formatter);

struct swLogFormatter
{
  // preformat function (returns the number of the bytes needed)
  swLogFormatterPreformatFunction preformatFunc;
  // format function (write into the buffer)
  swLogFormatterFormatFunction formatFunc;
  // clear function
  swLogFormatterClearFunction clearFunc;
  // data
  void *data;
};

void  swLogFormatterDataSet(swLogFormatter *formatter, void *data);
void *swLogFormatterDataGet(swLogFormatter *formatter);

// Available formatters
bool swLogStdoutFormatterInit(swLogFormatter *formatter);
bool swLogBufferFormatterInit(swLogFormatter *formatter);

typedef struct swLogWriter
{
  swLogSink sink;
  swLogFormatter formatter;
} swLogWriter;

bool swLogWriterInit(swLogWriter *writer, swLogSink sink, swLogFormatter formatter);
void swLogWriterClear(swLogWriter *writer);

typedef struct swLogManager
{
  // collection of all loggers -- probably hash table: name -> logger pointer
  swHashMapLinear loggerMap;
  // fast array of writers
  swFastArray logWriters;
  // log level
  swLogLevel level;
} swLogManager;

struct swLogger;

swLogManager *swLogManagerNew(swLogLevel level);
void swLogManagerDelete(swLogManager *manager);
void swLogManagerLevelSet(swLogManager *manager, swLogLevel level);
void swLogManagerLoggerLevelSet(swLogManager *manager, swStaticString *name, swLogLevel level, bool useManagerLevel);
// probably not needed as we are only going to have statically initialized loggers
// void swLogManagerLoggerAdd(swLogManager *manager, struct swLogger *logger);
// TODO: rethink writer initialization, we should be able to remove them as well
bool swLogManagerWriterAdd(swLogManager *manager, swLogWriter writer);

// bool swLoggingInit(swLogLevel level);
// void swLoggingShutdown();

#define SW_LOGGER_MAGIC (0xdeadbef1)

typedef struct swLogger
{
  swStaticString name;
  swLogManager *manager;
  swLogLevel level;
  uint32_t magic;
  unsigned int useManagerLevel : 1;
  uint64_t unused1;
  uint64_t unused2;
  uint64_t unused3;
} swLogger;

#define swLoggerDefineWithLevel(n, l)   {.name = swStaticStringDefine(n), .manager = NULL, .level = (l),            .magic = SW_LOGGER_MAGIC, .useManagerLevel = false}
#define swLoggerDefine(n)               {.name = swStaticStringDefine(n), .manager = NULL, .level = swLogLevelNone, .magic = SW_LOGGER_MAGIC, .useManagerLevel = true }
#define swLoggerDeclare(logger, n)              static swLogger logger __attribute__ ((unused, section(".logging"))) = swLoggerDefine(n)
#define swLoggerDeclareWithLevel(logger, n, l)  static swLogger logger __attribute__ ((unused, section(".logging"))) = swLoggerDefineWithLevel(n, l)
void swLoggerLevelSet(swLogger *logger, swLogLevel level, bool useManagerLevel);

void swLoggerLog(swLogger *logger, swLogLevel level, const char *file, const char *function, int line, const char *format, ...) __attribute__ ((format(printf, 6, 7)));

#define SW_LOG_GENERIC(logger, lvl, file, function, line, format, ...)  \
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Waddress\"") \
  if (lvl && lvl < swLogLevelMax) \
  { \
    if ((logger) && ((swLogger *)(logger))->manager) \
    { \
      if ((((swLogger *)(logger))->useManagerLevel && (lvl <= ((swLogger *)(logger))->manager->level)) || (!((swLogger *)(logger))->useManagerLevel && (lvl <= ((swLogger *)(logger))->level))) \
        swLoggerLog(logger, lvl, file, function, line, format "\n", ##__VA_ARGS__); \
    } \
    else \
    { \
      if (!(logger) || ((logger) && (lvl <= (((swLogger *)(logger))->level)))) \
        printf("%s%s " file ":%u %s():%s " format "\n", swLogLevelColorGet(lvl), swLogLevelTextGet(lvl), line, function, swLogLevelColorGet(swLogLevelNone), ##__VA_ARGS__); \
    } \
  } \
_Pragma("GCC diagnostic pop")

#define SW_LOG_GENERIC_CONTINUE(logger, lvl, format, ...)  \
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Waddress\"") \
  if (lvl && lvl < swLogLevelMax) \
  { \
    if ((logger) && ((swLogger *)(logger))->manager) \
    { \
      if ((((swLogger *)(logger))->useManagerLevel && (lvl <= ((swLogger *)(logger))->manager->level)) || (!((swLogger *)(logger))->useManagerLevel && (lvl <= ((swLogger *)(logger))->level))) \
        swLoggerLog(logger, lvl, NULL, NULL, 0, format "\n", ##__VA_ARGS__); \
    } \
    else \
    { \
      if (!(logger) || ((logger) && (lvl <= ((swLogger *)(logger))->level))) \
        printf("%s%s%s\t" format "\n", swLogLevelColorGet(lvl), swLogLevelTextGet(lvl), swLogLevelColorGet(swLogLevelNone), ##__VA_ARGS__); \
    } \
  } \
_Pragma("GCC diagnostic pop")

#define SW_LOG_FATAL(logger, format, ...)     SW_LOG_GENERIC(logger, swLogLevelFatal,    __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SW_LOG_ERROR(logger, format, ...)     SW_LOG_GENERIC(logger, swLogLevelError,    __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SW_LOG_WARNING(logger, format, ...)   SW_LOG_GENERIC(logger, swLogLevelWarning,  __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SW_LOG_INFO(logger, format, ...)      SW_LOG_GENERIC(logger, swLogLevelInfo,     __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SW_LOG_DEBUG(logger, format, ...)     SW_LOG_GENERIC(logger, swLogLevelDebug,    __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SW_LOG_TRACE(logger, format, ...)     SW_LOG_GENERIC(logger, swLogLevelTrace,    __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)

#define SW_LOG_FATAL_CONT(logger, format, ...)     SW_LOG_GENERIC_CONTINUE(logger, swLogLevelFatal,    format, ##__VA_ARGS__)
#define SW_LOG_ERROR_CONT(logger, format, ...)     SW_LOG_GENERIC_CONTINUE(logger, swLogLevelError,    format, ##__VA_ARGS__)
#define SW_LOG_WARNING_CONT(logger, format, ...)   SW_LOG_GENERIC_CONTINUE(logger, swLogLevelWarning,  format, ##__VA_ARGS__)
#define SW_LOG_INFO_CONT(logger, format, ...)      SW_LOG_GENERIC_CONTINUE(logger, swLogLevelInfo,     format, ##__VA_ARGS__)
#define SW_LOG_DEBUG_CONT(logger, format, ...)     SW_LOG_GENERIC_CONTINUE(logger, swLogLevelDebug,    format, ##__VA_ARGS__)
#define SW_LOG_TRACE_CONT(logger, format, ...)     SW_LOG_GENERIC_CONTINUE(logger, swLogLevelTrace,    format, ##__VA_ARGS__)

// the log manager collects all the loggers from the logging section of the program

// log function (gets passed logger and the rest through macro)
// should the logger have no reference to log manager and no association with log sinks, logs everything
// to stdout as is, otherwise invokes sink and its formatter

#endif  // SW_LOG_LOGMANAGER_H
