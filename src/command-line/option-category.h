#ifndef SW_COMMANDLINE_OPTIONCATEGORY_H
#define SW_COMMANDLINE_OPTIONCATEGORY_H

#include "command-line/option.h"

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

#endif // SW_COMMANDLINE_OPTIONCATEGORY_H
