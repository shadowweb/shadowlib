#include "command-line/command-line.h"
#include "command-line/command-line-error.h"
#include "command-line/option-token.h"
#include "command-line/option-value-pair.h"

#include "collections/dynamic-array.h"
#include "collections/fast-array.h"
#include "collections/hash-map-linear.h"
#include "collections/hash-set-linear.h"
#include "core/memory.h"
#include "storage/dynamic-string.h"
#include "utils/file.h"

static void swOptionValuePairArrayClear(swFastArray *array)
{
  if (array && array->size)
  {
    for (uint32_t i = 0; i < array->count; i++)
      swOptionValuePairClear(swFastArrayGetExistingPtr(*array, i, swOptionValuePair));
    swFastArrayClear(array);
  }
}

typedef struct swCommandLineData
{
  swFastArray         categories;
  swOptionCategory   *mainCategory;

  swFastArray         normalValues;       // normal options
  swFastArray         positionalValues;   // positional options
  swOptionValuePair   sinkValue;          // sink option
  swOptionValuePair   consumeAfterValue;  // consume after

  swHashMapLinear    *namedValues;        // hash of swOption:name => swOptionValuePair
  swFastArray         requiredValues;     // array of pointers to required swOptionValuePair
  swHashMapLinear    *prefixedValues;     // set of prefixed swOptionValuePair
  swHashMapLinear    *groupingValues;     // set of grouping swOption
  swDynamicString     programName;
  swDynamicString     argumentsString;
  swDynamicString     usageMessage;
  swCommandLineErrorData  errorData;
} swCommandLineData;

typedef struct swCommandLineState
{
  swCommandLineData    *clData;
  const char          **argv;
  swOptionToken        *tokens;
  uint32_t              argCount;
  uint32_t              currentArg;
  uint32_t              currentPositional;
} swCommandLineState;

swOptionCategoryModuleDeclare(optionCategoryGlobal, "CommandLine",
  swOptionDeclareScalar("help", "Print command line help", NULL, swOptionValueTypeBool, false)
);

static swCommandLineData *commandLineDataGlobal = NULL;

static void swCommandLineDataDelete(swCommandLineData *commandLineData)
{
  if (commandLineData)
  {
    if (commandLineData->groupingValues)
      swHashMapLinearDelete(commandLineData->groupingValues);
    if (commandLineData->prefixedValues)
      swHashMapLinearDelete(commandLineData->prefixedValues);
    if (commandLineData->requiredValues.size)
      swFastArrayClear(&(commandLineData->requiredValues));
    if (commandLineData->namedValues)
      swHashMapLinearDelete(commandLineData->namedValues);

    swOptionValuePairClear(&(commandLineData->consumeAfterValue));
    swOptionValuePairClear(&(commandLineData->sinkValue));
    swOptionValuePairArrayClear(&(commandLineData->positionalValues));
    swOptionValuePairArrayClear(&(commandLineData->normalValues));
    if (commandLineData->categories.size)
      swFastArrayClear(&(commandLineData->categories));

    swDynamicStringRelease(&(commandLineData->programName));
    swDynamicStringRelease(&(commandLineData->argumentsString));
    swDynamicStringRelease(&(commandLineData->usageMessage));
    swMemoryFree(commandLineData);
  }
}

static swCommandLineData *swCommandLineDataNew(uint32_t argumentCount)
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
            (commandLineDataNew->prefixedValues = swHashMapLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL)) &&
            (commandLineDataNew->groupingValues = swHashMapLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL)))
        {
          // defaults to pointer comparison
          if (swFastArrayInit(&(commandLineDataNew->requiredValues), sizeof(swOptionValuePair), argumentCount))
            rtn = commandLineDataNew;
        }
      }
    }
    if (!rtn)
      swCommandLineDataDelete(commandLineDataNew);
  }
  return rtn;
}

