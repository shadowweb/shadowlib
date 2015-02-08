#include "collections/lpm.h"
#include "core/memory.h"
#include "utils/random.h"

#include "unittest/unittest.h"

#include <stddef.h>

// static uint64_t ipCount = 4194304;   // 4 * 1024 * 1024
// static uint64_t ipCount = 1048576;      // 1 * 1024 * 10924
// static uint64_t ipCount = 2097152;      // 2 * 1024 * 1024
static uint64_t ipCount = 524288;       // 512 * 1024
// static uint64_t ipCount = 512;

typedef struct swLPMTestData
{
  swDynamicBuffer prefixes;
  size_t prefixSize;
  uint64_t duplicatesFound;
  uint16_t maxLen;
} swLPMTestData;

void swLPMTestDataDelete(swLPMTestData *lpmTestData)
{
  if (lpmTestData)
  {
    swDynamicBufferRelease(&(lpmTestData->prefixes));
    swMemoryFree(lpmTestData);
  }
}

// prefixBytes should be power of 2
swLPMTestData *swLPMTestDataNew(size_t prefixBytes)
{
  swLPMTestData *rtn = NULL;
  if (prefixBytes)
  {
    // printf ("sizeof(swLPMPrefix) = %zu, offsetof(swLPMPrefix, prefixBytes) = %zu\n", sizeof(swLPMPrefix), offsetof(swLPMPrefix, prefixBytes));
    FILE *file = fopen("/dev/urandom", "r");
    swLPM *lpm = swLPMNew(1);
    if (lpm)
    {
      swLPMTestData *lpmTestData = swMemoryCalloc(1, sizeof(*lpmTestData));
      if (lpmTestData)
      {
        lpmTestData->prefixSize = sizeof(swLPMPrefix) + prefixBytes;
        lpmTestData->maxLen = prefixBytes * 8;
        if (swDynamicBufferInit(&(lpmTestData->prefixes), ipCount * lpmTestData->prefixSize))
        {
          if (fread(lpmTestData->prefixes.data, lpmTestData->prefixes.size, 1, file))
          {
            uint8_t *dataBuffer = (uint8_t *)(lpmTestData->prefixes.data);
            uint8_t *dataBufferEnd = dataBuffer + (ipCount*lpmTestData->prefixSize);
            uint16_t mask = lpmTestData->maxLen -1;
            for (; dataBuffer < dataBufferEnd; dataBuffer += lpmTestData->prefixSize)
            {
              swLPMPrefix *prefix = (swLPMPrefix *)dataBuffer;
              bool inserted = false;
              while (!inserted)
              {
                prefix->len = (prefix->len + 8) & mask;
                while (!prefix->len)
                {
                  if (fread((char *)(&(prefix->len)), sizeof(prefix->len), 1, file))
                    prefix->len = (prefix->len + 8) & mask;
                  else
                    break;
                }
                if (prefix->len)
                {
                  uint16_t usedBytes = swLPMPrefixBytes(prefix);
                  size_t emptyBytes = prefixBytes - usedBytes;
                  if (emptyBytes)
                    memset(&prefix->prefixBytes[usedBytes], 0, emptyBytes);
                  uint8_t lastByteBitCount = (uint8_t)(prefix->len & 7);
                  if (lastByteBitCount)
                  {
                    uint8_t mask = ~((1 << (8 - lastByteBitCount)) - 1);
                    prefix->prefixBytes[usedBytes - 1] &= mask;
                  }
                  swLPMPrefix *storedPrefix = NULL;
                  if (!swLPMFind(lpm, prefix, &storedPrefix))
                  {
                    // swLPMPrefixPrint(prefix);
                    if (!(inserted = swLPMInsert(lpm, prefix, &storedPrefix)))
                    {
                      ASSERT_TRUE(false);
                      break;
                    }
                  }
                  else
                  {
                    if (!fread((char *)prefix, lpmTestData->prefixSize, 1, file))
                      break;
                  }
                }
                else
                  break;
              }
              if (!inserted)
                break;
            }
            if (dataBuffer == dataBufferEnd)
              rtn = lpmTestData;
          }
        }
        if (!rtn)
          swLPMTestDataDelete(lpmTestData);
      }
      swLPMDelete(lpm);
    }
    fclose(file);
  }
  return rtn;
}

