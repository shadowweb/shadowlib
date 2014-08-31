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

swTestDeclare(TestSame, NULL, NULL, swTestRun)
{
  swStaticString s1 = swStaticStringDefine("aaaa");
  swStaticString s2 = swStaticStringDefine("bbbb");
  swStaticString s3 = swStaticStringDefine("aaaa");
  swStaticString s4 = swStaticStringDefine("aaaaa");

  ASSERT_EQUAL(swStaticStringSame(&s1, &s2), false);
  ASSERT_EQUAL(swStaticStringSame(&s2, &s1), false);
  ASSERT_EQUAL(swStaticStringSame(&s1, &s1), true);
  // beware here, the data pointer will be the same
  ASSERT_EQUAL(swStaticStringSame(&s1, &s3), true);
  ASSERT_EQUAL(swStaticStringSame(&s1, &s4), false);
  ASSERT_EQUAL(swStaticStringSame(&s4, &s1), false);
  ASSERT_EQUAL(swStaticStringSame(&s2, &s4), false);
  ASSERT_EQUAL(swStaticStringSame(&s4, &s2), false);

  ASSERT_EQUAL(swStaticStringSame(NULL, NULL), true);
  ASSERT_EQUAL(swStaticStringSame(&s1, NULL), false);
  ASSERT_EQUAL(swStaticStringSame(NULL, &s1), false);

  char dataS1[] = "aaaa";
  char dataS2[] = "bbbb";
  char dataS3[] = "aaaa";
  char dataS4[] = "aaaaa";
  s1 = swStaticStringSetFromCstr(dataS1);
  s2 = swStaticStringSetFromCstr(dataS2);
  s3 = swStaticStringSetFromCstr(dataS3);
  s4 = swStaticStringSetFromCstr(dataS4);

  ASSERT_EQUAL(swStaticStringSame(&s1, &s2), false);
  ASSERT_EQUAL(swStaticStringSame(&s2, &s1), false);
  ASSERT_EQUAL(swStaticStringSame(&s1, &s1), true);
  ASSERT_EQUAL(swStaticStringSame(&s1, &s3), false);
  ASSERT_EQUAL(swStaticStringSame(&s1, &s4), false);
  ASSERT_EQUAL(swStaticStringSame(&s4, &s1), false);
  ASSERT_EQUAL(swStaticStringSame(&s2, &s4), false);
  ASSERT_EQUAL(swStaticStringSame(&s4, &s2), false);

  return true;
}

swTestDeclare(TestFindChar, NULL, NULL, swTestRun)
{
  swStaticString s1 = swStaticStringDefine("very long string = very long string");
  size_t position = 0;
  ASSERT_TRUE(swStaticStringFindChar(&s1, '=', &position));
  ASSERT_EQUAL(s1.data[position], '=');

  ASSERT_FALSE(swStaticStringFindChar(&s1, '.', &position));

  return true;
}

swTestDeclare(TestSetSubstring, NULL, NULL, swTestRun)
{
  swStaticString s1 = swStaticStringDefine("very long string = very long string");
  size_t position = 0;
  ASSERT_TRUE(swStaticStringFindChar(&s1, '=', &position));
  ASSERT_EQUAL(s1.data[position], '=');
  swStaticString s2 = swStaticStringDefineEmpty;
  ASSERT_TRUE(swStaticStringSetSubstring(&s1, &s2, 10, 15));
  ASSERT_EQUAL(s2.len, 5);
  ASSERT_TRUE(s2.data == &s1.data[10]);

  ASSERT_FALSE(swStaticStringSetSubstring(&s1, &s2, 10, s1.len + 1));
  ASSERT_FALSE(swStaticStringSetSubstring(&s1, &s2, 10, 10));

  return true;
}

