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
    if (pair->option->defaultValue && !pair->value.count)
      rtn = swDynamicArraySetFromStaticArray(&(pair->value), pair->option->defaultValue);
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

bool swOptionValuePairSetExternal(swOptionValuePair *pair)
{
  bool rtn = false;
  if (pair)
  {
    swOption *option = pair->option;
    if (option->external && pair->value.count)
    {
      if (option->isArray)
        *(swStaticArray *)(option->external) = *(swStaticArray *)&(pair->value);
      else
      {
        switch (option->valueType)
        {
          case swOptionValueTypeBool:
            *(bool *)(option->external) = *(bool *)(pair->value.data);
            break;
          case swOptionValueTypeInt:
          case swOptionValueTypeEnum:
            *(int64_t *)(option->external) = *(int64_t *)(pair->value.data);
            break;
          case swOptionValueTypeDouble:
            *(double *)(option->external) = *(double *)(pair->value.data);
            break;
          case swOptionValueTypeString:
            *(swStaticString *)(option->external) = *(swStaticString *)(pair->value.data);
            break;
        }
      }
    }
    rtn = true;
  }
  return rtn;
}
