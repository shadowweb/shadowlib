#include "unittest/unittest.h"

#include "log/log-manager.h"
#include "utils/colors.h"

swTestDeclare(TestPrintfLogging, NULL, NULL, swTestRun)
{
  SW_LOG_FATAL(NULL, "test format: %s", "string to format");
  SW_LOG_FATAL_CONT(NULL, "some more: test format: %s", "string to format");
  SW_LOG_FATAL(NULL, "test format without arguments");

  SW_LOG_ERROR(NULL, "test format: %s", "string to format");
  SW_LOG_ERROR_CONT(NULL, "some more: test format: %s", "string to format");
  SW_LOG_ERROR(NULL, "test format without arguments");

  SW_LOG_WARNING(NULL, "test format: %s", "string to format");
  SW_LOG_WARNING_CONT(NULL, "some more: test format: %s", "string to format");
  SW_LOG_WARNING(NULL, "test format without arguments");

  SW_LOG_INFO(NULL, "test format: %s", "string to format");
  SW_LOG_INFO_CONT(NULL, "some more: test format: %s", "string to format");
  SW_LOG_INFO(NULL, "test format without arguments");

  SW_LOG_DEBUG(NULL, "test format: %s", "string to format");
  SW_LOG_DEBUG_CONT(NULL, "some more: test format: %s", "string to format");
  SW_LOG_DEBUG(NULL, "test format without arguments");

  SW_LOG_TRACE(NULL, "test format: %s", "string to format");
  SW_LOG_TRACE_CONT(NULL, "some more: test format: %s", "string to format");
  SW_LOG_TRACE(NULL, "test format without arguments");

  for (uint32_t i = 0; i < 36; i++)
  {
    for (uint32_t j = 0; j < 36; j++)
      printf ("%s%um%04u%s  ", "\033[38;5;", (i*36 + j), (i*36 + j), SW_COLOR_ANSI_NORMAL);
    printf("\n");
  }
  return true;
}

swTestSuiteStructDeclare(BasicLogTest, NULL, NULL, swTestRun,
  &TestPrintfLogging);
