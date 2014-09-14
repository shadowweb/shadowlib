#ifndef SW_COMMANDLINE_OPTIONTOKEN_H
#define SW_COMMANDLINE_OPTIONTOKEN_H

#include "command-line/command-line-error.h"
#include "command-line/option-value-pair.h"

#include "collections/hash-map-linear.h"

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
  unsigned hasValue : 1;
  unsigned hasDashDashOnly : 1;
  unsigned hasDoubleDash : 1;
} swOptionToken;

bool swOptionTokenSet(swOptionToken *token, const char *argv, swHashMapLinear *namedValues, swHashMapLinear *prefixedValues, swCommandLineErrorData *errorData);

#endif // SW_COMMANDLINE_OPTIONTOKEN_H
