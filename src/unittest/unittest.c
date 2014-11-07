#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include "unittest/unittest.h"
#include "utils/colors.h"

#define SW_TIME_FORMAT  "(%05.3lfms %05.3lfus %"PRIu64"ns)"
#define SW_TIME_FORMAT_PRINT(t) (double)(t)/1000000.0, (double)(t)/1000.0, (t)

static const char *suiteNameGlobal = NULL;
static const char *testNameGlobal = NULL;
static const char *testOffset = "    ";
static uint32_t failedAssertsGlobal = 0;
static bool colorOutputGlobal = false;
static bool insideTest = false;

static swTestSuite testSuiteGlobal __attribute__ ((section(".unittest"))) = { 0 };

typedef bool (*swTestSuiteFilterFunc)(const char *suiteNameIn);
typedef bool (*swTestFilterFunc)(const char *testNameIn);

static bool suiteAll(const char *suiteNameIn)
{
  return true;
}

static bool testAll(const char *testNameIn)
{
  return true;
}

static bool suiteFilter(const char *suiteNameIn)
{
  return (strncmp(suiteNameGlobal, suiteNameIn, strlen(suiteNameIn)) == 0);
}

static bool testFilter(const char *testNameIn)
{
  return (strncmp(testNameGlobal, testNameIn, strlen(testNameIn)) == 0);
}

static uint64_t getCurrentTime()
{
  struct timespec ts;
  uint64_t now64 = 0;
  if (!clock_gettime(CLOCK_REALTIME, &ts))
    now64 = ts.tv_sec * 1000000000 + ts.tv_nsec;
  return now64;
}

static void printfColor(const char *color, char *fmt, ...)
{
  printf("%s", ((colorOutputGlobal)? color : ""));
  va_list argp;
  va_start(argp, fmt);
  vprintf(fmt, argp);
  va_end(argp);

  printf("%s", ((colorOutputGlobal)? SW_COLOR_ANSI_NORMAL : ""));
}

void swTestLogLine(char *fmt, ...)
{
  printf("%s%s", ((insideTest)? testOffset: ""), ((colorOutputGlobal)? SW_COLOR_ANSI_BMAGENTA : ""));
  va_list argp;
  va_start(argp, fmt);
  vprintf(fmt, argp);
  va_end(argp);

  printf("%s", ((colorOutputGlobal)? SW_COLOR_ANSI_NORMAL : ""));
}

static void swTestSuiteAction(swTestSuite *suite, const char *actionName, void (*action)(swTestSuite *))
{
  printfColor (SW_COLOR_ANSI_GREEN, "%s Suite %s\n", actionName, suite->suiteName);
  uint64_t actionRuntime = getCurrentTime();
  action(suite);
  actionRuntime = getCurrentTime() - actionRuntime;
  printfColor (SW_COLOR_ANSI_GREEN, "%s Suite %s "SW_TIME_FORMAT"\n",
    actionName, suite->suiteName, SW_TIME_FORMAT_PRINT(actionRuntime));
  return;
}

static void swTestAction(swTestSuite *suite, swTest *test, const char *actionName, void (*action)(swTestSuite *, swTest *))
{
  printfColor (SW_COLOR_ANSI_GREEN, "%s%s Test %s.%s\n", testOffset, actionName, suite->suiteName, test->testName);
  uint64_t actionRuntime = getCurrentTime();
  action(suite, test);
  actionRuntime = getCurrentTime() - actionRuntime;
  printfColor (SW_COLOR_ANSI_GREEN, "%s%s Test %s.%s "SW_TIME_FORMAT"\n",
    testOffset, actionName, suite->suiteName, test->testName, SW_TIME_FORMAT_PRINT(actionRuntime));
  return;
}

