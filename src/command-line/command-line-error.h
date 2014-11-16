#ifndef SW_COMMANDLINE_COMMANDLINEERROR_H
#define SW_COMMANDLINE_COMMANDLINEERROR_H

#include "command-line/option.h"
#include "command-line/option-category.h"
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
  swCommandLineErrorCodePrefixOption,
  swCommandLineErrorCodeInvalidDefault,
  swCommandLineErrorCodeRealPath,
  swCommandLineErrorCodeRequiredValue,
  swCommandLineErrorCodeArrayValueCount,
  swCommandLineErrorCodeArrayMultivalue,
  swCommandLineErrorCodeParse,
  swCommandLineErrorCodeNoPositional,
  swCommandLineErrorCodeValueAfterEnumName,
  swCommandLineErrorCodeMax
} swCommandLineErrorCode;

typedef struct swCommandLineErrorData
{
  swOption *option;
  swOptionCategory *category;
  const char *file;
  int line;
  swCommandLineErrorCode code;
} swCommandLineErrorData;

void swCommandLineErrorSet(swCommandLineErrorData *errorData, swCommandLineErrorCode errorCode, swDynamicString **errorString);

void swCommandLineErrorDataSet(swCommandLineErrorData *errorData, swOption *option, swOptionCategory *category, const char *file, int line, swCommandLineErrorCode errorCode);

#define swCommandLineErrorDataSet(ed, o, c, ec) swCommandLineErrorDataSet((ed), (o), (c), __FILE__, __LINE__, (ec))

#endif // SW_COMMANDLINE_COMMANDLINEERROR_H
