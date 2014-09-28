#include "core/benchmark.h"

#include "unittest/unittest.h"

static void testFunction(void)
{
  uint32_t j = 0;
  for (uint32_t i = 0; i < 100; i++)
    j++;
}

// TODO: verify correctness of benchmark calculations
swTestDeclare(BenchmarkBasicTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swBenchmark benchmark = { NULL };
  if (swBenchmarkInit(&benchmark, 100, 100, testFunction))
  {
    rtn = swBenchmarkRun(&benchmark);
    swTestLogLine("maxDeviationAll = %lu, totalVariance = %lu, varianceOfVariances = %lu, varianceOfMins = %lu, spurious = %u\n",
      benchmark.maxDeviationAll,
      benchmark.totalVariance,
      benchmark.varianceOfVariances,
      benchmark.varianceOfMins,
      benchmark.spurious
    );

    swBenchmarkRelease(&benchmark);
  }
  return rtn;
}

swTestSuiteStructDeclare(BenchmarkTest, NULL, NULL, swTestRun,
                         &BenchmarkBasicTest);

