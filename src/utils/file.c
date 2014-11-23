#include "file.h"

#include "core/memory.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

size_t swFileRead(const char *fileName, char **data)
{
  size_t rtn = 0;
  if (fileName && data)
  {
    FILE *fd = NULL;
    if ((fd = fopen(fileName, "r")))
    {
      struct stat file_stat;
      if ((fstat(fileno(fd), &file_stat) == 0) && file_stat.st_size)
      {
        *data = swMemoryMalloc(file_stat.st_size);
        if (*data)
        {
          size_t bytes_read = fread(*data, 1, file_stat.st_size, fd);
          if (bytes_read == (size_t)file_stat.st_size)
            rtn = bytes_read;
          else
          {
            swMemoryFree(*data);
            *data = NULL;
          }
        }
      }
      fclose(fd);
    }
  }
  return rtn;
}

bool swFileRealPath(const char *fileName, swDynamicString *resolvedFileName)
{
  bool rtn = false;
  if (fileName && resolvedFileName)
  {
    char realPathCString[PATH_MAX];
    if (realpath(fileName, realPathCString))
    {
      if (swDynamicStringSetFromCString(resolvedFileName, realPathCString))
        rtn = true;
    }
  }
  return rtn;
}

size_t swFileGetSize(const char *fileName)
{
  size_t rtn = 0;
  if (fileName)
  {
    struct stat st;
    if (stat(fileName, &st) == 0)
      rtn = st.st_size;
  }
  return rtn;
}