// main options category is not mandatory
static bool swOptionCommandLineSetCategories(swCommandLineData *commandLineData)
{
  bool rtn = false;
  if (commandLineData)
  {
    swOptionCategory *categoryBegin = (swOptionCategory *)(&optionCategoryGlobal);
    swOptionCategory *categoryEnd = (swOptionCategory *)(&optionCategoryGlobal);

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

static bool swOptionCommandLineProcessOptions(swCommandLineData *commandLineData)
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
        if (swHashMapLinearInsert(commandLineData->namedValues, &(option->name), &(valuePairs[i])))
        {
          if (option->modifier != swOptionModifierPrefix || swHashMapLinearInsert(commandLineData->prefixedValues, &(option->name), &(valuePairs[i])))
          {
            if (option->modifier != swOptionModifierGrouping || swHashMapLinearInsert(commandLineData->groupingValues, &(option->name), &(valuePairs[i])))
            {
              i++;
              continue;
            }
          }
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

static bool swOptionCommandLineValidateOption(swCommandLineData *commandLineData, swOption *option, bool isMainCategory)
{
  bool rtn = false;
  if (commandLineData && option && (option->optionType == swOptionTypeNormal || isMainCategory))
  {
    // validate type
    bool typeValid = false;
    swOptionValuePair *valuePairPtr = NULL;
    switch (option->optionType)
    {
      case swOptionTypeNormal:
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
        break;
      }
      case swOptionTypePositional:
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
        break;
      }
      case swOptionTypeConsumeAfter:
        if (!commandLineData->consumeAfterValue.option)
        {
          // TODO: verify that it is array of strings; is it feasible to have anything else?
          if (!option->name.len && option->isArray)
          {
            commandLineData->consumeAfterValue = swOptionValuePairSet(option);
            valuePairPtr = &(commandLineData->consumeAfterValue);
            typeValid = true;
          }
        }
        break;
      case swOptionTypeSink:
        if (!commandLineData->sinkValue.option)
        {
          // TODO: verify that it is array of strings; is it feasible to have anything else?
          if (!option->name.len && option->isArray)
          {
            commandLineData->sinkValue = swOptionValuePairSet(option);
            valuePairPtr = &(commandLineData->sinkValue);
            typeValid = true;
          }
        }
        break;
    }
    if (typeValid)
    {
      if (option->valueType > swOptionValueTypeNone && option->valueType < swOptionValueTypeMax)
      {
        // validate array type
        if (option->isArray || (option->arrayType != swOptionArrayTypeMultiValue && option->arrayType != swOptionArrayTypeCommaSeparated))
        {
          // validate modifier
          bool modifierValid = false;
          switch (option->modifier)
          {
            case swOptionModifierNone:
              modifierValid = true;
              break;
            case swOptionModifierPrefix:
              if (option->optionType == swOptionTypeNormal && (!option->isArray || (option->arrayType != swOptionArrayTypeMultiValue && option->arrayType != swOptionArrayTypeCommaSeparated)))
                modifierValid = true;
              break;
            case swOptionModifierGrouping:
              if (option->optionType == swOptionTypeNormal && !option->isArray && (option->name.len == 1) && option->valueType == swOptionValueTypeBool)
                modifierValid = true;
              break;
          }
          if (modifierValid)
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
            swCommandLineErrorDataSet(&(commandLineData->errorData), option, NULL, swCommandLineErrorCodeModifierType);
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

static bool swOptionCommandLineSetOptions(swCommandLineData *commandLineData)
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
          if (swOptionCommandLineValidateOption(commandLineData, option, category->type))
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
      if (swOptionCommandLineProcessOptions(commandLineData))
        rtn = true;
    }
  }
  return rtn;
}

// Possible formats:
// --optiion=value
// --option value
// -option=value
// -option value
// -option
// -[option]value -- option that prefixes its value

// Boolean option
// --option - set to true
// --option true - set to true
// --option=true - set to true
// --option TRUE - set to true
// --option=TRUE - set to true
// --nooption - set to false
// --option false - set to false
// --option=false - set to false
// --option FALSE - set to false
// --option=FALSE - set to false

// Array option
// -option=value1 -option=value2 ... - simple array
// -option value1 -option value2 ... - simple array
// -option=value1,value2,...  - should have comma separated array type
// -option value1 value2 value3 ..  - should have multivalue array type

// Positional (do not have - or --)
// option option ...    - accessed by position in the array of positional arguments
// after all positional arguments are identified, the rest goes to "consume after array"
// "-- " - starts positional arguments, no options after that

// sink argument collects all unknown options
// --option
// -option
// --option=value
// -option=value

static bool swOptionCommandLineScanConsumeAfter(swCommandLineState *state)
{
  bool rtn = true;
  swOptionValuePair *pair = &(state->clData->consumeAfterValue);
  while (state->currentArg < state->argCount)
  {
    swOptionToken *token = &state->tokens[state->currentArg];
    swOption *option = pair->option;
    if (swOptionCallParser(option, &(token->full), &(pair->value)))
    {
      state->currentArg++;
      continue;
    }
    swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeParse);
    rtn = false;
    break;
  }
  return rtn;
}

static bool swOptionCommandLineScanPositional(swCommandLineState *state)
{
  bool rtn = false;
  if ((state->currentPositional < state->clData->positionalValues.count) && (state->currentArg < state->argCount))
  {
    swOptionValuePair *pair = swFastArrayGetPtr(state->clData->positionalValues, state->currentPositional, swOptionValuePair);
    if (pair)
    {
      swOptionToken *token = &state->tokens[state->currentArg];
      swOption *option = pair->option;
      if (swOptionCallParser(option, &(token->full), &(pair->value)))
      {
        state->currentArg++;
        state->currentPositional++;
        rtn = true;
      }
      else
        swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeParse);
    }
    else
      swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
  }
  return rtn;
}

static bool swOptionCommandLineScanAllPositional(swCommandLineState *state)
{
  bool rtn = false;
  if (state)
  {
    if (state->currentArg < state->argCount)
    {
      bool failure = false;
      while ((state->currentPositional < state->clData->positionalValues.count) && (state->currentArg < state->argCount))
      {
        if (swOptionCommandLineScanPositional(state))
          continue;
        failure = true;
        break;
      }
      if (!failure)
      {
        if (state->currentArg == state->argCount)
          rtn = true;
        else if (state->currentPositional == state->clData->positionalValues.count)
        {
          if (state->clData->consumeAfterValue.option)
            rtn = swOptionCommandLineScanConsumeAfter(state);
          else
            swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeNoConsumeAfter);
        }
      }
      else if (!state->clData->positionalValues.count)
        swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeNoPositional);
    }
    else
      rtn = true;
  }
  return rtn;
}

