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

typedef enum swOptionArrayType
{
  swOptionArrayTypeSimple,
  swOptionArrayTypeMultiValue,
  swOptionArrayTypeCommaSeparated
} swOptionArrayType;

// typedef struct swOption swOption;
// typedef bool (*swOptionValidatorFunction)(swOption *option);

// TODO: need a new array type to collect everything between two options

// TODO: add custom validator
// TODO: add custom parser

// TODO: add enums
// TODO: add bit vectors

typedef struct swOption
{
  swStaticString name;            // separate aliases in the name by using '|'-character
  swDynamicArray aliases;         // dynamically allocated array of alias names
  char *description;
  char *valueDescription;
  swStaticArray defaultValue;     // default value
  void *external;                 // pointer to external storage, expected to be the following
                                  // swOptionValueTypeBool    => bool *
                                  // swOptionValueTypeString  => swStaticString *
                                  // swOptionValueTypeInt     => int64_t *
                                  // swOptionValueTypeDouble  => double *

  uint16_t valueCount;            // for Array, 0 unlimited count
  swOptionValueType valueType   : 3;
  // array option input:
  //    normal, multi value, comma separated
  swOptionArrayType arrayType   : 2;

  // grouping: can't be array; single letter option, normal option type, boolean value type
  //          that can be grouped with other option with a single '-'

  //  if array, can have more than one element
  //    (valueCount=0 -- any number of elements)
  //    (valueCount>0 -- expect so many values for this option)
  unsigned isArray    : 1;
  // if required, should have at least one element
  unsigned isRequired : 1;
  // prefix: if array, can't be multi value or comma separated; normal option type only
  unsigned isPrefix : 1;
  // positional: does not start with '-' or '--', processed in defined order
  unsigned isPositional : 1;
} swOption;

#define swOptionDeclare(n, d, vd, def, e, vc, vt, at, a, r, p, pos)  \
{                                                                    \
  .name = n,                                                         \
  .description = d,                                                  \
  .valueDescription = vd,                                            \
  .defaultValue = def,                                               \
  .external = (e),                                                   \
  .valueCount = (vc),                                                \
  .valueType = (vt),                                                 \
  .arrayType = (at),                                                 \
  .isArray = (a),                                                    \
  .isRequired = (r),                                                 \
  .isPrefix = (p),                                                   \
  .isPositional = (pos)                                              \
}

#define swOptionDeclareScalar(n, d, vd, e, vt, r)                            swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, e, 0, vt, swOptionArrayTypeSimple, false, r, false, false)
#define swOptionDeclareScalarWithDefault(n, d, vd, def, e, vt, r)            swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, e, 0, vt, swOptionArrayTypeSimple, false, r, false, false)

#define swOptionDeclareArray(n, d, vd, e, vc, vt, at, r)                     swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, e, vc, vt, at, true, r, false, false)
#define swOptionDeclareArrayWithDefault(n, d, vd, def, e, vc, vt, at, r)     swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, e, vc, vt, at, true, r, false, false)

#define swOptionDeclarePrefixScalar(n, d, vd, e, vt, r)                      swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, e, 0, vt, swOptionArrayTypeSimple, false, r, true, false)
#define swOptionDeclarePrefixScalarWithDefault(n, d, vd, def, e, vt, r)      swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, e, 0, vt, swOptionArrayTypeSimple, false, r, true, false)

#define swOptionDeclarePrefixArray(n, d, vd, e, vc, vt, r)                   swOptionDeclare(swStaticStringDefine(n), d, vd, swStaticArrayDefineEmpty, e, vc, vt, swOptionArrayTypeSimple, true, r, true, false)
#define swOptionDeclarePrefixArrayWithDefault(n, d, vd, def, e, vc, vt, r)   swOptionDeclare(swStaticStringDefine(n), d, vd,                      def, e, vc, vt, swOptionArrayTypeSimple, true, r, true, false)

#define swOptionDeclarePositionalScalar(d, vd, e, vt, r)                     swOptionDeclare(swStaticStringDefineEmpty, d, vd, swStaticArrayDefineEmpty, e, 0, vt, swOptionArrayTypeSimple, false, r, false, true)
#define swOptionDeclarePositionalScalarWithDefault(d, vd, def, e, vt, r)     swOptionDeclare(swStaticStringDefineEmpty, d, vd,                      def, e, 0, vt, swOptionArrayTypeSimple, false, r, false, true)

#define swOptionDeclarePositionalArray(d, vd, e, vc, vt, r)                  swOptionDeclare(swStaticStringDefineEmpty, d, vd, swStaticArrayDefineEmpty, e, vc, vt, swOptionArrayTypeCommaSeparated, true, r, false, true)
#define swOptionDeclarePositionalArrayWithDefault(d, vd, def, e, vc, vt, r)  swOptionDeclare(swStaticStringDefineEmpty, d, vd,                      def, e, vc, vt, swOptionArrayTypeCommaSeparated, true, r, false, true)

bool swOptionValidateDefaultValue(swOption *option);
bool swOptionCallParser(swOption *option, swStaticString *valueString, swDynamicArray *valueArray);
size_t swOptionValueTypeSizeGet(swOptionValueType type);
char *swOptionValueTypeNameGet(swOptionValueType type);
void swOptionPrint(swOption *option);

extern bool trueValue;
extern bool falseValue;

#endif // SW_COMMANDLINE_OPTION_H