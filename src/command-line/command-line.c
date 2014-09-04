#include "command-line/command-line.h"
#include "command-line/command-line-error.h"

#include "collections/dynamic-array.h"
#include "collections/fast-array.h"
#include "collections/hash-map-linear.h"
#include "collections/hash-set-linear.h"
#include "core/memory.h"
#include "storage/dynamic-string.h"
#include "utils/file.h"

#include <errno.h>
#include <math.h>

typedef struct swOptionValuePair
{
  swOption      *option;
  swDynamicArray value;
} swOptionValuePair;

static void swOptionValuePairClear(swOptionValuePair *pair)
{
  if (pair && pair->value.size)
    swDynamicArrayRelease(&(pair->value));
}

static void swOptionValuePairArrayClear(swFastArray *array)
{
  if (array && array->size)
  {
    for (uint32_t i = 0; i < array->count; i++)
      swOptionValuePairClear(swFastArrayGetExistingPtr(*array, i, swOptionValuePair));
    swFastArrayClear(array);
  }
}

typedef struct swOptionToken
{
  const char *argv;
  swStaticString name;
  swOptionValuePair *namePair;
  swStaticString noName;
  swOptionValuePair *noNamePair;
  swStaticString value;
  swStaticString full;
  unsigned hasName : 1;
  unsigned hasNoName : 1;
  unsigned hasValue : 1;
  unsigned hasDashDashOnly : 1;
} swOptionToken;

typedef struct swCommandLineOptions
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
} swCommandLineOptions;

typedef struct swCommandLineOptionsState
{
  swCommandLineOptions *clOptions;
  const char          **argv;
  swOptionToken        *tokens;
  uint32_t              argCount;
  uint32_t              currentArg;
  uint32_t              currentPositional;
} swCommandLineOptionsState;

swOptionCategoryModuleDeclare(optionCategoryGlobal, "CommandLine",
  swOptionDeclareScalar("help", "Print command line help", NULL, swOptionValueTypeBool, false)
);

// static swOptionCategory optionCategoryGlobal __attribute__ ((section(".commandline"))) = { .magic = SW_COMMANDLINE_MAGIC };
static swCommandLineOptions *commandLineOptionsGlobal = NULL;

static void swCommandLineOptionsDelete(swCommandLineOptions *commandLineOptions)
{
  if (commandLineOptions)
  {
    if (commandLineOptions->groupingValues)
      swHashMapLinearDelete(commandLineOptions->groupingValues);
    if (commandLineOptions->prefixedValues)
      swHashMapLinearDelete(commandLineOptions->prefixedValues);
    if (commandLineOptions->requiredValues.size)
      swFastArrayClear(&(commandLineOptions->requiredValues));
    if (commandLineOptions->namedValues)
      swHashMapLinearDelete(commandLineOptions->namedValues);

    swOptionValuePairClear(&(commandLineOptions->consumeAfterValue));
    swOptionValuePairClear(&(commandLineOptions->sinkValue));
    swOptionValuePairArrayClear(&(commandLineOptions->positionalValues));
    swOptionValuePairArrayClear(&(commandLineOptions->normalValues));
    if (commandLineOptions->categories.size)
      swFastArrayClear(&(commandLineOptions->categories));

    swDynamicStringRelease(&(commandLineOptions->programName));
    swDynamicStringRelease(&(commandLineOptions->argumentsString));
    swDynamicStringRelease(&(commandLineOptions->usageMessage));
    swMemoryFree(commandLineOptions);
  }
}

static swCommandLineOptions *swCommandLineOptionsNew(uint32_t argumentCount)
{
  swCommandLineOptions *rtn = NULL;
  swCommandLineOptions *commandLineOptionsNew = swMemoryCalloc(1, sizeof(swCommandLineOptions));
  if (commandLineOptionsNew)
  {
    if ((swFastArrayInit(&(commandLineOptionsNew->categories), sizeof(swOptionCategory *), 4)))
    {
      if ((swFastArrayInit(&(commandLineOptionsNew->normalValues), sizeof(swOptionValuePair), argumentCount)) &&
          (swFastArrayInit(&(commandLineOptionsNew->positionalValues), sizeof(swOptionValuePair), argumentCount)))
      {
        if ((commandLineOptionsNew->namedValues    = swHashMapLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL)) &&
            (commandLineOptionsNew->prefixedValues = swHashMapLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL)) &&
            (commandLineOptionsNew->groupingValues = swHashMapLinearNew((swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL)))
        {
          // defaults to pointer comparison
          if (swFastArrayInit(&(commandLineOptionsNew->requiredValues), sizeof(swOptionValuePair), argumentCount))
            rtn = commandLineOptionsNew;
        }
      }
    }
    if (!rtn)
      swCommandLineOptionsDelete(commandLineOptionsNew);
  }
  return rtn;
}

