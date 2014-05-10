#include "hash-map-linear.h"

#include "unittest/unittest.h"
#include "storage/static-string.h"
#include "core/memory.h"
#include "utils/file.h"

void basicTestSetup(swTestSuite *suite, swTest *test)
{
  swTestLogLine("Creating hash map ...\n");
  swHashMapLinear *map = swHashMapLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL);
  ASSERT_NOT_NULL(map);
  swTestDataSet(test, map);
}

void basicTestTeardown(swTestSuite *suite, swTest *test)
{
  swTestLogLine("Deleting hash map ...\n");
  swHashMapLinear *map = swTestDataGet(test);
  ASSERT_NOT_NULL(map);
  swHashMapLinearDelete(map);
}

swTestDeclare(BasicTest, basicTestSetup, basicTestTeardown, swTestRun)
{
  swHashMapLinear *map = swTestDataGet(test);
  ASSERT_NOT_NULL(map);

  swStaticString s1 = swStaticStringDefine("test string");
  swStaticString s2 = swStaticStringDefineFromCstr("test string");

  uint32_t v1 = 10;
  uint32_t v2 = 5;

  uint32_t *value = NULL;

  swTestLogLine ("s1.len = %lu\n", s1.len);
  swTestLogLine("s2.len = %lu\n", s2.len);
  ASSERT_DATA(s1.data, s1.len, s2.data, s2.len);
  ASSERT_EQUAL(swStaticStringEqual(&s1, &s2), true);

  ASSERT_TRUE (swHashMapLinearInsert(map, &s1, &v1));
  ASSERT_EQUAL(swHashMapLinearCount(map), 1);
  ASSERT_TRUE (swHashMapLinearValueGet(map, &s2, (void **)&value));
  ASSERT_TRUE(value == &v1);
  ASSERT_TRUE(swHashMapLinearRemove(map, &s2));
  ASSERT_EQUAL(swHashMapLinearCount(map), 0);
  ASSERT_TRUE (swHashMapLinearInsert(map, &s1, &v1));
  ASSERT_TRUE(swHashMapLinearExtract(map, &s2, (void **)&value) == &s1);
  ASSERT_EQUAL(swHashMapLinearCount(map), 0);
  ASSERT_TRUE(value == &v1);
  ASSERT_TRUE (swHashMapLinearInsert(map, &s1, &v1));
  ASSERT_TRUE (swHashMapLinearUpsert(map, &s2, &v2));
  ASSERT_TRUE(swHashMapLinearExtract(map, &s1, (void **)&value) == &s2);
  ASSERT_EQUAL(swHashMapLinearCount(map), 0);
  ASSERT_TRUE(value == &v2);

  return true;
}

swTestSuiteStructDeclare(HashMapLinearTest, NULL, NULL, swTestRun, &BasicTest);

typedef struct swDictionaryTestData
{
  swHashMapLinear *map;
  swStaticString fileData;
  swStaticString *fileSlices;
  uint32_t *values;
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
      if ((data->values = swMemoryMalloc(slicesCount * sizeof(uint32_t))))
      {
        size_t sliceIndex = 0;
        size_t i = 0, j = 0, len = data->fileData.len;
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
                {
                  data->fileSlices = newSlices;
                  uint32_t *newValues = swMemoryRealloc(data->values, slicesCount * sizeof(uint32_t));
                  if (newValues)
                    data->values = newValues;
                  else
                    break;
                }
                else
                  break;
              }
              data->fileSlices[sliceIndex] = swStaticStringSetWithLength(&str[j], i-j);
              data->values[sliceIndex] = sliceIndex;
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
  }
  return rtn;
}

void swDictionaryTestDataDelete(swDictionaryTestData *data)
{
  if (data)
  {
    if (data->map)
      swHashMapLinearDelete(data->map);
    if (data->fileSlices)
      swMemoryFree(data->fileSlices);
    if (data->values)
      swMemoryFree(data->values);
    if (data->fileData.data)
      swMemoryFree(data->fileData.data);
    swMemoryFree(data);
  }
}

swDictionaryTestData *swDictionaryTestDataNew(const char *fileName, bool useDelete)
{
  swDictionaryTestData *rtn = NULL;
  if (fileName)
  {
    swDictionaryTestData *dictionaryTestData = swMemoryCalloc(1, sizeof(swDictionaryTestData));
    if (dictionaryTestData)
    {
      if ((dictionaryTestData->map = swHashMapLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, (useDelete? swMemoryFree: NULL), (useDelete? swMemoryFree: NULL))))
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
    if (!swHashMapLinearInsert(dictionaryTestData->map, &(dictionaryTestData->fileSlices[sliceIndex]), &(dictionaryTestData->values[sliceIndex])))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashMapLinearCount(dictionaryTestData->map));
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
    uint32_t *value = NULL;
    if (!swHashMapLinearValueGet(dictionaryTestData->map, &key, (void **)&value) || (value != &(dictionaryTestData->values[sliceIndex])))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  swTestLogLine("All the %zu words are found\n", sliceIndex);
  return true;
}