void setupIPv4Addresses(swTestSuite *suite)
{
  swLPMTestData *testData = swLPMTestDataNew(4);
  ASSERT_NOT_NULL(testData);
  swTestSuiteDataSet(suite, testData);
}

void setupIPv6Addresses(swTestSuite *suite)
{
  swLPMTestData *testData = swLPMTestDataNew(16);
  ASSERT_NOT_NULL(testData);
  swTestSuiteDataSet(suite, testData);
}

void teardownAddresses(swTestSuite *suite)
{
  swLPMTestData *testData = swTestSuiteDataGet(suite);
  swTestSuiteDataSet(suite, NULL);
  swLPMTestDataDelete(testData);
}

void setupInsertTestWithFactor(swTest *test, uint8_t factor)
{
  swLPM *lpm = swLPMNew(factor);
  ASSERT_NOT_NULL(lpm);
  swTestDataSet(test, lpm);
}

void setupInsertTestFactorOne(swTestSuite *suite, swTest *test)
{
  setupInsertTestWithFactor(test, 1);
}

void setupInsertTestFactorTwo(swTestSuite *suite, swTest *test)
{
  setupInsertTestWithFactor(test, 2);
}

void setupInsertTestFactorThree(swTestSuite *suite, swTest *test)
{
  setupInsertTestWithFactor(test, 3);
}

void setupInsertTestFactorFour(swTestSuite *suite, swTest *test)
{
  setupInsertTestWithFactor(test, 4);
}

void setupInsertTestFactorFive(swTestSuite *suite, swTest *test)
{
  setupInsertTestWithFactor(test, 5);
}

void setupInsertTestFactorSix(swTestSuite *suite, swTest *test)
{
  setupInsertTestWithFactor(test, 6);
}

void setupInsertTestFactorSeven(swTestSuite *suite, swTest *test)
{
  setupInsertTestWithFactor(test, 7);
}

void setupInsertTestFactorEight(swTestSuite *suite, swTest *test)
{
  setupInsertTestWithFactor(test, 8);
}

void teardownTest(swTestSuite *suite, swTest *test)
{
  swLPM *lpm = swTestDataGet(test);
  swTestDataSet(test, NULL);
  ASSERT_TRUE(swLPMValidate(lpm, false));
  swLPMDelete(lpm);
  swLPMTestData *testData = swTestSuiteDataGet(suite);
  swTestLogLine("Found %lu duplicates\n", testData->duplicatesFound);
  testData->duplicatesFound = 0;
}

static inline bool insertTestWithFactor(swTestSuite *suite, swTest *test)
{
  bool rtn = false;
  swLPMTestData *testData = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(testData);
  swLPM *lpm = swTestDataGet(test);
  ASSERT_NOT_NULL(lpm);
  uint8_t *buffer = (uint8_t *)(testData->prefixes.data);
  uint8_t *bufferEnd = buffer + (ipCount * testData->prefixSize);
  swLPMPrefix *prefix = NULL;
  swLPMPrefix *storedPrefix = NULL;
  while(buffer < bufferEnd)
  {
    prefix = (swLPMPrefix *)buffer;
    storedPrefix = NULL;
    if (!swLPMInsert(lpm, prefix, &storedPrefix))
    {
      if (storedPrefix)
      {
        testData->duplicatesFound++;
        // ASSERT_TRUE(swLPMPrefixEqual(prefix, storedPrefix));
      }
      else
        break;
    }
    buffer += testData->prefixSize;
  }
  ASSERT_EQUAL(lpm->count, ipCount);
  if ((buffer == bufferEnd) && (lpm->count == ipCount))
    rtn = true;
  return rtn;
}

