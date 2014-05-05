#include <storage/static-string.h>
#include <unittest/unittest.h>

swTestDeclare(TestCompare, NULL, NULL, swTestRun)
{
  swStaticString s1 = swStaticStringDefine("aaaa");
  swStaticString s2 = swStaticStringDefine("bbbb");
  swStaticString s3 = swStaticStringDefine("aaaa");
  swStaticString s4 = swStaticStringDefine("aaaaa");

  ASSERT_EQUAL(swStaticStringCompare(&s1, &s2), -1);
  ASSERT_EQUAL(swStaticStringCompare(&s2, &s1), 1);
  ASSERT_EQUAL(swStaticStringCompare(&s1, &s1), 0);
  ASSERT_EQUAL(swStaticStringCompare(&s1, &s3), 0);
  ASSERT_EQUAL(swStaticStringCompare(&s1, &s4), -1);
  ASSERT_EQUAL(swStaticStringCompare(&s4, &s1), 1);
  ASSERT_EQUAL(swStaticStringCompare(&s2, &s4), 1);
  ASSERT_EQUAL(swStaticStringCompare(&s4, &s2), -1);
  ASSERT_EQUAL(swStaticStringCompare(NULL, NULL), 0);
  ASSERT_EQUAL(swStaticStringCompare(&s1, NULL), 1);
  ASSERT_EQUAL(swStaticStringCompare(NULL, &s1), -1);

  return true;
}

swTestDeclare(TestEqual, NULL, NULL, swTestRun)
{
  swStaticString s1 = swStaticStringDefine("aaaa");
  swStaticString s2 = swStaticStringDefine("bbbb");
  swStaticString s3 = swStaticStringDefine("aaaa");
  swStaticString s4 = swStaticStringDefine("aaaaa");

  ASSERT_EQUAL(swStaticStringEqual(&s1, &s2), false);
  ASSERT_EQUAL(swStaticStringEqual(&s2, &s1), false);
  ASSERT_EQUAL(swStaticStringEqual(&s1, &s1), true);
  ASSERT_EQUAL(swStaticStringEqual(&s1, &s3), true);
  ASSERT_EQUAL(swStaticStringEqual(&s1, &s4), false);
  ASSERT_EQUAL(swStaticStringEqual(&s4, &s1), false);
  ASSERT_EQUAL(swStaticStringEqual(&s2, &s4), false);
  ASSERT_EQUAL(swStaticStringEqual(&s4, &s2), false);
  ASSERT_EQUAL(swStaticStringEqual(NULL, NULL), true);
  ASSERT_EQUAL(swStaticStringEqual(&s1, NULL), false);
  ASSERT_EQUAL(swStaticStringEqual(NULL, &s1), false);

  return true;
}

swTestDeclare(TestHash, NULL, NULL, swTestRun)
{
  swStaticString s1 = swStaticStringDefine("aaaa");

  ASSERT_NOT_EQUAL(swStaticStringHash(&s1), 0);
  ASSERT_EQUAL(swStaticStringHash(NULL), 0);

  return true;
}

swTestSuiteStructDeclare(StaticStrungUnittest, NULL, NULL, swTestRun,
  &TestCompare, &TestEqual, &TestHash);