swTestDeclare(TestSlitChar, NULL, NULL, swTestRun)
{
  swStaticString slices[5] = {swStaticStringDefineEmpty};
  uint32_t slicesCount = 0;

  swStaticString s1 = swStaticStringDefine("very long string = very long string");
  ASSERT_TRUE(swStaticStringSplitChar(&s1, '=', slices, 5, &slicesCount, 0));
  ASSERT_EQUAL(slicesCount, 2);
  ASSERT_EQUAL(slices[0].len, 17);
  ASSERT_EQUAL(slices[1].len, 17);
  ASSERT_TRUE(swStaticStringSplitChar(&s1, '=', slices, 5, &slicesCount, (swStaticStringSearchAllowFirst | swStaticStringSearchAllowLast | swStaticStringSearchAllowAdjacent)));
  ASSERT_EQUAL(slicesCount, 2);
  ASSERT_EQUAL(slices[0].len, 17);
  ASSERT_EQUAL(slices[1].len, 17);

  swStaticString s2 = swStaticStringDefine("=very long string = very long string");
  ASSERT_FALSE(swStaticStringSplitChar(&s2, '=', slices, 5, &slicesCount, 0));
  ASSERT_TRUE(swStaticStringSplitChar(&s2, '=', slices, 5, &slicesCount, (swStaticStringSearchAllowFirst | swStaticStringSearchAllowLast | swStaticStringSearchAllowAdjacent)));
  ASSERT_EQUAL(slicesCount, 3);
  ASSERT_EQUAL(slices[0].len, 0);
  ASSERT_EQUAL(slices[1].len, 17);
  ASSERT_EQUAL(slices[2].len, 17);

  swStaticString s3 = swStaticStringDefine("very long string = very long string=");
  ASSERT_FALSE(swStaticStringSplitChar(&s3, '=', slices, 5, &slicesCount, 0));
  ASSERT_TRUE(swStaticStringSplitChar(&s3, '=', slices, 5, &slicesCount, (swStaticStringSearchAllowFirst | swStaticStringSearchAllowLast | swStaticStringSearchAllowAdjacent)));
  ASSERT_EQUAL(slicesCount, 2);
  ASSERT_EQUAL(slices[0].len, 17);
  ASSERT_EQUAL(slices[1].len, 17);

  swStaticString s4 = swStaticStringDefine("very long string == very long string");
  ASSERT_FALSE(swStaticStringSplitChar(&s4, '=', slices, 5, &slicesCount, 0));
  ASSERT_TRUE(swStaticStringSplitChar(&s4, '=', slices, 5, &slicesCount, (swStaticStringSearchAllowFirst | swStaticStringSearchAllowLast | swStaticStringSearchAllowAdjacent)));
  ASSERT_EQUAL(slicesCount, 3);
  ASSERT_EQUAL(slices[0].len, 17);
  ASSERT_EQUAL(slices[1].len, 0);
  ASSERT_EQUAL(slices[2].len, 17);

  swStaticString s5 = swStaticStringDefine("very long string -- very long string");
  ASSERT_TRUE(swStaticStringSplitChar(&s5, '=', slices, 5, &slicesCount, 0));
  ASSERT_EQUAL(slicesCount, 1);
  ASSERT_TRUE(swStaticStringSame(&s5, &slices[0]));
  return true;
}

swTestDeclare(TestCountChar, NULL, NULL, swTestRun)
{
  uint32_t charCount = 0;

  swStaticString s1 = swStaticStringDefine("very long string = very long string");
  ASSERT_TRUE(swStaticStringCountChar(&s1, '=', &charCount));
  ASSERT_EQUAL(charCount, 1);

  swStaticString s2 = swStaticStringDefine("=very long string = very long string");
  ASSERT_TRUE(swStaticStringCountChar(&s2, '=', &charCount));
  ASSERT_EQUAL(charCount, 2);

  swStaticString s3 = swStaticStringDefine("very long string = very long string=");
  ASSERT_TRUE(swStaticStringCountChar(&s3, '=', &charCount));
  ASSERT_EQUAL(charCount, 2);

  swStaticString s4 = swStaticStringDefine("very long string == very long string");
  ASSERT_TRUE(swStaticStringCountChar(&s4, '=', &charCount));
  ASSERT_EQUAL(charCount, 2);

  swStaticString s5 = swStaticStringDefine("very long string -- very long string");
  ASSERT_TRUE(swStaticStringCountChar(&s5, '=', &charCount));
  ASSERT_EQUAL(charCount, 0);

  return true;
}

swTestSuiteStructDeclare(StaticStrungUnittest, NULL, NULL, swTestRun,
  &TestCompare, &TestEqual, &TestHash, &TestSame, &TestFindChar, &TestSetSubstring, &TestCountChar);

