#include "hash-set-linear.h"

#include "unittest/unittest.h"
#include "storage/static-string.h"
#include "core/memory.h"

// TODO: finish test implementation
// TODO: add dictionary test

void setSetup(swTestSuite *suite, swTest *test)
{
  swTestLogLine("Creating hash set ...\n");
  swHashSetLinear *set = swHashSetLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL);
  ASSERT_NOT_NULL(set);
  swTestDataSet(test, set);
}

void setTeardown(swTestSuite *suite, swTest *test)
{
  swTestLogLine("Deleting hash set ...\n");
  swHashSetLinear *set = swTestDataGet(test);
  ASSERT_NOT_NULL(set);
  swHashSetLinearDelete(set);
}

swTestDeclare(BasicTest, setSetup, setTeardown, swTestRun)
{
  swHashSetLinear *set = swTestDataGet(test);
  ASSERT_NOT_NULL(set);

  swStaticString s1 = swStaticStringDefine("test string");
  swStaticString s2 = swStaticStringDefineFromCstr("test string");

  swTestLogLine ("s1.len = %lu\n", s1.len);
  swTestLogLine("s2.len = %lu\n", s2.len);
  ASSERT_DATA(s1.data, s1.len, s2.data, s2.len);
  ASSERT_EQUAL(swStaticStringEqual(&s1, &s2), true);

  ASSERT_TRUE (swHashSetLinearInsert(set, &s1));
  ASSERT_EQUAL(swHashSetLinearCount(set), 1);
  ASSERT_TRUE (swHashSetLinearContains(set, &s2));
  ASSERT_TRUE(swHashSetLinearRemove(set, &s2));
  ASSERT_EQUAL(swHashSetLinearCount(set), 0);
  ASSERT_TRUE (swHashSetLinearInsert(set, &s1));
  ASSERT_TRUE(swHashSetLinearExtract(set, &s2) == &s1);
  ASSERT_EQUAL(swHashSetLinearCount(set), 0);
  ASSERT_TRUE (swHashSetLinearInsert(set, &s1));
  ASSERT_TRUE (swHashSetLinearUpsert(set, &s2));
  ASSERT_TRUE(swHashSetLinearExtract(set, &s1) == &s2);
  ASSERT_EQUAL(swHashSetLinearCount(set), 0);

  return true;
}

swTestSuiteStructDeclare(HashSetLinearTest, NULL, NULL, swTestRun, &BasicTest);