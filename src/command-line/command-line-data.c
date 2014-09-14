#include "command-line/command-line-data.h"
#include "command-line/command-line-state.h"
#include "core/memory.h"

static void _fastArrayClear(swFastArray *array)
{
  if (array && array->size)
  {
    for (uint32_t i = 0; i < array->count; i++)
      swOptionValuePairClear(swFastArrayGetExistingPtr(*array, i, swOptionValuePair));
    swFastArrayClear(array);
  }
}

static void _clearOptionAliases(swCommandLineData *commandLineData)
{
  if (commandLineData)
  {
    for (uint32_t i = 0; i < swFastArrayCount(commandLineData->categories); i++)
    {
      swOptionCategory *category = NULL;
      if (swFastArrayGet(commandLineData->categories, i, category))
      {
        swOption *option = category->options;
        while (option->valueType > swOptionValueTypeNone)
        {
          if (option->aliases.size)
            swDynamicArrayRelease(&(option->aliases));
          option++;
        }
      }
    }
  }
}

void swCommandLineDataDelete(swCommandLineData *commandLineData)
{
  if (commandLineData)
  {
    if (commandLineData->prefixedValues)
      swHashMapLinearDelete(commandLineData->prefixedValues);
    if (commandLineData->requiredValues.size)
      swFastArrayClear(&(commandLineData->requiredValues));
    if (commandLineData->namedValues)
      swHashMapLinearDelete(commandLineData->namedValues);

    swDynamicArrayRelease(&(commandLineData->consumeAfterValue));
    swDynamicArrayRelease(&(commandLineData->sinkValue));

    _fastArrayClear(&(commandLineData->positionalValues));
    _fastArrayClear(&(commandLineData->normalValues));
    _clearOptionAliases(commandLineData);
    if (commandLineData->categories.size)
      swFastArrayClear(&(commandLineData->categories));

    swDynamicStringRelease(&(commandLineData->programName));
    swDynamicStringRelease(&(commandLineData->argumentsString));
    swDynamicStringRelease(&(commandLineData->usageMessage));
    swMemoryFree(commandLineData);
  }
}

swCommandLineData *swCommandLineDataNew(uint32_t argumentCount)
{
  swCommandLineData *rtn = NULL;
  swCommandLineData *commandLineDataNew = swMemoryCalloc(1, sizeof(swCommandLineData));
  if (commandLineDataNew)
  {
    if ((swFastArrayInit(&(commandLineDataNew->categories), sizeof(swOptionCategory *), 4)))
    {
      if ((swFastArrayInit(&(commandLineDataNew->normalValues), sizeof(swOptionValuePair), argumentCount)) &&
          (swFastArrayInit(&(commandLineDataNew->positionalValues), sizeof(swOptionValuePair), argumentCount)))
      {
        if ((commandLineDataNew->namedValues    = swHashMapLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL)) &&
            (commandLineDataNew->prefixedValues = swHashMapLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL)))
        {
          // defaults to pointer comparison
          if (swFastArrayInit(&(commandLineDataNew->requiredValues), sizeof(swOptionValuePair), argumentCount))
          {
            commandLineDataNew->consumeAfterValue = swDynamicArraySetEmpty(swOptionValueTypeSizeGet(swOptionValueTypeString));
            commandLineDataNew->sinkValue = swDynamicArraySetEmpty(swOptionValueTypeSizeGet(swOptionValueTypeString));
            rtn = commandLineDataNew;
          }
        }
      }
    }
    if (!rtn)
      swCommandLineDataDelete(commandLineDataNew);
  }
  return rtn;
}

// main options category is not mandatory
bool swCommandLineDataSetCategories(swCommandLineData *commandLineData, swOptionCategory *globalCategory)
{
  bool rtn = false;
  if (commandLineData)
  {
    swOptionCategory *categoryBegin = globalCategory;
    swOptionCategory *categoryEnd = globalCategory;

    // find begin and end of section by comparing magics
    while (1)
    {
      swOptionCategory *current = categoryBegin - 1;
      if (current->magic != SW_COMMANDLINE_MAGIC)
        break;
      categoryBegin--;
    }
    while (1)
    {
      categoryEnd++;
      if (categoryEnd->magic != SW_COMMANDLINE_MAGIC)
        break;
    }

    swOptionCategory *category = categoryBegin;
    for (; category != categoryEnd; category++)
    {
      if (category->type == swOptionCategoryTypeMain)
      {
        // no more than one main category
        if (!commandLineData->mainCategory)
          commandLineData->mainCategory = category;
        else
        {
          swCommandLineErrorDataSet(&(commandLineData->errorData), NULL, category, swCommandLineErrorCodeMainCategory);
          break;
        }
      }
      swFastArrayPush(commandLineData->categories, category);
    }
    if (category == categoryEnd)
      rtn = true;
  }
  return rtn;
}