static bool swOptionCommandLineScanSink(swCommandLineState *state)
{
  bool rtn = false;
  swOptionToken *token = &state->tokens[state->currentArg];
  swOption *option = state->clData->sinkValue.option;
  if (option)
  {
    if (swOptionCallParser(option, &(token->full), &(state->clData->sinkValue.value)))
    {
      state->currentArg++;
      rtn = true;
    }
    else
      swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeParse);
  }
  else
    swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeNoSink);
  return rtn;
}

static bool swOptionCommandLineScanArguments(swCommandLineState *state)
{
  bool rtn = false;
  if (state)
  {
    swOptionToken *token = &state->tokens[state->currentArg];
    if (token->namePair || token->noNamePair)
    {
      // process normal (modifier: none || prefix)
      if (token->namePair)
      {
        bool isMultiValueArray = token->namePair->option->isArray && token->namePair->option->arrayType == swOptionArrayTypeMultiValue;
        if (token->hasValue)
        {
          swOption *option = token->namePair->option;
          if (!isMultiValueArray)
          {
            if (swOptionCallParser(option, &(token->value), &(token->namePair->value)))
            {
              state->currentArg++;
              rtn = true;
            }
            else
              swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeParse);
          }
          else
            swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeArrayMultivalue);
        }
        else
        {
          swOptionToken *nextToken = ((state->currentArg + 1) < state->argCount)? &state->tokens[state->currentArg + 1] : NULL;
          if (nextToken && !nextToken->hasName && nextToken->hasValue)
          {
            state->currentArg++;
            do
            {
              swOption *option = token->namePair->option;
              if (swOptionCallParser(option, &(nextToken->value), &(token->namePair->value)))
              {
                state->currentArg++;
                rtn = true;
                if (isMultiValueArray)
                {
                  nextToken = (state->currentArg < state->argCount)? &state->tokens[state->currentArg] : NULL;
                  continue;
                }
              }
              else
              {
                swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeParse);
                rtn = false;
              }
              break;
            }
            while(nextToken && !nextToken->hasName && nextToken->hasValue);
          }
          else if (token->namePair->option->valueType == swOptionValueTypeBool && token->namePair->option->arrayType == swOptionArrayTypeSimple)
          {
            if (swOptionValuePairValueSet(&(token->namePair->value), trueValue, token->namePair->option->isArray))
            {
              state->currentArg++;
              rtn = true;
            }
            else
              swCommandLineErrorDataSet(&(state->clData->errorData), token->namePair->option, NULL, swCommandLineErrorCodeInternal);
          }
        }
      }
      else if (token->noNamePair->option->valueType == swOptionValueTypeBool && token->noNamePair->option->arrayType == swOptionArrayTypeSimple)
      {
        if (swOptionValuePairValueSet(&(token->noNamePair->value), falseValue, token->noNamePair->option->isArray))
        {
          state->currentArg++;
          rtn = true;
        }
        else
          swCommandLineErrorDataSet(&(state->clData->errorData), token->noNamePair->option, NULL, swCommandLineErrorCodeInternal);
      }
    }
    else if (token->hasName)
    {
      // process normal (modifier: grouping)
      swOptionValuePair *pairs[token->name.len];
      memset(pairs, 0, sizeof(pairs));
      swStaticString nameSubstring = swStaticStringDefineEmpty;
      size_t position = 0;
      bool failure = false;
      while (position < token->name.len)
      {
        if (swStaticStringSetSubstring(&(token->name), &nameSubstring, position, position + 1))
        {
          if (!swHashMapLinearValueGet(state->clData->groupingValues, &nameSubstring, (void **)(&pairs[position])))
            break;
        }
        else // substring failure
        {
          swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
          failure = true;
          break;
        }
        position++;
      }
      if (position == token->name.len)
      {
        position = 0;
        while (position < token->name.len)
        {
          if (!swOptionValuePairValueSet(&(pairs[position]->value), trueValue, pairs[position]->option->isArray))
          {
            swCommandLineErrorDataSet(&(state->clData->errorData), pairs[position]->option, NULL, swCommandLineErrorCodeInternal);
            break;
          }
          position++;
        }
        if (position == token->name.len)
        {
          state->currentArg++;
          rtn = true;
        }
      }
      else if (!failure)
      {
        if (state->clData->sinkValue.option)
          rtn = swOptionCommandLineScanSink(state);
      }
    }
    else // token does not have name
    {
      if (token->hasDashDashOnly)
      {
        state->currentArg++;
        // consume positional only
        rtn = swOptionCommandLineScanAllPositional(state);
      }
      else if (state->currentPositional < state->clData->positionalValues.count)
        rtn = swOptionCommandLineScanPositional(state);
      else if (state->clData->positionalValues.count && state->clData->consumeAfterValue.option)
        rtn = swOptionCommandLineScanConsumeAfter(state);
      else
        rtn = swOptionCommandLineScanSink(state);
    }
  }
  return rtn;
}

