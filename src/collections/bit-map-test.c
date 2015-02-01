#include "collections/bit-map.h"

#include "unittest/unittest.h"

bool testBitMap(uint16_t bitSize)
{
  bool rtn = false;
  swTestLogLine("Testing bit size %u\n", bitSize);
  swBitMap *map = swBitMapNew(bitSize);
  if (map)
  {
    ASSERT_EQUAL(swBitMapSize(map), bitSize);
    ASSERT_EQUAL(swBitMapCount(map), 0);

    for (uint16_t j = 0; j < bitSize; j++)
    {
      swBitMapSet(map, j);
      ASSERT_TRUE(swBitMapIsSet(map, j));
      ASSERT_EQUAL(swBitMapCount(map), 1);
      swBitMapClear(map, j);
      ASSERT_FALSE(swBitMapIsSet(map, j));
      ASSERT_EQUAL(swBitMapCount(map), 0);
    }

    for (uint16_t j = 0; j < bitSize; j++)
      swBitMapSet(map, j);
    ASSERT_EQUAL(swBitMapCount(map), bitSize);
    for (uint16_t j = 0; j < bitSize; j++)
      ASSERT_TRUE(swBitMapIsSet(map, j));

    for (uint16_t j = 0; j < bitSize; j++)
      swBitMapClear(map, j);
    ASSERT_EQUAL(swBitMapCount(map), 0);
    for (uint16_t j = 0; j < bitSize; j++)
      ASSERT_FALSE(swBitMapIsSet(map, j));

    swBitMapSetAll(map);
    ASSERT_EQUAL(swBitMapCount(map), swBitMapSize(map));

    uint16_t byteSize = ((bitSize - 1) >> 3) + 1;
    for (uint16_t j = 0; j < byteSize; j++)
      ASSERT_EQUAL(map->bytes[j], UINT8_MAX);

    swBitMapClearAll(map);
    ASSERT_EQUAL(swBitMapCount(map), 0);
    for (uint16_t j = 0; j < byteSize; j++)
      ASSERT_EQUAL(map->bytes[j], 0);

    swBitMapDelete(map);
    rtn = true;
  }
  return rtn;
}

swTestDeclare(SmallTest, NULL, NULL, swTestRun)
{
  bool rtn = true;
  for (uint16_t i = 1; i <= sizeof(uint32_t) * 8; i++)
  {
    rtn = testBitMap(i);
    if (!rtn)
      break;
  }
  return rtn;
}

swTestDeclare(MediumTest, NULL, NULL, swTestRun)
{
  bool rtn = true;
  uint16_t startBitSize = sizeof(uint32_t) * 8 + 1;
  uint16_t endBitSize = startBitSize + sizeof(uint64_t) * 8;
  for (uint16_t i = startBitSize; i < endBitSize; i++)
  {
    rtn = testBitMap(i);
    if (!rtn)
      break;
  }
  return rtn;
}

swTestDeclare(LargeTest, NULL, NULL, swTestRun)
{
  bool rtn = true;
  uint16_t startBitSize = (sizeof(uint32_t) + sizeof(uint64_t)) * 8 + 1;
  uint16_t endBitSize = startBitSize + sizeof(uint64_t) * 8 * 10;
  for (uint16_t i = startBitSize; i < endBitSize; i++)
  {
    rtn = testBitMap(i);
    if (!rtn)
      break;
  }
  return rtn;
}

swTestSuiteStructDeclare(BitMapBasicSuite, NULL, NULL, swTestRun,
                         &SmallTest, &MediumTest, &LargeTest);


bool testFindFirstSet(uint16_t bitSize)
{
  bool rtn = false;
  swTestLogLine("Testing find first set for size %u\n", bitSize);
  swBitMap *map = swBitMapNew(bitSize);
  if (map)
  {
    uint16_t position = 0;
    for (uint16_t j = 0; j < bitSize; j++)
    {
      swBitMapSet(map, j);
      ASSERT_TRUE(swBitMapFindFirstSet(map, &position));
      ASSERT_EQUAL(position, j);
      swBitMapClear(map, j);
      ASSERT_FALSE(swBitMapFindFirstSet(map, &position));
    }

    swBitMapSetAll(map);
    for (uint16_t j = 0; j < bitSize; j++)
    {
      ASSERT_TRUE(swBitMapFindFirstSet(map, &position));
      ASSERT_EQUAL(position, j);
      swBitMapClear(map, j);
    }
    ASSERT_FALSE(swBitMapFindFirstSet(map, &position));

    swBitMapDelete(map);
    rtn = true;
  }
  return rtn;
}