static bool swTestRunExecute(swTestSuite *suite, swTest *test)
{
  bool ret = false;
  printfColor (SW_COLOR_ANSI_GREEN, "%s[ RUN      ] Test %s.%s\n", testOffset, suite->suiteName, test->testName);
  uint64_t testRuntime = getCurrentTime();
  ret = test->runFunc(suite, test);
  testRuntime = getCurrentTime() - testRuntime;
  ret = ret && (failedAssertsGlobal == 0);
  if (ret)
    printfColor (SW_COLOR_ANSI_GREEN, "%s[       OK ] Test %s.%s "SW_TIME_FORMAT"\n",
                 testOffset, suite->suiteName, test->testName, SW_TIME_FORMAT_PRINT(testRuntime));
  else
    printfColor (SW_COLOR_ANSI_RED, "%s[   FAILED ] Test %s.%s "SW_TIME_FORMAT"\n",
                 testOffset, suite->suiteName, test->testName, SW_TIME_FORMAT_PRINT(testRuntime));
  return ret;
}

int swTestMain(int argc, char *argv[])
{
  uint32_t totalSuite = 0;
  uint32_t totalTest = 0;
  uint32_t numOK = 0;
  uint32_t numFail = 0;
  uint32_t numSuiteSkip = 0;
  uint32_t numTestSkip = 0;
  uint32_t numFailedAsserts = 0;
  swTestSuiteFilterFunc suiteFilterFunc = suiteAll;
  swTestFilterFunc testFilterFunc = testAll;
  int ret = EXIT_FAILURE;

  if (argc == 2)
  {
    suiteNameGlobal = argv[1];
    suiteFilterFunc = suiteFilter;
  }

  if (argc == 3)
  {
    testNameGlobal = argv[2];
    testFilterFunc = testFilter;
  }

  colorOutputGlobal = isatty(1);

  swTestSuite *suiteBegin = &testSuiteGlobal;
  swTestSuite *suiteEnd = &testSuiteGlobal;

  // find begin and end of section by comparing magics
  while (1)
  {
    swTestSuite *current = suiteBegin - 1;
    if (current->magic != SW_TEST_MAGIC)
      break;
    suiteBegin--;
  }
  while (1)
  {
    suiteEnd++;
    if (suiteEnd->magic != SW_TEST_MAGIC)
      break;
  }

  uint64_t allSuiteRuntime = getCurrentTime();
  for (swTestSuite *suite = suiteBegin; suite != suiteEnd; suite++)
  {
    if ((suite != &testSuiteGlobal))
    {
      if ((suiteFilterFunc(suite->suiteName)) && !suite->skip)
      {
        totalSuite++;
        // print suite start
        printfColor(SW_COLOR_ANSI_BLUE, "[ START    ] Suite: '%s'\n", suite->suiteName);
        uint64_t suiteRuntime = getCurrentTime();
        if (suite->setupFunc)
          swTestSuiteAction(suite, "[ SETUP    ]", suite->setupFunc);

        insideTest = true;
        for (uint32_t i = 0; suite->tests[i] != NULL; i++)
        {
          swTest *test = suite->tests[i];
          if (testFilterFunc(test->testName) && !test->skip)
          {
            totalTest++;
            printfColor(SW_COLOR_ANSI_BLUE, "%s[ START    ] Test: '%s'\n", testOffset, test->testName);
            if (test->setupFunc)
              swTestAction(suite, test, "[ SETUP    ]", test->setupFunc);
            bool result = swTestRunExecute(suite, test);
            numOK += result;
            numFail += !result;
            numFailedAsserts += failedAssertsGlobal;
            failedAssertsGlobal = 0;
            if (test->teardownFunc)
              swTestAction(suite, test, "[ TEARDOWN ]", test->teardownFunc);
            printfColor(SW_COLOR_ANSI_BLUE, "%s[ END      ] Test: '%s'\n", testOffset, test->testName);
          }
          else
          {
            // print test skip
            printfColor(SW_COLOR_ANSI_GREY, "%s[ SKIPPED  ] Test: '%s'\n", testOffset, test->testName);
            numTestSkip++;
          }
        }
        insideTest = false;
        if (suite->teardownFunc)
          swTestSuiteAction(suite, "[ TEARDOWN ]", suite->teardownFunc);
        suiteRuntime = getCurrentTime() - suiteRuntime;
        printfColor(SW_COLOR_ANSI_BLUE, "[ END      ] Suite: '%s'\n", suite->suiteName);
      }
      else
      {
        printfColor(SW_COLOR_ANSI_GREY, "[ SKIPPED  ] Suite: '%s'\n", suite->suiteName);
        numSuiteSkip++;
      }
    }
  }
  allSuiteRuntime = getCurrentTime() - allSuiteRuntime;
  if (!numFail)
    ret = EXIT_SUCCESS;

  const char* color = (numFail) ? SW_COLOR_ANSI_BRED : SW_COLOR_ANSI_GREEN;
  printfColor(color, "[ ======== ]: %d tests from %d suites run "SW_TIME_FORMAT"\n",
              totalTest, totalSuite, SW_TIME_FORMAT_PRINT(allSuiteRuntime));
  if (numSuiteSkip || numTestSkip)
    printfColor(color, "[ ======== ]: Skipped %d suites and %d tests\n",
                numSuiteSkip, numTestSkip);
  printfColor(color, "[ PASSED   ]: %d tests\n", numOK);
  printfColor(color, "[ FAILED   ]: %d tests\n", numFail);
  printfColor(color, "[ ASSERTS  ]: %d failed asserts\n", numFailedAsserts);
  return ret;
}

