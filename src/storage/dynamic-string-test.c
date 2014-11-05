#include <storage/dynamic-string.h>
#include <unittest/unittest.h>

#include <stdio.h>

swTestDeclare(TestNewFromFormat, NULL, NULL, swTestRun)
{
  swDynamicString *dynamicString = swDynamicStringNewFromFormat("test");
  ASSERT_NOT_NULL(dynamicString);
  swStaticString staticString1 = swStaticStringDefine("test");
  ASSERT_EQUAL(swStaticStringCompare((swStaticString *)dynamicString, &staticString1), 0);
  swDynamicStringDelete(dynamicString);
  dynamicString = swDynamicStringNewFromFormat("test: %d", 111);
  ASSERT_NOT_NULL(dynamicString);
  swStaticString staticString2 = swStaticStringDefine("test: 111");
  ASSERT_EQUAL(swStaticStringCompare((swStaticString *)dynamicString, &staticString2), 0);
  swDynamicStringDelete(dynamicString);

  return true;
}

swTestDeclare(TestSetFromFormat, NULL, NULL, swTestRun)
{
  swDynamicString *dynamicString = swDynamicStringNew(16);
  ASSERT_NOT_NULL(dynamicString);
  ASSERT_TRUE(swDynamicStringSetFromFormat(dynamicString, "test, test, test, test"));
  swStaticString staticString1 = swStaticStringDefine("test, test, test, test");
  ASSERT_EQUAL(swStaticStringCompare((swStaticString *)dynamicString, &staticString1), 0);
  ASSERT_TRUE(swDynamicStringSetFromFormat(dynamicString, "test, test, test, test: %d, %d, %d, %d", 1, 2, 3, 4));
  swStaticString staticString2 = swStaticStringDefine("test, test, test, test: 1, 2, 3, 4");
  ASSERT_EQUAL(swStaticStringCompare((swStaticString *)dynamicString, &staticString2), 0);
  swDynamicStringDelete(dynamicString);

  return true;
}

swTestSuiteStructDeclare(DynamicStringUnittest, NULL, NULL, swTestRun,
  &TestNewFromFormat, &TestSetFromFormat);
