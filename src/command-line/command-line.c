#include "command-line/command-line.h"
#include "command-line/command-line-data.h"
#include "command-line/command-line-error.h"

#include "utils/file.h"

static swCommandLineData *commandLineDataGlobal = NULL;

swOptionCategoryModuleDeclare(optionCategoryGlobal, "CommandLine",
  swOptionDeclareScalar("h|?|help", "Print command line help", NULL, NULL, swOptionValueTypeBool, false)
);

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

bool swCommandLineInit(int argc, const char *argv[], const char *title, const char *usageMessage, swDynamicString **errorString)
{
  bool rtn = false;
  if (!commandLineDataGlobal && (argc > 0) && argv)
  {
    // init command line options global
    if ((commandLineDataGlobal = swCommandLineDataNew(argc - 1)))
    {
      // collect all command lines option categories
      // walk through all categories and collect all command line options and required options
      if (swCommandLineDataSetCategories(commandLineDataGlobal, (swOptionCategory *)(&optionCategoryGlobal)) && swCommandLineDataSetOptions(commandLineDataGlobal))
      {
        // set programName
        if (swFileRealPath(argv[0], &(commandLineDataGlobal->programNameLong)))
        {
          if (swDynamicStringSetFromCString(&(commandLineDataGlobal->programNameShort), argv[0]))
          {
            if (!title || swDynamicStringSetFromCString(&(commandLineDataGlobal->title), title))
            {
              if (!usageMessage || swDynamicStringSetFromCString(&(commandLineDataGlobal->usageMessage), usageMessage))
              {
                // walk through the arguments, argumentsString, and optionValues
                rtn = swCommandLineDataSetValues(commandLineDataGlobal, argc - 1, &argv[1]);
                if (rtn)
                {
                  bool printUsage = false;
                  if (swOptionValueGetBool(&(swStaticStringSetFromCstr("help")), &printUsage) && printUsage)
                  {
                    swCommandLinePrintUsage();
                    rtn = false;
                  }
                }
                else
                  swCommandLinePrintUsage();
              }
              else
                swCommandLineErrorDataSet(&(commandLineDataGlobal->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
            }
            else
              swCommandLineErrorDataSet(&(commandLineDataGlobal->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
          }
          else
            swCommandLineErrorDataSet(&(commandLineDataGlobal->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
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

void swCommandLineShutdown()
{
  if (commandLineDataGlobal)
  {
    swCommandLineDataDelete(commandLineDataGlobal);
    commandLineDataGlobal = NULL;
  }
}

void swCommandLinePrintUsage()
{
  if (commandLineDataGlobal)
    swCommandLineDataPrintOptions(commandLineDataGlobal);
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

bool swOptionValueGetEnum(swStaticString *name, int64_t *value)
{
  swOptionValueGetImplement(name, value, swOptionValueTypeEnum);
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

bool swOptionValueGetEnumArray(swStaticString *name, swStaticArray *value)
{
  swOptionValueGetArrayImplement(name, value, swOptionValueTypeEnum);
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

bool swPositionalOptionValueGetEnum (uint32_t position, int64_t *value)
{
  swPositionalOptionValueGetImplement(position, value, swOptionValueTypeEnum);
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

bool swPositionalOptionValueGetEnumArray(uint32_t position, swStaticArray *value)
{
  swPositionalOptionValueGetArrayImplement(position, value, swOptionValueTypeEnum);
}

bool swSinkOptionValueGet(swStaticArray *value)
{
  bool rtn = false;
  if (value && commandLineDataGlobal)
  {
    *(value) = *(swStaticArray *)(&(commandLineDataGlobal->sinkValue));
    rtn = true;
  }
  return rtn;
}

bool swConsumeAfterOptionValueGet(swStaticArray *value)
{
  bool rtn = false;
  if (value && commandLineDataGlobal)
  {
    *(value) = *(swStaticArray *)(&(commandLineDataGlobal->consumeAfterValue));
    rtn = true;
  }
  return rtn;
}