swTestDeclare(SmallBitFindSetTest, NULL, NULL, swTestRun)
{
  bool rtn = true;
  for (uint16_t i = 1; i <= sizeof(uint32_t) * 8; i++)
  {
    rtn = testFindFirstSet(i);
    if (!rtn)
      break;
  }
  return rtn;
}

swTestDeclare(MediumBitFindSetTest, NULL, NULL, swTestRun)
{
  bool rtn = true;
  uint16_t startBitSize = sizeof(uint32_t) * 8 + 1;
  uint16_t endBitSize = startBitSize + sizeof(uint64_t) * 8;
  for (uint16_t i = startBitSize; i < endBitSize; i++)
  {
    rtn = testFindFirstSet(i);
    if (!rtn)
      break;
  }
  return rtn;
}

swTestDeclare(LargeBitFindSetTest, NULL, NULL, swTestRun)
{
  bool rtn = true;
  uint16_t startBitSize = (sizeof(uint32_t) + sizeof(uint64_t)) * 8 + 1;
  uint16_t endBitSize = startBitSize + sizeof(uint64_t) * 8 * 10;
  for (uint16_t i = startBitSize; i < endBitSize; i++)
  {
    rtn = testFindFirstSet(i);
    if (!rtn)
      break;
  }
  return rtn;
}

bool testFindFirstClear(uint16_t bitSize)
{
  bool rtn = false;
  swTestLogLine("Testing find first clear for size %u\n", bitSize);
  swBitMap *map = swBitMapNew(bitSize);
  if (map)
  {
    swBitMapSetAll(map);

    uint16_t position = 0;
    for (uint16_t j = 0; j < bitSize; j++)
    {
      swBitMapClear(map, j);
      ASSERT_TRUE(swBitMapFindFirstClear(map, &position));
      ASSERT_EQUAL(position, j);
      swBitMapSet(map, j);
      ASSERT_FALSE(swBitMapFindFirstClear(map, &position));
    }

    swBitMapClearAll(map);
    for (uint16_t j = 0; j < bitSize; j++)
    {
      ASSERT_TRUE(swBitMapFindFirstClear(map, &position));
      ASSERT_EQUAL(position, j);
      swBitMapSet(map, j);
    }
    ASSERT_FALSE(swBitMapFindFirstClear(map, &position));

    swBitMapDelete(map);
    rtn = true;
  }
  return rtn;
}

swTestDeclare(SmallBitFindClearTest, NULL, NULL, swTestRun)
{
  bool rtn = true;
  for (uint16_t i = 1; i <= sizeof(uint32_t) * 8; i++)
  {
    rtn = testFindFirstClear(i);
    if (!rtn)
      break;
  }
  return rtn;
}

swTestDeclare(MediumBitFindClearTest, NULL, NULL, swTestRun)
{
  bool rtn = true;
  uint16_t startBitSize = sizeof(uint32_t) * 8 + 1;
  uint16_t endBitSize = startBitSize + sizeof(uint64_t) * 8;
  for (uint16_t i = startBitSize; i < endBitSize; i++)
  {
    rtn = testFindFirstClear(i);
    if (!rtn)
      break;
  }
  return rtn;
}

swTestDeclare(LargeBitFindClearTest, NULL, NULL, swTestRun)
{
  bool rtn = true;
  uint16_t startBitSize = (sizeof(uint32_t) + sizeof(uint64_t)) * 8 + 1;
  uint16_t endBitSize = startBitSize + sizeof(uint64_t) * 8 * 10;
  for (uint16_t i = startBitSize; i < endBitSize; i++)
  {
    rtn = testFindFirstClear(i);
    if (!rtn)
      break;
  }
  return rtn;
}

swTestSuiteStructDeclare(BitMapFindFirstSuite, NULL, NULL, swTestRun,
                         &SmallBitFindSetTest, &MediumBitFindSetTest, &LargeBitFindSetTest,
                         &SmallBitFindClearTest, &MediumBitFindClearTest, &LargeBitFindClearTest );