static bool swCommandLineOptionsTokenize(swCommandLineState *state)
{
  bool rtn = false;
  if (state)
  {
    uint32_t i = 0;
    for (; i < state->argCount; i++)
    {
      if (!swOptionTokenSet(&(state->tokens[i]), state->argv[i], state->clData->namedValues, state->clData->prefixedValues, &(state->clData->errorData)))
        break;
    }
    if (i == state->argCount)
      rtn = true;
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

static bool swOptionCommandLineWalkPairs(swCommandLineData *commandLineData, bool (*pairFunction)(swOptionValuePair *, swCommandLineErrorData *errorData))
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
    {
      swOptionValuePair *valuePairs[] = {&commandLineData->sinkValue, &commandLineData->consumeAfterValue, NULL};
      swOptionValuePair **valuePairsPtr = valuePairs;
      while (*valuePairsPtr)
      {
        if (!pairFunction(*valuePairsPtr, &(commandLineData->errorData)))
          break;
        valuePairsPtr++;
      }
      if (!(*valuePairsPtr))
        rtn = true;
    }
  }
  return rtn;
}

static bool swOptionCommandLineCheckRequired(swCommandLineData *commandLineData)
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

static bool swOptionCommandLineSetValues(swCommandLineData *commandLineData, int argc, const char *argv[])
{
  bool rtn = false;
  if (commandLineData && argc && argv)
  {
    uint32_t argumentCount = (uint32_t)argc;
    swOptionToken tokens[argumentCount];
    memset(tokens, 0, sizeof(tokens));

    swCommandLineState state = {.clData = commandLineData, .argv = argv, .tokens = &tokens[0], .argCount = argumentCount};
    if (swCommandLineOptionsTokenize(&state))
    {
      state.currentArg = 0;
      while (state.currentArg < state.argCount)
      {
        if (!swOptionCommandLineScanArguments(&state))
          break;
      }
      if (state.currentArg == state.argCount)
      {
        // check all options that are not set and set them to default values
        if (swOptionCommandLineWalkPairs(commandLineData, _setDefaultsAndCheckArrays))
          rtn = swOptionCommandLineCheckRequired(commandLineData);
      }
    }
  }
  return rtn;
}

