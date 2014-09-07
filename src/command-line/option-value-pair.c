#include "command-line/option-value-pair.h"

void swOptionValuePairClear(swOptionValuePair *pair)
{
  if (pair && pair->value.size)
    swDynamicArrayRelease(&(pair->value));
}
