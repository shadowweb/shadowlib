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

#define swOptionDeclare(n, d, vd, def, vc, vt, ot, at, mod, a, r) \
{                                                                 \
  .name = n,                                                      \
  .description = d,                                               \
  .valueDescription = vd,                                         \
  .defaultValue = def,                                            \
  .valueCount = (vc),                                             \
  .valueType = (vt),                                              \
  .optionType = (ot),                                             \
  .arrayType = (at),                                              \
  .modifier = (mod),                                              \
  .isArray = (a),                                                 \
  .isRequired = (r)                                               \
}

#define swOptionDeclareScalar(n, d, vd, vt, r)                            swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, 0, vt, swOptionTypeNormal, swOptionArrayTypeSimple, swOptionModifierNone, false, r)
#define swOptionDeclareScalarWithDefault(n, d, vd, def, vt, r)            swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, 0, vt, swOptionTypeNormal, swOptionArrayTypeSimple, swOptionModifierNone, false, r)

#define swOptionDeclareArray(n, d, vd, vc, vt, at, r)                     swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, vc, vt, swOptionTypeNormal, at, swOptionModifierNone, true, r)
#define swOptionDeclareArrayWithDefault(n, d, vd, def, vc, vt, at, r)     swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, vc, vt, swOptionTypeNormal, at, swOptionModifierNone, true, r)

#define swOptionDeclarePrefixScalar(n, d, vd, vt, r)                      swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, 0, vt, swOptionTypeNormal, swOptionArrayTypeSimple, swOptionModifierPrefix, false, r)
#define swOptionDeclarePrefixScalarWithDefault(n, d, vd, def, vt, r)      swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, 0, vt, swOptionTypeNormal, swOptionArrayTypeSimple, swOptionModifierPrefix, false, r)

#define swOptionDeclarePrefixArray(n, d, vd, vc, vt, r)                   swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, vc, vt, swOptionTypeNormal, swOptionArrayTypeSimple, swOptionModifierPrefix, true, r)
#define swOptionDeclarePrefixArrayWithDefault(n, d, vd, def, vc, vt, r)   swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, vc, vt, swOptionTypeNormal, swOptionArrayTypeSimple, swOptionModifierPrefix, true, r)

#define swOptionDeclareGrouping(n, d, vd, r)                              swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, 0, swOptionValueTypeBool, swOptionTypeNormal, swOptionArrayTypeSimple, swOptionModifierGrouping, false, r)
#define swOptionDeclareGroupingWithDefault(n, d, vd, def, r)              swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, 0, swOptionValueTypeBool, swOptionTypeNormal, swOptionArrayTypeSimple, swOptionModifierGrouping, false, r)

#define swOptionDeclarePositionalScalar(d, vd, vt, r)                     swOptionDeclare(swStaticStringDefineEmpty, d, vd, swStaticArrayDefineEmpty, 0, vt, swOptionTypePositional, swOptionArrayTypeSimple, swOptionModifierNone, false, r)
#define swOptionDeclarePositionalScalarWithDefault(d, vd, def, vt, r)     swOptionDeclare(swStaticStringDefineEmpty, d, vd,                      def, 0, vt, swOptionTypePositional, swOptionArrayTypeSimple, swOptionModifierNone, false, r)

#define swOptionDeclarePositionalArray(d, vd, vc, vt, r)                  swOptionDeclare(swStaticStringDefineEmpty, d, vd, swStaticArrayDefineEmpty, vc, vt, swOptionTypePositional, swOptionArrayTypeCommaSeparated, swOptionModifierNone, true, r)
#define swOptionDeclarePositionalArrayWithDefault(d, vd, def, vc, vt, r)  swOptionDeclare(swStaticStringDefineEmpty, d, vd,                      def, vc, vt, swOptionTypePositional, swOptionArrayTypeCommaSeparated, swOptionModifierNone, true, r)

#define swOptionDeclareConsumeAfter(d, vd, vc, vt, r)                     swOptionDeclare(swStaticStringDefineEmpty, d, vd, swStaticArrayDefineEmpty, vc, vt, swOptionTypeConsumeAfter, swOptionArrayTypeSimple, swOptionModifierNone, true, r)
#define swOptionDeclareConsumeAfterWithDefault(d, vd, def, vc, vt, r)     swOptionDeclare(swStaticStringDefineEmpty, d, vd,                      def, vc, vt, swOptionTypeConsumeAfter, swOptionArrayTypeSimple, swOptionModifierNone, true, r)

#define swOptionDeclareSink(d, vd, vc, vt, r)                             swOptionDeclare(swStaticStringDefineEmpty, d, vd, swStaticArrayDefineEmpty, vc, vt, swOptionTypeSink, swOptionArrayTypeSimple, swOptionModifierNone, true, r)
#define swOptionDeclareSinkWithDefault(d, vd, def, vc, vt, r)             swOptionDeclare(swStaticStringDefineEmpty, d, vd,                      def, vc, vt, swOptionTypeSink, swOptionArrayTypeSimple, swOptionModifierNone, true, r)

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

bool swOptionCommandLineInit(int argc, const char *argv[], const char *usageMessage);
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