// main options category is not mandatory
static bool swOptionCommandLineSetCategories(swCommandLineOptions *commandLineOptions)
{
  bool rtn = false;
  if (commandLineOptions)
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
        if (!commandLineOptions->mainCategory)
          commandLineOptions->mainCategory = category;
        else
        {
          swCommandLineErrorDataSet(&(commandLineOptions->errorData), NULL, category, swCommandLineErrorCodeMainCategory);
          break;
        }
      }
      swFastArrayPush(commandLineOptions->categories, category);
    }
    if (category == categoryEnd)
      rtn = true;
  }
  return rtn;
}

static size_t valueSizes[] =
{
  [swOptionValueTypeBool]   = sizeof(bool),
  [swOptionValueTypeString] = sizeof(swStaticString),
  [swOptionValueTypeInt]    = sizeof(int64_t),
  [swOptionValueTypeDouble] = sizeof(double),
};

#define swOptionValuePairInit(o)  {.option = (o), .value = swDynamicArrayInitEmpty(valueSizes[(o)->valueType])}
#define swOptionValuePairSet(o)   *(swOptionValuePair[]){swOptionValuePairInit(o)}

static bool swOptionValidateDefaultValue(swOption *option)
{
  bool rtn = false;
  if (option)
  {
    if (option->defaultValue.count)
    {
      if ((!option->isArray && (option->defaultValue.count == 1)) || (!option->valueCount || option->defaultValue.count == option->valueCount))
        rtn = (option->defaultValue.elementSize == valueSizes[option->valueType]);
    }
    else
      rtn = true;
  }
  return rtn;
}

static bool swOptionCommandLineProcessOption(swCommandLineOptions *commandLineOptions, swOption *option, bool isMainCategory)
{
  bool rtn = false;
  if (commandLineOptions && option && (option->optionType == swOptionTypeNormal || isMainCategory))
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
          if (swFastArrayPush(commandLineOptions->normalValues, valuePair))
          {
            valuePairPtr = swFastArrayGetExistingPtr(commandLineOptions->normalValues, (commandLineOptions->normalValues.count - 1), swOptionValuePair);
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
          if (swFastArrayPush(commandLineOptions->positionalValues, valuePair))
          {
            valuePairPtr = swFastArrayGetExistingPtr(commandLineOptions->positionalValues, (commandLineOptions->positionalValues.count - 1), swOptionValuePair);
            typeValid = true;
          }
        }
        break;
      }
      case swOptionTypeConsumeAfter:
        if (!commandLineOptions->consumeAfterValue.option)
        {
          // TODO: verify that it is array of strings; is it feasible to have anything else?
          if (!option->name.len && option->isArray)
          {
            commandLineOptions->consumeAfterValue = swOptionValuePairSet(option);
            typeValid = true;
          }
        }
        break;
      case swOptionTypeSink:
        if (!commandLineOptions->sinkValue.option)
        {
          // TODO: verify that it is array of strings; is it feasible to have anything else?
          if (!option->name.len && option->isArray)
          {
            commandLineOptions->sinkValue = swOptionValuePairSet(option);
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
              if (option->optionType == swOptionTypeNormal &&
                  (!option->isArray || (option->arrayType != swOptionArrayTypeMultiValue && option->arrayType != swOptionArrayTypeCommaSeparated)))
              {
                if (swHashMapLinearInsert(commandLineOptions->prefixedValues, &(option->name), valuePairPtr))
                  modifierValid = true;
              }
              break;
            case swOptionModifierGrouping:
              if (option->optionType == swOptionTypeNormal && !option->isArray && (option->name.len == 1) && option->valueType == swOptionValueTypeBool)
              {
                if (swHashMapLinearInsert(commandLineOptions->groupingValues, &(option->name), valuePairPtr))
                  modifierValid = true;
              }
              break;
          }
          if (modifierValid)
          {
            // validate default value
            if (swOptionValidateDefaultValue(option))
            {
              if (!option->name.len || swHashMapLinearInsert(commandLineOptions->namedValues, &(option->name), valuePairPtr))
              {
                if (!option->isRequired || swFastArrayPush(commandLineOptions->requiredValues, valuePairPtr))
                  rtn = true;
              }
              if (!rtn)
                swCommandLineErrorDataSet(&(commandLineOptions->errorData), option, NULL, swCommandLineErrorCodeInternal);
            }
            else
              swCommandLineErrorDataSet(&(commandLineOptions->errorData), option, NULL, swCommandLineErrorCodeInvalidDefault);
          }
          else
            swCommandLineErrorDataSet(&(commandLineOptions->errorData), option, NULL, swCommandLineErrorCodeModifierType);
        }
        else
          swCommandLineErrorDataSet(&(commandLineOptions->errorData), option, NULL, swCommandLineErrorCodeArrayType);
      }
      else
        swCommandLineErrorDataSet(&(commandLineOptions->errorData), option, NULL, swCommandLineErrorCodeValueType);
    }
    else
      swCommandLineErrorDataSet(&(commandLineOptions->errorData), option, NULL, swCommandLineErrorCodeOptionType);
  }
  return rtn;
}

