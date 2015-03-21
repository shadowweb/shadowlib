#include "protocol/checksum.h"
#include "unittest/unittest.h"
#include "utils/random.h"

bool verifyChecksum(size_t size)
{
  bool rtn = false;
  swChecksum checksum = swChecksumSetEmpty;

  swDynamicBuffer buffer = {.size = 0};

  if (swUtilFillURandom(&buffer, size))
  {
    uint16_t simpleChecksum = swChecksumCalculate((swStaticBuffer *)&buffer);
    swChecksumAdd(&checksum, (swStaticBuffer *)&buffer);
    swChecksumFinalize(&checksum);
    ASSERT_EQUAL(swChecksumGet(checksum), simpleChecksum);
    rtn = true;
    swDynamicBufferRelease(&buffer);
  }
  return rtn;
}

swTestDeclare(ChecksumVerificationEvenTest, NULL, NULL, swTestRun)
{
  return verifyChecksum(100);
}

swTestDeclare(ChecksumVerificationOddTest, NULL, NULL, swTestRun)
{
  return verifyChecksum(101);
}

swTestSuiteStructDeclare(ChecksumVerification, NULL, NULL, swTestRun,
  &ChecksumVerificationEvenTest, &ChecksumVerificationOddTest);

typedef struct swChecksumTestData
{
  swChecksum checkSum;
  swDynamicBuffer buffer;
} swChecksumTestData;

swChecksumTestData data = { .checkSum = {.final = false } };

bool swChecksumTestDataInit(size_t bufferSize)
{
  bool rtn = false;
  if (bufferSize)
  {
    data.checkSum = swChecksumSetEmpty;
    if (swDynamicBufferInit(&(data.buffer), bufferSize))
    {
      if (swUtilFillURandom(&(data.buffer), bufferSize))
        rtn = true;
      else
        swDynamicBufferRelease(&(data.buffer));
    }
  }
  return rtn;
}

void swChecksumTestDataRelease()
{
  data.checkSum = swChecksumSetEmpty;
  swDynamicBufferRelease(&(data.buffer));
}

void swChecksumTestTeardown(swTestSuite *suite, swTest *test)
{
  uint16_t checksumValue = swChecksumCalculate((swStaticBuffer *)&(data.buffer));
  ASSERT_EQUAL(swChecksumGet(data.checkSum), checksumValue);
  swChecksumTestDataRelease();
}

void swChecksumTestLargeBufferSetup(swTestSuite *suite, swTest *test)
{
  ASSERT_TRUE(swChecksumTestDataInit(UINT16_MAX + 1));
}

swTestDeclare(LargeBufferTest, swChecksumTestLargeBufferSetup, swChecksumTestTeardown, swTestRun)
{
  swChecksumAdd(&(data.checkSum), (swStaticBuffer *)&(data.buffer));
  swChecksumFinalize(&(data.checkSum));
  return true;
}

void swChecksumTestMediumBufferSetup(swTestSuite *suite, swTest *test)
{
  ASSERT_TRUE(swChecksumTestDataInit(UINT8_MAX + 1));
  // ASSERT_TRUE(swChecksumTestDataInit(4));
  // *(uint32_t *)(data.buffer.data) = 1;
}

swTestDeclare(MediumBufferReplaceEvenBeforeFinalizeTest, swChecksumTestMediumBufferSetup, swChecksumTestTeardown, swTestRun)
{
  swChecksumAdd(&(data.checkSum), (swStaticBuffer *)&(data.buffer));
  uint32_t *value = (uint32_t *)data.buffer.data;
  swStaticBuffer oldValueBuffer = swStaticBufferDefineWithLength(value, sizeof(uint32_t));
  uint32_t newValue = (*value) + 1;
  swStaticBuffer newValueBuffer = swStaticBufferDefineWithLength(&newValue, sizeof(uint32_t));
  swChecksumReplace(&(data.checkSum), &oldValueBuffer, &newValueBuffer);
  swChecksumFinalize(&(data.checkSum));
  *value = newValue;
  return true;
}

swTestDeclare(MediumBufferReplaceEvenAfterFinalizeTest, swChecksumTestMediumBufferSetup, swChecksumTestTeardown, swTestRun)
{
  swChecksumAdd(&(data.checkSum), (swStaticBuffer *)&(data.buffer));
  swChecksumFinalize(&(data.checkSum));
  uint32_t *value = (uint32_t *)data.buffer.data;
  swStaticBuffer oldValueBuffer = swStaticBufferDefineWithLength(value, sizeof(uint32_t));
  uint32_t newValue = (*value) + 127;
  swStaticBuffer newValueBuffer = swStaticBufferDefineWithLength(&newValue, sizeof(uint32_t));
  swChecksumReplace(&(data.checkSum), &oldValueBuffer, &newValueBuffer);
  *value = newValue;
  return true;
}

void swChecksumTestOddMediumBufferSetup(swTestSuite *suite, swTest *test)
{
  ASSERT_TRUE(swChecksumTestDataInit(UINT8_MAX));
}

swTestDeclare(MediumBufferReplaceOddBeforeFinalizeTest, swChecksumTestOddMediumBufferSetup, swChecksumTestTeardown, swTestRun)
{
  swChecksumAdd(&(data.checkSum), (swStaticBuffer *)&(data.buffer));
  swStaticBuffer oldValueBuffer = swStaticBufferDefineWithLength(data.buffer.data, sizeof(uint32_t) + sizeof(uint8_t));
  uint8_t buff[5] = {data.buffer.data[0] + 1, data.buffer.data[0] + 2, data.buffer.data[0] +3 , data.buffer.data[0] + 4, data.buffer.data[0] + 5};
  swStaticBuffer newValueBuffer = swStaticBufferDefineWithLength(buff, sizeof(uint32_t) + sizeof(uint8_t));
  swChecksumReplace(&(data.checkSum), &oldValueBuffer, &newValueBuffer);
  swChecksumFinalize(&(data.checkSum));
  memcpy(data.buffer.data, buff, sizeof(buff));
  return true;
}

swTestDeclare(MediumBufferReplaceOddAfterFinalizeTest, swChecksumTestOddMediumBufferSetup, swChecksumTestTeardown, swTestRun)
{
  swChecksumAdd(&(data.checkSum), (swStaticBuffer *)&(data.buffer));
  swChecksumFinalize(&(data.checkSum));
  swStaticBuffer oldValueBuffer = swStaticBufferDefineWithLength(data.buffer.data, sizeof(uint32_t) + sizeof(uint8_t));
  uint8_t buff[5] = {data.buffer.data[0] + 1, data.buffer.data[0] + 2, data.buffer.data[0] +3 , data.buffer.data[0] + 4, data.buffer.data[0] + 5};
  swStaticBuffer newValueBuffer = swStaticBufferDefineWithLength(buff, sizeof(uint32_t) + sizeof(uint8_t));
  swChecksumReplace(&(data.checkSum), &oldValueBuffer, &newValueBuffer);
  memcpy(data.buffer.data, buff, sizeof(buff));
  return true;
}

swTestSuiteStructDeclare(ChecksumTestSuite, NULL, NULL, swTestRun,
  &LargeBufferTest, &MediumBufferReplaceEvenBeforeFinalizeTest, &MediumBufferReplaceEvenAfterFinalizeTest,
  &MediumBufferReplaceOddBeforeFinalizeTest, &MediumBufferReplaceOddAfterFinalizeTest);
