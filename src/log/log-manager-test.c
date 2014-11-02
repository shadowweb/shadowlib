#include "unittest/unittest.h"

#include "log/log-manager.h"
#include "utils/colors.h"

swLoggerDeclareWithLevel(testLogger, "TestLogger", swLogLevelInfo);

swTestDeclare(TestColors, NULL, NULL, swTestRun)
{
  uint32_t firstRowCount = 16;
  uint32_t lastRowCount = 24;
  uint32_t width = 6;
  uint32_t depth = (256 - firstRowCount - lastRowCount)/width;
  uint32_t current = 0;
  for (; current < firstRowCount; current++)
    printf ("%s%um%03u%s  ", "\033[38;5;", current, current, SW_COLOR_ANSI_NORMAL);
  printf("\n");

  for (uint32_t i = 0; i < depth; i++)
  {
    for (uint32_t j = 0; j < width; j++, current++)
      printf ("%s%um%03u%s  ", "\033[38;5;", current, current, SW_COLOR_ANSI_NORMAL);
    printf("\n");
  }

  for (; current < 256; current++)
    printf ("%s%um%03u%s  ", "\033[38;5;", current, current, SW_COLOR_ANSI_NORMAL);
  printf("\n");
  return true;
}

static void printLogs(swLogger *logger)
{
  SW_LOG_FATAL(logger, "test format: %s", "string to format");
  SW_LOG_FATAL_CONT(logger, "some more: test format: %s", "string to format");
  SW_LOG_FATAL(logger, "test format without arguments");

  SW_LOG_ERROR(logger, "test format: %s", "string to format");
  SW_LOG_ERROR_CONT(logger, "some more: test format: %s", "string to format");
  SW_LOG_ERROR(logger, "test format without arguments");

  SW_LOG_WARNING(logger, "test format: %s", "string to format");
  SW_LOG_WARNING_CONT(logger, "some more: test format: %s", "string to format");
  SW_LOG_WARNING(logger, "test format without arguments");

  SW_LOG_INFO(logger, "test format: %s", "string to format");
  SW_LOG_INFO_CONT(logger, "some more: test format: %s", "string to format");
  SW_LOG_INFO(logger, "test format without arguments");

  SW_LOG_DEBUG(logger, "test format: %s", "string to format");
  SW_LOG_DEBUG_CONT(logger, "some more: test format: %s", "string to format");
  SW_LOG_DEBUG(logger, "test format without arguments");

  SW_LOG_TRACE(logger, "test format: %s", "string to format");
  SW_LOG_TRACE_CONT(logger, "some more: test format: %s", "string to format");
  SW_LOG_TRACE(logger, "test format without arguments");
}

swTestDeclare(TestLoggingWithoutLogger, NULL, NULL, swTestRun)
{
  SW_LOG_TRACE(NULL, "test format: %s", "string to format");
  SW_LOG_TRACE_CONT(NULL, "some more: test format: %s", "string to format");
  SW_LOG_TRACE(NULL, "test format without arguments");

  printLogs(NULL);

  return true;
}

swTestDeclare(TestLoggingWithLogger, NULL, NULL, swTestRun)
{
  SW_LOG_TRACE(&testLogger, "test format: %s", "string to format");
  SW_LOG_TRACE_CONT(&testLogger, "some more: test format: %s", "string to format");
  SW_LOG_TRACE(&testLogger, "test format without arguments");

  printLogs(&testLogger);
  swLoggerLevelSet(&testLogger, swLogLevelTrace, true);
  printLogs(&testLogger);
  swLoggerLevelSet(&testLogger, swLogLevelInfo, true);
  return true;
}

swTestDeclare(TestLoggingWithLogManager, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swLogManager *manager = swLogManagerNew(swLogLevelInfo);
  if (manager)
  {
    SW_LOG_TRACE(&testLogger, "test format: %s", "string to format");
    SW_LOG_TRACE_CONT(&testLogger, "some more: test format: %s", "string to format");
    SW_LOG_TRACE(&testLogger, "test format without arguments");

    ASSERT_NOT_NULL(testLogger.manager);
    printLogs(&testLogger);
    swLogManagerLevelSet(manager, swLogLevelTrace);
    printLogs(&testLogger);
    swLogManagerLevelSet(manager, swLogLevelInfo);
    rtn = true;
    swLogManagerDelete(manager);
  }
  return rtn;
}

swTestDeclare(TestLoggingWithStdoutFormatter, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swLogManager *manager = swLogManagerNew(swLogLevelInfo);
  if (manager)
  {
    swLogSink sink = {NULL};
    swLogFormatter formatter = {NULL};
    if (swLogStdoutFormatterInit(&formatter))
    {
      swLogWriter writer;
      if (swLogWriterInit(&writer, sink, formatter) && swLogManagerWriterAdd(manager, writer))
      {
        SW_LOG_TRACE(&testLogger, "test format: %s", "string to format");
        SW_LOG_TRACE_CONT(&testLogger, "some more: test format: %s", "string to format");
        SW_LOG_TRACE(&testLogger, "test format without arguments");

        ASSERT_NOT_NULL(testLogger.manager);
        printLogs(&testLogger);
        swLogManagerLevelSet(manager, swLogLevelTrace);
        printLogs(&testLogger);
        swLogManagerLevelSet(manager, swLogLevelInfo);
        rtn = true;
      }
    }
    swLogManagerDelete(manager);
  }
  return rtn;
}

swTestSuiteStructDeclare(BasicLogTest, NULL, NULL, swTestRun,
                         &TestColors, &TestLoggingWithoutLogger, &TestLoggingWithLogger, &TestLoggingWithLogManager, &TestLoggingWithStdoutFormatter);
