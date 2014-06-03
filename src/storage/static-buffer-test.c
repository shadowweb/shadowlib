#include <storage/static-buffer.h>
#include <unittest/unittest.h>

swTestDeclare(TestCompare, NULL, NULL, swTestRun)
{
  char d1[] = {0x01, 0x01, 0x01, 0x01};
  char d2[] = {0x02, 0x02, 0x02, 0x02};
  char d3[] = {0x01, 0x01, 0x01, 0x01};
  char d4[] = {0x01, 0x01, 0x01, 0x01, 0x01};
  swStaticBuffer b1 = swStaticBufferDefine(d1);
  swStaticBuffer b2 = swStaticBufferDefine(d2);
  swStaticBuffer b3 = swStaticBufferDefine(d3);
  swStaticBuffer b4 = swStaticBufferDefine(d4);

  ASSERT_EQUAL(swStaticBufferCompare(&b1, &b2), -1);
  ASSERT_EQUAL(swStaticBufferCompare(&b2, &b1), 1);
  ASSERT_EQUAL(swStaticBufferCompare(&b1, &b1), 0);
  ASSERT_EQUAL(swStaticBufferCompare(&b1, &b3), 0);
  ASSERT_EQUAL(swStaticBufferCompare(&b1, &b4), -1);
  ASSERT_EQUAL(swStaticBufferCompare(&b4, &b1), 1);
  ASSERT_EQUAL(swStaticBufferCompare(&b2, &b4), 1);
  ASSERT_EQUAL(swStaticBufferCompare(&b4, &b2), -1);
  ASSERT_EQUAL(swStaticBufferCompare(NULL, NULL), 0);
  ASSERT_EQUAL(swStaticBufferCompare(&b1, NULL), 1);
  ASSERT_EQUAL(swStaticBufferCompare(NULL, &b1), -1);

  return true;
}

swTestDeclare(TestEqual, NULL, NULL, swTestRun)
{
  char d1[] = {0x01, 0x01, 0x01, 0x01};
  char d2[] = {0x02, 0x02, 0x02, 0x02};
  char d3[] = {0x01, 0x01, 0x01, 0x01};
  char d4[] = {0x01, 0x01, 0x01, 0x01, 0x01};
  swStaticBuffer b1 = swStaticBufferDefine(d1);
  swStaticBuffer b2 = swStaticBufferDefine(d2);
  swStaticBuffer b3 = swStaticBufferDefine(d3);
  swStaticBuffer b4 = swStaticBufferDefine(d4);

  ASSERT_EQUAL(swStaticBufferEqual(&b1, &b2), false);
  ASSERT_EQUAL(swStaticBufferEqual(&b2, &b1), false);
  ASSERT_EQUAL(swStaticBufferEqual(&b1, &b1), true);
  ASSERT_EQUAL(swStaticBufferEqual(&b1, &b3), true);
  ASSERT_EQUAL(swStaticBufferEqual(&b1, &b4), false);
  ASSERT_EQUAL(swStaticBufferEqual(&b4, &b1), false);
  ASSERT_EQUAL(swStaticBufferEqual(&b2, &b4), false);
  ASSERT_EQUAL(swStaticBufferEqual(&b4, &b2), false);
  ASSERT_EQUAL(swStaticBufferEqual(NULL, NULL), true);
  ASSERT_EQUAL(swStaticBufferEqual(&b1, NULL), false);
  ASSERT_EQUAL(swStaticBufferEqual(NULL, &b1), false);

  return true;
}

swTestDeclare(TestHash, NULL, NULL, swTestRun)
{
  char d1[] = {0x01, 0x01, 0x01, 0x01};
  swStaticBuffer b1 = swStaticBufferDefine(d1);

  ASSERT_NOT_EQUAL(swStaticBufferHash(&b1), 0);
  ASSERT_EQUAL(swStaticBufferHash(NULL), 0);

  return true;
}

swTestSuiteStructDeclare(StaticBufferUnittest, NULL, NULL, swTestRun,
  &TestCompare, &TestEqual, &TestHash);

