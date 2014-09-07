#include "command-line/option-value-pair.h"

void swOptionValuePairClear(swOptionValuePair *pair)
{
  if (pair && pair->value.size)
    swDynamicArrayRelease(&(pair->value));
}

bool swOptionValuePairSetDefaults(swOptionValuePair *pair)
{
  bool rtn = false;
  if (pair)
  {
    if (pair->option->defaultValue.count && !pair->value.count)
      rtn = swDynamicArraySetFromStaticArray(&(pair->value), &(pair->option->defaultValue));
    else
      rtn = true;
  }
  return rtn;
}

bool swOptionValuePairCheckArrays(swOptionValuePair *pair)
{
  bool rtn = false;
  if (pair)
  {
    if (pair->value.count && pair->option->valueCount)
      rtn = (pair->value.count == pair->option->valueCount);
    else
      rtn = true;
  }
  return rtn;
}
