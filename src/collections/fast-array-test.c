#include "unittest/unittest.h"
#include "collections/fast-array.h"

swTestDeclare(BasicTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swTestLogLine("Creating static array ...\n");
  swFastArray array = {0};

  if (swFastArrayInit(&array, sizeof(uint32_t), 0))
  {
    ASSERT_TRUE(array.size > 0);
    swTestLogLine("Populating static array ...\n");
    for (uint32_t i = 0; i < 1000; i++)
    {
      // swTestLogLine("Populating item %u ...\n", i);
      ASSERT_TRUE(swFastArraySet(array, i, i));
    }
    uint32_t val;
    for (uint32_t i = 0; i < 1000; i++)
    {
      ASSERT_TRUE(swFastArrayGet(array, i, val));
      ASSERT_EQUAL(i, val);
    }
    ASSERT_FALSE(swFastArrayGet(array, array.size, val));
    swTestLogLine("Cleaning static array up...\n");
    swFastArrayClear(&array);
    rtn = true;
  }
  return rtn;
}

swTestDeclare(CountTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swTestLogLine("Creating static array ...\n");
  swFastArray array = {0};

  if (swFastArrayInit(&array, sizeof(uint32_t), 0))
  {
    ASSERT_TRUE(array.size > 0);
    swTestLogLine("Populating static array ...\n");
    for (uint32_t i = 0; i < 1000; i++)
    {
      // swTestLogLine("Populating item %u ...\n", i);
      ASSERT_TRUE(swFastArrayPush(array, i));
      ASSERT_EQUAL(swFastArrayCount(array), (i+1));
    }
    uint32_t val;
    for (uint32_t i = 1000; i > 0; i--)
    {
      ASSERT_TRUE(swFastArrayPop(array, val));
      ASSERT_EQUAL(swFastArrayCount(array), (i-1));
    }
    swTestLogLine("Cleaning static array up...\n");
    swFastArrayClear(&array);
    rtn = true;
  }
  return rtn;
}

swTestSuiteStructDeclare(FastArrayTest, NULL, NULL, swTestRun, &BasicTest, &CountTest);