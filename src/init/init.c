#include "init/init.h"

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
          printf ("Initializing: %s\n", dataPtr->name);
        if (!dataPtr->startFunc())
        {
          if (dataPtr->name)
            printf ("Initialization failure: %s\n", dataPtr->name);
          break;
        }
        if (dataPtr->name)
          printf ("Initialization successful: %s\n", dataPtr->name);
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
              printf ("Tearing down: %s\n", dataPtr->name);
            dataPtr->stopFunc();
            if (dataPtr->name)
              printf ("Tearing down done: %s\n", dataPtr->name);
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
            printf ("Tearing down: %s\n", dataPtr->name);
          dataPtr->stopFunc();
          if (dataPtr->name)
            printf ("Tearing down done: %s\n", dataPtr->name);
        }
        i--;
      } while (i > 0);
    }
  }
}
