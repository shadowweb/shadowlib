#ifndef SW_COMMANDLINE_COMMANDLINESTATE_H
#define SW_COMMANDLINE_COMMANDLINESTATE_H

#include "command-line/command-line-data.h"
#include "command-line/option-token.h"

typedef struct swCommandLineState
{
  swCommandLineData    *clData;
  const char          **argv;
  swOptionToken        *tokens;
  uint32_t              argCount;
  uint32_t              currentArg;
  uint32_t              currentPositional;
} swCommandLineState;

bool swCommandLineStateTokenize(swCommandLineState *state);
bool swCommandLineStateScanArguments(swCommandLineState *state);

#endif // SW_COMMANDLINE_COMMANDLINESTATE_H