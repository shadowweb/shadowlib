#include "collections/lpm.h"
#include "core/memory.h"
#include "utils/random.h"

#include "unittest/unittest.h"

static uint64_t ipCount = 4194304;   // 4 * 1024 * 1024
// static uint64_t ipCount = 1048576;      // 1 * 1024 * 10924
// static uint64_t ipCount = 2097152;      // 2 * 1024 * 1024

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
                  // printf("Last byte bit count = %u\n", lastByteBitCount);
                  if (lastByteBitCount)
                  {
                    uint8_t mask = ~((1 << (8 - lastByteBitCount)) - 1);
                    prefix->prefixBytes[usedBytes - 1] &= mask;
                    // printf("Last bit mask = 0x%x\n", mask);
                  }
                  swLPMPrefix *storedPrefix = NULL;
                  if (!swLPMFind(lpm, prefix, &storedPrefix))
                  {
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
                  // swLPMPrefixPrint(prefix);
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

void setupTestWithFactor(swTest *test, uint8_t factor)
{
  swLPM *lpm = swLPMNew(factor);
  ASSERT_NOT_NULL(lpm);
  swTestDataSet(test, lpm);
}

void setupTestFactorOne(swTestSuite *suite, swTest *test)
{
  setupTestWithFactor(test, 1);
}

void setupTestFactorTwo(swTestSuite *suite, swTest *test)
{
  setupTestWithFactor(test, 2);
}

void setupTestFactorThree(swTestSuite *suite, swTest *test)
{
  setupTestWithFactor(test, 3);
}

void setupTestFactorFour(swTestSuite *suite, swTest *test)
{
  setupTestWithFactor(test, 4);
}

void setupTestFactorFive(swTestSuite *suite, swTest *test)
{
  setupTestWithFactor(test, 5);
}

void setupTestFactorSix(swTestSuite *suite, swTest *test)
{
  setupTestWithFactor(test, 6);
}

void setupTestFactorSeven(swTestSuite *suite, swTest *test)
{
  setupTestWithFactor(test, 7);
}

void setupTestFactorEight(swTestSuite *suite, swTest *test)
{
  setupTestWithFactor(test, 8);
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
        ASSERT_TRUE(swLPMPrefixEqual(prefix, storedPrefix));
      }
      else
        break;
    }
    buffer += testData->prefixSize;
  }
  if (buffer == bufferEnd)
    rtn = true;
  return rtn;
}

swTestDeclare(InsertFactorOneTest, setupTestFactorOne, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorTwoTest, setupTestFactorTwo, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorThreeTest, setupTestFactorThree, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorFourTest, setupTestFactorFour, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorFiveTest, setupTestFactorFive, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorSixTest, setupTestFactorSix, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorSevenTest, setupTestFactorSeven, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestDeclare(InsertFactorEightTest, setupTestFactorEight, teardownTest, swTestRun)
{
  return insertTestWithFactor(suite, test);
}

swTestSuiteStructDeclare(LPMIPv4TestSuite, setupIPv4Addresses, teardownAddresses, swTestRun,
  &InsertFactorOneTest, &InsertFactorTwoTest, &InsertFactorThreeTest, &InsertFactorFourTest,
  &InsertFactorFiveTest, &InsertFactorSixTest, &InsertFactorSevenTest, &InsertFactorEightTest
);

swTestSuiteStructDeclare(LPMIPv6TestSuite, setupIPv6Addresses, teardownAddresses, swTestRun,
  &InsertFactorOneTest, &InsertFactorTwoTest, &InsertFactorThreeTest, &InsertFactorFourTest,
  &InsertFactorFiveTest, &InsertFactorSixTest, &InsertFactorSevenTest, &InsertFactorEightTest
);

// TODO: test match and removal