bool swOptionCommandLineInit(int argc, const char *argv[], const char *usageMessage, swDynamicString **errorString)
{
  bool rtn = false;
  if (!commandLineDataGlobal && (argc > 0) && argv)
  {
    // init command line options global
    if ((commandLineDataGlobal = swCommandLineDataNew(argc - 1)))
    {
      // collect all command lines option categories
      // walk through all categories and collect all command line options and required options
      if (swOptionCommandLineSetCategories(commandLineDataGlobal) && swOptionCommandLineSetOptions(commandLineDataGlobal))
      {
        // set programName
        if (swFileRealPath(argv[0], &(commandLineDataGlobal->programName)))
        {
          // walk through the arguments, argumentsString, and optionValues
          if ((argc == 1) || swOptionCommandLineSetValues(commandLineDataGlobal, argc - 1, &argv[1]))
          {
            if (!usageMessage || swDynamicStringSetFromCString(&(commandLineDataGlobal->usageMessage), usageMessage))
              rtn = true;
          }
        }
        else
          swCommandLineErrorDataSet(&(commandLineDataGlobal->errorData), NULL, NULL, swCommandLineErrorCodeRealPath);
      }
      if (!rtn)
      {
        swCommandLineErrorSet(&(commandLineDataGlobal->errorData), swCommandLineErrorCodeNone, errorString);
        swCommandLineDataDelete(commandLineDataGlobal);
        commandLineDataGlobal = NULL;
      }
    }
    else
      swCommandLineErrorSet(NULL, swCommandLineErrorCodeInternal, errorString);
  }
  else
    swCommandLineErrorSet(NULL,swCommandLineErrorCodeInvalidInput, errorString);
  return rtn;
}

void swOptionCommandLineShutdown()
{
  if (commandLineDataGlobal)
  {
    swCommandLineDataDelete(commandLineDataGlobal);
    commandLineDataGlobal = NULL;
  }
}

void swOptionCommandLinePrintUsage()
{
}

static void *swOptionValueGetInternal(swStaticString *name, swOptionValueType type)
{
  if (commandLineDataGlobal && name)
  {
    swOptionValuePair *pair = NULL;
    if (swHashMapLinearValueGet(commandLineDataGlobal->namedValues, name, (void **)&pair))
    {
      if (pair->option->valueType == type && !pair->option->isArray && pair->value.count)
        return pair->value.data;
    }
  }
  return NULL;
}

#define swOptionValueGetImplement(n, v, vt) \
{ \
  bool rtn = false; \
  if (v) \
  { \
    typeof(v) sv = (typeof(v))swOptionValueGetInternal(n, vt); \
    if (sv) \
    { \
      *(v) = *sv; \
      rtn = true; \
    } \
  } \
  return rtn; \
}

