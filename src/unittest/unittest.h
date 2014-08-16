#ifndef SW_UNITTEST_UNITTEST_H
#define SW_UNITTEST_UNITTEST_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#define SW_TEST_MAGIC (0xdeadbeef)

typedef struct swTest swTest;
typedef struct swTestSuite swTestSuite;

typedef void (*swTestSetupFunc)(swTestSuite *suite, swTest *test);
typedef void (*swTestTeardownFunc)(swTestSuite *suite, swTest *test);
typedef bool (*swTestRunFunc)(swTestSuite *suite, swTest *test);


struct swTest
{
  const char *testName;
  swTestSetupFunc setupFunc;
  swTestTeardownFunc teardownFunc;
  swTestRunFunc runFunc;
  void *data;

  unsigned skip : 1;
};

static inline void swTestDataSet(swTest *test, void *data)
{
  if(test)
    test->data = data;
}

static inline void *swTestDataGet(swTest *test)
{
  return (test)? test->data : NULL;
}

typedef void (*swTestSuiteSetupFunc)(swTestSuite *suite);
typedef void (*swTestSuiteTeardownFunc)(swTestSuite *suite);

struct swTestSuite
{
  const char *suiteName;
  swTestSuiteSetupFunc setupFunc;
  swTestSuiteTeardownFunc teardownFunc;
  swTest **tests;
  void *data;

  uint32_t magic;
  unsigned skip : 1;
  uint64_t unused1;
  uint64_t unused2;
};

static inline void swTestSuiteDataSet(swTestSuite *suite, void *data)
{
  if(suite)
    suite->data = data;
}

static inline void *swTestSuiteDataGet(swTestSuite *suite)
{
  return (suite)? suite->data : NULL;
}

#define swTestSkip  true
#define swTestRun   false

#define swTestSuiteStructDeclare(suiteNameIn, setup, teardown, s, ...) \
  static swTestSuite suiteNameIn __attribute__ ((unused, section(".unittest"))) = \
  { \
    .suiteName = #suiteNameIn, \
    .setupFunc = (swTestSuiteSetupFunc) setup, \
    .teardownFunc = (swTestSuiteTeardownFunc) teardown, \
    .data = NULL, \
    .tests = (swTest *[]){ __VA_ARGS__, NULL}, \
    .magic = SW_TEST_MAGIC, \
    .skip = (s) \
  }

#define swTestDeclare(testNameIn, setup, teardown, s) \
  bool testNameIn##Run(struct swTestSuite *suite, struct swTest *test); \
  swTest testNameIn = \
  { \
    .testName = #testNameIn, \
    .setupFunc = (swTestSetupFunc) setup, \
    .teardownFunc = (swTestTeardownFunc) teardown, \
    .runFunc = (swTestRunFunc) testNameIn##Run, \
    .data = NULL, \
    .skip = (s) \
  }; \
  bool testNameIn##Run(struct swTestSuite *suite, struct swTest *test)

void swTestLogLine(char *fmt, ...);
#define swTestLog(fmt, ...) swTestLogLine("'%s():%d': " fmt, __func__, __LINE__, ##__VA_ARGS__)

void assert_str(const char *exp, const char *value, const char *conditionExp, const char *conditionValue, const char *caller, int line);
#define ASSERT_STR(e, v)        assert_str(e, v, #e, #v, __FILE__, __LINE__)

void assert_data(const char *exp, size_t expSize,
                 const char *value, size_t valueSize,
                 const char *conditionExp, const char *conditionValue,
                 const char *caller, int line);
#define ASSERT_DATA(e, es, v, vs) assert_data(e, es, v, vs, #e, #v,__FILE__, __LINE__)

void assert_equal(long exp, long value, const char *conditionExp, const char *conditionValue, const char *caller, int line);
#define ASSERT_EQUAL(e, v)      assert_equal(e, v, #e, #v, __FILE__, __LINE__)

void assert_not_equal(long exp, long value, const char *conditionExp, const char *conditionValue, const char *caller, int line);
#define ASSERT_NOT_EQUAL(e, v)  assert_not_equal(e, v, #e, #v, __FILE__, __LINE__)

void assert_null(void *value, const char *condition, const char *caller, int line);
#define ASSERT_NULL(v)          assert_null((void*)v, #v, __FILE__, __LINE__)

void assert_not_null(void *value, const char *condition, const char *caller, int line);
#define ASSERT_NOT_NULL(v)      assert_not_null((void*)v, #v,__FILE__, __LINE__)

void assert_true(bool value, const char *condition, const char *caller, int line);
#define ASSERT_TRUE(v)          assert_true(v, #v, __FILE__, __LINE__)

void assert_false(bool value, const char *condition, const char *caller, int line);
#define ASSERT_FALSE(v)         assert_false(v, #v, __FILE__, __LINE__)

void assert_fail(const char *caller, int line);
#define ASSERT_FAIL()           assert_fail(__FILE__, __LINE__)

#endif // SW_UNITTEST_UNITTEST_H
