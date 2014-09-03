#ifndef SW_COMMANDLINE_COMMANDLINEERROR_H
#define SW_COMMANDLINE_COMMANDLINEERROR_H

#include "command-line/command-line.h"
#include "storage/dynamic-string.h"

typedef enum swCommandLineErrorCode
{
  swCommandLineErrorCodeNone,
  swCommandLineErrorCodeInvalidInput,
  swCommandLineErrorCodeInternal,
  swCommandLineErrorCodeMainCategory,
  swCommandLineErrorCodeOptionType,
  swCommandLineErrorCodeValueType,
  swCommandLineErrorCodeArrayType,
  swCommandLineErrorCodeModifierType,
  swCommandLineErrorCodeInvalidDefault,
  swCommandLineErrorCodeRealPath,
  swCommandLineErrorCodeRequiredValue,
  swCommandLineErrorCodeArrayValueCount,
  swCommandLineErrorCodeParse,
  swCommandLineErrorCodeNoPositional,
  swCommandLineErrorCodeNoConsumeAfter,
  swCommandLineErrorCodeNoSink,
  swCommandLineErrorCodeMax
} swCommandLineErrorCode;

static char *swCommandLineErrorCodeText[] __attribute__((unused)) =
{
  [swCommandLineErrorCodeInvalidInput]    = "Invalid Input",
  [swCommandLineErrorCodeInternal]        = "Internal Error",
  [swCommandLineErrorCodeMainCategory]    = "Only one main category is allowed",
  [swCommandLineErrorCodeOptionType]      = "Option type validation failed",
  [swCommandLineErrorCodeValueType]       = "Invalid value type",
  [swCommandLineErrorCodeArrayType]       = "Invalid array type for scalar option",
  [swCommandLineErrorCodeModifierType]    = "Invalid modifier specified for option",
  [swCommandLineErrorCodeInvalidDefault]  = "Invalid default value specified for option",
  [swCommandLineErrorCodeRealPath]        = "Failed to find real path for this executable",
  [swCommandLineErrorCodeRequiredValue]   = "Missing required value for option",
  [swCommandLineErrorCodeArrayValueCount] = "Wrong number of values for array value count",
  [swCommandLineErrorCodeParse]           = "Failed to parse option value",
  [swCommandLineErrorCodeNoPositional]    = "No positional arguments expected",
  [swCommandLineErrorCodeNoConsumeAfter]  = "No consume after arguments expected",
  [swCommandLineErrorCodeNoSink]          = "No sink arguments expected",
};

typedef struct swCommandLineErrorData
{
  swOption *option;
  swOptionCategory *category;
  const char *file;
  const char *line;
  swCommandLineErrorCode code;
} swCommandLineErrorData;

void swCommandLineErrorSet(swCommandLineErrorData *errorData, swCommandLineErrorCode errorCode, swDynamicString **errorString)
{
  swCommandLineErrorCode code = (errorData)? errorData->code : errorCode;
  if (code && code < swCommandLineErrorCodeMax)
  {
    if (errorString)
    {
      // populate error message
    }
  }
}

void swCommandLineErrorDataSet(swCommandLineErrorData *errorData, swOption *option, swOptionCategory *category, const char *file, const char *line, swCommandLineErrorCode errorCode)
{
  if (errorData && !errorData->code)
  {
    errorData->option = option;
    errorData->category = category;
    errorData->file = file;
    errorData->line = line;
    errorData->code = errorCode;
  }
  // else
  // TODO: print something as sanity check; we do not want to set error twice
}

#define swCommandLineErrorDataSet(ed, o, c, ec) swCommandLineErrorDataSet((ed), (o), (c), __FILE__, __LINE__, (ec))

#endif // SW_COMMANDLINE_COMMANDLINEERROR_H
