#ifndef SW_COMMANDLINE_COMMANDLINE_H
#define SW_COMMANDLINE_COMMANDLINE_H

#include "collections/static-array.h"
#include "storage/static-string.h"

#define SW_COMMANDLINE_MAGIC (0xbeefdead)

typedef enum swOptionValueType
{
  swOptionValueTypeNone,
  swOptionValueTypeBool,
  swOptionValueTypeString,
  swOptionValueTypeInt,  // 64 bits
  swOptionValueTypeDouble,
  swOptionValueTypeMax,
} swOptionValueType;

typedef enum swOptionType
{
  swOptionTypeNormal,
  swOptionTypePositional,
  swOptionTypeConsumeAfter,
  swOptionTypeSink
} swOptionType;

typedef enum swOptionArrayType
{
  swOptionArrayTypeSimple,
  swOptionArrayTypeMultiValue,
  swOptionArrayTypeCommaSeparated
} swOptionArrayType;

typedef enum swOptionModifier
{
  swOptionModifierNone,
  swOptionModifierPrefix,
  swOptionModifierGrouping,
} swOptionModifier;

// typedef struct swOption swOption;
// typedef bool (*swOptionValidatorFunction)(swOption *option);

// TODO: need a way of specifying external storage
// TODO: need a way to specify aliases
// TODO: provide access functions to option values
// TODO: consider spliting the names into -short (one letter) and --long names
//       (bool short options can do grouping automatically,
//        without specifying it explicitly)
// TODO: add custom validator
// TODO: add custom parser
// TODO: add enums
// TODO: add bit vectors

typedef struct swOption
{
  swStaticString name;
  char *description;
  char *valueDescription;
  swStaticArray defaultValue;
  uint16_t valueCount;  // for Array, 0 unlimited count
  swOptionValueType valueType   : 3;
  // option type:
  //    normal -- define in any module
  //    positional, sink, consume after -- define in main module (once per program)
  // positional: does not start with '-' or '--', processed in defined order
  // sink: should be array, no more than one
  // consume after: should be array, collected after positional, no more than one
  swOptionType      optionType  : 2;
  // array option input:
  //    normal, multi value, comma separated
  swOptionArrayType arrayType   : 2;
  // normal option modifier:
  //    none, prefix, grouping
  // prefix: if array, can't be multi value or comma separated; normal option type only
  // grouping: can't be array; single letter option, normal option type, boolean value type
  //          that can be grouped with other option with a single '-'

  swOptionModifier  modifier    : 2;
  // option size:
  //    single value, array (valueCount=0 -- any)
  //                        (valueCount>0 -- expect so many values for this option)
  // if required, should have at least one element
  unsigned isArray    : 1;
  // option mandatory:
  //    optional, required
  unsigned isRequired : 1;

} swOption;

#define swOptionScalarDeclareNormal(n, d, vd, def, vt, mod, r)\
{                                                             \
  .name = swStaticStringDefine(n),                            \
  .description = d,                                           \
  .valueDescription = vd,                                     \
  .defaultValue = def,                                        \
  .valueCount = 0,                                            \
  .valueType = (vt),                                          \
  .optionType = swOptionTypeNormal,                           \
  .arrayType = swOptionArrayTypeSimple,                       \
  .modifier = (mod),                                          \
  .isArray = false,                                           \
  .isRequired = (r)                                           \
}

#define swOptionScalarDeclareGrouping(n, d, vd, def, r)       \
{                                                             \
  .name = swStaticStringDefineFromCstr(n),                    \
  .description = d,                                           \
  .valueDescription = vd,                                     \
  .defaultValue = def,                                        \
  .valueCount = 0,                                            \
  .valueType = swOptionValueTypeBool,                         \
  .optionType = swOptionTypeNormal,                           \
  .arrayType = swOptionArrayTypeSimple,                       \
  .modifier = swOptionModifierGrouping,                       \
  .isArray = false,                                           \
  .isRequired = (r)                                           \
}

#define swOptionScalarDeclarePositional(d, vd, def, vt, mod, r)  \
{                                                             \
  .name = swStaticStringDefineEmpty,                          \
  .description = #d,                                          \
  .valueDescription = #vd,                                    \
  .defaultValue = (def),                                      \
  .valueCount = 0,                                            \
  .valueType = (vt),                                          \
  .optionType = swOptionTypePositional,                       \
  .arrayType = swOptionArrayTypeSimple,                       \
  .modifier = (mod),                                          \
  .isArray = false,                                           \
  .isRequired = (r)                                           \
}

#define swOptionArrayDeclareNormal(n, d, vd, def, vc, vt, at, mod, r)  \
{                                                             \
  .name = swStaticStringDefineFromCstr(#n),                   \
  .description = #d,                                          \
  .valueDescription = #vd,                                    \
  .defaultValue = (def),                                      \
  .valueCount = (vc),                                         \
  .valueType = (vt),                                          \
  .optionType = swOptionTypeNormal,                           \
  .arrayType = (at),                                          \
  .modifier = (mod),                                          \
  .isArray = true,                                            \
  .isRequired = (r)                                           \
}

#define swOptionArrayDeclarePositional(d, vd, def, vc, vt, mod, r)  \
{                                                             \
  .name = swStaticStringDefineEmpty,                          \
  .description = #d,                                          \
  .valueDescription = #vd,                                    \
  .defaultValue = (def),                                      \
  .valueCount = (vc),                                         \
  .valueType = (vt),                                          \
  .optionType = swOptionTypePositional,                       \
  .arrayType = swOptionArrayTypeCommaSeparated,               \
  .modifier = (mod),                                          \
  .isArray = true,                                            \
  .isRequired = (r)                                           \
}

#define swOptionDeclareConsumeAfter(d, vd, def, vc, vt, mod, r)  \
{                                                             \
  .name = swStaticStringDefineEmpty,                          \
  .description = #d,                                          \
  .valueDescription = #vd,                                    \
  .defaultValue = (def),                                      \
  .valueCount = (vc),                                         \
  .valueType = (vt),                                          \
  .optionType = swOptionTypeConsumeAfter,                     \
  .arrayType = swOptionArrayTypeSimple,                       \
  .modifier = (mod),                                          \
  .isArray = true,                                            \
  .isRequired = (r)                                           \
}


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

bool swOptionCommandLineInit(int argc, const char *argv[]);
void swOptionCommandLineShutdown();

bool swOptionCommandLineSetUsage(const char *usageMessage);
void swOptionCommandLinePrintUsage();

/*
#define swOptionDeclareSimple(name, d, t, e, v, f, m, validator, data) \
{ \
  .name = #name, \
  .description = (d), \
  .type = (t), \
  .expected = e, \
  .visibility = v, \
  .formating = f, \
  .misc = m, \
  .validator = validator, \
  _Generic((data), \
    bool: .boolPtr, \
    swStaticString: .stringPtr, \
    int64_t: .signedIntPtr, \
    uint64_t: .unsignedIntPtr, \
    float: .floatPtr, \
    double: .doublePtr, \
    swStaticArray: .arrayPtr \
  ) = (data), \
  .isArray = _Generic ((data), \
      swStaticArray: true, \
      default: false \
    ) \
}
// .data = (void *)data,
*/

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

#endif // SW_COMMANDLINE_COMMANDLINE_H