bool swOptionValueGetBool(swStaticString *name, bool *value)
{
  swOptionValueGetImplement(name, value, swOptionValueTypeBool);
}

bool swOptionValueGetInt(swStaticString *name, int64_t *value)
{
  swOptionValueGetImplement(name, value, swOptionValueTypeInt);
}

bool swOptionValueGetDouble(swStaticString *name, double *value)
{
  swOptionValueGetImplement(name, value, swOptionValueTypeDouble);
}

bool swOptionValueGetString(swStaticString *name, swStaticString *value)
{
  swOptionValueGetImplement(name, value, swOptionValueTypeString);
}

static swStaticArray *swOptionValueGetArrayInternal(swStaticString *name, swOptionValueType type)
{
  if (commandLineDataGlobal && name)
  {
    swOptionValuePair *pair = NULL;
    if (swHashMapLinearValueGet(commandLineDataGlobal->namedValues, name, (void **)&pair))
    {
      if (pair->option->valueType == type && pair->option->isArray)
        return (swStaticArray *)(&(pair->value));
    }
  }
  return NULL;
}

#define swOptionValueGetArrayImplement(n, v, vt) \
{ \
  bool rtn = false; \
  if (v) \
  { \
    swStaticArray *sv = swOptionValueGetArrayInternal(n, vt); \
    if (sv) \
    { \
      *(v) = *sv; \
      rtn = true; \
    } \
  } \
  return rtn; \
}

bool swOptionValueGetBoolArray(swStaticString *name, swStaticArray *value)
{
  swOptionValueGetArrayImplement(name, value, swOptionValueTypeBool);
}

bool swOptionValueGetIntArray(swStaticString *name, swStaticArray *value)
{
  swOptionValueGetArrayImplement(name, value, swOptionValueTypeInt);
}

bool swOptionValueGetDoubleArray(swStaticString *name, swStaticArray *value)
{
  swOptionValueGetArrayImplement(name, value, swOptionValueTypeDouble);
}

bool swOptionValueGetStringArray(swStaticString *name, swStaticArray *value)
{
  swOptionValueGetArrayImplement(name, value, swOptionValueTypeString);
}

static void *swPositionalOptionValueGetInternal(uint32_t position, swOptionValueType type)
{
  if (commandLineDataGlobal)
  {
    swOptionValuePair *pair = swFastArrayGetExistingPtr(commandLineDataGlobal->positionalValues, position, swOptionValuePair);
    if (pair)
    {
      if (pair->option->valueType == type && !pair->option->isArray && pair->value.count)
        return pair->value.data;
    }
  }
  return NULL;
}

#define swPositionalOptionValueGetImplement(p, v, vt) \
{ \
  bool rtn = false; \
  if (v) \
  { \
    typeof(v) sv = (typeof(v))swPositionalOptionValueGetInternal(p, vt); \
    if (sv) \
    { \
      *(v) = *sv; \
      rtn = true; \
    } \
  } \
  return rtn; \
}

bool swPositionalOptionValueGetBool(uint32_t position, bool *value)
{
  swPositionalOptionValueGetImplement(position, value, swOptionValueTypeBool);
}

bool swPositionalOptionValueGetInt(uint32_t position, int64_t *value)
{
  swPositionalOptionValueGetImplement(position, value, swOptionValueTypeInt);
}

bool swPositionalOptionValueGetDouble (uint32_t position, double *value)
{
  swPositionalOptionValueGetImplement(position, value, swOptionValueTypeDouble);
}

bool swPositionalOptionValueGetString (uint32_t position, swStaticString *value)
{
  swPositionalOptionValueGetImplement(position, value, swOptionValueTypeString);
}

static swStaticArray *swPositionalOptionValueGetArrayInternal(uint32_t position, swOptionValueType type)
{
  if (commandLineDataGlobal)
  {
    swOptionValuePair *pair = swFastArrayGetExistingPtr(commandLineDataGlobal->positionalValues, position, swOptionValuePair);
    if (pair)
    {
      if (pair->option->valueType == type && pair->option->isArray)
        return (swStaticArray *)(&(pair->value));
    }
  }
  return NULL;
}

