#include "init/init.h"
#include "utils/colors.h"

#include <stdint.h>
#include <stdio.h>

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
          printf ("%sInitializing:%s %s\n", SW_COLOR_ANSI_GREEN, SW_COLOR_ANSI_NORMAL, dataPtr->name);
        if (!dataPtr->startFunc())
        {
          if (dataPtr->name)
            printf ("%sInitialization failure:%s %s\n", SW_COLOR_ANSI_RED, SW_COLOR_ANSI_NORMAL, dataPtr->name);
          break;
        }
        if (dataPtr->name)
          printf ("%sInitialization successful:%s %s\n", SW_COLOR_ANSI_GREEN, SW_COLOR_ANSI_NORMAL, dataPtr->name);
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
              printf ("%sTearing down:%s %s\n", SW_COLOR_ANSI_RED, SW_COLOR_ANSI_NORMAL, dataPtr->name);
            dataPtr->stopFunc();
            if (dataPtr->name)
              printf ("%sTearing down done:%s %s\n", SW_COLOR_ANSI_RED, SW_COLOR_ANSI_NORMAL, dataPtr->name);
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
            printf ("%sTearing down:%s %s\n", SW_COLOR_ANSI_BLUE, SW_COLOR_ANSI_NORMAL, dataPtr->name);
          dataPtr->stopFunc();
          if (dataPtr->name)
            printf ("%sTearing down done:%s %s\n", SW_COLOR_ANSI_BLUE, SW_COLOR_ANSI_NORMAL, dataPtr->name);
        }
        i--;
      } while (i > 0);
    }
  }
}