static bool swOptionCommandLineSetOptions(swCommandLineOptions *commandLineOptions)
{
  bool rtn = false;
  if (commandLineOptions && swFastArrayCount(commandLineOptions->categories))
  {
    uint32_t i = 0;
    for (; i < swFastArrayCount(commandLineOptions->categories); i++)
    {
      swOptionCategory *category = NULL;
      if (swFastArrayGet(commandLineOptions->categories, i, category))
      {
        swOption *option = category->options;
        while (option->valueType > swOptionValueTypeNone)
        {
          if (swOptionCommandLineProcessOption(commandLineOptions, option, category->type))
            option++;
          else
            break;
        }
        if (option->name.len)
          break;
      }
      else
      {
        commandLineOptions->errorData.code = swCommandLineErrorCodeInternal;
        break;
      }
    }
    if (i == swFastArrayCount(commandLineOptions->categories))
      rtn = true;
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

#define swOptionValuePairValueSet(valueArray, value, isArray) \
  (isArray)? \
    swDynamicArrayPush((valueArray), &(value)) \
  : \
    (swDynamicArraySet((valueArray), 0, &(value)), (valueArray)->count == 1)

static const swStaticString trueString = swStaticStringDefine("true");
static const swStaticString falseString = swStaticStringDefine("false");
static bool trueValue = true;
static bool falseValue = false;

static bool swOptionValueBoolParser(swStaticString *valueString, swDynamicArray *valueArray, bool isArray)
{
  bool rtn = false;
  if (!swStaticStringCompareCaseless(valueString, &trueString))
    rtn = swOptionValuePairValueSet(valueArray, trueValue, isArray);
  else if (!swStaticStringCompareCaseless(valueString, &falseString))
    rtn = swOptionValuePairValueSet(valueArray, falseValue, isArray);
  return rtn;
}

static bool swOptionValueStringParser(swStaticString *valueString, swDynamicArray *valueArray, bool isArray)
{
  return swOptionValuePairValueSet(valueArray, *valueString, isArray);
}

static bool swOptionValueIntParser(swStaticString *valueString, swDynamicArray *valueArray, bool isArray)
{
  char *endPtr = NULL;
  int64_t value = strtol(valueString->data, &endPtr, 0);
  if ((errno != ERANGE) && (errno != EINVAL) && (size_t)(endPtr - valueString->data) == valueString->len)
    return swOptionValuePairValueSet(valueArray, value, isArray);
  return false;
}

static bool swOptionValueDoubleParser(swStaticString *valueString, swDynamicArray *valueArray, bool isArray)
{
  char *endPtr = NULL;
  double value = strtod(valueString->data, &endPtr);
  if ((errno != ERANGE) && (errno != EINVAL) && (value != NAN) && (value != INFINITY) && (size_t)(endPtr - valueString->data) == valueString->len)
    return swOptionValuePairValueSet(valueArray, value, isArray);
  return false;
}

typedef bool (*swOptionValueParser)(swStaticString *valueString, swDynamicArray *valueArray, bool isArray);

static swOptionValueParser parsers[] =
{
  [swOptionValueTypeBool]   = swOptionValueBoolParser,
  [swOptionValueTypeString] = swOptionValueStringParser,
  [swOptionValueTypeInt]    = swOptionValueIntParser,
  [swOptionValueTypeDouble] = swOptionValueDoubleParser,
};

static bool swOptionCallParser(swOption *option, swStaticString *valueString, swDynamicArray *valueArray)
{
  bool rtn = false;
  bool isArray = option->isArray;
  if (!isArray || option->arrayType != swOptionArrayTypeCommaSeparated)
    rtn = parsers[option->valueType](valueString, valueArray, isArray);
  else
  {
    uint32_t slicesCount = 0;
    if (swStaticStringCountChar(valueString, ',', &slicesCount))
    {
      slicesCount++;
      swStaticString slices[slicesCount];
      memset(slices, 0, sizeof(slices));
      uint32_t foundSlices = 0;
      if (swStaticStringSplitChar(valueString, ',', slices, slicesCount, &foundSlices, 0) && foundSlices == slicesCount)
      {
        uint32_t i = 0;
        while (i < slicesCount)
        {
          if (!parsers[option->valueType](valueString, valueArray, isArray))
            break;
          i++;
        }
        if (i == slicesCount)
          rtn = true;
      }
    }
  }
  return rtn;
}

static bool swOptionCommandLineScanConsumeAfter(swCommandLineOptionsState *state)
{
  bool rtn = true;
  swOptionValuePair *pair = &(state->clOptions->consumeAfterValue);
  while (state->currentArg < state->argCount)
  {
    swOptionToken *token = &state->tokens[state->currentArg];
    swOption *option = pair->option;
    if (swOptionCallParser(option, &(token->full), &(pair->value)))
    {
      state->currentArg++;
      continue;
    }
    swCommandLineErrorDataSet(&(state->clOptions->errorData), option, NULL, swCommandLineErrorCodeParse);
    rtn = false;
    break;
  }
  return rtn;
}

static bool swOptionCommandLineScanPositional(swCommandLineOptionsState *state)
{
  bool rtn = false;
  if ((state->currentPositional < state->clOptions->positionalValues.count) && (state->currentArg < state->argCount))
  {
    swOptionValuePair *pair = swFastArrayGetPtr(state->clOptions->positionalValues, state->currentPositional, swOptionValuePair);
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
        swCommandLineErrorDataSet(&(state->clOptions->errorData), option, NULL, swCommandLineErrorCodeParse);
    }
    else
      swCommandLineErrorDataSet(&(state->clOptions->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
  }
  return rtn;
}

static bool swOptionCommandLineScanAllPositional(swCommandLineOptionsState *state)
{
  bool rtn = false;
  if (state)
  {
    if (state->currentArg < state->argCount)
    {
      bool failure = false;
      while ((state->currentPositional < state->clOptions->positionalValues.count) && (state->currentArg < state->argCount))
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
        else if (state->currentPositional == state->clOptions->positionalValues.count)
        {
          if (state->clOptions->consumeAfterValue.option)
            rtn = swOptionCommandLineScanConsumeAfter(state);
          else
            swCommandLineErrorDataSet(&(state->clOptions->errorData), NULL, NULL, swCommandLineErrorCodeNoConsumeAfter);
        }
      }
      else if (!state->clOptions->positionalValues.count)
        swCommandLineErrorDataSet(&(state->clOptions->errorData), NULL, NULL, swCommandLineErrorCodeNoPositional);
    }
    else
      rtn = true;
  }
  return rtn;
}

static bool swOptionCommandLineScanSink(swCommandLineOptionsState *state)
{
  bool rtn = false;
  swOptionToken *token = &state->tokens[state->currentArg];
  swOption *option = state->clOptions->sinkValue.option;
  if (option)
  {
    if (swOptionCallParser(option, &(token->full), &(state->clOptions->sinkValue.value)))
    {
      state->currentArg++;
      rtn = true;
    }
    else
      swCommandLineErrorDataSet(&(state->clOptions->errorData), option, NULL, swCommandLineErrorCodeParse);
  }
  else
    swCommandLineErrorDataSet(&(state->clOptions->errorData), NULL, NULL, swCommandLineErrorCodeNoSink);
  return rtn;
}

static bool swOptionCommandLineScanArguments(swCommandLineOptionsState *state)
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
        if (token->hasValue)
        {
          swOption *option = token->namePair->option;
          if (swOptionCallParser(option, &(token->value), &(token->namePair->value)))
          {
            state->currentArg++;
            rtn = true;
          }
          else
            swCommandLineErrorDataSet(&(state->clOptions->errorData), option, NULL, swCommandLineErrorCodeParse);
        }
        else
        {
          swOptionToken *nextToken = ((state->currentArg + 1) < state->argCount)? &state->tokens[state->currentArg + 1] : NULL;
          if (nextToken && !nextToken->hasName && nextToken->hasValue)
          {
            state->currentArg++;
            bool isMultiValueArray = token->namePair->option->isArray && token->namePair->option->arrayType == swOptionArrayTypeMultiValue;
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
                swCommandLineErrorDataSet(&(state->clOptions->errorData), option, NULL, swCommandLineErrorCodeParse);
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
              swCommandLineErrorDataSet(&(state->clOptions->errorData), token->namePair->option, NULL, swCommandLineErrorCodeInternal);
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
          swCommandLineErrorDataSet(&(state->clOptions->errorData), token->noNamePair->option, NULL, swCommandLineErrorCodeInternal);
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
          if (!swHashMapLinearValueGet(state->clOptions->groupingValues, &nameSubstring, (void **)(&pairs[position])))
            break;
        }
        // substring failure
        else
        {
          swCommandLineErrorDataSet(&(state->clOptions->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
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
            state->clOptions->errorData.code = swCommandLineErrorCodeInternal;
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
        if (state->currentPositional < state->clOptions->positionalValues.count)
          rtn = swOptionCommandLineScanPositional(state);
        else if (state->clOptions->positionalValues.count && state->clOptions->consumeAfterValue.option)
          rtn = swOptionCommandLineScanConsumeAfter(state);
        else
          rtn = swOptionCommandLineScanSink(state);
      }
    }
    // does not have name
    else
    {
      if (token->hasDashDashOnly)
      {
        state->currentArg++;
        // consume positional only
        rtn = swOptionCommandLineScanAllPositional(state);
      }
      else if (state->currentPositional < state->clOptions->positionalValues.count)
        rtn = swOptionCommandLineScanPositional(state);
      else if (state->clOptions->positionalValues.count && state->clOptions->consumeAfterValue.option)
        rtn = swOptionCommandLineScanConsumeAfter(state);
      else
        rtn = swOptionCommandLineScanSink(state);
    }
  }
  return rtn;
}

static bool swCommandLineOptionsTokenize(swCommandLineOptionsState *state)
{
  bool rtn = false;
  if (state)
  {
    uint32_t i = 0;
    for (; i < state->argCount; i++)
    {
      swOptionToken *token = &(state->tokens[i]);
      token->argv = state->argv[i];
      token->full = swStaticStringSetFromCstr((char *)(token->argv));

      // check for '-' or '--'
      size_t startPosition = 0;
      if (swStaticStringCharEqual(token->full, startPosition, '-'))
      {
        startPosition++;
        if (swStaticStringCharEqual(token->full, startPosition, '-'))
        {
          startPosition++;
          if (token->full.len == 2)
          {
            token->hasDashDashOnly = true;
            continue;
          }
        }

        if (swStaticStringSetSubstring(&(token->full), &(token->name), startPosition, token->full.len))
        {
          token->hasName = true;
          swStaticString slices[2] = {swStaticStringDefineEmpty};
          uint32_t foundSlices = 0;
          if (swStaticStringSplitChar(&(token->name), '=', slices, 2, &foundSlices, 0))
          {
            if (foundSlices == 2)
            {
              token->name = slices[0];
              token->value = slices[1];
              token->hasValue = true;
            }
            // check for 'no'
            else if (swStaticStringCharEqual(token->full, startPosition, 'n') && swStaticStringCharEqual(token->full, startPosition + 1, 'o'))
            {
              startPosition += 2;
              if (swStaticStringSetSubstring(&(token->name), &(token->noName), startPosition, token->name.len))
                token->hasNoName = true;
              else
              {
                swCommandLineErrorDataSet(&(state->clOptions->errorData),NULL, NULL, swCommandLineErrorCodeInternal);
                break;
              }
            }
          }
          else
          {
            swCommandLineErrorDataSet(&(state->clOptions->errorData),NULL, NULL, swCommandLineErrorCodeInternal);
            break;
          }
        }
        else
        {
          swCommandLineErrorDataSet(&(state->clOptions->errorData),NULL, NULL, swCommandLineErrorCodeInternal);
          break;
        }
      }
      else
      {
        token->value = token->full;
        token->hasValue = true;
      }
      if (token->hasName)
      {
        if (!swHashMapLinearValueGet(state->clOptions->namedValues, &(token->name), (void **)(&(token->namePair))) && !token->hasValue)
        {
          // find prefix name/value
          swStaticString nameSubstring = swStaticStringDefineEmpty;
          swStaticString argumentValue = swStaticStringDefineEmpty;
          size_t len = 1;
          while (len < token->name.len)
          {
            if (swStaticStringSetSubstring(&(token->name), &nameSubstring, 0, len) &&
                swStaticStringSetSubstring(&(token->name), &argumentValue, len, token->name.len))
            {
              if (swHashMapLinearValueGet(state->clOptions->prefixedValues, &nameSubstring, (void **)(&(token->namePair))))
              {
                token->name = nameSubstring;
                token->value = argumentValue;
                token->hasValue = true;
                break;
              }
            }
            else
            {
              swCommandLineErrorDataSet(&(state->clOptions->errorData),NULL, NULL, swCommandLineErrorCodeInternal);
              break;
            }
            len++;
          }
          if (i < token->name.len && !token->hasValue)
            break;
        }
      }
      if (token->hasNoName)
        swHashMapLinearValueGet(state->clOptions->namedValues, &(token->noName), (void **)(&(token->noNamePair)));
    }
    if (i == state->argCount)
      rtn = true;
  }
  return rtn;
}

static bool _setDefaultsAndCheckArrays(swOptionValuePair *pair, swCommandLineErrorData *errorData)
{
  bool rtn = false;
  if (!pair->option->defaultValue.count || (!pair->value.count && swDynamicArraySetFromStaticArray(&(pair->value), &(pair->option->defaultValue))))
  {
    if (!pair->value.count || !pair->option->valueCount || pair->value.count == pair->option->valueCount)
      rtn = true;
    else
      swCommandLineErrorDataSet(errorData, pair->option, NULL, swCommandLineErrorCodeArrayValueCount);
  }
  else
    swCommandLineErrorDataSet(errorData, NULL, NULL, swCommandLineErrorCodeInternal);
    errorData->code = swCommandLineErrorCodeInternal;
  return rtn;
}

static bool swOptionCommandLineWalkPairs(swCommandLineOptions *commandLineOptions, bool (*pairFunction)(swOptionValuePair *, swCommandLineErrorData *errorData))
{
  bool rtn = false;
  if (commandLineOptions && pairFunction)
  {
    swFastArray *valuePairsList[] = {&commandLineOptions->normalValues, &commandLineOptions->positionalValues, NULL};
    swFastArray **valuePairsListPtr = valuePairsList;
    while (*valuePairsListPtr)
    {
      swOptionValuePair *valuePairPtr = (swOptionValuePair *)(*valuePairsListPtr)->storage;
      uint32_t i = 0;
      for (; i < (*valuePairsListPtr)->count; i++)
      {
        if (!pairFunction(valuePairPtr, &(commandLineOptions->errorData)))
          break;
        valuePairPtr++;
      }
      if (i < (*valuePairsListPtr)->count)
        break;
      valuePairsListPtr++;
    }
    if (!(*valuePairsListPtr))
    {
      swOptionValuePair *valuePairs[] = {&commandLineOptions->sinkValue, &commandLineOptions->consumeAfterValue, NULL};
      swOptionValuePair **valuePairsPtr = valuePairs;
      while (*valuePairsPtr)
      {
        if (!pairFunction(*valuePairsPtr, &(commandLineOptions->errorData)))
          break;
        valuePairsPtr++;
      }
      if (!(*valuePairsPtr))
        rtn = true;
    }
  }
  return rtn;
}

static bool swOptionCommandLineCheckRequired(swCommandLineOptions *commandLineOptions)
{
  bool rtn = false;
  if (commandLineOptions)
  {
    if (commandLineOptions->requiredValues.count)
    {
      swOptionValuePair **valuePairs = (swOptionValuePair **)commandLineOptions->requiredValues.storage;
      uint32_t i = 0;
      while (i < commandLineOptions->requiredValues.count)
      {
        if (!valuePairs[i]->value.count)
        {
          swCommandLineErrorDataSet(&(commandLineOptions->errorData), valuePairs[i]->option, NULL, swCommandLineErrorCodeRequiredValue);
          break;
        }
        i++;
      }
      if (i == commandLineOptions->requiredValues.count)
        rtn = true;
    }
    else
      rtn = true;
  }
  return rtn;
}

static bool swOptionCommandLineSetValues(swCommandLineOptions *commandLineOptions, int argc, const char *argv[])
{
  bool rtn = false;
  if (commandLineOptions && argc && argv)
  {
    uint32_t argumentCount = (uint32_t)argc;
    swOptionToken tokens[argumentCount];
    memset(tokens, 0, sizeof(tokens));

    swCommandLineOptionsState state = {.clOptions = commandLineOptions, .argv = argv, .tokens = &tokens[0], .argCount = argumentCount};
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
        if (swOptionCommandLineWalkPairs(commandLineOptions, _setDefaultsAndCheckArrays))
          rtn = swOptionCommandLineCheckRequired(commandLineOptions);
      }
    }
  }
  return rtn;
}

