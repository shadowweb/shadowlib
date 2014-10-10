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
    swStaticString *word = NULL;
    uint32_t index = 0;
    while (i < len)
    {
      if (str[i] == '\n')
      {
        if (i > j)
        {
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

static uint32_t itemsPerBlock = 64;

swDictionaryTestData *swDictionaryTestDataNew(const char *fileName, bool useKeyDelete)
{
  swDictionaryTestData *rtn = NULL;
  if (fileName)
  {
    swDictionaryTestData *dictionaryTestData = swMemoryCalloc(1, sizeof(swDictionaryTestData));
    if (dictionaryTestData)
    {
      if (swSparseArrayInit(&(dictionaryTestData->array), sizeof(swStaticString), itemsPerBlock, 8))
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

  ASSERT_FALSE(swSparseArrayRemove(&(dictionaryTestData->array), count));
  ASSERT_EQUAL(swSparseArrayCount(dictionaryTestData->array), 0);
  swTestLogLine("Array count %u\n", swSparseArrayCount(dictionaryTestData->array));
  return true;
}

swTestSuiteStructDeclare(SparseArrayDictionaryTest, dictionaryTestSuiteSetup, dictionaryTestSuiteTeardown, swTestRun,
                         &DictionaryTestVerify, &DictionaryTestRemove);

swTestDeclare(DictionaryTestExtract, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  uint32_t count = swSparseArrayCount(dictionaryTestData->array);
  swStaticString extractedStrings[itemsPerBlock];

  for (uint32_t i = itemsPerBlock; i < itemsPerBlock * 2; i++)
  {
    ASSERT_TRUE(swSparseArrayExtract(&(dictionaryTestData->array), i, &(extractedStrings[i - itemsPerBlock])));
    ASSERT_NOT_NULL(extractedStrings[i - itemsPerBlock].data);
    ASSERT_NOT_EQUAL(extractedStrings[i - itemsPerBlock].len, 0);
  }
  ASSERT_EQUAL(swSparseArrayCount(dictionaryTestData->array), count - itemsPerBlock);

  uint32_t index = 0;
  swStaticString *insertString = NULL;
  for (uint32_t i = 0; i < itemsPerBlock; i++)
  {
    ASSERT_TRUE(swSparseArrayAcquireFirstFree(&(dictionaryTestData->array), &index, (void **)(&insertString)));
    ASSERT_NOT_NULL(insertString);
    ASSERT_EQUAL(index, i + itemsPerBlock);
    *insertString = extractedStrings[i];
    index = 0;
    insertString = NULL;
  }
  ASSERT_EQUAL(swSparseArrayCount(dictionaryTestData->array), count);

  uint32_t halfItemsPerBlock = itemsPerBlock / 2;
  for (uint32_t i = halfItemsPerBlock; i < (halfItemsPerBlock + itemsPerBlock); i++)
  {
    ASSERT_TRUE(swSparseArrayExtract(&(dictionaryTestData->array), i, &(extractedStrings[i - halfItemsPerBlock])));
    ASSERT_NOT_NULL(extractedStrings[i - halfItemsPerBlock].data);
    ASSERT_NOT_EQUAL(extractedStrings[i - halfItemsPerBlock].len, 0);
  }
  ASSERT_EQUAL(swSparseArrayCount(dictionaryTestData->array), count - itemsPerBlock);

  index = 0;
  insertString = NULL;
  for (uint32_t i = 0; i < itemsPerBlock; i++)
  {
    ASSERT_TRUE(swSparseArrayAcquireFirstFree(&(dictionaryTestData->array), &index, (void **)(&insertString)));
    ASSERT_NOT_NULL(insertString);
    ASSERT_EQUAL(index, i + halfItemsPerBlock);
    *insertString = extractedStrings[i];
    index = 0;
    insertString = NULL;
  }
  ASSERT_EQUAL(swSparseArrayCount(dictionaryTestData->array), count);

  swTestLogLine("Array count %u\n", swSparseArrayCount(dictionaryTestData->array));
  return true;
}

bool walkFunction(void *ptr)
{
  swStaticString *string = ptr;
  if (string && string->data && string->len)
    return true;
  return false;
}

swTestDeclare(DictionaryTestWalk, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  ASSERT_TRUE(swSparseArrayWalk(&(dictionaryTestData->array), walkFunction));
  return true;
}

swTestSuiteStructDeclare(SparseArrayDictionaryExtractTest, dictionaryTestSuiteSetup, dictionaryTestSuiteTeardown, swTestRun,
                         &DictionaryTestExtract, &DictionaryTestWalk);
