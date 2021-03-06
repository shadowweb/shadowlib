#include "command-line/option.h"
#include "command-line/option-value-pair.h"
#include "storage/dynamic-string.h"
#include "utils/colors.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>

static size_t valueSizes[] =
{
  [swOptionValueTypeBool]   = sizeof(bool),
  [swOptionValueTypeString] = sizeof(swStaticString),
  [swOptionValueTypeInt]    = sizeof(int64_t),
  [swOptionValueTypeDouble] = sizeof(double),
  [swOptionValueTypeEnum]   = sizeof(int64_t),
};

bool swOptionValidateDefaultValue(swOption *option)
{
  bool rtn = false;
  if (option)
  {
    if (option->defaultValue)
    {
      if (option->defaultValue->count)
      {
        if ((!option->isArray && (option->defaultValue->count == 1)) || (!option->valueCount || option->defaultValue->count == option->valueCount))
          rtn = (option->defaultValue->elementSize == valueSizes[option->valueType]);
      }
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

static inline bool swOptionValueBoolParser(swStaticString *valueString, swDynamicArray *valueArray, bool isArray, swHashMapLinear *valueNames, swStaticString *nameString)
{
  bool rtn = false;
  if (!swStaticStringCompareCaseless(valueString, &trueString))
    rtn = swOptionValuePairValueSet(valueArray, trueValue, isArray);
  else if (!swStaticStringCompareCaseless(valueString, &falseString))
    rtn = swOptionValuePairValueSet(valueArray, falseValue, isArray);
  return rtn;
}

static inline bool swOptionValueStringParser(swStaticString *valueString, swDynamicArray *valueArray, bool isArray, swHashMapLinear *valueNames, swStaticString *nameString)
{
  return swOptionValuePairValueSet(valueArray, *valueString, isArray);
}

static inline bool swOptionValueIntParser(swStaticString *valueString, swDynamicArray *valueArray, bool isArray, swHashMapLinear *valueNames, swStaticString *nameString)
{
  char *endPtr = NULL;
  int64_t value = strtol(valueString->data, &endPtr, 0);
  if ((errno != ERANGE) && (errno != EINVAL) && (size_t)(endPtr - valueString->data) == valueString->len)
    return swOptionValuePairValueSet(valueArray, value, isArray);
  return false;
}

static inline bool swOptionValueDoubleParser(swStaticString *valueString, swDynamicArray *valueArray, bool isArray, swHashMapLinear *valueNames, swStaticString *nameString)
{
  char *endPtr = NULL;
  double value = strtod(valueString->data, &endPtr);
  if ((errno != ERANGE) && (errno != EINVAL) && (value != NAN) && (value != INFINITY) && (size_t)(endPtr - valueString->data) == valueString->len)
    return swOptionValuePairValueSet(valueArray, value, isArray);
  return false;
}

static inline bool swOptionValueEnumParser(swStaticString *valueString, swDynamicArray *valueArray, bool isArray, swHashMapLinear *valueNames, swStaticString *nameString)
{
  bool rtn = false;
  char *endPtr = NULL;
  int64_t value = strtol(valueString->data, &endPtr, 0);
  if ((errno != ERANGE) && (errno != EINVAL) && (size_t)(endPtr - valueString->data) == valueString->len)
    rtn = swOptionValuePairValueSet(valueArray, value, isArray);
  if (!rtn)
  {
    swDynamicString *optionValue = swDynamicStringNewFromFormat("%.*s%.*s", (int)(nameString->len), nameString->data, (int)(valueString->len), valueString->data);
    if (optionValue)
    {
      int64_t *value = NULL;
      if (swHashMapLinearValueGet(valueNames, (swStaticString *)optionValue, (void **)&value))
        rtn = swOptionValuePairValueSet(valueArray, *value, isArray);
      swDynamicStringDelete(optionValue);
    }
  }
  return rtn;
}

typedef bool (*swOptionValueParser)(swStaticString *valueString, swDynamicArray *valueArray, bool isArray, swHashMapLinear *valueNames, swStaticString *nameString);

static swOptionValueParser parsers[] =
{
  [swOptionValueTypeBool]   = swOptionValueBoolParser,
  [swOptionValueTypeString] = swOptionValueStringParser,
  [swOptionValueTypeInt]    = swOptionValueIntParser,
  [swOptionValueTypeDouble] = swOptionValueDoubleParser,
  [swOptionValueTypeEnum]   = swOptionValueEnumParser,
};

bool swOptionCallParser(swOption *option, swStaticString *valueString, swDynamicArray *valueArray, swHashMapLinear *valueNames, swStaticString *nameString)
{
  bool rtn = false;
  bool isArray = option->isArray;
  if (!isArray || option->arrayType != swOptionArrayTypeCommaSeparated)
    rtn = parsers[option->valueType](valueString, valueArray, isArray, valueNames, nameString);
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
          if (!parsers[option->valueType](&slices[i], valueArray, isArray, valueNames, nameString))
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

static char *swOptionValueTypeName[] =
{
  [swOptionValueTypeBool]   = "BOOL",
  [swOptionValueTypeString] = "STRING",
  [swOptionValueTypeInt]    = "INT",
  [swOptionValueTypeDouble] = "DOUBLE",
  [swOptionValueTypeEnum]   = "ENUM",
};

char *swOptionValueTypeNameGet(swOptionValueType type)
{
  if (type > swOptionValueTypeNone && type < swOptionValueTypeMax)
    return swOptionValueTypeName[type];
  return NULL;
}

static char *spacePrefix = "                                        ";
#define SW_OPTION_DESCRIPTION_OFFSET    40

swDynamicString *swOptionPrintEnumValues(swOptionEnumValueName (*enumNames)[])
{
  swDynamicString *rtn = NULL;
  if (enumNames)
  {
    swDynamicString *enumString = swDynamicStringNew(16);
    if (enumString)
    {
      swOptionEnumValueName *enumSpecs = &((*enumNames)[0]);
      if (enumSpecs)
      {
        bool firstTime = true;
        while (enumSpecs->optionName || enumSpecs->valueName)
        {
          if (enumSpecs->valueName)
          {
            if (firstTime)
            {
              swDynamicStringAppendCString(enumString, "(");
              firstTime = false;
            }
            else
              swDynamicStringAppendCString(enumString, "|");
            swDynamicStringAppendCString(enumString, enumSpecs->valueName);
          }
          enumSpecs++;
        }
        if (!enumSpecs->optionName && !enumSpecs->valueName && !firstTime)
        {
          swDynamicStringAppendCString(enumString, ")");
          rtn = enumString;
        }
      }
      if (!rtn)
        swDynamicStringDelete(enumString);
    }
  }
  return rtn;
}

void swOptionPrint(swOption *option)
{
  if (option)
  {
    int charactersCount = 0;
    char *valueName = (option->valueDescription)? option->valueDescription : swOptionValueTypeNameGet(option->valueType);
    swDynamicString *enumValuesString = NULL;
    if (option->valueType == swOptionValueTypeEnum)
      enumValuesString = swOptionPrintEnumValues(option->enumNames);
    charactersCount += printf ("  ");
    printf ("%s", SW_COLOR_ANSI_GREEN);
    if (option->aliases.count)
    {
      uint32_t oneLetterCount = 0;
      swStaticString *aliases = (swStaticString *)(option->aliases.data);
      for (uint32_t i = 0; i < option->aliases.count; i++)
      {
        if (aliases[i].len == 1)
        {
          charactersCount += printf ("%s%s%.*s%s%s", ((oneLetterCount)? ", ": ""), "-", (int)(aliases[i].len), aliases[i].data,
                                      ((option->valueType != swOptionValueTypeBool)? " " : ""),
                                      ((option->valueType != swOptionValueTypeBool)? valueName : ""));
          oneLetterCount++;
        }
      }
      if (oneLetterCount)
        charactersCount += printf(", ");

      uint32_t otherCount = 0;
      for (uint32_t i = 0; i < option->aliases.count; i++)
      {
        if (aliases[i].len > 1)
        {
          charactersCount += printf ("%s%s%.*s=%s", ((otherCount)? ", ": ""), "--", (int)(aliases[i].len), aliases[i].data, valueName);
          otherCount++;
        }
      }
    }
    else
    {
      bool isOneLetter = (option->name.len == 1);
      charactersCount += printf ("%s%.*s%s%s", ((isOneLetter)? "-" : "--"), (int)(option->name.len), option->name.data, ((isOneLetter)? " " : "="),
              ((isOneLetter && option->valueType == swOptionValueTypeBool)? "" : valueName));
    }
    if (enumValuesString)
      charactersCount += printf (", %s = %.*s", valueName, (int)(enumValuesString->len), enumValuesString->data);
    printf ("%s", SW_COLOR_ANSI_NORMAL);
    if (charactersCount >= SW_OPTION_DESCRIPTION_OFFSET)
      printf ("\n%.*s", SW_OPTION_DESCRIPTION_OFFSET, spacePrefix);
    else
      printf ("%.*s", (SW_OPTION_DESCRIPTION_OFFSET - charactersCount), spacePrefix);
    printf ("%s\n", ((option->description)? option->description : "NO DESCRIPTION"));
    if (enumValuesString)
      swDynamicStringDelete(enumValuesString);
  }
}