static bool swCommandLineDataSetOptionAliases(swCommandLineData *commandLineData, swOption *option)
{
  bool rtn = false;
  if (commandLineData && option)
  {
    uint32_t foundCount = 0;
    if (swStaticStringCountChar(&(option->name), '|', &foundCount))
    {
      if (foundCount)
      {
        if (swDynamicArrayInit(&(option->aliases), sizeof(swStaticString), (foundCount + 1)))
        {
          swStaticString *aliases = (swStaticString *)option->aliases.data;
          foundCount = 0;
          if (swStaticStringSplitChar(&(option->name), '|', aliases, option->aliases.size, &foundCount, 0) && (foundCount == option->aliases.size))
          {
            option->aliases.count = foundCount;
            rtn = true;
          }
        }
      }
      else
        rtn = true;
    }
    if (!rtn)
      swCommandLineErrorDataSet(&(commandLineData->errorData), option, NULL, swCommandLineErrorCodeInternal);
  }
  return rtn;
}

static bool swCommandLineDataProcessOptions(swCommandLineData *commandLineData)
{
  bool rtn = false;
  if (commandLineData)
  {
    swOptionValuePair *valuePairs = (swOptionValuePair *)(commandLineData->normalValues.storage);
    if (valuePairs)
    {
      uint32_t i = 0;
      while (i < commandLineData->normalValues.count)
      {
        swOption *option = valuePairs[i].option;

        uint32_t optionNamesCount = 1;
        swStaticString *optionNames = &(option->name);
        if (option->aliases.count)
        {
          optionNamesCount = option->aliases.count;
          optionNames = (swStaticString *)(option->aliases.data);
        }
        uint32_t j = 0;
        while (j < optionNamesCount)
        {
          if (swHashMapLinearInsert(commandLineData->namedValues, &(optionNames[j]), &(valuePairs[i])))
          {
            if (!option->isPrefix || swHashMapLinearInsert(commandLineData->prefixedValues, &(optionNames[j]), &(valuePairs[i])))
            {
              j++;
              continue;
            }
          }
          break;
        }
        if (j == optionNamesCount)
        {
          i++;
          continue;
        }
        swCommandLineErrorDataSet(&(commandLineData->errorData), option, NULL, swCommandLineErrorCodeInternal);
        break;
      }
      if (i == commandLineData->normalValues.count)
        rtn = true;
    }
  }
  return rtn;
}

static bool swCommandLineDataValidateOption(swCommandLineData *commandLineData, swOption *option, bool isMainCategory)
{
  bool rtn = false;
  if (commandLineData && option && (!option->isPositional || isMainCategory))
  {
    // validate type
    bool typeValid = false;
    swOptionValuePair *valuePairPtr = NULL;
    if (option->isPositional)
    {
      if (!option->name.len && (!option->isArray || option->arrayType == swOptionArrayTypeCommaSeparated))
      {
        swOptionValuePair valuePair = swOptionValuePairInit(option);
        if (swFastArrayPush(commandLineData->positionalValues, valuePair))
        {
          valuePairPtr = swFastArrayGetExistingPtr(commandLineData->positionalValues, (commandLineData->positionalValues.count - 1), swOptionValuePair);
          typeValid = true;
        }
      }
    }
    else
    {
      if (option->name.len)
      {
        swOptionValuePair valuePair = swOptionValuePairInit(option);
        if (swFastArrayPush(commandLineData->normalValues, valuePair))
        {
          valuePairPtr = swFastArrayGetExistingPtr(commandLineData->normalValues, (commandLineData->normalValues.count - 1), swOptionValuePair);
          typeValid = true;
        }
      }
    }
    if (typeValid)
    {
      if (option->valueType > swOptionValueTypeNone && option->valueType < swOptionValueTypeMax)
      {
        // validate array type
        if ((!option->valueCount || option->isArray) &&
          (option->isArray || (option->arrayType != swOptionArrayTypeMultiValue && option->arrayType != swOptionArrayTypeCommaSeparated)))
        {
          // validate modifier
          if (!option->isPrefix || (!option->isPositional && (!option->isArray || option->arrayType == swOptionArrayTypeSimple)))
          {
            // validate default value
            if (swOptionValidateDefaultValue(option))
            {
              if (!option->isRequired || swFastArrayPush(commandLineData->requiredValues, valuePairPtr))
                rtn = true;
              else
                swCommandLineErrorDataSet(&(commandLineData->errorData), option, NULL, swCommandLineErrorCodeInternal);
            }
            else
              swCommandLineErrorDataSet(&(commandLineData->errorData), option, NULL, swCommandLineErrorCodeInvalidDefault);
          }
          else
            swCommandLineErrorDataSet(&(commandLineData->errorData), option, NULL, swCommandLineErrorCodePrefixOption);
        }
        else
          swCommandLineErrorDataSet(&(commandLineData->errorData), option, NULL, swCommandLineErrorCodeArrayType);
      }
      else
        swCommandLineErrorDataSet(&(commandLineData->errorData), option, NULL, swCommandLineErrorCodeValueType);
    }
    else
      swCommandLineErrorDataSet(&(commandLineData->errorData), option, NULL, swCommandLineErrorCodeOptionType);
  }
  return rtn;
}

