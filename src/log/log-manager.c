#include "log/log-manager.h"
#include "utils/colors.h"
#include <core/memory.h>

void swLoggerLevelSet(swLogger *logger, swLogLevel level, bool useManagerLevel)
{
  if (logger && ((!level && useManagerLevel) || (level && level < swLogLevelMax && !useManagerLevel)))
  {
    logger->level = level;
    logger->useManagerLevel = useManagerLevel;
  }
}

static const char *swLogLevelText[] =
{
  [swLogLevelFatal]   = "[F]",
  [swLogLevelError]   = "[E]",
  [swLogLevelWarning] = "[W]",
  [swLogLevelInfo]    = "[I]",
  [swLogLevelDebug]   = "[D]",
  [swLogLevelTrace]   = "[T]"
};

const char *swLogLevelTextGet(swLogLevel level)
{
  if (level && level < swLogLevelMax)
    return swLogLevelText[level];
  return NULL;
}

static const char *swLogLevelColor[] =
{
  [swLogLevelNone]    = SW_COLOR_ANSI_NORMAL,
  [swLogLevelFatal]   = SW_COLOR_ANSI_CUSTOMBRIGHTRED,
  [swLogLevelError]   = SW_COLOR_ANSI_CUSTOMORANGE,
  [swLogLevelWarning] = SW_COLOR_ANSI_CUSTOMYELLOW,
  [swLogLevelInfo]    = SW_COLOR_ANSI_CUSTOMGREEN,
  [swLogLevelDebug]   = SW_COLOR_ANSI_CUSTOMBLUE,
  [swLogLevelTrace]   = SW_COLOR_ANSI_CUSTOMLIGHTBLUE
};

const char *swLogLevelColorGet(swLogLevel level)
{
  if (level >= 0 && level < swLogLevelMax)
    return swLogLevelColor[level];
  return NULL;
}

swLoggerDeclare(managerGlobalLogger, "manager");

static bool swLogManagerCollectLoggers(swLogManager *manager)
{
  bool rtn = false;
  if (manager)
  {
    swLogger *loggerBegin = &managerGlobalLogger;
    swLogger *loggerEnd = &managerGlobalLogger;

    // find begin and end of section by comparing magics
    while (1)
    {
      swLogger *current = loggerBegin - 1;
      if (current->magic != SW_LOGGER_MAGIC)
        break;
      loggerBegin--;
    }
    while (1)
    {
      loggerEnd++;
      if (loggerEnd->magic != SW_LOGGER_MAGIC)
        break;
    }

    swLogger *logger = loggerBegin;
    for (; logger != loggerEnd; logger++)
    {
      if (!swHashMapLinearInsert(&(manager->loggerMap), &(logger->name), logger))
        break;
    }
    if (logger == loggerEnd)
      rtn = true;

  }
  return rtn;
}

swLogManager *swLogManagerNew(swLogLevel level)
{
  swLogManager *rtn = NULL;
  if (level && level < swLogLevelMax)
  {
    swLogManager *manager = swMemoryMalloc(sizeof(*manager));
    if (manager)
    {
      if (swHashMapLinearInit(&(manager->loggerMap), (swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL))
      {
        if (swFastArrayInit(&(manager->logWriters), sizeof(swLogWriter), 5))
        {
          if (swLogManagerCollectLoggers(manager))
          {
            manager->level = level;
            rtn = manager;
          }
          if (!rtn)
            swFastArrayClear(&(manager->logWriters));
        }
        if (!rtn)
          swHashMapLinearRelease(&(manager->loggerMap));
      }
      if (!rtn)
        swMemoryFree(manager);
    }
  }
  return rtn;
}

void swLogManagerDelete(swLogManager *manager)
{
  if(manager)
  {
    swLogWriter *writers = (swLogWriter *)swFastArrayData(manager->logWriters);
    for (uint32_t i = 0; i < swFastArrayCount(manager->logWriters); i++)
      swLogWriterClear(&(writers[i]));
    swFastArrayClear(&(manager->logWriters));
    swHashMapLinearRelease(&(manager->loggerMap));
    swMemoryFree(manager);
  }
}

void swLogManagerLevelSet(swLogManager *manager, swLogLevel level)
{
  if (manager && level && level < swLogLevelMax)
    manager->level = level;
}

void swLogManagerLoggerLevelSet(swLogManager *manager, swStaticString *name, swLogLevel level, bool useManagerLevel)
{
  if (manager && name)
  {
    swLogger *logger = NULL;
    if (swHashMapLinearValueGet(&(manager->loggerMap), name, (void **)&logger) && logger)
      swLoggerLevelSet(logger, level, useManagerLevel);
  }
}

// void swLogManagerLoggerAdd(swLogManager *manager, struct swLogger *logger);
bool swLogManagerWriterAdd(swLogManager *manager, swLogWriter writer)
{
  return manager && swFastArrayPush(manager->logWriters, writer);
}

bool swLogWriterInit(swLogWriter *writer, swLogSink sink, swLogFormatter formatter)
{
  bool rtn = false;
  if (writer)
  {
    writer->sink = sink;
    writer->formatter = formatter;
    rtn = true;
  }
  return rtn;
}

void swLogWriterClear(swLogWriter *writer)
{
  if (writer)
  {
    writer->formatter.clearFunc(&(writer->formatter));
    writer->sink.clearFunc(&(writer->sink));
  }
}
