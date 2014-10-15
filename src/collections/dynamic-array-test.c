#include "collections/dynamic-array.h"
#include "storage/static-string.h"
#include "core/memory.h"
#include "unittest/unittest.h"
#include "utils/file.h"

typedef struct swDictionaryTestData
{
  swDynamicArray *array;
  swStaticString fileData;
} swDictionaryTestData;

bool swDictionaryTestDataSlicesSet(swDictionaryTestData *data)
{
  bool rtn = false;
  if (data)
  {
    size_t i = 0;
    size_t j = 0;
    size_t len = data->fileData.len;
    char *str = data->fileData.data;
    while (i < len)
    {
      if (str[i] == '\n')
      {
        if (i > j)
        {
          swStaticString word = swStaticStringDefineWithLength(&str[j], i-j);
          if (!swDynamicArrayPush(data->array, &word))
            break;
        }
        j = i+1;
      }
      i++;
    }
    if (i == len)
      rtn = true;
  }
  return rtn;
}

void swDictionaryTestDataDelete(swDictionaryTestData *data)
{
  if (data)
  {
    if (data->array)
      swDynamicArrayDelete(data->array);
    if (data->fileData.data)
      swMemoryFree(data->fileData.data);
    swMemoryFree(data);
  }
}

swDictionaryTestData *swDictionaryTestDataNew(const char *fileName, bool useKeyDelete)
{
  swDictionaryTestData *rtn = NULL;
  if (fileName)
  {
    swDictionaryTestData *dictionaryTestData = swMemoryCalloc(1, sizeof(swDictionaryTestData));
    if (dictionaryTestData)
    {
      if ((dictionaryTestData->array = swDynamicArrayNew(sizeof(swStaticString), 8)))
      {
        if ((dictionaryTestData->fileData.len = swFileRead(fileName, &(dictionaryTestData->fileData.data))))
        {
          if (swDictionaryTestDataSlicesSet(dictionaryTestData))
            rtn = dictionaryTestData;
        }
      }
      if (!rtn)
        swDictionaryTestDataDelete(dictionaryTestData);
    }
  }
  return rtn;
}

void dictionaryTestSuiteSetup(swTestSuite *suite)
{
  swTestLogLine("Creating dictionary test data ...\n");
  swDictionaryTestData *dictionaryTestData = swDictionaryTestDataNew("/usr/share/dict/ngerman", false);
  ASSERT_NOT_NULL(dictionaryTestData);
  swTestSuiteDataSet(suite, dictionaryTestData);
}

void dictionaryTestSuiteTeardown(swTestSuite *suite)
{
  swTestLogLine("Deleting dictionary test data ...\n");
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(dictionaryTestData);
  swDictionaryTestDataDelete(dictionaryTestData);
}

swTestDeclare(DictionaryTestVerify, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  ASSERT_TRUE(swDynamicArrayCount(*(dictionaryTestData->array)) <= swDynamicArraySize(*(dictionaryTestData->array)));
  swTestLogLine("Array count %u; Array size %u\n", swDynamicArrayCount(*(dictionaryTestData->array)), swDynamicArraySize(*(dictionaryTestData->array)));
  return true;
}

swTestDeclare(DictionaryTestPop, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  uint32_t count = swDynamicArrayCount(*(dictionaryTestData->array));
  swStaticString element = swStaticStringDefineEmpty;
  for (uint32_t i = 0; i < count; i++)
  {
    ASSERT_TRUE(swDynamicArrayPop(dictionaryTestData->array, &element));
    ASSERT_NOT_NULL(element.data);
    ASSERT_TRUE(element.len != 0);
  }
  ASSERT_FALSE(swDynamicArrayPop(dictionaryTestData->array, &element));
  ASSERT_EQUAL(swDynamicArrayCount(*(dictionaryTestData->array)), 0);
  swTestLogLine("Array count %u; Array size %u\n", swDynamicArrayCount(*(dictionaryTestData->array)), swDynamicArraySize(*(dictionaryTestData->array)));
  return true;
}

swTestSuiteStructDeclare(DynamicArrayDictionaryTest, dictionaryTestSuiteSetup, dictionaryTestSuiteTeardown, swTestRun,
                         &DictionaryTestVerify, &DictionaryTestPop);
