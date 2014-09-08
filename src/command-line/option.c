#include "command-line/option.h"
#include "command-line/option-value-pair.h"

#include <errno.h>
#include <math.h>

static size_t valueSizes[] =
{
  [swOptionValueTypeBool]   = sizeof(bool),
  [swOptionValueTypeString] = sizeof(swStaticString),
  [swOptionValueTypeInt]    = sizeof(int64_t),
  [swOptionValueTypeDouble] = sizeof(double),
};

bool swOptionValidateDefaultValue(swOption *option)
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

size_t swOptionValueTypeSizeGet(swOptionValueType type)
{
  if (type > swOptionValueTypeNone && type < swOptionValueTypeMax)
    return valueSizes[type];
  return 0;
}

static const swStaticString trueString = swStaticStringDefine("true");
static const swStaticString falseString = swStaticStringDefine("false");
bool trueValue = true;
bool falseValue = false;

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

bool swOptionCallParser(swOption *option, swStaticString *valueString, swDynamicArray *valueArray)
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
          if (!parsers[option->valueType](&slices[i], valueArray, isArray))
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
