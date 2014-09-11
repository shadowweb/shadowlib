#include <stdio.h>

#include "command-line/command-line-error.h"
#undef swCommandLineErrorDataSet

static const char *swCommandLineErrorCodeText[] __attribute__((unused)) =
{
  [swCommandLineErrorCodeInvalidInput]    = "Invalid Input",
  [swCommandLineErrorCodeInternal]        = "Internal Error",
  [swCommandLineErrorCodeMainCategory]    = "Only one main category is allowed",
  [swCommandLineErrorCodeOptionType]      = "Option type validation failed",
  [swCommandLineErrorCodeValueType]       = "Invalid value type",
  [swCommandLineErrorCodeArrayType]       = "Invalid array type for scalar option",
  [swCommandLineErrorCodePrefixOption]    = "Invalid prefix option",
  [swCommandLineErrorCodeInvalidDefault]  = "Invalid default value specified for option",
  [swCommandLineErrorCodeRealPath]        = "Failed to find real path for this executable",
  [swCommandLineErrorCodeRequiredValue]   = "Missing required value for option",
  [swCommandLineErrorCodeArrayValueCount] = "Wrong number of values for array value count",
  [swCommandLineErrorCodeArrayMultivalue] = "Invalid way to specify multivalue array value",
  [swCommandLineErrorCodeParse]           = "Failed to parse option value",
  [swCommandLineErrorCodeNoPositional]    = "No positional arguments expected",
};

void swCommandLineErrorSet(swCommandLineErrorData *errorData, swCommandLineErrorCode errorCode, swDynamicString **errorString)
{
  swCommandLineErrorCode code = (errorData)? errorData->code : errorCode;
  if (code && code < swCommandLineErrorCodeMax)
  {
    // populate error message
    if (errorString && (*errorString = swDynamicStringNew(1024)))
    {
      if (errorData)
      {
        swDynamicStringAppendCString(*errorString, "File: ");
        swDynamicStringAppendCString(*errorString, errorData->file);
        swDynamicStringAppendCString(*errorString, "; Line: ");
        (*errorString)->len += sprintf(&((*errorString)->data[(*errorString)->len]), "%d", errorData->line);
        swDynamicStringAppendCString(*errorString, "; ");
        if (errorData->category)
        {
          swDynamicStringAppendCString(*errorString, "Category: ");
          swDynamicStringAppendCString(*errorString, errorData->category->name);
          swDynamicStringAppendCString(*errorString, "; ");
        }
        if (errorData->option)
        {
          swDynamicStringAppendCString(*errorString, "Option: ");
          if (errorData->option->name.len)
            swDynamicStringAppendStaticString(*errorString, &(errorData->option->name));
          else if (errorData->option->optionType == swOptionTypePositional)
            swDynamicStringAppendCString(*errorString, "Positional");
          swDynamicStringAppendCString(*errorString, "; ");
        }
      }
    }
    swDynamicStringAppendCString(*errorString, "Error: ");
    swDynamicStringAppendCString(*errorString, swCommandLineErrorCodeText[code]);
  }
}

void swCommandLineErrorDataSet(swCommandLineErrorData *errorData, swOption *option, swOptionCategory *category, const char *file, int line, swCommandLineErrorCode errorCode)
{
  if (errorData && !errorData->code)
  {
    errorData->option = option;
    errorData->category = category;
    errorData->file = file;
    errorData->line = line;
    errorData->code = errorCode;
  }
  else
    fprintf(stderr, "Failed to set error data\n");
}
