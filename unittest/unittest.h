#ifndef SW_UNITTEST_TEST_H
#define SW_UNITTEST_TEST_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#define SW_TEST_MAGIC (0xdeadbeef)

typedef void (*swTestSetupFunc)(void *, void *);
typedef void (*swTestTeardownFunc)(void *, void *);
typedef bool (*swTestRunFunc)(void *, void *);

typedef struct swTest
{
    const char *testName;
    swTestSetupFunc setupFunc;
    swTestTeardownFunc teardownFunc;
    swTestRunFunc runFunc;
    void *data;

    unsigned skip : 1;
} swTest;

typedef void (*swTestSuiteSetupFunc)(void *);
typedef void (*swTestSuiteTeardownFunc)(void *);

typedef struct swTestSuite
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
} swTestSuite;

#define swTestSkip  skip
#define swTestRun   false

#define swTestSuiteDataDeclare(suiteName) \
    typedef struct suiteName##Data suiteName##Data; \
    struct suiteName##Data

#define swTestSuiteSetupDeclare(suiteName) \
    void suiteName##Setup(suiteName##Data *suiteData)

#define swTestSuiteTeardownDeclare(suiteName) \
    void suiteName##Teardown(suiteName##Data *suiteData)

#define swTestSuiteStructDeclare(suiteNameIn, s, ...) \
    static suiteNameIn##Data suiteNameIn##DataStorage; \
    static swTestSuite suiteNameIn __attribute__ ((unused, section(".unittest"))) = \
    { \
      .suiteName = #suiteNameIn, \
      .setupFunc = (swTestSuiteSetupFunc) suiteNameIn##Setup, \
      .teardownFunc = (swTestSuiteTeardownFunc) suiteNameIn##Teardown, \
      .data = &suiteNameIn##DataStorage, \
      .tests = (swTest *[]){ __VA_ARGS__, NULL}, \
      .magic = SW_TEST_MAGIC, \
      .skip = (s) \
    }

#define swTestDataDeclare(testName) \
    typedef struct testName##Data testName##Data; \
    struct testName##Data

#define swTestSetupDeclare(suiteName, testName) \
    void testName##Setup(suiteName##Data *suiteData, testName##Data *testData)

#define swTestTeardownDeclare(suiteName,testName) \
    void testName##Teardown(suiteName##Data *suiteData, testName##Data *testData)

#define swTestRunDeclare(suiteName, testName) \
    bool testName##Run(suiteName##Data *suiteData, testName##Data *testData)

#define swTestStructDeclare(testNameIn, s) \
    static testNameIn##Data testNameIn##DataStorage; \
    swTest testNameIn = \
    { \
      .testName = #testNameIn, \
      .setupFunc = (swTestSetupFunc) testNameIn##Setup, \
      .teardownFunc = (swTestTeardownFunc) testNameIn##Teardown, \
      .runFunc = (swTestRunFunc) testNameIn##Run, \
      .data = &testNameIn##DataStorage, \
      .skip = (s) \
    }

int swTestMain(int argc, char *argv[]);

void assert_str(const char *exp, const char *value, const char *conditionExp, const char *conditionValue, const char* caller, int line);
#define ASSERT_STR(e, v)        assert_str(e, v, #e, #v, __FILE__, __LINE__)

void assert_data(const unsigned char *exp, size_t expSize,
                 const unsigned char *value, size_t valueSize,
                 const char *conditionExp, const char *conditionValue,
                 const char* caller, int line);
#define ASSERT_DATA(e, es, v, vs) \
                                assert_data(e, es, v, vs, #e, #v,__FILE__, __LINE__)

void assert_equal(long exp, long value, const char *conditionExp, const char *conditionValue, const char* caller, int line);
#define ASSERT_EQUAL(e, v)      assert_equal(e, v, #e, #v, __FILE__, __LINE__)

void assert_not_equal(long exp, long value, const char *conditionExp, const char *conditionValue, const char* caller, int line);
#define ASSERT_NOT_EQUAL(e, v)  assert_not_equal(e, v, #e, #v, __FILE__, __LINE__)

void assert_null(void *value, const char *condition, const char* caller, int line);
#define ASSERT_NULL(v)          assert_null((void*)v, #v, __FILE__, __LINE__)

void assert_not_null(void *value, const char *condition, const char* caller, int line);
#define ASSERT_NOT_NULL(v)      assert_not_null((void*)v, #v,__FILE__, __LINE__)

void assert_true(bool value, const char *condition, const char* caller, int line);
#define ASSERT_TRUE(v)          assert_true(v, #v, __FILE__, __LINE__)

void assert_false(bool value, const char *condition, const char* caller, int line);
#define ASSERT_FALSE(v)         assert_false(v, #v, __FILE__, __LINE__)

void assert_fail(const char* caller, int line);
#define ASSERT_FAIL()           assert_fail(__FILE__, __LINE__)

#endif
