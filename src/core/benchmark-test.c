#include "core/benchmark.h"

#include "unittest/unittest.h"

static void testFunction(void)
{
  uint32_t j = 0;
  for (uint32_t i = 0; i < 100; i++)
    j++;
}

swTestDeclare(BenchmarkBasicTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swBenchmark benchmark = { NULL };
  if (swBenchmarkReset(&benchmark, 10000000, testFunction))
  {
    rtn = swBenchmarkRun(&benchmark);
    swTestLogLine("min = %lu, max = %lu, deviation = %lu, variance = %lu, samples = %u\n",
                  benchmark.min, benchmark.max, benchmark.deviation, benchmark.variance, benchmark.sampleSize);
  }
  return rtn;
}

swTestSuiteStructDeclare(BenchmarkTest, NULL, NULL, swTestRun, &BenchmarkBasicTest);

