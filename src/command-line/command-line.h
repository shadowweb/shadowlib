#ifndef SW_COMMANDLINE_COMMANDLINE_H
#define SW_COMMANDLINE_COMMANDLINE_H

#include "command-line/option.h"
#include "storage/dynamic-string.h"

#define SW_COMMANDLINE_MAGIC (0xbeefdead)

typedef enum swOptionCategoryType
{
  swOptionCategoryTypeModule,
  swOptionCategoryTypeMain,
} swOptionCategoryType;

typedef struct swOptionCategory
{
  char *name;
  char *file;
  swOption *options;
  uint32_t magic;
  swOptionCategoryType type : 1;
} swOptionCategory;

bool swOptionCommandLineInit(int argc, const char *argv[], const char *usageMessage, swDynamicString **errorString);
void swOptionCommandLineShutdown();

void swOptionCommandLinePrintUsage();

#define swOptionCategoryModuleDeclare(varName, n, ...) \
static const swOptionCategory varName __attribute__ ((unused, section(".commandline"))) = \
{ \
  .name = n, \
  .file = __FILE__, \
  .options = (swOption []){ __VA_ARGS__, {.valueType = swOptionValueTypeNone}}, \
  .magic = SW_COMMANDLINE_MAGIC, \
  .type = swOptionCategoryTypeModule \
}

#define swOptionCategoryMainDeclare(varName, n, ...) \
static const swOptionCategory varName __attribute__ ((unused, section(".commandline"))) = \
{ \
  .name = n, \
  .file = __FILE__, \
  .options = (swOption []){ __VA_ARGS__, {.valueType = swOptionValueTypeNone}}, \
  .magic = SW_COMMANDLINE_MAGIC, \
  .type = swOptionCategoryTypeMain \
}

bool swOptionValueGetBool   (swStaticString *name, bool *value);
bool swOptionValueGetInt    (swStaticString *name, int64_t *value);
bool swOptionValueGetDouble (swStaticString *name, double *value);
bool swOptionValueGetString (swStaticString *name, swStaticString *value);

bool swOptionValueGetBoolArray   (swStaticString *name, swStaticArray *value);
bool swOptionValueGetIntArray    (swStaticString *name, swStaticArray *value);
bool swOptionValueGetDoubleArray (swStaticString *name, swStaticArray *value);
bool swOptionValueGetStringArray (swStaticString *name, swStaticArray *value);

bool swPositionalOptionValueGetBool   (uint32_t position, bool *value);
bool swPositionalOptionValueGetInt    (uint32_t position, int64_t *value);
bool swPositionalOptionValueGetDouble (uint32_t position, double *value);
bool swPositionalOptionValueGetString (uint32_t position, swStaticString *value);

bool swPositionalOptionValueGetBoolArray   (uint32_t position, swStaticArray *value);
bool swPositionalOptionValueGetIntArray    (uint32_t position, swStaticArray *value);
bool swPositionalOptionValueGetDoubleArray (uint32_t position, swStaticArray *value);
bool swPositionalOptionValueGetStringArray (uint32_t position, swStaticArray *value);

bool swSinkOptionValueGetBoolArray   (swStaticArray *value);
bool swSinkOptionValueGetIntArray    (swStaticArray *value);
bool swSinkOptionValueGetDoubleArray (swStaticArray *value);
bool swSinkOptionValueGetStringArray (swStaticArray *value);

bool swConsumeAfterOptionValueGetBoolArray   (swStaticArray *value);
bool swConsumeAfterOptionValueGetIntArray    (swStaticArray *value);
bool swConsumeAfterOptionValueGetDoubleArray (swStaticArray *value);
bool swConsumeAfterOptionValueGetStringArray (swStaticArray *value);

#endif // SW_COMMANDLINE_COMMANDLINE_H

