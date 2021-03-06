#include "hash-set-linear.h"

#include "unittest/unittest.h"
#include "storage/static-string.h"
#include "core/memory.h"
#include "utils/file.h"

void basicTestSetup(swTestSuite *suite, swTest *test)
{
  swTestLogLine("Creating hash set ...\n");
  swHashSetLinear *set = swHashSetLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL);
  ASSERT_NOT_NULL(set);
  swTestDataSet(test, set);
}

void basicTestTeardown(swTestSuite *suite, swTest *test)
{
  swTestLogLine("Deleting hash set ...\n");
  swHashSetLinear *set = swTestDataGet(test);
  ASSERT_NOT_NULL(set);
  swHashSetLinearDelete(set);
}

swTestDeclare(BasicTest, basicTestSetup, basicTestTeardown, swTestRun)
{
  swHashSetLinear *set = swTestDataGet(test);
  ASSERT_NOT_NULL(set);

  swStaticString s1 = swStaticStringDefine("test string");
  swStaticString s2 = swStaticStringDefineFromCstr("test string");

  swTestLogLine ("s1.len = %lu\n", s1.len);
  swTestLogLine("s2.len = %lu\n", s2.len);
  ASSERT_DATA(s1.data, s1.len, s2.data, s2.len);
  ASSERT_EQUAL(swStaticStringEqual(&s1, &s2), true);

  ASSERT_TRUE (swHashSetLinearInsert(set, &s1));
  ASSERT_EQUAL(swHashSetLinearCount(set), 1);
  ASSERT_TRUE (swHashSetLinearContains(set, &s2));
  ASSERT_TRUE(swHashSetLinearRemove(set, &s2));
  ASSERT_EQUAL(swHashSetLinearCount(set), 0);
  ASSERT_TRUE (swHashSetLinearInsert(set, &s1));
  ASSERT_TRUE(swHashSetLinearExtract(set, &s2) == &s1);
  ASSERT_EQUAL(swHashSetLinearCount(set), 0);
  ASSERT_TRUE (swHashSetLinearInsert(set, &s1));
  ASSERT_TRUE (swHashSetLinearUpsert(set, &s2));
  ASSERT_TRUE(swHashSetLinearExtract(set, &s1) == &s2);
  ASSERT_EQUAL(swHashSetLinearCount(set), 0);

  return true;
}

swTestSuiteStructDeclare(HashSetLinearTest, NULL, NULL, swTestRun, &BasicTest);

typedef struct swDictionaryTestData
{
  swHashSetLinear *set;
  swStaticString fileData;
  swStaticString *fileSlices;
  size_t  slicesCount;
} swDictionaryTestData;


bool swDictionaryTestDataSlicesSet(swDictionaryTestData *data)
{
  bool rtn = false;
  if (data)
  {
    size_t slicesCount = 2;
    if ((data->fileSlices = swMemoryMalloc(slicesCount * sizeof(swStaticString))))
    {
      size_t sliceIndex = 0;
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
            if (sliceIndex == slicesCount)
            {
              slicesCount *= 2;
              swStaticString *newSlices = swMemoryRealloc(data->fileSlices, slicesCount * sizeof(swStaticString));
              if (newSlices)
                data->fileSlices = newSlices;
              else
                break;
            }
            data->fileSlices[sliceIndex] = swStaticStringSetWithLength(&str[j], i-j);
            sliceIndex++;
          }
          j = i+1;
        }
        i++;
      }
      if (i == len)
      {
        data->slicesCount = sliceIndex;
        rtn = true;
      }
    }
  }
  return rtn;
}


void swDictionaryTestDataDelete(swDictionaryTestData *data)
{
  if (data)
  {
    if (data->set)
      swHashSetLinearDelete(data->set);
    if (data->fileSlices)
      swMemoryFree(data->fileSlices);
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
      if ((dictionaryTestData->set = swHashSetLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, (useKeyDelete? swMemoryFree: NULL))))
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

swTestDeclare(DictionaryTestInsert, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    if (!swHashSetLinearInsert(dictionaryTestData->set, &(dictionaryTestData->fileSlices[sliceIndex])))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashSetLinearCount(dictionaryTestData->set));
  swTestLogLine("Inserted %u slices\n", sliceIndex);
  return true;
}

swTestDeclare(DictionaryTestContains, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    if (!swHashSetLinearContains(dictionaryTestData->set, &key))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  swTestLogLine("All the %zu words are found\n", sliceIndex);
  return true;
}

swTestDeclare(DictionaryTestClear, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  swHashSetLinearClear(dictionaryTestData->set);
  ASSERT_EQUAL(swHashSetLinearCount(dictionaryTestData->set), 0);
  ASSERT_EQUAL(dictionaryTestData->set->used, 0);
  ASSERT_EQUAL(dictionaryTestData->set->size, 8);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    if (swHashSetLinearContains(dictionaryTestData->set, &key))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  return true;
}

swTestDeclare(DictionaryTestUpsert, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    if (!swHashSetLinearInsert(dictionaryTestData->set, &key))
      break;
    if (!swHashSetLinearUpsert(dictionaryTestData->set, &(dictionaryTestData->fileSlices[sliceIndex])))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashSetLinearCount(dictionaryTestData->set));
  swTestLogLine("Upserted %u slices\n", sliceIndex);
  return true;
}

