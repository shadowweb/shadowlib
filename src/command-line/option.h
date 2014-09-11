#ifndef SW_COMMANDLINE_OPTION_H
#define SW_COMMANDLINE_OPTION_H

#include "collections/dynamic-array.h"
#include "collections/static-array.h"
#include "storage/static-string.h"

typedef enum swOptionValueType
{
  swOptionValueTypeNone,
  swOptionValueTypeBool,
  swOptionValueTypeString,
  swOptionValueTypeInt,  // 64 bits
  swOptionValueTypeDouble,
  swOptionValueTypeMax,
} swOptionValueType;

// TODO: this whole thing can be get rid of
// all we need here is bool flag to indicate that the option is positional
// use default sink and consume after (use consume after only if we have positional arguments)
// basically, no need to explicitly specify those, no need to worry about them not being present,
// less error checking
typedef enum swOptionType
{
  swOptionTypeNormal,
  swOptionTypePositional
} swOptionType;

typedef enum swOptionArrayType
{
  swOptionArrayTypeSimple,
  swOptionArrayTypeMultiValue,
  swOptionArrayTypeCommaSeparated
} swOptionArrayType;

// typedef struct swOption swOption;
// typedef bool (*swOptionValidatorFunction)(swOption *option);

// TODO: need a new array type to collect everything between two options
// TODO: need a way of specifying external storage
// TODO: need a way to specify aliases


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
  // TODO: aliases can be done by using '|'-character in the name, would need to dynamicaly allocate slices
  // for alias names and have them associated with the option
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

  // grouping: can't be array; single letter option, normal option type, boolean value type
  //          that can be grouped with other option with a single '-'

  // option size:
  //    single value, array (valueCount=0 -- any)
  //                        (valueCount>0 -- expect so many values for this option)
  // if required, should have at least one element
  unsigned isArray    : 1;
  // option mandatory:
  //    optional, required
  unsigned isRequired : 1;
  // prefix: if array, can't be multi value or comma separated; normal option type only
  unsigned isPrefix : 1;

} swOption;

#define swOptionDeclare(n, d, vd, def, vc, vt, ot, at, a, r, p) \
{                                                                 \
  .name = n,                                                      \
  .description = d,                                               \
  .valueDescription = vd,                                         \
  .defaultValue = def,                                            \
  .valueCount = (vc),                                             \
  .valueType = (vt),                                              \
  .optionType = (ot),                                             \
  .arrayType = (at),                                              \
  .isArray = (a),                                                 \
  .isRequired = (r),                                              \
  .isPrefix = (p)                                                 \
}

#define swOptionDeclareScalar(n, d, vd, vt, r)                            swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, 0, vt, swOptionTypeNormal, swOptionArrayTypeSimple, false, r, false)
#define swOptionDeclareScalarWithDefault(n, d, vd, def, vt, r)            swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, 0, vt, swOptionTypeNormal, swOptionArrayTypeSimple, false, r, false)

#define swOptionDeclareArray(n, d, vd, vc, vt, at, r)                     swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, vc, vt, swOptionTypeNormal, at, true, r, false)
#define swOptionDeclareArrayWithDefault(n, d, vd, def, vc, vt, at, r)     swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, vc, vt, swOptionTypeNormal, at, true, r, false)

#define swOptionDeclarePrefixScalar(n, d, vd, vt, r)                      swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, 0, vt, swOptionTypeNormal, swOptionArrayTypeSimple, false, r, true)
#define swOptionDeclarePrefixScalarWithDefault(n, d, vd, def, vt, r)      swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, 0, vt, swOptionTypeNormal, swOptionArrayTypeSimple, false, r, true)

#define swOptionDeclarePrefixArray(n, d, vd, vc, vt, r)                   swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, vc, vt, swOptionTypeNormal, swOptionArrayTypeSimple, true, r, true)
#define swOptionDeclarePrefixArrayWithDefault(n, d, vd, def, vc, vt, r)   swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, vc, vt, swOptionTypeNormal, swOptionArrayTypeSimple, true, r, true)

#define swOptionDeclarePositionalScalar(d, vd, vt, r)                     swOptionDeclare(swStaticStringDefineEmpty, d, vd, swStaticArrayDefineEmpty, 0, vt, swOptionTypePositional, swOptionArrayTypeSimple, false, r, false)
#define swOptionDeclarePositionalScalarWithDefault(d, vd, def, vt, r)     swOptionDeclare(swStaticStringDefineEmpty, d, vd,                      def, 0, vt, swOptionTypePositional, swOptionArrayTypeSimple, false, r, false)

#define swOptionDeclarePositionalArray(d, vd, vc, vt, r)                  swOptionDeclare(swStaticStringDefineEmpty, d, vd, swStaticArrayDefineEmpty, vc, vt, swOptionTypePositional, swOptionArrayTypeCommaSeparated, true, r, false)
#define swOptionDeclarePositionalArrayWithDefault(d, vd, def, vc, vt, r)  swOptionDeclare(swStaticStringDefineEmpty, d, vd,                      def, vc, vt, swOptionTypePositional, swOptionArrayTypeCommaSeparated, true, r, false)

bool swOptionValidateDefaultValue(swOption *option);
bool swOptionCallParser(swOption *option, swStaticString *valueString, swDynamicArray *valueArray);
size_t swOptionValueTypeSizeGet(swOptionValueType type);

extern bool trueValue;
extern bool falseValue;


#endif // SW_COMMANDLINE_OPTION_H