#include "command-line/command-line-state.h"

bool swCommandLineStateTokenize(swCommandLineState *state)
{
  bool rtn = false;
  if (state)
  {
    uint32_t i = 0;
    for (; i < state->argCount; i++)
    {
      if (!swOptionTokenSet(&(state->tokens[i]), state->argv[i], state->clData->namedValues, state->clData->prefixedValues, &(state->clData->errorData)))
        break;
    }
    if (i == state->argCount)
      rtn = true;
  }
  return rtn;
}

static bool swCommandLineStateScanConsumeAfter(swCommandLineState *state)
{
  bool rtn = true;
  while (state->currentArg < state->argCount)
  {
    swOptionToken *token = &state->tokens[state->currentArg];
    if (swDynamicArrayPush(&(state->clData->consumeAfterValue), &(token->full)))
    {
      state->currentArg++;
      continue;
    }
    swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
    rtn = false;
    break;
  }
  return rtn;
}

static bool swCommandLineStateScanPositional(swCommandLineState *state)
{
  bool rtn = false;
  if ((state->currentPositional < state->clData->positionalValues.count) && (state->currentArg < state->argCount))
  {
    swOptionValuePair *pair = swFastArrayGetPtr(state->clData->positionalValues, state->currentPositional, swOptionValuePair);
    if (pair)
    {
      swOptionToken *token = &state->tokens[state->currentArg];
      swOption *option = pair->option;
      if (swOptionCallParser(option, &(token->full), &(pair->value), state->clData->enumValues, &(option->name)))
      {
        state->currentArg++;
        state->currentPositional++;
        rtn = true;
      }
      else
        swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeParse);
    }
    else
      swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
  }
  return rtn;
}

static bool swCommandLineStateScanAllPositional(swCommandLineState *state)
{
  bool rtn = false;
  if (state)
  {
    if (state->currentArg < state->argCount)
    {
      bool failure = false;
      while ((state->currentPositional < state->clData->positionalValues.count) && (state->currentArg < state->argCount))
      {
        if (swCommandLineStateScanPositional(state))
          continue;
        failure = true;
        break;
      }
      if (!failure)
      {
        if (state->currentArg == state->argCount)
          rtn = true;
        else if (state->currentPositional == state->clData->positionalValues.count)
          rtn = swCommandLineStateScanConsumeAfter(state);
      }
      else if (!state->clData->positionalValues.count)
        swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeNoPositional);
    }
    else
      rtn = true;
  }
  return rtn;
}

static bool swCommandLineStateScanSink(swCommandLineState *state)
{
  bool rtn = false;
  swOptionToken *token = &state->tokens[state->currentArg];
  if (swDynamicArrayPush(&(state->clData->sinkValue), &(token->full)))
  {
    state->currentArg++;
    rtn = true;
  }
  else
    swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
  return rtn;
}

static bool swCommandLineStateExtractEnumName(swCommandLineState *state, swOptionToken *token, swOption *option)
{
  bool rtn = false;
  if (option->valueType == swOptionValueTypeEnum)
  {
    int64_t *value = NULL;
    if (swHashMapLinearValueGet(state->clData->enumNames, &(token->name), (void **)&value))
      rtn = swOptionValuePairValueSet(&(token->namePair->value), *value, option->isArray);
  }
  return rtn;
}

