#include "array.h"

#include "unittest/unittest.h"

swTestDeclare(BasicTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swTestLogLine("Creating static array ...\n");
  swStaticArray array = {0};

  if (swStaticArrayInit(&array, sizeof(uint32_t), 0))
  {
    ASSERT_TRUE(array.size > 0);
    swTestLogLine("Populating static array ...\n");
    for (uint32_t i = 0; i < 1000; i++)
    {
      // swTestLogLine("Populating item %u ...\n", i);
      ASSERT_TRUE(swStaticArraySet(array, i, i));
    }
    uint32_t val;
    for (uint32_t i = 0; i < 1000; i++)
    {
      ASSERT_TRUE(swStaticArrayGet(array, i, val));
      ASSERT_EQUAL(i, val);
    }
    ASSERT_FALSE(swStaticArrayGet(array, array.size, val));
    swTestLogLine("Cleaning static array up...\n");
    swStaticArrayClear(&array);
    rtn = true;
  }
  return rtn;
}

swTestDeclare(CountTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swTestLogLine("Creating static array ...\n");
  swStaticArray array = {0};

  if (swStaticArrayInit(&array, sizeof(uint32_t), 0))
  {
    ASSERT_TRUE(array.size > 0);
    swTestLogLine("Populating static array ...\n");
    for (uint32_t i = 0; i < 1000; i++)
    {
      // swTestLogLine("Populating item %u ...\n", i);
      ASSERT_TRUE(swStaticArrayPush(array, i));
      ASSERT_EQUAL(swStaticArrayCount(array), (i+1));
    }
    uint32_t val;
    for (uint32_t i = 1000; i > 0; i--)
    {
      ASSERT_TRUE(swStaticArrayPop(array, val));
      ASSERT_EQUAL(swStaticArrayCount(array), (i-1));
    }
    swTestLogLine("Cleaning static array up...\n");
    swStaticArrayClear(&array);
    rtn = true;
  }
  return rtn;
}

swTestSuiteStructDeclare(StaticArrayTest, NULL, NULL, swTestRun, &BasicTest, &CountTest);