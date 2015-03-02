#include "protocol/ethernet.h"
#include "unittest/unittest.h"

swTestDeclare(TestInit, NULL, NULL, swTestRun)
{
  bool rtn = false;
  if (swInterfaceInfoInit())
  {
    swInterfaceInfoPrint();
    rtn = true;
    swInterfaceInfoRelease();
  }
  return rtn;
}

swTestDeclare(TestRefresh, NULL, NULL, swTestRun)
{
  bool rtn = false;
  if (swInterfaceInfoInit())
  {
    ASSERT_TRUE(swInterfaceInfoRefreshAll());
    swInterfaceInfoPrint();
    rtn = true;
    swInterfaceInfoRelease();
  }
  return rtn;
}

swTestSuiteStructDeclare(InterfaceDiscovery, NULL, NULL, swTestRun, &TestInit, &TestRefresh);