bool swCommandLineDataSetOptions(swCommandLineData *commandLineData)
{
  bool rtn = false;
  if (commandLineData && swFastArrayCount(commandLineData->categories))
  {
    uint32_t i = 0;
    for (; i < swFastArrayCount(commandLineData->categories); i++)
    {
      swOptionCategory *category = NULL;
      if (swFastArrayGet(commandLineData->categories, i, category))
      {
        swOption *option = category->options;
        while (option->valueType > swOptionValueTypeNone)
        {
          if ((option->isPositional || swCommandLineDataSetOptionAliases(commandLineData, option)) && swCommandLineDataValidateOption(commandLineData, option, category->type))
            option++;
          else
            break;
        }
        if (option->name.len)
          break;
      }
      else
      {
        commandLineData->errorData.code = swCommandLineErrorCodeInternal;
        break;
      }
    }
    if (i == swFastArrayCount(commandLineData->categories))
    {
      if (swCommandLineDataProcessOptions(commandLineData))
        rtn = true;
    }
  }
  return rtn;
}

static bool _setDefaultsAndCheckArrays(swOptionValuePair *pair, swCommandLineErrorData *errorData)
{
  bool rtn = false;
  if (swOptionValuePairSetDefaults(pair))
  {
    if (swOptionValuePairCheckArrays(pair))
      rtn = true;
    else
      swCommandLineErrorDataSet(errorData, pair->option, NULL, swCommandLineErrorCodeArrayValueCount);
  }
  else
    swCommandLineErrorDataSet(errorData, pair->option, NULL, swCommandLineErrorCodeInternal);
  return rtn;
}

static bool _setExternalValue(swOptionValuePair *pair, swCommandLineErrorData *errorData)
{
  return swOptionValuePairSetExternal(pair);
}

static bool swCommandLineDataWalkPairs(swCommandLineData *commandLineData, bool (*pairFunction)(swOptionValuePair *, swCommandLineErrorData *errorData))
{
  bool rtn = false;
  if (commandLineData && pairFunction)
  {
    swFastArray *valuePairsList[] = {&commandLineData->normalValues, &commandLineData->positionalValues, NULL};
    swFastArray **valuePairsListPtr = valuePairsList;
    while (*valuePairsListPtr)
    {
      swOptionValuePair *valuePairPtr = (swOptionValuePair *)(*valuePairsListPtr)->storage;
      uint32_t i = 0;
      for (; i < (*valuePairsListPtr)->count; i++)
      {
        if (!pairFunction(valuePairPtr, &(commandLineData->errorData)))
          break;
        valuePairPtr++;
      }
      if (i < (*valuePairsListPtr)->count)
        break;
      valuePairsListPtr++;
    }
    if (!(*valuePairsListPtr))
      rtn = true;
  }
  return rtn;
}

static bool swCommandLineDataCheckRequired(swCommandLineData *commandLineData)
{
  bool rtn = false;
  if (commandLineData)
  {
    if (commandLineData->requiredValues.count)
    {
      swOptionValuePair **valuePairs = (swOptionValuePair **)commandLineData->requiredValues.storage;
      uint32_t i = 0;
      while (i < commandLineData->requiredValues.count)
      {
        if (!valuePairs[i]->value.count)
        {
          swCommandLineErrorDataSet(&(commandLineData->errorData), valuePairs[i]->option, NULL, swCommandLineErrorCodeRequiredValue);
          break;
        }
        i++;
      }
      if (i == commandLineData->requiredValues.count)
        rtn = true;
    }
    else
      rtn = true;
  }
  return rtn;
}

bool swCommandLineDataSetValues(swCommandLineData *commandLineData, int argc, const char *argv[])
{
  bool rtn = false;
  if (commandLineData && argc && argv)
  {
    uint32_t argumentCount = (uint32_t)argc;
    swOptionToken tokens[argumentCount];
    memset(tokens, 0, sizeof(tokens));

    swCommandLineState state = {.clData = commandLineData, .argv = argv, .tokens = &tokens[0], .argCount = argumentCount};
    if (swCommandLineStateTokenize(&state))
    {
      state.currentArg = 0;
      while (state.currentArg < state.argCount)
      {
        if (!swCommandLineStateScanArguments(&state))
          break;
      }
      if (state.currentArg == state.argCount)
      {
        // check all options that are not set and set them to default values
        if (swCommandLineDataWalkPairs(commandLineData, _setDefaultsAndCheckArrays) && swCommandLineDataCheckRequired(commandLineData))
          rtn = swCommandLineDataWalkPairs(commandLineData, _setExternalValue);
      }
    }
  }
  return rtn;
}
