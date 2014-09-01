#define _GNU_SOURCE
#include <errno.h>

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
  swOptionDeclarePrefixArray("bpa", "prefix bool array",    "bool",   0, swOptionValueTypeBool,    false),
  swOptionDeclarePrefixArray("ipa", "prefix int array",     "int",    0, swOptionValueTypeInt,     false),
  swOptionDeclarePrefixArray("dpa", "prefix double array",  "bool",   0, swOptionValueTypeDouble,  false),
  swOptionDeclarePrefixArray("spa", "prefix string array",  "string", 0, swOptionValueTypeString,  false),
  swOptionDeclareGrouping("a", "grouping bool", "bool",  false),
  swOptionDeclareGrouping("b", "grouping bool", "bool",  false),
  swOptionDeclareGrouping("c", "grouping bool", "bool",  false),
  swOptionDeclareGrouping("d", "grouping bool", "bool",  false)
);

swTestDeclare(BasicTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  int argc = 17;
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
    "--bool-name-array=false",
    "--string-name-array=bla-bla2",
    "--double-name-array=13.132",
    "--int-name-array=3",
    "--bool-name-array=false",
    "--string-name-array=bla-bla3",
    "--double-name-array=13.133"
  };
  if (swOptionCommandLineInit(argc, argv, "This is basic test"))
  {
    rtn = true;
    swOptionCommandLineShutdown();
  }
  return rtn;
}


swTestSuiteStructDeclare(CommandLineSimpleTest, NULL, NULL, swTestRun, &BasicTest);