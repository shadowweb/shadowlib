#ifndef SW_INIT_INIT_H
#define SW_INIT_INIT_H

#include <stdbool.h>

typedef bool (*swInitStartFunction)();
typedef void (*swInitStopFunction)();

typedef struct swInitData
{
  swInitStartFunction startFunc;
  swInitStopFunction  stopFunc;
  char *name;
} swInitData;

bool swInitStart  (swInitData *data[]);
void swInitStop   (swInitData *data[]);

#endif // SW_INIT_INIT_H
