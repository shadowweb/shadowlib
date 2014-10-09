#include "unittest/unittest.h"
#include "collections/sparse-array.h"
#include "storage/static-string.h"
#include "core/memory.h"
#include "utils/file.h"

typedef struct swDictionaryTestData
{
  swSparseArray array;
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
          swStaticString *word = NULL;
          uint32_t index = 0;
          if (!swSparseArrayAcquireFirstFree(&(data->array), &index, (void **)(&word)))
            break;
          *word = swStaticStringSetWithLength(&str[j], i-j);
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
    swSparseArrayRelease(&(data->array));
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
      if (swSparseArrayInit(&(dictionaryTestData->array), sizeof(swStaticString), 64, 8))
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
  ASSERT_TRUE(swSparseArrayCount(dictionaryTestData->array) > 0);
  swTestLogLine("Array count %u\n", swSparseArrayCount(dictionaryTestData->array));
  return true;
}

swTestDeclare(DictionaryTestRemove, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  uint32_t count = swSparseArrayCount(dictionaryTestData->array);

  for (uint32_t i = 0; i < count; i++)
    ASSERT_TRUE(swSparseArrayRemove(&(dictionaryTestData->array), i));
  // for (uint32_t i = count; i > 0; i--)
  //   ASSERT_TRUE(swSparseArrayRemove(&(dictionaryTestData->array), i - 1));

  ASSERT_FALSE(swSparseArrayRemove(&(dictionaryTestData->array), count));
  ASSERT_EQUAL(swSparseArrayCount(dictionaryTestData->array), 0);
  swTestLogLine("Array count %u\n", swSparseArrayCount(dictionaryTestData->array));
  return true;
}


// TODO: add the following tests
// read all elements randomly
// remove half of the elements; read elements randomly
// add some elements back; read elements randomly
// remove all one by one

swTestSuiteStructDeclare(SparseArrayDictionaryTest, dictionaryTestSuiteSetup, dictionaryTestSuiteTeardown, swTestRun,
                         &DictionaryTestVerify, &DictionaryTestRemove);
