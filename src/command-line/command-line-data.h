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
  swDynamicArray      sinkValue;
  swDynamicArray      consumeAfterValue;

  swHashMapLinear    *namedValues;        // hash map of swOption:name => swOptionValuePair
  swFastArray         requiredValues;     // array of pointers to required swOptionValuePair
  swHashMapLinear    *prefixedValues;     // hash map of prefixed swOptionValuePair
  swHashMapLinear    *enumNames;          // hash map of enum option names
  swHashMapLinear    *enumValues;         // hash map of enum option values (prefixed by option name to prevent name collisions)
  swDynamicString     programNameShort;
  swDynamicString     programNameLong;
  swDynamicString     argumentsString;
  swDynamicString     title;
  swDynamicString     usageMessage;
  swCommandLineErrorData  errorData;
} swCommandLineData;

void swCommandLineDataDelete(swCommandLineData *commandLineData);
swCommandLineData *swCommandLineDataNew(uint32_t argumentCount);
bool swCommandLineDataSetCategories(swCommandLineData *commandLineData, swOptionCategory *globalCategory);
bool swCommandLineDataSetOptions(swCommandLineData *commandLineData);
bool swCommandLineDataSetValues(swCommandLineData *commandLineData, int argc, const char *argv[]);
void swCommandLineDataPrintOptions(swCommandLineData *commandLineData);

#endif // SW_COMMANDLINE_COMMANDLINEDATA_H