void assert_str(const char *exp, const char *value, const char *conditionExp, const char *conditionValue, const char* caller, int line)
{

  if ((exp != value) && (!exp || !value || (strcmp(exp, value) != 0)))
  {
    printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', %s != %s ('%s' != '%s')\n", caller, line, conditionExp, conditionValue, exp, value);
    failedAssertsGlobal++;
  }
}

void assert_data(const char *exp, size_t expSize,
                 const char *value, size_t valueSize,
                 const char *conditionExp, const char *conditionValue,
                 const char* caller, int line)
{
  if (expSize != valueSize)
  {
    printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', %s != %s (size %zu != %zu)\n", caller, line, conditionExp, conditionValue, expSize, valueSize);
    failedAssertsGlobal++;
  }
  else
  {
    if ((exp != value) && (memcmp(exp, value, expSize) != 0))
    {
      printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', %s != %s\n", caller, line, conditionExp, conditionValue);
      failedAssertsGlobal++;
    }
  }
}


void assert_equal(long exp, long value, const char *conditionExp, const char *conditionValue, const char* caller, int line)
{
  if (exp != value)
  {
    printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', %s != %s (%ld != %ld)\n", caller, line, conditionExp, conditionValue, exp, value);
    failedAssertsGlobal++;
  }
}

void assert_not_equal(long exp, long value, const char *conditionExp, const char *conditionValue, const char* caller, int line)
{
  if (exp == value)
  {
    printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', %s == %s (%ld == %ld)\n", caller, line, conditionExp, conditionValue, exp, value);
    failedAssertsGlobal++;
  }
}

void assert_null(void *value, const char *condition, const char* caller, int line)
{
  if (value != NULL)
  {
    printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', %s == %p\n", caller, line, condition, value);
    failedAssertsGlobal++;
  }
}

void assert_not_null(void *value, const char *condition, const char* caller, int line)
{
  if (value == NULL)
  {
    printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', %s == NULL\n", caller, line, condition);
    failedAssertsGlobal++;
  }
}

void assert_true(bool value, const char *condition, const char* caller, int line)
{
  if (!value)
  {
    printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', %s == false\n", caller, line, condition);
    failedAssertsGlobal++;
  }
}

void assert_false(bool value, const char *condition, const char* caller, int line)
{
  if (value)
  {
    printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', %s == true\n", caller, line, condition);
    failedAssertsGlobal++;
  }
}

void assert_fail(const char* caller, int line)
{
  printfColor(SW_COLOR_ANSI_RED, "\t[   ASSERT ] Called from '%s:%d', failed\n", caller, line);
  failedAssertsGlobal++;
}