swTestDeclare(DictionaryTestExtract, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    if (swHashSetLinearExtract(dictionaryTestData->set, &key) != &(dictionaryTestData->fileSlices[sliceIndex]))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashSetLinearCount(dictionaryTestData->set), 0);
  swTestLogLine("Extracted %u slices\n", sliceIndex);
  return true;
}

swTestDeclare(DictionaryTestRemove, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    if (!swHashSetLinearRemove(dictionaryTestData->set, &key))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashSetLinearCount(dictionaryTestData->set), 0);
  swTestLogLine("Removed %u slices, size = %zu, used = %zu, count = %zu\n",
    sliceIndex, dictionaryTestData->set->size, dictionaryTestData->set->used, dictionaryTestData->set->count);
  return true;
}

swTestSuiteStructDeclare(HashSetLinearDictionaryTest, dictionaryTestSuiteSetup, dictionaryTestSuiteTeardown, swTestRun,
                         &DictionaryTestInsert, &DictionaryTestContains, &DictionaryTestClear, &DictionaryTestUpsert,
                         &DictionaryTestExtract, &DictionaryTestInsert, &DictionaryTestRemove);

void dictionaryRemoveTestSuiteSetup(swTestSuite *suite)
{
  swTestLogLine("Creating dictionary test data ...\n");
  swDictionaryTestData *dictionaryTestData = swDictionaryTestDataNew("/usr/share/dict/ngerman", true);
  ASSERT_NOT_NULL(dictionaryTestData);
  swTestSuiteDataSet(suite, dictionaryTestData);
}

void dictionaryRemoveTestSuiteTeardown(swTestSuite *suite)
{
  swTestLogLine("Deleting dictionary test data ...\n");
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  ASSERT_NOT_NULL(dictionaryTestData);
  swDictionaryTestDataDelete(dictionaryTestData);
}

swTestDeclare(DictionaryRemoveTestInsertRemove, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString *sliceCopy = swMemoryDuplicate(&(dictionaryTestData->fileSlices[sliceIndex]), sizeof(swStaticString));
    if (sliceCopy)
    {
      if (!swHashSetLinearInsert(dictionaryTestData->set, sliceCopy))
        break;
    }
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashSetLinearCount(dictionaryTestData->set));
  swTestLogLine("Inserted %u slices\n", sliceIndex);

  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    if (!swHashSetLinearRemove(dictionaryTestData->set, &key))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashSetLinearCount(dictionaryTestData->set), 0);
  swTestLogLine("Removed %u slices, size = %zu, used = %zu, count = %zu\n",
    sliceIndex, dictionaryTestData->set->size, dictionaryTestData->set->used, dictionaryTestData->set->count);
  return true;
}

swTestDeclare(DictionaryRemoveTestInsertClear, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString *sliceCopy = swMemoryDuplicate(&(dictionaryTestData->fileSlices[sliceIndex]), sizeof(swStaticString));
    if (sliceCopy)
    {
      if (!swHashSetLinearInsert(dictionaryTestData->set, sliceCopy))
        break;
    }
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashSetLinearCount(dictionaryTestData->set));
  swTestLogLine("Inserted %u slices\n", sliceIndex);

  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString *sliceCopy = swMemoryDuplicate(&(dictionaryTestData->fileSlices[sliceIndex]), sizeof(swStaticString));
    if (sliceCopy)
    {
      if (!swHashSetLinearUpsert(dictionaryTestData->set, sliceCopy))
        break;
    }
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashSetLinearCount(dictionaryTestData->set));
  swTestLogLine("Upserted %u slices\n", sliceIndex);

  swHashSetLinearClear(dictionaryTestData->set);
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashSetLinearCount(dictionaryTestData->set), 0);
  swTestLogLine("Removed %u slices, size = %zu, used = %zu, count = %zu\n",
    sliceIndex, dictionaryTestData->set->size, dictionaryTestData->set->used, dictionaryTestData->set->count);
  return true;
}

swTestDeclare(DictionaryRemoveTestInsertExtract, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString *keyCopy = swMemoryDuplicate(&(dictionaryTestData->fileSlices[sliceIndex]), sizeof(swStaticString));
    if (keyCopy)
    {
      if (!swHashSetLinearInsert(dictionaryTestData->set, keyCopy))
        break;
    }
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashSetLinearCount(dictionaryTestData->set));
  swTestLogLine("Inserted %u slices\n", sliceIndex);

  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    swStaticString *keyCopy = swHashSetLinearExtract(dictionaryTestData->set, &key);
    if (keyCopy == &(dictionaryTestData->fileSlices[sliceIndex]))
      break;
    swMemoryFree(keyCopy);
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashSetLinearCount(dictionaryTestData->set), 0);
  swTestLogLine("Extracted %u slices, size = %zu, used = %zu, count = %zu\n",
    sliceIndex, dictionaryTestData->set->size, dictionaryTestData->set->used, dictionaryTestData->set->count);
  return true;
}

swTestSuiteStructDeclare(HashSetLinearDictionaryRemoveTest, dictionaryRemoveTestSuiteSetup, dictionaryRemoveTestSuiteTeardown, swTestRun,
                         &DictionaryRemoveTestInsertRemove, &DictionaryRemoveTestInsertClear, &DictionaryRemoveTestInsertExtract);

