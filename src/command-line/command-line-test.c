#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#undef _GNU_SOURCE
#include <stdarg.h>

#include "unittest/unittest.h"
#include "command-line/command-line.h"
#include "command-line/option.h"
#include "command-line/option-category.h"

swOptionCategoryMainDeclare(mainArgs, "Command Line Test",
  swOptionDeclarePositionalScalar("postest1", "pt1", swOptionValueTypeBool, false),
  swOptionDeclarePositionalScalar("postest2", "pt2", swOptionValueTypeInt, false),
  swOptionDeclarePositionalScalar("postest3", "pt3", swOptionValueTypeDouble, false),
  swOptionDeclarePositionalScalar("postest4", "pt4", swOptionValueTypeString, false),
  swOptionDeclarePositionalArray("posarraytest1", "pat1", 0, swOptionValueTypeBool, false),
  swOptionDeclarePositionalArray("posarraytest2", "pat2", 0, swOptionValueTypeInt, false),
  swOptionDeclarePositionalArray("posarraytest3", "pat3", 0, swOptionValueTypeDouble, false),
  swOptionDeclarePositionalArray("posarraytest4", "pat4", 0, swOptionValueTypeString, false),
  swOptionDeclareConsumeAfter("consume after", "...", 0, swOptionValueTypeString, false),
  swOptionDeclareSink("sink", NULL, 0, swOptionValueTypeString, false)
);

swOptionCategoryModuleDeclare(basicTestArgs, "Basic Test Arguments",
  swOptionDeclareScalar("bool-special", "bool special name description", "bool", swOptionValueTypeBool, false),
  swOptionDeclareScalar("bool-name",    "bool name description",    "bool",   swOptionValueTypeBool,    false),
  swOptionDeclareScalar("int-name",     "int name description",     "int",    swOptionValueTypeInt,     false),
  swOptionDeclareScalar("double-name",  "double name description",  "double", swOptionValueTypeDouble,  false),
  swOptionDeclareScalar("string-name",  "string name description",  "string", swOptionValueTypeString,  false),
  swOptionDeclareArray("bool-name-array",   "bool name array description",    "bool",   0, swOptionValueTypeBool,   swOptionArrayTypeSimple, false),
  swOptionDeclareArray("int-name-array",    "int name array description",     "int",    0, swOptionValueTypeInt,    swOptionArrayTypeSimple, false),
  swOptionDeclareArray("double-name-array", "double name array description",  "double", 0, swOptionValueTypeDouble, swOptionArrayTypeSimple, false),
  swOptionDeclareArray("string-name-array", "string name array description",  "string", 0, swOptionValueTypeString, swOptionArrayTypeSimple, false),
  swOptionDeclareArray("bool-name-array-mv",   "bool name multivalue array description",    "bool",   0, swOptionValueTypeBool,   swOptionArrayTypeMultiValue, false),
  swOptionDeclareArray("int-name-array-mv",    "int name multivalue array description",     "int",    0, swOptionValueTypeInt,    swOptionArrayTypeMultiValue, false),
  swOptionDeclareArray("double-name-array-mv", "double name multivalue array description",  "double", 0, swOptionValueTypeDouble, swOptionArrayTypeMultiValue, false),
  swOptionDeclareArray("string-name-array-mv", "string name multivalue array description",  "string", 0, swOptionValueTypeString, swOptionArrayTypeMultiValue, false),
  swOptionDeclareArray("bool-name-array-cs",   "bool name comma separated array description",    "bool",   0, swOptionValueTypeBool,   swOptionArrayTypeCommaSeparated, false),
  swOptionDeclareArray("int-name-array-cs",    "int name comma separated array description",     "int",    0, swOptionValueTypeInt,    swOptionArrayTypeCommaSeparated, false),
  swOptionDeclareArray("double-name-array-cs", "double name comma separated array description",  "double", 0, swOptionValueTypeDouble, swOptionArrayTypeCommaSeparated, false),
  swOptionDeclareArray("string-name-array-cs", "string name comma separated array description",  "string", 0, swOptionValueTypeString, swOptionArrayTypeCommaSeparated, false),
  swOptionDeclarePrefixScalar("bp", "prefix bool",    "bool",   swOptionValueTypeBool,    false),
  swOptionDeclarePrefixScalar("ip", "prefix int",     "int",    swOptionValueTypeInt,     false),
  swOptionDeclarePrefixScalar("dp", "prefix double",  "bool",   swOptionValueTypeDouble,  false),
  swOptionDeclarePrefixScalar("sp", "prefix string",  "string", swOptionValueTypeString,  false),
  swOptionDeclarePrefixArray("bap", "prefix bool array",    "bool",   0, swOptionValueTypeBool,    false),
  swOptionDeclarePrefixArray("iap", "prefix int array",     "int",    0, swOptionValueTypeInt,     false),
  swOptionDeclarePrefixArray("dap", "prefix double array",  "bool",   0, swOptionValueTypeDouble,  false),
  swOptionDeclarePrefixArray("sap", "prefix string array",  "string", 0, swOptionValueTypeString,  false),
  swOptionDeclareGrouping("a", "grouping bool", "bool",  false),
  swOptionDeclareGrouping("b", "grouping bool", "bool",  false),
  swOptionDeclareGrouping("c", "grouping bool", "bool",  false),
  swOptionDeclareGrouping("d", "grouping bool", "bool",  false)
);

