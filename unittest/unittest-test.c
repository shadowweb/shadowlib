#include <stdio.h>

#include "unittest.h"

// _Bool TestAssertStrRun(struct swTestSuite *suite, struct swTest *test);

swTestDeclare(TestAssertStr, NULL, NULL, swTestRun)
{
  ASSERT_STR("foo", "foo");
  ASSERT_STR("foo", "bar");
  return true;
}

swTestDeclare(TestAssertNotEqual, NULL, NULL, swTestRun)
{
  ASSERT_NOT_EQUAL(123, 456);
  ASSERT_NOT_EQUAL(123, 123);
  return true;
}
swTestDeclare(TestAssertNull, NULL, NULL, swTestRun)
{
  ASSERT_NULL(NULL);
  ASSERT_NULL((void*)0xdeadbeef);
  return true;
}

swTestDeclare(TestAssertNotNull, NULL, NULL, swTestRun)
{
  ASSERT_NOT_NULL((void*)0xdeadbeef);
  ASSERT_NOT_NULL(NULL);
  return true;
}

swTestDeclare(TestAssertTrue, NULL, NULL, swTestRun)
{
  ASSERT_TRUE(1);
  ASSERT_TRUE(0);
  return true;
}

swTestDeclare(TestAssertFalse, NULL, NULL, swTestRun)
{
  ASSERT_FALSE(0);
  ASSERT_FALSE(1);
  return true;
}

swTestDeclare(TestSkip, NULL, NULL, swTestSkip)
{
  ASSERT_FAIL();
  return true;
}

swTestDeclare(TestAssertFail, NULL, NULL, swTestRun)
{
  ASSERT_FAIL();
  return true;
}

/* Test that NULL-strings won't result in segv */
swTestDeclare(TestNullNull, NULL, NULL, swTestRun)
{
  ASSERT_STR(NULL, NULL);
  return true;
}

swTestDeclare(TestNullString, NULL, NULL, swTestRun)
{
  ASSERT_STR(NULL, "shouldfail");
  return true;
}

swTestDeclare(TestStringNull, NULL, NULL, swTestRun)
{
  ASSERT_STR("shouldfail", NULL);
  return true;
}

swTestDeclare(TestStringDifferentPointers, NULL, NULL, swTestRun)
{
  const char *str = "abc\0abc";
  ASSERT_STR(str, str+4);
  return true;
}

swTestDeclare(TestLargeNumbers, NULL, NULL, swTestRun)
{
  long exp = 7200000000;
  ASSERT_EQUAL(exp, 7200000000);
  ASSERT_NOT_EQUAL(exp, 1200000000);
  return true;
}

swTestSuiteStructDeclare(Unittest, NULL, NULL, swTestRun,
    &TestAssertStr, &TestAssertNotEqual, &TestAssertNull, &TestAssertNotNull,
    &TestAssertTrue, &TestAssertFalse, &TestSkip, &TestAssertFail,
    &TestNullNull, &TestNullString, &TestStringNull, &TestStringDifferentPointers,
    &TestLargeNumbers);


swTestDeclare(Test1, NULL, NULL, swTestRun)
{
  printf ("'%s': is running\n", __func__);
  return true;
}

void swNoSetupTestTeardown(swTestSuite *suite)
{
  printf ("'%s': is running\n", __func__);
}

swTestSuiteStructDeclare(NoSetup, NULL, swNoSetupTestTeardown, swTestRun, &Test1);

struct swMemtestData
{
  unsigned char* buffer;
} memtestData;

void swMemtestSetup(swTestSuite *suite, swTest *test)
{
  memtestData.buffer = (unsigned char*)malloc(1024);
  ASSERT_NOT_NULL(memtestData.buffer);
  swTestDataSet(test, &memtestData);
}

void swMemtestTeardown(swTestSuite *suite, swTest *test)
{
  struct swMemtestData *data = swTestDataGet(test);
  if (data->buffer)
  {
    free(data->buffer);
    data->buffer = NULL;
  }
}

swTestDeclare(MemTest1, swMemtestSetup, swMemtestTeardown, swTestRun)
{
  struct swMemtestData *data = swTestDataGet(test);
  printf("'%s': data = %p, buffer = %p\n", __func__, data, data->buffer);
  return true;
}

swTestDeclare(MemTest2, swMemtestSetup, swMemtestTeardown, swTestRun)
{
  struct swMemtestData *data = swTestDataGet(test);
  printf("'%s': data = %p, buffer = %p\n", __func__, data, data->buffer);
  ASSERT_FAIL();
  return true;
}

swTestDeclare(MemTest3, swMemtestSetup, swMemtestTeardown, swTestSkip)
{
  ASSERT_FAIL();
  return true;
}

swTestSuiteStructDeclare(MemTest, NULL, NULL, swTestRun, &MemTest1, &MemTest2, &MemTest3);

/*
swTestSuiteDataDeclare(testSuiteBla)
{
  int a;
};
swTestSuiteSetupDeclare(testSuiteBla)
{
  suiteData->a = 1;
  return;
}
swTestSuiteTeardownDeclare(testSuiteBla)
{
  suiteData->a = 0;
  return;
}

swTestDataDeclare(testNameBla)
{
  int a;
};

swTestSetupDeclare(testSuiteBla, testNameBla)
{
  suiteData->a++;
  testData->a = 1;
  return;
}

swTestTeardownDeclare(testSuiteBla, testNameBla)
{
  suiteData->a--;
  testData->a = 0;
  return;
}

swTestRunDeclare(testSuiteBla, testNameBla)
{
  return true;
}

swTestStructDeclare(testNameBla, swTestRun);

swTestDataDeclare(testNameBla1)
{
  int a;
};

swTestSetupDeclare(testSuiteBla, testNameBla1)
{
  testData->a = 1;
  return;
}

swTestTeardownDeclare(testSuiteBla, testNameBla1)
{
  testData->a = 0;
  return;
}

swTestRunDeclare(testSuiteBla, testNameBla1)
{
  ASSERT_FAIL();
  return true;
}

swTestStructDeclare(testNameBla1, swTestRun);

swTestSuiteStructDeclare(testSuiteBla, swTestRun, &testNameBla, &testNameBla1);
*/