swTestDeclare(DictionaryTestClear, NULL, NULL, swTestRun)
{
  swDictionaryTestData *dictionaryTestData = swTestSuiteDataGet(suite);
  swHashMapLinearClear(dictionaryTestData->map);
  ASSERT_EQUAL(swHashMapLinearCount(dictionaryTestData->map), 0);
  ASSERT_EQUAL(dictionaryTestData->map->used, 0);
  ASSERT_EQUAL(dictionaryTestData->map->size, 8);
  size_t sliceIndex = 0;
  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    uint32_t *value = NULL;
    if (swHashMapLinearValueGet(dictionaryTestData->map, &key, (void **)&value) || value)
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
    uint32_t value = dictionaryTestData->values[sliceIndex];
    if (!swHashMapLinearInsert(dictionaryTestData->map, &key, &value))
      break;
    if (!swHashMapLinearUpsert(dictionaryTestData->map, &(dictionaryTestData->fileSlices[sliceIndex]), &(dictionaryTestData->values[sliceIndex])))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashMapLinearCount(dictionaryTestData->map));
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
    uint32_t *value = NULL;
    if (swHashMapLinearExtract(dictionaryTestData->map, &key, (void **)&value) != &(dictionaryTestData->fileSlices[sliceIndex]) ||
        value != &(dictionaryTestData->values[sliceIndex]))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashMapLinearCount(dictionaryTestData->map), 0);
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
    if (!swHashMapLinearRemove(dictionaryTestData->map, &key))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashMapLinearCount(dictionaryTestData->map), 0);
  swTestLogLine("Removed %u slices, size = %zu, used = %zu, count = %zu\n",
    sliceIndex, dictionaryTestData->map->size, dictionaryTestData->map->used, dictionaryTestData->map->count);
  return true;
}

swTestSuiteStructDeclare(HashMapLinearDictionaryTest, dictionaryTestSuiteSetup, dictionaryTestSuiteTeardown, swTestRun,
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
      uint32_t *valueCopy = swMemoryDuplicate(&(dictionaryTestData->values[sliceIndex]), sizeof(uint32_t));
      if (valueCopy)
      {
        if (!swHashMapLinearInsert(dictionaryTestData->map, sliceCopy, valueCopy))
          break;
      }
    }
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashMapLinearCount(dictionaryTestData->map));
  swTestLogLine("Inserted %u slices\n", sliceIndex);

  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    if (!swHashMapLinearRemove(dictionaryTestData->map, &key))
      break;
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashMapLinearCount(dictionaryTestData->map), 0);
  swTestLogLine("Removed %u slices, size = %zu, used = %zu, count = %zu\n",
    sliceIndex, dictionaryTestData->map->size, dictionaryTestData->map->used, dictionaryTestData->map->count);
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
      uint32_t *valueCopy = swMemoryDuplicate(&(dictionaryTestData->values[sliceIndex]), sizeof(uint32_t));
      if (valueCopy)
      {
        if (!swHashMapLinearInsert(dictionaryTestData->map, sliceCopy, valueCopy))
          break;
      }
    }
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashMapLinearCount(dictionaryTestData->map));
  swTestLogLine("Inserted %u slices\n", sliceIndex);

  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString *sliceCopy = swMemoryDuplicate(&(dictionaryTestData->fileSlices[sliceIndex]), sizeof(swStaticString));
    if (sliceCopy)
    {
      uint32_t *valueCopy = swMemoryDuplicate(&(dictionaryTestData->values[sliceIndex]), sizeof(uint32_t));
      if (valueCopy)
      {
        if (!swHashMapLinearUpsert(dictionaryTestData->map, sliceCopy, valueCopy))
          break;
      }
    }
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashMapLinearCount(dictionaryTestData->map));
  swTestLogLine("Upserted %u slices\n", sliceIndex);

  swHashMapLinearClear(dictionaryTestData->map);
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashMapLinearCount(dictionaryTestData->map), 0);
  swTestLogLine("Removed %u slices, size = %zu, used = %zu, count = %zu\n",
    sliceIndex, dictionaryTestData->map->size, dictionaryTestData->map->used, dictionaryTestData->map->count);
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
      uint32_t *valueCopy = swMemoryDuplicate(&(dictionaryTestData->values[sliceIndex]), sizeof(uint32_t));
      if (valueCopy)
      {
        if (!swHashMapLinearInsert(dictionaryTestData->map, keyCopy, valueCopy))
          break;
      }
    }
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(sliceIndex, swHashMapLinearCount(dictionaryTestData->map));
  swTestLogLine("Inserted %u slices\n", sliceIndex);

  for (sliceIndex = 0; sliceIndex < dictionaryTestData->slicesCount; sliceIndex++)
  {
    swStaticString key = dictionaryTestData->fileSlices[sliceIndex];
    uint32_t *valueCopy = NULL;
    swStaticString *keyCopy = swHashMapLinearExtract(dictionaryTestData->map, &key, (void **)&valueCopy);
    ASSERT_NOT_NULL(valueCopy);
    if (keyCopy == &(dictionaryTestData->fileSlices[sliceIndex]) ||
        valueCopy == &(dictionaryTestData->values[sliceIndex]))
      break;
    swMemoryFree(keyCopy);
    swMemoryFree(valueCopy);
  }
  ASSERT_EQUAL(sliceIndex, dictionaryTestData->slicesCount);
  ASSERT_EQUAL(swHashMapLinearCount(dictionaryTestData->map), 0);
  swTestLogLine("Extracted %u slices, size = %zu, used = %zu, count = %zu\n",
    sliceIndex, dictionaryTestData->map->size, dictionaryTestData->map->used, dictionaryTestData->map->count);
  return true;
}

swTestSuiteStructDeclare(HashMapLinearDictionaryRemoveTest, dictionaryRemoveTestSuiteSetup, dictionaryRemoveTestSuiteTeardown, swTestRun,
                         &DictionaryRemoveTestInsertRemove, &DictionaryRemoveTestInsertClear, &DictionaryRemoveTestInsertExtract);