static void checkInt(char *name, int64_t value) __attribute__((unused));
static void checkInt(char *name, int64_t value)
{
  swStaticString intName = swStaticStringDefineFromCstr(name);
  int64_t intValue = 0;
  ASSERT_TRUE(swOptionValueGetInt(&intName, &intValue));
  ASSERT_EQUAL(intValue, value);
}

static void checkIntArray(char *name, uint32_t count, ...) __attribute__((unused));
static void checkIntArray(char *name, uint32_t count, ...)
{
  swStaticString intNameArray = swStaticStringDefineFromCstr(name);
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swOptionValueGetIntArray(&intNameArray, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  int64_t *intValues = (int64_t *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
    ASSERT_EQUAL(intValues[i], va_arg(argPtr, int64_t));
  va_end(argPtr);
}

static void checkBool(char *name, bool value) __attribute__((unused));
static void checkBool(char *name, bool value)
{
  swStaticString boolName = swStaticStringDefineFromCstr(name);
  bool boolValue = false;
  ASSERT_TRUE(swOptionValueGetBool(&boolName, &boolValue));
  ASSERT_EQUAL(boolValue, value);
}

static void checkBoolArray(char *name, uint32_t count, ...) __attribute__((unused));
static void checkBoolArray(char *name, uint32_t count, ...)
{
  swStaticString boolNameArray = swStaticStringDefineFromCstr(name);
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swOptionValueGetBoolArray(&boolNameArray, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  bool *boolValues = (bool *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
    ASSERT_EQUAL(boolValues[i], va_arg(argPtr, int));
  va_end(argPtr);
}

static void checkString(char *name, char *value) __attribute__((unused));
static void checkString(char *name, char *value)
{
  swStaticString stringName = swStaticStringDefineFromCstr(name);
  swStaticString stringValue = swStaticStringDefineEmpty;
  swStaticString stringRealValue = swStaticStringDefineFromCstr(value);
  ASSERT_TRUE(swOptionValueGetString(&stringName, &stringValue));
  ASSERT_TRUE(swStaticStringEqual(&stringValue, &stringRealValue));
}

static void checkStringArray(char *name, uint32_t count, ...) __attribute__((unused));
static void checkStringArray(char *name, uint32_t count, ...)
{
  swStaticString stringNameArray = swStaticStringDefineFromCstr(name);
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swOptionValueGetStringArray(&stringNameArray, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  swStaticString *stringValues = (swStaticString *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
  {
    char *value = va_arg(argPtr, char *);
    swStaticString stringRealValue = swStaticStringDefineFromCstr(value);
    ASSERT_TRUE(swStaticStringEqual(&stringValues[i], &stringRealValue));
  }
  va_end(argPtr);
}

static void checkDouble(char *name, double value) __attribute__((unused));
static void checkDouble(char *name, double value)
{
  swStaticString doubleName = swStaticStringDefineFromCstr(name);
  double doubleValue = 0.0;
  ASSERT_TRUE(swOptionValueGetDouble(&doubleName, &doubleValue));
  ASSERT_EQUAL(doubleValue, value);
}

static void checkDoubleArray(char *name, uint32_t count, ...) __attribute__((unused));
static void checkDoubleArray(char *name, uint32_t count, ...)
{
  swStaticString doubleNameArray = swStaticStringDefineFromCstr(name);
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swOptionValueGetDoubleArray(&doubleNameArray, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  double *doubleValues = (double *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
    ASSERT_EQUAL(doubleValues[i], va_arg(argPtr, double));
  va_end(argPtr);
}

static void checkPositionalInt(uint32_t position, int64_t value) __attribute__((unused));
static void checkPositionalInt(uint32_t position, int64_t value)
{
  int64_t intValue = 0;
  ASSERT_TRUE(swPositionalOptionValueGetInt(position, &intValue));
  ASSERT_EQUAL(intValue, value);
}

static void checkPositionalIntArray(uint32_t position, uint32_t count, ...) __attribute__((unused));
static void checkPositionalIntArray(uint32_t position, uint32_t count, ...)
{
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swPositionalOptionValueGetIntArray(position, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  int64_t *intValues = (int64_t *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
    ASSERT_EQUAL(intValues[i], va_arg(argPtr, int64_t));
  va_end(argPtr);
}

static void checkPositionalBool(uint32_t position, bool value) __attribute__((unused));
static void checkPositionalBool(uint32_t position, bool value)
{
  bool boolValue = false;
  ASSERT_TRUE(swPositionalOptionValueGetBool(position, &boolValue));
  ASSERT_EQUAL(boolValue, value);
}

static void checkPositionalBoolArray(uint32_t position, uint32_t count, ...) __attribute__((unused));
static void checkPositionalBoolArray(uint32_t position, uint32_t count, ...)
{
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swPositionalOptionValueGetBoolArray(position, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  bool *boolValues = (bool *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
    ASSERT_EQUAL(boolValues[i], va_arg(argPtr, int));
  va_end(argPtr);
}

static void checkPositionalString(uint32_t position, char *value) __attribute__((unused));
static void checkPositionalString(uint32_t position, char *value)
{
  swStaticString stringValue = swStaticStringDefineEmpty;
  swStaticString stringRealValue = swStaticStringDefineFromCstr(value);
  ASSERT_TRUE(swPositionalOptionValueGetString(position, &stringValue));
  ASSERT_TRUE(swStaticStringEqual(&stringValue, &stringRealValue));
}

static void checkPositionalStringArray(uint32_t position, uint32_t count, ...) __attribute__((unused));
static void checkPositionalStringArray(uint32_t position, uint32_t count, ...)
{
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swPositionalOptionValueGetStringArray(position, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  swStaticString *stringValues = (swStaticString *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
  {
    char *value = va_arg(argPtr, char *);
    swStaticString stringRealValue = swStaticStringDefineFromCstr(value);
    ASSERT_TRUE(swStaticStringEqual(&stringValues[i], &stringRealValue));
  }
  va_end(argPtr);
}

static void checkPositionalDouble(uint32_t position, double value) __attribute__((unused));
static void checkPositionalDouble(uint32_t position, double value)
{
  double doubleValue = 0.0;
  ASSERT_TRUE(swPositionalOptionValueGetDouble(position, &doubleValue));
  ASSERT_EQUAL(doubleValue, value);
}

static void checkPositionalDoubleArray(uint32_t position, uint32_t count, ...) __attribute__((unused));
static void checkPositionalDoubleArray(uint32_t position, uint32_t count, ...)
{
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swPositionalOptionValueGetDoubleArray(position, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  double *doubleValues = (double *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
    ASSERT_EQUAL(doubleValues[i], va_arg(argPtr, double));
  va_end(argPtr);
}

static bool testFramework(int argc, const char *argv[], void (*checkFunc)())
{
  bool rtn = false;
  swDynamicString *errorString = NULL;
  if (swCommandLineInit(argc, argv, "This is basic test", &errorString))
  {
    checkFunc();
    rtn = true;
    swCommandLineShutdown();
  }
  else
  {
    if (errorString)
    {
      swTestLogLine("Error processing arguments: '%.*s'\n", (int)(errorString->len), errorString->data);
      swDynamicStringDelete(errorString);
    }
  }
  return rtn;
}

static void checkConsumeAfterStringArray(uint32_t count, ...) __attribute__((unused));
static void checkConsumeAfterStringArray(uint32_t count, ...)
{
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swConsumeAfterOptionValueGetStringArray(&valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  swStaticString *stringValues = (swStaticString *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
  {
    char *value = va_arg(argPtr, char *);
    swStaticString stringRealValue = swStaticStringDefineFromCstr(value);
    ASSERT_TRUE(swStaticStringEqual(&stringValues[i], &stringRealValue));
  }
  va_end(argPtr);
}

static void checkSinkStringArray(uint32_t count, ...) __attribute__((unused));
static void checkSinkStringArray(uint32_t count, ...)
{
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swSinkOptionValueGetStringArray(&valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  swStaticString *stringValues = (swStaticString *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < valueArray.count; i++)
  {
    char *value = va_arg(argPtr, char *);
    swStaticString stringRealValue = swStaticStringDefineFromCstr(value);
    ASSERT_TRUE(swStaticStringEqual(&stringValues[i], &stringRealValue));
  }
  va_end(argPtr);
}

static void basicTestCheck()
{
  checkInt("int-name", 1);
  checkIntArray("int-name-array", 3, 1L, 2L, 3L);

  checkBool("bool-name", false);
  checkBoolArray("bool-name-array", 3, false, true, false);

  checkString("string-name", "bla-bla");
  checkStringArray("string-name-array", 3, "bla-bla1", "bla-bla2", "bla-bla3");

  checkDouble("double-name", 13.13);
  checkDoubleArray("double-name-array", 3, 13.131, 13.132, 13.133);

  checkInt("ip", 2);
  checkIntArray("iap", 3, 1, 2, 3);

  checkBool("bp", false);
  checkBoolArray("bap", 3, true, false, true);

  checkString("sp", "xyz");
  checkStringArray("sap", 3, "bla-bla1", "bla-bla2", "bla-bla3");

  checkDouble("dp", 1.2);
  checkDoubleArray("dap", 3, 13.131, 13.132, 13.133);

  checkBool("b", true);
  checkBool("c", true);
  checkBool("d", true);

  checkBool("bool-special", false);
}

swTestDeclare(BasicTest, NULL, NULL, swTestRun)
{
  int argc = 39;
  const char *argv[] = {
    program_invocation_name,
    "--int-name=1",
    "--bool-name=false",
    "--string-name=bla-bla",
    "--double-name=13.13",
    "--int-name-array=1",
    "--bool-name-array=false",
    "--string-name-array=bla-bla1",
    "--double-name-array=13.131",
    "--int-name-array=2",
    "--bool-name-array=true",
    "--string-name-array=bla-bla2",
    "--double-name-array=13.132",
    "--int-name-array=3",
    "--bool-name-array=false",
    "--string-name-array=bla-bla3",
    "--double-name-array=13.133",
    "--nobool-special",
    "--bptrue",
    "--bpfalse",
    "--ip1",
    "--ip2",
    "--dp1.1",
    "--dp1.2",
    "--spabcd",
    "--spxyz",
    "--iap1",
    "--iap2",
    "--iap3",
    "--baptrue",
    "--bapfalse",
    "--baptrue",
    "--sapbla-bla1",
    "--sapbla-bla2",
    "--sapbla-bla3",
    "--dap13.131",
    "--dap13.132",
    "--dap13.133",
    "-bcd"
  };

  return testFramework(argc, argv, basicTestCheck);
}

static void boolValueTest1Check()
{
  checkBool("bool-special", false);
}

swTestDeclare(BoolValueTest1, NULL, NULL, swTestRun)
{
  int argc = 2;
  const char *argv[] = {
    program_invocation_name,
    "--nobool-special",
  };

  return testFramework(argc, argv, boolValueTest1Check);
}

static void boolValueTest2Check()
{
  checkBool("bool-special", true);
}

swTestDeclare(BoolValueTest2, NULL, NULL, swTestRun)
{
  int argc = 2;
  const char *argv[] = {
    program_invocation_name,
    "--bool-special",
  };

  return testFramework(argc, argv, boolValueTest2Check);
}

static void boolValueTest3Check()
{
  checkBool("bool-special", false);
}

swTestDeclare(BoolValueTest3, NULL, NULL, swTestRun)
{
  int argc = 3;
  const char *argv[] = {
    program_invocation_name,
    "--bool-special",
    "FALSE"
  };

  return testFramework(argc, argv, boolValueTest3Check);
}

static void boolValueTest4Check()
{
  checkBool("bool-special", true);
}

swTestDeclare(BoolValueTest4, NULL, NULL, swTestRun)
{
  int argc = 3;
  const char *argv[] = {
    program_invocation_name,
    "--bool-special",
    "TRUE"
  };

  return testFramework(argc, argv, boolValueTest4Check);
}

static void boolValueTest5Check()
{
  checkBool("bool-special", false);
}

swTestDeclare(BoolValueTest5, NULL, NULL, swTestRun)
{
  int argc = 2;
  const char *argv[] = {
    program_invocation_name,
    "--bool-special=false",
  };

  return testFramework(argc, argv, boolValueTest5Check);
}
static void boolValueTest6Check()
{
  checkBool("bool-special", true);
}

swTestDeclare(BoolValueTest6, NULL, NULL, swTestRun)
{
  int argc = 2;
  const char *argv[] = {
    program_invocation_name,
    "--bool-special=true",
  };

  return testFramework(argc, argv, boolValueTest6Check);
}

static void multiValueTestCheck()
{
  checkBoolArray("bool-name-array-mv", 3, true, false, true);
  checkIntArray("int-name-array-mv", 3, 1, 2, 3);
  checkDoubleArray("double-name-array-mv", 3, 13.131, 13.132, 13.133);
  checkStringArray("string-name-array-mv", 3, "bla-bla1", "bla-bla2", "bla-bla3");
}

swTestDeclare(MultiValueTest, NULL, NULL, swTestRun)
{
  int argc = 17;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-mv",
    "true",
    "false",
    "true",
    "--int-name-array-mv",
    "1",
    "2",
    "3",
    "--double-name-array-mv",
    "13.131",
    "13.132",
    "13.133",
    "--string-name-array-mv",
    "bla-bla1",
    "bla-bla2",
    "bla-bla3",
  };

  return testFramework(argc, argv, multiValueTestCheck);
}

static void commaSeparatedValueTestCheck()
{
  checkBoolArray("bool-name-array-cs", 3, true, false, true);
  checkIntArray("int-name-array-cs", 3, 1, 2, 3);
  checkDoubleArray("double-name-array-cs", 3, 13.131, 13.132, 13.133);
  checkStringArray("string-name-array-cs", 3, "bla-bla1", "bla-bla2", "bla-bla3");
}

swTestDeclare(CommaSeparatedValueTest1, NULL, NULL, swTestRun)
{
  int argc = 9;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "--int-name-array-cs",
    "1,2,3",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
  };

  return testFramework(argc, argv, commaSeparatedValueTestCheck);
}

swTestDeclare(CommaSeparatedValueTest2, NULL, NULL, swTestRun)
{
  int argc = 5;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs=true,false,true",
    "--int-name-array-cs=1,2,3",
    "--double-name-array-cs=13.131,13.132,13.133",
    "--string-name-array-cs=bla-bla1,bla-bla2,bla-bla3",
  };

  return testFramework(argc, argv, commaSeparatedValueTestCheck);
}

/*
  swOptionDeclarePositionalArray("posarraytest1", "pat1", 0, swOptionValueTypeBool, false),
  swOptionDeclarePositionalArray("posarraytest2", "pat2", 0, swOptionValueTypeInt, false),
  swOptionDeclarePositionalArray("posarraytest3", "pat3", 0, swOptionValueTypeDouble, false),
  swOptionDeclarePositionalArray("posarraytest4", "pat4", 0, swOptionValueTypeString, false),
  swOptionDeclareConsumeAfter("consume after", "...", 0, swOptionValueTypeString, false),
  swOptionDeclareSink("sink", NULL, 0, swOptionValueTypeString, false)
*/

static void positionalValueTestCheck()
{
  checkBoolArray("bool-name-array-cs", 3, true, false, true);
  checkIntArray("int-name-array-cs", 3, 1, 2, 3);
  checkDoubleArray("double-name-array-cs", 3, 13.131, 13.132, 13.133);
  checkStringArray("string-name-array-cs", 3, "bla-bla1", "bla-bla2", "bla-bla3");
  checkPositionalBool(0, false);
  checkPositionalInt(1, 13);
  checkPositionalDouble(2, 13.13);
  checkPositionalString(3, "bla-bla");
}

swTestDeclare(PositionalValueTest1, NULL, NULL, swTestRun)
{
  int argc = 13;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "--int-name-array-cs",
    "1,2,3",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "false",
    "13",
    "13.13",
    "bla-bla"
  };

  return testFramework(argc, argv, positionalValueTestCheck);
}

swTestDeclare(PositionalValueTest2, NULL, NULL, swTestRun)
{
  int argc = 14;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "--int-name-array-cs",
    "1,2,3",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "--",
    "false",
    "13",
    "13.13",
    "bla-bla"
  };

  return testFramework(argc, argv, positionalValueTestCheck);
}

swTestDeclare(PositionalValueTest3, NULL, NULL, swTestRun)
{
  int argc = 13;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "false",
    "--int-name-array-cs",
    "1,2,3",
    "13",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "13.13",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "bla-bla"
  };

  return testFramework(argc, argv, positionalValueTestCheck);
}

static void positionalArrayValueTestCheck()
{
  checkBoolArray("bool-name-array-cs", 3, true, false, true);
  checkIntArray("int-name-array-cs", 3, 1, 2, 3);
  checkDoubleArray("double-name-array-cs", 3, 13.131, 13.132, 13.133);
  checkStringArray("string-name-array-cs", 3, "bla-bla1", "bla-bla2", "bla-bla3");
  checkPositionalBool(0, false);
  checkPositionalInt(1, 13);
  checkPositionalDouble(2, 13.13);
  checkPositionalString(3, "bla-bla");
  checkPositionalBoolArray(4, 3, true, false, true);
  checkPositionalIntArray(5, 3, 1, 2, 3);
  checkPositionalDoubleArray(6, 3, 13.131, 13.132, 13.133);
  checkPositionalStringArray(7, 3, "bla-bla1", "bla-bla2", "bla-bla3");
}

swTestDeclare(PositionalValueTest4, NULL, NULL, swTestRun)
{
  int argc = 17;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "--int-name-array-cs",
    "1,2,3",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "false",
    "13",
    "13.13",
    "bla-bla",
    "true,false,true",
    "1,2,3",
    "13.131,13.132,13.133",
    "bla-bla1,bla-bla2,bla-bla3",
  };

  return testFramework(argc, argv, positionalArrayValueTestCheck);
}

swTestDeclare(PositionalValueTest5, NULL, NULL, swTestRun)
{
  int argc = 18;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "--int-name-array-cs",
    "1,2,3",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "--",
    "false",
    "13",
    "13.13",
    "bla-bla",
    "true,false,true",
    "1,2,3",
    "13.131,13.132,13.133",
    "bla-bla1,bla-bla2,bla-bla3",
  };

  return testFramework(argc, argv, positionalArrayValueTestCheck);
}

swTestDeclare(PositionalValueTest6, NULL, NULL, swTestRun)
{
  int argc = 17;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "false",
    "13",
    "--int-name-array-cs",
    "1,2,3",
    "13.13",
    "bla-bla",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "true,false,true",
    "1,2,3",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "13.131,13.132,13.133",
    "bla-bla1,bla-bla2,bla-bla3",
  };

  return testFramework(argc, argv, positionalArrayValueTestCheck);
}

static void consumeAfterArrayValueTestCheck()
{
  checkBoolArray("bool-name-array-cs", 3, true, false, true);
  checkIntArray("int-name-array-cs", 3, 1, 2, 3);
  checkDoubleArray("double-name-array-cs", 3, 13.131, 13.132, 13.133);
  checkStringArray("string-name-array-cs", 3, "bla-bla1", "bla-bla2", "bla-bla3");
  checkPositionalBool(0, false);
  checkPositionalInt(1, 13);
  checkPositionalDouble(2, 13.13);
  checkPositionalString(3, "bla-bla");
  checkPositionalBoolArray(4, 3, true, false, true);
  checkPositionalIntArray(5, 3, 1, 2, 3);
  checkPositionalDoubleArray(6, 3, 13.131, 13.132, 13.133);
  checkPositionalStringArray(7, 3, "bla-bla1", "bla-bla2", "bla-bla3");
  checkConsumeAfterStringArray(3, "bla1", "bla2", "bla3");
}

swTestDeclare(ConsumeAfterValueTest1, NULL, NULL, swTestRun)
{
  int argc = 20;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "--int-name-array-cs",
    "1,2,3",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "false",
    "13",
    "13.13",
    "bla-bla",
    "true,false,true",
    "1,2,3",
    "13.131,13.132,13.133",
    "bla-bla1,bla-bla2,bla-bla3",
    "bla1",
    "bla2",
    "bla3",
  };

  return testFramework(argc, argv, consumeAfterArrayValueTestCheck);
}

swTestDeclare(ConsumeAfterValueTest2, NULL, NULL, swTestRun)
{
  int argc = 21;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "--int-name-array-cs",
    "1,2,3",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "--",
    "false",
    "13",
    "13.13",
    "bla-bla",
    "true,false,true",
    "1,2,3",
    "13.131,13.132,13.133",
    "bla-bla1,bla-bla2,bla-bla3",
    "bla1",
    "bla2",
    "bla3",
  };

  return testFramework(argc, argv, consumeAfterArrayValueTestCheck);
}

swTestDeclare(ConsumeAfterValueTest3, NULL, NULL, swTestRun)
{
  int argc = 20;
  const char *argv[] = {
    program_invocation_name,
    "--bool-name-array-cs",
    "true,false,true",
    "false",
    "13",
    "--int-name-array-cs",
    "1,2,3",
    "13.13",
    "bla-bla",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "true,false,true",
    "1,2,3",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "13.131,13.132,13.133",
    "bla-bla1,bla-bla2,bla-bla3",
    "bla1",
    "bla2",
    "bla3",
  };

  return testFramework(argc, argv, consumeAfterArrayValueTestCheck);
}

static void sinkArrayValueTestCheck()
{
  checkBoolArray("bool-name-array-cs", 3, true, false, true);
  checkIntArray("int-name-array-cs", 3, 1, 2, 3);
  checkDoubleArray("double-name-array-cs", 3, 13.131, 13.132, 13.133);
  checkStringArray("string-name-array-cs", 3, "bla-bla1", "bla-bla2", "bla-bla3");
  checkPositionalBool(0, false);
  checkPositionalInt(1, 13);
  checkPositionalDouble(2, 13.13);
  checkPositionalString(3, "bla-bla");
  checkPositionalBoolArray(4, 3, true, false, true);
  checkPositionalIntArray(5, 3, 1, 2, 3);
  checkPositionalDoubleArray(6, 3, 13.131, 13.132, 13.133);
  checkPositionalStringArray(7, 3, "bla-bla1", "bla-bla2", "bla-bla3");
  checkConsumeAfterStringArray(3, "bla1", "bla2", "bla3");
  checkSinkStringArray(4, "--bla1", "--bla2", "--bla3", "--bla4");
}

swTestDeclare(SinkValueTest, NULL, NULL, swTestRun)
{
  int argc = 24;
  const char *argv[] = {
    program_invocation_name,
    "--bla1",
    "--bool-name-array-cs",
    "true,false,true",
    "false",
    "13",
    "--bla2",
    "--int-name-array-cs",
    "1,2,3",
    "13.13",
    "bla-bla",
    "--bla3",
    "--double-name-array-cs",
    "13.131,13.132,13.133",
    "true,false,true",
    "1,2,3",
    "--bla4",
    "--string-name-array-cs",
    "bla-bla1,bla-bla2,bla-bla3",
    "13.131,13.132,13.133",
    "bla-bla1,bla-bla2,bla-bla3",
    "bla1",
    "bla2",
    "bla3",
  };

  return testFramework(argc, argv, sinkArrayValueTestCheck);
}

swTestSuiteStructDeclare(CommandLineSimpleTest, NULL, NULL, swTestRun,
                         &BasicTest,
                         &BoolValueTest1, &BoolValueTest2, &BoolValueTest3, &BoolValueTest4, &BoolValueTest5, &BoolValueTest6,
                         &MultiValueTest, &CommaSeparatedValueTest1, &CommaSeparatedValueTest2,
                         &PositionalValueTest1, &PositionalValueTest2, &PositionalValueTest3, &PositionalValueTest4, &PositionalValueTest5, &PositionalValueTest6,
                         &ConsumeAfterValueTest1, &ConsumeAfterValueTest2, &ConsumeAfterValueTest3,
                         &SinkValueTest);