#define swPositionalOptionValueGetArrayImplement(p, v, vt) \
{ \
  bool rtn = false; \
  if (v) \
  { \
    swStaticArray *sv = swPositionalOptionValueGetArrayInternal(p, vt); \
    if (sv) \
    { \
      *(v) = *sv; \
      rtn = true; \
    } \
  } \
  return rtn; \
}

bool swPositionalOptionValueGetBoolArray(uint32_t position, swStaticArray *value)
{
  swPositionalOptionValueGetArrayImplement(position, value, swOptionValueTypeBool);
}

bool swPositionalOptionValueGetIntArray(uint32_t position, swStaticArray *value)
{
  swPositionalOptionValueGetArrayImplement(position, value, swOptionValueTypeInt);
}

bool swPositionalOptionValueGetDoubleArray(uint32_t position, swStaticArray *value)
{
  swPositionalOptionValueGetArrayImplement(position, value, swOptionValueTypeDouble);
}

bool swPositionalOptionValueGetStringArray(uint32_t position, swStaticArray *value)
{
  swPositionalOptionValueGetArrayImplement(position, value, swOptionValueTypeString);
}

static swStaticArray *swSinkOptionValueGetArrayInternal(swOptionValueType type)
{
  if (commandLineDataGlobal && commandLineDataGlobal->sinkValue.option)
  {
    if (commandLineDataGlobal->sinkValue.option->valueType == type)
      return (swStaticArray *)(&(commandLineDataGlobal->sinkValue.value));
  }
  return NULL;
}

#define swSinkOptionValueGetArrayImplement(v, vt) \
{ \
  bool rtn = false; \
  if (v) \
  { \
    swStaticArray *sv = swSinkOptionValueGetArrayInternal(vt); \
    if (sv) \
    { \
      *(v) = *sv; \
      rtn = true; \
    } \
  } \
  return rtn; \
}

bool swSinkOptionValueGetBoolArray(swStaticArray *value)
{
  swSinkOptionValueGetArrayImplement(value, swOptionValueTypeBool);
}

bool swSinkOptionValueGetIntArray(swStaticArray *value)
{
  swSinkOptionValueGetArrayImplement(value, swOptionValueTypeInt);
}

bool swSinkOptionValueGetDoubleArray(swStaticArray *value)
{
  swSinkOptionValueGetArrayImplement(value, swOptionValueTypeDouble);
}

bool swSinkOptionValueGetStringArray(swStaticArray *value)
{
  swSinkOptionValueGetArrayImplement(value, swOptionValueTypeString);
}

static swStaticArray *swConsumeAfterOptionValueGetArrayInternal(swOptionValueType type)
{
  if (commandLineDataGlobal && commandLineDataGlobal->consumeAfterValue.option)
  {
    if (commandLineDataGlobal->consumeAfterValue.option->valueType == type)
      return (swStaticArray *)(&(commandLineDataGlobal->consumeAfterValue.value));
  }
  return NULL;
}

#define swConsumeAfterOptionValueGetArrayImplement(v, vt) \
{ \
  bool rtn = false; \
  if (v) \
  { \
    swStaticArray *sv = swConsumeAfterOptionValueGetArrayInternal(vt); \
    if (sv) \
    { \
      *(v) = *sv; \
      rtn = true; \
    } \
  } \
  return rtn; \
}

bool swConsumeAfterOptionValueGetBoolArray(swStaticArray *value)
{
  swConsumeAfterOptionValueGetArrayImplement(value, swOptionValueTypeBool);
}

bool swConsumeAfterOptionValueGetIntArray(swStaticArray *value)
{
  swConsumeAfterOptionValueGetArrayImplement(value, swOptionValueTypeInt);
}

bool swConsumeAfterOptionValueGetDoubleArray(swStaticArray *value)
{
  swConsumeAfterOptionValueGetArrayImplement(value, swOptionValueTypeDouble);
}

bool swConsumeAfterOptionValueGetStringArray(swStaticArray *value)
{
  swConsumeAfterOptionValueGetArrayImplement(value, swOptionValueTypeString);
}
