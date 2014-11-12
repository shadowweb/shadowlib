#include "init/init.h"

#include "log/log-manager.h"
#include "utils/colors.h"

#include <stdint.h>
#include <stdio.h>

swLoggerDeclareWithLevel(initLogger, "Init", swLogLevelInfo);

bool swInitStart (swInitData *data[])
{
  bool rtn = false;
  if (data)
  {
    uint32_t i = 0;
    while (data[i])
    {
      swInitData *dataPtr = data[i];
      if (dataPtr->startFunc)
      {
        if (dataPtr->name)
          SW_LOG_INFO(&initLogger, "%sInitializing:%s %s", SW_COLOR_ANSI_GREEN, SW_COLOR_ANSI_NORMAL, dataPtr->name);
        if (!dataPtr->startFunc())
        {
          if (dataPtr->name)
            SW_LOG_FATAL(&initLogger, "%sInitialization failure:%s %s", SW_COLOR_ANSI_RED, SW_COLOR_ANSI_NORMAL, dataPtr->name);
          break;
        }
        if (dataPtr->name)
          SW_LOG_INFO(&initLogger, "%sInitialization successful:%s %s", SW_COLOR_ANSI_GREEN, SW_COLOR_ANSI_NORMAL, dataPtr->name);
      }
      i++;
    }
    if (!data[i])
      rtn = true;
    else
    {
      if (i > 0)
      {
        do
        {
          swInitData *dataPtr = data[i - 1];
          if (dataPtr->stopFunc)
          {
            if (dataPtr->name)
              SW_LOG_INFO(&initLogger, "%sTearing down:%s %s", SW_COLOR_ANSI_RED, SW_COLOR_ANSI_NORMAL, dataPtr->name);
            dataPtr->stopFunc();
            if (dataPtr->name)
              SW_LOG_INFO(&initLogger, "%sTearing down done:%s %s", SW_COLOR_ANSI_RED, SW_COLOR_ANSI_NORMAL, dataPtr->name);
          }
          i--;
        } while (i > 0);
      }
    }
  }
  return rtn;
}

void swInitStop (swInitData *data[])
{
  if (data)
  {
    uint32_t i = 0;
    while (data[i])
      i++;
    if (i > 0)
    {
      do
      {
        swInitData *dataPtr = data[i - 1];
        if (dataPtr->stopFunc)
        {
          if (dataPtr->name)
            SW_LOG_INFO(&initLogger, "%sTearing down:%s %s", SW_COLOR_ANSI_BLUE, SW_COLOR_ANSI_NORMAL, dataPtr->name);
          dataPtr->stopFunc();
          if (dataPtr->name)
            SW_LOG_INFO(&initLogger, "%sTearing down done:%s %s", SW_COLOR_ANSI_BLUE, SW_COLOR_ANSI_NORMAL, dataPtr->name);
        }
        i--;
      } while (i > 0);
    }
  }
}
