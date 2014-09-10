#ifndef SW_COMMANDLINE_COMMANDLINEDATA_H
#define SW_COMMANDLINE_COMMANDLINEDATA_H

#include "command-line/command-line-error.h"
#include "command-line/option-category.h"
#include "command-line/option-value-pair.h"

#include "collections/fast-array.h"
#include "collections/hash-map-linear.h"

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
  swDynamicString     programName;
  swDynamicString     argumentsString;
  swDynamicString     usageMessage;
  swCommandLineErrorData  errorData;
} swCommandLineData;

void swCommandLineDataDelete(swCommandLineData *commandLineData);
swCommandLineData *swCommandLineDataNew(uint32_t argumentCount);
bool swCommandLineDataSetCategories(swCommandLineData *commandLineData, swOptionCategory *globalCategory);
bool swCommandLineDataSetOptions(swCommandLineData *commandLineData);
bool swCommandLineDataSetValues(swCommandLineData *commandLineData, int argc, const char *argv[]);

#endif // SW_COMMANDLINE_COMMANDLINEDATA_H