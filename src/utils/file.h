#ifndef SW_UTILS_FILE_H
#define SW_UTILS_FILE_H

#include "storage/dynamic-string.h"

#include <stddef.h>

// TODO: fix swFileRead to use dynamic string
size_t swFileRead(const char *fileName, char **data);
bool swFileRealPath(const char *fileName, swDynamicString *resolvedFileName);

#endif // SW_UTILS_FILE_H
