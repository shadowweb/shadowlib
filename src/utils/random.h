#ifndef SW_UTILS_RANDOM_H
#define SW_UTILS_RANDOM_H

#include "storage/dynamic-buffer.h"

bool swUtilFillRandom(swDynamicBuffer *buffer, size_t bytesNeeded);
bool swUtilFillURandom(swDynamicBuffer *buffer, size_t bytesNeeded);

#endif  // SW_UTIL_RANDOM_H
