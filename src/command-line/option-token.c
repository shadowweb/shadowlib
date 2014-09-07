#include "command-line/option-token.h"

bool swOptionTokenSet(swOptionToken *token, const char *argv, swHashMapLinear *namedValues, swHashMapLinear *prefixedValues, swCommandLineErrorData *errorData)
{
  bool rtn = false;
  if (token && argv && namedValues && prefixedValues && errorData)
  {
    token->argv = argv;
    token->full = swStaticStringSetFromCstr((char *)(token->argv));

    // check for '-' or '--'
    size_t startPosition = 0;
    if (swStaticStringCharEqual(token->full, startPosition, '-'))
    {
      startPosition++;
      if (swStaticStringCharEqual(token->full, startPosition, '-'))
      {
        startPosition++;
        if (token->full.len == 2)
        {
          token->hasDashDashOnly = true;
          rtn = true;
        }
      }

      if (!rtn)
      {
        if (swStaticStringSetSubstring(&(token->full), &(token->name), startPosition, token->full.len))
        {
          token->hasName = true;
          swStaticString slices[2] = {swStaticStringDefineEmpty};
          uint32_t foundSlices = 0;
          if (swStaticStringSplitChar(&(token->name), '=', slices, 2, &foundSlices, 0))
          {
            if (foundSlices == 2)
            {
              token->name = slices[0];
              token->value = slices[1];
              token->hasValue = true;
              rtn = true;
            }
            // check for 'no'
            else if (swStaticStringCharEqual(token->full, startPosition, 'n') && swStaticStringCharEqual(token->full, startPosition + 1, 'o'))
            {
              startPosition += 2;
              if (swStaticStringSetSubstring(&(token->full), &(token->noName), startPosition, token->full.len))
              {
                token->hasNoName = true;
                rtn = true;
              }
              else
                swCommandLineErrorDataSet(errorData, NULL, NULL, swCommandLineErrorCodeInternal);
            }
            else
              rtn = true;
          }
          else
            swCommandLineErrorDataSet(errorData, NULL, NULL, swCommandLineErrorCodeInternal);
        }
        else
          swCommandLineErrorDataSet(errorData, NULL, NULL, swCommandLineErrorCodeInternal);
      }
    }
    else
    {
      token->value = token->full;
      token->hasValue = true;
      rtn = true;
    }
    if (rtn && token->hasName)
    {
      if (!swHashMapLinearValueGet(namedValues, &(token->name), (void **)(&(token->namePair))) && !token->hasValue)
      {
        // find prefix name/value
        swStaticString nameSubstring = swStaticStringDefineEmpty;
        swStaticString argumentValue = swStaticStringDefineEmpty;
        size_t len = 1;
        while (len < token->name.len)
        {
          if (swStaticStringSetSubstring(&(token->name), &nameSubstring, 0, len) &&
              swStaticStringSetSubstring(&(token->name), &argumentValue, len, token->name.len))
          {
            if (swHashMapLinearValueGet(prefixedValues, &nameSubstring, (void **)(&(token->namePair))))
            {
              token->name = nameSubstring;
              token->value = argumentValue;
              token->hasValue = true;
              break;
            }
          }
          else
          {
            swCommandLineErrorDataSet(errorData, NULL, NULL, swCommandLineErrorCodeInternal);
            rtn = false;
            break;
          }
          len++;
        }
      }
    }
    if (token->hasNoName)
      swHashMapLinearValueGet(namedValues, &(token->noName), (void **)(&(token->noNamePair)));
  }
  return rtn;
}
