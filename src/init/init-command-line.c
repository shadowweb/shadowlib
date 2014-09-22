#include "init/init-command-line.h"

#include "command-line/command-line.h"

#include <stdio.h>

static void *commandLineArrayData[4] = {NULL};

static bool swInitCommandLineStart()
{
  bool rtn = false;
  int argc = *(int *)commandLineArrayData[0];
  char **argv = (char **)commandLineArrayData[1];
  char *title = (char *)commandLineArrayData[2];
  char *usageMessage = (char *)commandLineArrayData[3];

  swDynamicString *errorString = NULL;
  if (swCommandLineInit(argc, (const char **)argv, title, usageMessage, &errorString))
    rtn = true;
  else if (errorString)
  {
    printf("Error processing arguments: '%.*s'\n", (int)(errorString->len), errorString->data);
    swDynamicStringDelete(errorString);
  }
  return rtn;
}

static void swCommandLineStop()
{
  swCommandLineShutdown();
}

static swInitData commandLineData = {.startFunc = swInitCommandLineStart, .stopFunc = swCommandLineStop, .name = "Command Line"};

swInitData *swInitCommandLineDataGet(int *argc, char **argv, const char *title, const char *usageMessage)
{
  commandLineArrayData[0] = argc;
  commandLineArrayData[1] = argv;
  commandLineArrayData[2] = (void *)title;
  commandLineArrayData[3] = (void *)usageMessage;
  return &commandLineData;
}