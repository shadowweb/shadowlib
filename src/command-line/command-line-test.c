#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#undef _GNU_SOURCE
#include <stdarg.h>

#include "unittest/unittest.h"
#include "command-line/command-line.h"

swOptionCategoryMainDeclare(mainArgs, "Command Line Test",
  swOptionDeclarePositionalScalar("postest1", "pt1", swOptionValueTypeBool, false),
  swOptionDeclarePositionalScalar("postest2", "pt2", swOptionValueTypeInt, false),
  swOptionDeclarePositionalScalar("postest3", "pt3", swOptionValueTypeDouble, false),
  swOptionDeclarePositionalScalar("postest4", "pt4", swOptionValueTypeString, false),
  swOptionDeclareConsumeAfter("consume after", "...", 0, swOptionValueTypeString, false),
  swOptionDeclareSink("sink", NULL, 0, swOptionValueTypeString, false)
);

swOptionCategoryModuleDeclare(basicTestArgs, "Basic Test Arguments",
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

static void checkInt(char *name, int64_t value)
{
  swStaticString intName = swStaticStringDefineFromCstr(name);
  int64_t intValue = 0;
  ASSERT_TRUE(swOptionValueGetInt(&intName, &intValue));
  ASSERT_EQUAL(intValue, value);
}

static void checkIntArray(char *name, uint32_t count, ...)
{
  swStaticString intNameArray = swStaticStringDefineFromCstr(name);
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swOptionValueGetIntArray(&intNameArray, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  int64_t *intValues = (int64_t *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < count; i++)
    ASSERT_EQUAL(intValues[i], va_arg(argPtr, int64_t));
  va_end(argPtr);
}

static void checkBool(char *name, bool value)
{
  swStaticString boolName = swStaticStringDefineFromCstr(name);
  bool boolValue = false;
  ASSERT_TRUE(swOptionValueGetBool(&boolName, &boolValue));
  ASSERT_EQUAL(boolValue, value);
}

static void checkBoolArray(char *name, uint32_t count, ...)
{
  swStaticString boolNameArray = swStaticStringDefineFromCstr(name);
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swOptionValueGetBoolArray(&boolNameArray, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  bool *boolValues = (bool *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < count; i++)
    ASSERT_EQUAL(boolValues[i], va_arg(argPtr, int));
  va_end(argPtr);
}

static void checkString(char *name, char *value)
{
  swStaticString stringName = swStaticStringDefineFromCstr(name);
  swStaticString stringValue = swStaticStringDefineEmpty;
  swStaticString stringRealValue = swStaticStringDefineFromCstr(value);
  ASSERT_TRUE(swOptionValueGetString(&stringName, &stringValue));
  ASSERT_TRUE(swStaticStringEqual(&stringValue, &stringRealValue));
}

static void checkStringArray(char *name, uint32_t count, ...)
{
  swStaticString stringNameArray = swStaticStringDefineFromCstr(name);
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swOptionValueGetStringArray(&stringNameArray, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  swStaticString *stringValues = (swStaticString *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < count; i++)
  {
    char *value = va_arg(argPtr, char *);
    swStaticString stringRealValue = swStaticStringDefineFromCstr(value);
    ASSERT_TRUE(swStaticStringEqual(&stringValues[i], &stringRealValue));
  }
  va_end(argPtr);
}

static void checkDouble(char *name, double value)
{
  swStaticString doubleName = swStaticStringDefineFromCstr(name);
  double doubleValue = 0.0;
  ASSERT_TRUE(swOptionValueGetDouble(&doubleName, &doubleValue));
  ASSERT_EQUAL(doubleValue, value);
}

static void checkDoubleArray(char *name, uint32_t count, ...)
{
  swStaticString doubleNameArray = swStaticStringDefineFromCstr(name);
  swStaticArray valueArray = swStaticArrayDefineEmpty;
  ASSERT_TRUE(swOptionValueGetDoubleArray(&doubleNameArray, &valueArray));
  ASSERT_EQUAL(valueArray.count, count);
  double *doubleValues = (double *)valueArray.data;

  va_list argPtr;
  va_start(argPtr, count);
  for (uint32_t i = 0; i < count; i++)
    ASSERT_EQUAL(doubleValues[i], va_arg(argPtr, double));
  va_end(argPtr);
}

swTestDeclare(BasicTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  int argc = 37;
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
    "--dap13.133"
  };
  swDynamicString *errorString = NULL;
  if (swOptionCommandLineInit(argc, argv, "This is basic test", &errorString))
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
    checkIntArray("ipa", 3, 1, 2, 3);

    checkBool("bp", false);
    // checkBoolArray("bpa", 3, true, false, true);

    checkString("sp", "xyz");
    // checkStringArray("spa", 3, "bla-bla1", "bla-bla2", "bla-bla3");

    checkDouble("dp", 1.2);
    // checkDoubleArray("dpa", 3, 13.131, 13.132, 13.133);

    rtn = true;
    swOptionCommandLineShutdown();
  }
  else
  {
    swTestLogLine("Error processing arguments: '%.*s'\n", (int)(errorString->len), errorString->data);
    swDynamicStringDelete(errorString);
  }
  return rtn;
}

swTestSuiteStructDeclare(CommandLineSimpleTest, NULL, NULL, swTestRun, &BasicTest);