#include "init/init-log-manager.h"

#include "command-line/option-category.h"
#include "storage/dynamic-string.h"

bool stdoutWriter = false;
bool asyncWriter  = false;

int64_t logLevel = swLogLevelInfo;
int64_t asyncWriterMaxFileSize  = 32 * 1024 * 1024;
int64_t asyncWriterMaxFileCount = 0;

swStaticString  asyncWriterDir   = swStaticStringDefineEmpty;
swStaticString  asyncWriterFile  = swStaticStringDefineEmpty;

swOptionEnumValueName logLevelEnum[] =
{
  { swLogLevelFatal,    "-fatal",   "fatal"   },
  { swLogLevelError,    "-error",   "error"   },
  { swLogLevelWarning,  "-warning", "warning" },
  { swLogLevelInfo,     "-info",    "info"    },
  { swLogLevelDebug,    "-debug",   "debug"   },
  { swLogLevelTrace,    "-trace",   "trace"   },
  { 0,                  NULL,       NULL      }
};

swOptionCategoryModuleDeclare(swLogManagerOptions, "Log Manager Options",
  swOptionDeclareScalarEnum("log-level",    "Log level",
    NULL,   &logLevel,              &logLevelEnum,        false),

  swOptionDeclareScalar("log-stdout",        "Set up log manager with stdout writer",
    NULL,   &stdoutWriter,          swOptionValueTypeBool, false),
  swOptionDeclareScalar("log-async",         "Set up log manager with asynchronous writer",
    NULL,   &asyncWriter,           swOptionValueTypeBool, false),

  swOptionDeclareScalar("log-asynch-max-file-size",   "File size limit for asynchronous writer",
    NULL,   &asyncWriterMaxFileSize,        swOptionValueTypeInt, false),
  swOptionDeclareScalar("log-asynch-max-file-count",  "Maximum number of archived files for asynchronous writer, (0 means unlimited)",
    NULL,   &asyncWriterMaxFileCount,       swOptionValueTypeInt, false),

  swOptionDeclareScalar("log-asynch-dir",     "Directory to log to with async writer",
    NULL,   &asyncWriterDir,                swOptionValueTypeString, false),
  swOptionDeclareScalar("log-asynch-file",    "File name to log to with async writer",
    NULL,   &asyncWriterFile,               swOptionValueTypeString, false)
);

static bool swInitLogManagerValidateInput()
{
  bool rtn = false;
  if (logLevel >= swLogLevelNone && logLevel < swLogLevelMax)
  {
    if (!asyncWriter || (asyncWriterFile.len && (asyncWriterMaxFileSize > 0) && (asyncWriterMaxFileCount >= 0)))
      rtn = true;
  }
  return rtn;
}

static void *logManagerArrayData[1] = {NULL};
static swLogManager *logManager = NULL;

static bool swInitLogManagerStdoutWriterAdd()
{
  bool rtn = false;
  if (logManager)
  {
    if (stdoutWriter)
    {
      swLogSink sink = {NULL};
      swLogFormatter formatter = {NULL};
      if (swLogStdoutFormatterInit(&formatter))
      {
        swLogWriter writer;
        if (swLogWriterInit(&writer, sink, formatter) && swLogManagerWriterAdd(logManager, writer))
          rtn = true;
      }
    }
    else
      rtn = true;
  }
  return rtn;
}

static bool swInitLogManagerAsyncWriterAdd(swThreadManager *threadManager)
{
  bool rtn = false;
  if (logManager && threadManager)
  {
    if (asyncWriter)
    {
      swDynamicString *logFile = swDynamicStringNewFromFormat("%s%s%s",
                                                             ((asyncWriterDir.len)? (asyncWriterDir.data) : ""),
                                                             ((asyncWriterDir.len)? "/" : ""),
                                                             asyncWriterFile.data);
      if (logFile)
      {
        swLogSink sink = {NULL};
        swLogFormatter formatter = {NULL};
        if (swLogBufferFormatterInit(&formatter) && swLogFileSinkInit(&sink, threadManager, (size_t)asyncWriterMaxFileSize, (uint32_t)asyncWriterMaxFileCount, (swStaticString *)logFile))
        {
          swLogWriter writer;
          if (swLogWriterInit(&writer, sink, formatter) && swLogManagerWriterAdd(logManager, writer))
            rtn = true;
          if (!rtn)
            sink.clearFunc(&sink);
        }
        swDynamicStringDelete(logFile);
      }
    }
    else
      rtn = true;
  }
  return rtn;
}

static bool swInitLogManagerStart()
{
  bool rtn = false;
  swThreadManager **threadManager = logManagerArrayData[0];
  if (!logManager)
  {
    if (swInitLogManagerValidateInput() && (logManager = swLogManagerNew((swLogLevel)logLevel)))
    {
      if (!stdoutWriter || swInitLogManagerStdoutWriterAdd())
      {
        if (!asyncWriter || (threadManager && *threadManager && swInitLogManagerAsyncWriterAdd(*threadManager)))
          rtn = true;
      }
    }
    if (!rtn)
    {
      swLogManagerDelete(logManager);
      logManager = NULL;
    }
  }
  return rtn;
}

static void swInitLogManagerStop()
{
  if (logManager)
  {
    swLogManagerDelete(logManager);
    logManager = NULL;
  }
  logManagerArrayData[0] = NULL;
}

static swInitData logManagerData = {.startFunc = swInitLogManagerStart, .stopFunc = swInitLogManagerStop, .name = "Log Manager"};

swInitData *swInitLogManagerDataGet(swThreadManager **threadManager)
{
  logManagerArrayData[0] = threadManager;
  return &logManagerData;
}
