#ifndef SW_COMMANDLINE_OPTIONVALUEPAIR_H
#define SW_COMMANDLINE_OPTIONVALUEPAIR_H

#include "collections/dynamic-array.h"
#include "command-line/option.h"

typedef struct swOptionValuePair
{
  swOption      *option;
  swDynamicArray value;
} swOptionValuePair;

#define swOptionValuePairInit(o)  {.option = (o), .value = swDynamicArrayInitEmpty(swOptionValueTypeSizeGet((o)->valueType))}
#define swOptionValuePairSet(o)   *(swOptionValuePair[]){swOptionValuePairInit(o)}

#define swOptionValuePairValueSet(valueArray, value, isArray) \
  ((isArray)? \
    swDynamicArrayPush((valueArray), &(value)) \
  : \
    (swDynamicArraySet((valueArray), 0, &(value)), ((valueArray)->count == 1)))

void swOptionValuePairClear(swOptionValuePair *pair);
bool swOptionValuePairSetDefaults(swOptionValuePair *pair);
bool swOptionValuePairCheckArrays(swOptionValuePair *pair);

#endif // SW_COMMANDLINE_OPTIONVALUEPAIR_H