bool swOptionCommandLineInit(int argc, const char *argv[], const char *usageMessage, swDynamicString **errorString)
{
  bool rtn = false;
  if (!commandLineOptionsGlobal && (argc > 0) && argv)
  {
    // init command line options global
    if ((commandLineOptionsGlobal = swCommandLineOptionsNew(argc - 1)))
    {
      // collect all command lines option categories
      // walk through all categories and collect all command line options and required options
      if (swOptionCommandLineSetCategories(commandLineOptionsGlobal) && swOptionCommandLineSetOptions(commandLineOptionsGlobal))
      {
        // set programName
        if (swFileRealPath(argv[0], &(commandLineOptionsGlobal->programName)))
        {
          // walk through the arguments, argumentsString, and optionValues
          if ((argc == 1) || swOptionCommandLineSetValues(commandLineOptionsGlobal, argc - 1, &argv[1]))
          {
            if (!usageMessage || swDynamicStringSetFromCString(&(commandLineOptionsGlobal->usageMessage), usageMessage))
              rtn = true;
          }
        }
        else
          swCommandLineErrorDataSet(&(commandLineOptionsGlobal->errorData), NULL, NULL, swCommandLineErrorCodeRealPath);
      }
      if (!rtn)
      {
        swCommandLineErrorSet(&(commandLineOptionsGlobal->errorData), swCommandLineErrorCodeNone, errorString);
        swCommandLineOptionsDelete(commandLineOptionsGlobal);
        commandLineOptionsGlobal = NULL;
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
  if (commandLineOptionsGlobal)
  {
    swCommandLineOptionsDelete(commandLineOptionsGlobal);
    commandLineOptionsGlobal = NULL;
  }
}

void swOptionCommandLinePrintUsage()
{
}

static void *swOptionValueGetInternal(swStaticString *name, swOptionValueType type)
{
  if (commandLineOptionsGlobal && name)
  {
    swOptionValuePair *pair = NULL;
    if (swHashMapLinearValueGet(commandLineOptionsGlobal->namedValues, name, (void **)&pair))
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
  if (commandLineOptionsGlobal && name)
  {
    swOptionValuePair *pair = NULL;
    if (swHashMapLinearValueGet(commandLineOptionsGlobal->namedValues, name, (void **)&pair))
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
  if (commandLineOptionsGlobal)
  {
    swOptionValuePair *pair = swFastArrayGetExistingPtr(commandLineOptionsGlobal->positionalValues, position, swOptionValuePair);
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
  if (commandLineOptionsGlobal)
  {
    swOptionValuePair *pair = swFastArrayGetExistingPtr(commandLineOptionsGlobal->positionalValues, position, swOptionValuePair);
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
  if (commandLineOptionsGlobal && commandLineOptionsGlobal->sinkValue.option)
  {
    if (commandLineOptionsGlobal->sinkValue.option->valueType == type)
      return (swStaticArray *)(&(commandLineOptionsGlobal->sinkValue.value));
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
  if (commandLineOptionsGlobal && commandLineOptionsGlobal->consumeAfterValue.option)
  {
    if (commandLineOptionsGlobal->consumeAfterValue.option->valueType == type)
      return (swStaticArray *)(&(commandLineOptionsGlobal->consumeAfterValue.value));
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