bool swCommandLineStateScanArguments(swCommandLineState *state)
{
  bool rtn = false;
  if (state)
  {
    swOptionToken *token = &state->tokens[state->currentArg];
    if (token->hasName)
    {
      // process named option (with or without prefix)
      if (token->namePair)
      {
        bool isMultiValueArray = token->namePair->option->isArray && token->namePair->option->arrayType == swOptionArrayTypeMultiValue;
        swOption *option = token->namePair->option;
        if (token->hasValue)
        {
          if (!isMultiValueArray)
          {
            if (option->valueType != swOptionValueTypeEnum || !swHashMapLinearValueGet(state->clData->enumNames, &(token->name), NULL))
            {
              if (swOptionCallParser(option, &(token->value), &(token->namePair->value), state->clData->enumValues, &(token->name)))
              {
                state->currentArg++;
                rtn = true;
              }
              else
                swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeParse);
            }
            else
              swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeValueAfterEnumName);
          }
          else
            swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeArrayMultivalue);
        }
        else if (swCommandLineStateExtractEnumName(state, token, option))
        {
          state->currentArg++;
          rtn = true;
        }
        else
        {
          swOptionToken *nextToken = ((state->currentArg + 1) < state->argCount)? &state->tokens[state->currentArg + 1] : NULL;
          if (nextToken && !nextToken->hasName && !nextToken->hasDashDashOnly)
          {
            state->currentArg++;
            do
            {
              if (swOptionCallParser(option, &(nextToken->value), &(token->namePair->value), state->clData->enumValues, &(token->name)))
              {
                state->currentArg++;
                rtn = true;
                if (isMultiValueArray)
                {
                  nextToken = (state->currentArg < state->argCount)? &state->tokens[state->currentArg] : NULL;
                  continue;
                }
              }
              else
              {
                swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeParse);
                rtn = false;
              }
              break;
            }
            while (nextToken && !nextToken->hasName && !nextToken->hasDashDashOnly);
          }
          else if (option->valueType == swOptionValueTypeBool && option->arrayType == swOptionArrayTypeSimple)
          {
            if (swOptionValuePairValueSet(&(token->namePair->value), trueValue, option->isArray))
            {
              state->currentArg++;
              rtn = true;
            }
            else
              swCommandLineErrorDataSet(&(state->clData->errorData), option, NULL, swCommandLineErrorCodeInternal);
          }
        }
      }
      else if (token->noNamePair && token->noNamePair->option->valueType == swOptionValueTypeBool && token->noNamePair->option->arrayType == swOptionArrayTypeSimple)
      {
        if (swOptionValuePairValueSet(&(token->noNamePair->value), falseValue, token->noNamePair->option->isArray))
        {
          state->currentArg++;
          rtn = true;
        }
        else
          swCommandLineErrorDataSet(&(state->clData->errorData), token->noNamePair->option, NULL, swCommandLineErrorCodeInternal);
      }
      else
      {
        // process named options that allow grouping
        swOptionValuePair *pairs[token->name.len];
        memset(pairs, 0, sizeof(pairs));
        swStaticString nameSubstring = swStaticStringDefineEmpty;
        size_t position = 0;
        bool failure = false;
        if (!token->hasDoubleDash)
        {
          while (position < token->name.len)
          {
            if (swStaticStringSetSubstring(&(token->name), &nameSubstring, position, position + 1))
            {
              if (swHashMapLinearValueGet(state->clData->namedValues, &nameSubstring, (void **)(&pairs[position])))
              {
                swOption *option = pairs[position]->option;
                if (option->isArray || option->valueType != swOptionValueTypeBool)
                  break;
              }
              else
                break;
            }
            else // substring failure
            {
              swCommandLineErrorDataSet(&(state->clData->errorData), NULL, NULL, swCommandLineErrorCodeInternal);
              failure = true;
              break;
            }
            position++;
          }
        }
        if (position == token->name.len)
        {
          position = 0;
          while (position < token->name.len)
          {
            if (!swOptionValuePairValueSet(&(pairs[position]->value), trueValue, pairs[position]->option->isArray))
            {
              swCommandLineErrorDataSet(&(state->clData->errorData), pairs[position]->option, NULL, swCommandLineErrorCodeInternal);
              break;
            }
            position++;
          }
          if (position == token->name.len)
          {
            state->currentArg++;
            rtn = true;
          }
        }
        else if (!failure)
          rtn = swCommandLineStateScanSink(state);
      }
    }
    else // token does not have name
    {
      if (token->hasDashDashOnly)
      {
        state->currentArg++;
        // consume positional only
        rtn = swCommandLineStateScanAllPositional(state);
      }
      else if (state->currentPositional < state->clData->positionalValues.count)
        rtn = swCommandLineStateScanPositional(state);
      else if (state->clData->positionalValues.count)
        rtn = swCommandLineStateScanConsumeAfter(state);
      else
        rtn = swCommandLineStateScanSink(state);
    }
  }
  return rtn;
}
