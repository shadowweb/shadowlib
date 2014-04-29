#include "unittest.h"

swTestSuiteDataDeclare(testSuiteBla)
{
  int a;
};
swTestSuiteSetupDeclare(testSuiteBla)
{
  suiteData->a = 1;
  return;
}
swTestSuiteTeardownDeclare(testSuiteBla)
{
  suiteData->a = 0;
  return;
}

swTestDataDeclare(testNameBla)
{
  int a;
};

swTestSetupDeclare(testSuiteBla, testNameBla)
{
  suiteData->a++;
  testData->a = 1;
  return;
}

swTestTeardownDeclare(testSuiteBla, testNameBla)
{
  suiteData->a--;
  testData->a = 0;
  return;
}

swTestRunDeclare(testSuiteBla, testNameBla)
{
  return true;
}

swTestStructDeclare(testNameBla, swTestRun);

swTestDataDeclare(testNameBla1)
{
  int a;
};

swTestSetupDeclare(testSuiteBla, testNameBla1)
{
  testData->a = 1;
  return;
}

swTestTeardownDeclare(testSuiteBla, testNameBla1)
{
  testData->a = 0;
  return;
}

swTestRunDeclare(testSuiteBla, testNameBla1)
{
  ASSERT_FAIL();
  return true;
}

swTestStructDeclare(testNameBla1, swTestRun);

swTestSuiteStructDeclare(testSuiteBla, swTestRun, &testNameBla, &testNameBla1);