swTestDeclare(InsertFactorOneTest, setupInsertTestFactorOne, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorTwoTest, setupInsertTestFactorTwo, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorThreeTest, setupInsertTestFactorThree, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorFourTest, setupInsertTestFactorFour, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorFiveTest, setupInsertTestFactorFive, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorSixTest, setupInsertTestFactorSix, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorSevenTest, setupInsertTestFactorSeven, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorEightTest, setupInsertTestFactorEight, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

void setupTestWithInsertWithFactor(swTestSuite *suite, swTest *test, uint8_t factor)
{
  swLPM *lpm = swLPMNew(factor);
  ASSERT_NOT_NULL(lpm);
  swTestDataSet(test, lpm);
  ASSERT_TRUE(insertTestWithFactor(suite, test));
  ASSERT_TRUE(swLPMValidate(lpm, false));
}

void setupTestWithInsertFactorOne(swTestSuite *suite, swTest *test)
{
  setupTestWithInsertWithFactor(suite, test, 1);
}

void setupTestWitInsertFactorTwo(swTestSuite *suite, swTest *test)
{
  setupTestWithInsertWithFactor(suite, test, 2);
}

void setupTestWithInsertFactorThree(swTestSuite *suite, swTest *test)
{
  setupTestWithInsertWithFactor(suite, test, 3);
}

void setupTestWithInsertFactorFour(swTestSuite *suite, swTest *test)
{
  setupTestWithInsertWithFactor(suite, test, 4);
}

void setupTestWithInsertFactorFive(swTestSuite *suite, swTest *test)
{
  setupTestWithInsertWithFactor(suite, test, 5);
}

void setupTestWithInsertFactorSix(swTestSuite *suite, swTest *test)
{
  setupTestWithInsertWithFactor(suite, test, 6);
}

void setupTestWithInsertFactorSeven(swTestSuite *suite, swTest *test)
{
  setupTestWithInsertWithFactor(suite, test, 7);
}

void setupTestWithInsertFactorEight(swTestSuite *suite, swTest *test)
{
  setupTestWithInsertWithFactor(suite, test, 8);
}

static inline bool findTestWithFactor(swTestSuite *suite, swTest *test)
{
  bool rtn = false;
  swLPMTestData *testData = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(testData);
  swLPM *lpm = swTestDataGet(test);
  ASSERT_NOT_NULL(lpm);
  uint8_t *buffer = (uint8_t *)(testData->prefixes.data);
  uint8_t *bufferEnd = buffer + (ipCount * testData->prefixSize);
  swLPMPrefix *prefix = NULL;
  swLPMPrefix *storedPrefix = NULL;
  bool found = false;
  while(buffer < bufferEnd)
  {
    found = false;
    storedPrefix = NULL;
    prefix = (swLPMPrefix *)buffer;
    if (swLPMFind(lpm, prefix, &storedPrefix))
    {
      if (storedPrefix)
        ASSERT_TRUE((found = (prefix == storedPrefix)));
    }
    if (found)
      buffer += testData->prefixSize;
    else
      break;
  }
  if (buffer == bufferEnd)
    rtn = true;
  return rtn;
}

swTestDeclare(FindFactorOneTest, setupTestWithInsertFactorOne, teardownTest, swTestRun)
{
  return findTestWithFactor(suite, test);
}

swTestDeclare(FindFactorTwoTest, setupTestWitInsertFactorTwo, teardownTest, swTestRun)
{
  return findTestWithFactor(suite, test);
}

swTestDeclare(FindFactorThreeTest, setupTestWithInsertFactorThree, teardownTest, swTestRun)
{
  return findTestWithFactor(suite, test);
}

swTestDeclare(FindFactorFourTest, setupTestWithInsertFactorFour, teardownTest, swTestRun)
{
  return findTestWithFactor(suite, test);
}

swTestDeclare(FindFactorFiveTest, setupTestWithInsertFactorFive, teardownTest, swTestRun)
{
  return findTestWithFactor(suite, test);
}

swTestDeclare(FindFactorSixTest, setupTestWithInsertFactorSix, teardownTest, swTestRun)
{
  return findTestWithFactor(suite, test);
}

swTestDeclare(FindFactorSevenTest, setupTestWithInsertFactorSeven, teardownTest, swTestRun)
{
  return findTestWithFactor(suite, test);
}

swTestDeclare(FindFactorEightTest, setupTestWithInsertFactorEight, teardownTest, swTestRun)
{
  return findTestWithFactor(suite, test);
}

static inline bool matchTestWithFactor(swTestSuite *suite, swTest *test)
{
  bool rtn = false;
  swLPMTestData *testData = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(testData);
  swLPM *lpm = swTestDataGet(test);
  ASSERT_NOT_NULL(lpm);
  uint8_t *buffer = (uint8_t *)(testData->prefixes.data);
  uint8_t *bufferEnd = buffer + (ipCount * testData->prefixSize);
  swLPMPrefix *storedPrefix = NULL;
  swStaticBuffer ipValue = swStaticBufferDefineEmpty;
  bool found = false;
  size_t dataSize = testData->prefixSize - sizeof(swLPMPrefix);
  while(buffer < bufferEnd)
  {
    found = false;
    storedPrefix = NULL;
    ipValue = swStaticBufferSetWithLength((char *)(buffer + sizeof(swLPMPrefix)), dataSize);
    if (swLPMMatch(lpm, &ipValue, &storedPrefix))
    {
      if (storedPrefix)
        found = true;
    }
    if (found)
      buffer += testData->prefixSize;
    else
      break;
  }
  if (buffer == bufferEnd)
    rtn = true;
  return rtn;
}

swTestDeclare(MatchFactorOneTest, setupTestWithInsertFactorOne, teardownTest, swTestRun)
{
  return matchTestWithFactor(suite, test);
}

swTestDeclare(MatchFactorTwoTest, setupTestWitInsertFactorTwo, teardownTest, swTestRun)
{
  return matchTestWithFactor(suite, test);
}

swTestDeclare(MatchFactorThreeTest, setupTestWithInsertFactorThree, teardownTest, swTestRun)
{
  return matchTestWithFactor(suite, test);
}

swTestDeclare(MatchFactorFourTest, setupTestWithInsertFactorFour, teardownTest, swTestRun)
{
  return matchTestWithFactor(suite, test);
}

swTestDeclare(MatchFactorFiveTest, setupTestWithInsertFactorFive, teardownTest, swTestRun)
{
  return matchTestWithFactor(suite, test);
}

swTestDeclare(MatchFactorSixTest, setupTestWithInsertFactorSix, teardownTest, swTestRun)
{
  return matchTestWithFactor(suite, test);
}

swTestDeclare(MatchFactorSevenTest, setupTestWithInsertFactorSeven, teardownTest, swTestRun)
{
  return matchTestWithFactor(suite, test);
}

swTestDeclare(MatchFactorEightTest, setupTestWithInsertFactorEight, teardownTest, swTestRun)
{
  return matchTestWithFactor(suite, test);
}

static inline bool removeTestWithFactor(swTestSuite *suite, swTest *test)
{
  bool rtn = false;
  swLPMTestData *testData = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(testData);
  swLPM *lpm = swTestDataGet(test);
  ASSERT_NOT_NULL(lpm);
  uint8_t *buffer = (uint8_t *)(testData->prefixes.data);
  uint8_t *bufferEnd = buffer + (ipCount * testData->prefixSize);
  swLPMPrefix *prefix = NULL;
  swLPMPrefix *storedPrefix = NULL;
  bool found = false;
  uint64_t prevCount = lpm->count;
  while(buffer < bufferEnd)
  {
    found = false;
    storedPrefix = NULL;
    prefix = (swLPMPrefix *)buffer;
    if (swLPMRemove(lpm, prefix, &storedPrefix))
    {
      ASSERT_EQUAL(--prevCount, lpm->count);
      if (storedPrefix)
        ASSERT_TRUE((found = (prefix == storedPrefix)));
    }
    if (found)
      buffer += testData->prefixSize;
    else
      break;
  }
  if (buffer == bufferEnd)
    rtn = true;
  return rtn;
}

swTestDeclare(RemoveFactorOneTest, setupTestWithInsertFactorOne, teardownTest, swTestRun)
{
  return removeTestWithFactor(suite, test);
}

swTestDeclare(RemoveFactorTwoTest, setupTestWitInsertFactorTwo, teardownTest, swTestRun)
{
  return removeTestWithFactor(suite, test);
}

swTestDeclare(RemoveFactorThreeTest, setupTestWithInsertFactorThree, teardownTest, swTestRun)
{
  return removeTestWithFactor(suite, test);
}

swTestDeclare(RemoveFactorFourTest, setupTestWithInsertFactorFour, teardownTest, swTestRun)
{
  return removeTestWithFactor(suite, test);
}

swTestDeclare(RemoveFactorFiveTest, setupTestWithInsertFactorFive, teardownTest, swTestRun)
{
  return removeTestWithFactor(suite, test);
}

swTestDeclare(RemoveFactorSixTest, setupTestWithInsertFactorSix, teardownTest, swTestRun)
{
  return removeTestWithFactor(suite, test);
}

swTestDeclare(RemoveFactorSevenTest, setupTestWithInsertFactorSeven, teardownTest, swTestRun)
{
  return removeTestWithFactor(suite, test);
}

swTestDeclare(RemoveFactorEightTest, setupTestWithInsertFactorEight, teardownTest, swTestRun)
{
  return removeTestWithFactor(suite, test);
}

swTestSuiteStructDeclare(LPMIPv4TestSuite, setupIPv4Addresses, teardownAddresses, swTestRun,
  &InsertFactorOneTest,  &InsertFactorTwoTest, &InsertFactorThreeTest, &InsertFactorFourTest,
  &InsertFactorFiveTest, &InsertFactorSixTest, &InsertFactorSevenTest, &InsertFactorEightTest,
  &FindFactorOneTest,  &FindFactorTwoTest, &FindFactorThreeTest, &FindFactorFourTest,
  &FindFactorFiveTest, &FindFactorSixTest, &FindFactorSevenTest, &FindFactorEightTest,
  &MatchFactorOneTest,  &MatchFactorTwoTest, &MatchFactorThreeTest, &MatchFactorFourTest,
  &MatchFactorFiveTest, &MatchFactorSixTest, &MatchFactorSevenTest, &MatchFactorEightTest,
  &RemoveFactorOneTest,  &RemoveFactorTwoTest, &RemoveFactorThreeTest, &RemoveFactorFourTest,
  &RemoveFactorFiveTest, &RemoveFactorSixTest, &RemoveFactorSevenTest, &RemoveFactorEightTest
);

swTestSuiteStructDeclare(LPMIPv6TestSuite, setupIPv6Addresses, teardownAddresses, swTestRun,
  &InsertFactorOneTest,  &InsertFactorTwoTest, &InsertFactorThreeTest, &InsertFactorFourTest,
  &InsertFactorFiveTest, &InsertFactorSixTest, &InsertFactorSevenTest, &InsertFactorEightTest,
  &FindFactorOneTest,  &FindFactorTwoTest, &FindFactorThreeTest, &FindFactorFourTest,
  &FindFactorFiveTest, &FindFactorSixTest, &FindFactorSevenTest, &FindFactorEightTest,
  &MatchFactorOneTest,  &MatchFactorTwoTest, &MatchFactorThreeTest, &MatchFactorFourTest,
  &MatchFactorFiveTest, &MatchFactorSixTest, &MatchFactorSevenTest, &MatchFactorEightTest,
  &RemoveFactorOneTest,  &RemoveFactorTwoTest, &RemoveFactorThreeTest, &RemoveFactorFourTest,
  &RemoveFactorFiveTest, &RemoveFactorSixTest, &RemoveFactorSevenTest, &RemoveFactorEightTest
);
