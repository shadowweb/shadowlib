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
  if (swBenchmarkTicksReset(&benchmark, 10000000, testFunction))
  {
    rtn = swBenchmarkTicksRun(&benchmark);
    swTestLogLine("min = %lu, max = %lu, deviation = %lu, variance = %lu, samples = %u\n",
                  benchmark.min, benchmark.max, benchmark.deviation, benchmark.variance, benchmark.sampleSize);
  }
  return rtn;
}

swTestDeclare(BenchmarkRealTimeTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swBenchmark benchmark = { NULL };
  if (swBenchmarkTimeReset(&benchmark, CLOCK_REALTIME, 10000000, testFunction))
  {
    rtn = swBenchmarkTimeRun(&benchmark);
    swTestLogLine("min = %lu, max = %lu, deviation = %lu, variance = %lu, samples = %u\n",
                  benchmark.min, benchmark.max, benchmark.deviation, benchmark.variance, benchmark.sampleSize);
  }
  return rtn;
}

swTestDeclare(BenchmarkMonotonicTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swBenchmark benchmark = { NULL };
  if (swBenchmarkTimeReset(&benchmark, CLOCK_MONOTONIC, 10000000, testFunction))
  {
    rtn = swBenchmarkTimeRun(&benchmark);
    swTestLogLine("min = %lu, max = %lu, deviation = %lu, variance = %lu, samples = %u\n",
                  benchmark.min, benchmark.max, benchmark.deviation, benchmark.variance, benchmark.sampleSize);
  }
  return rtn;
}

swTestDeclare(BenchmarkMonotonicRawTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swBenchmark benchmark = { NULL };
  if (swBenchmarkTimeReset(&benchmark, CLOCK_MONOTONIC_RAW, 10000000, testFunction))
  {
    rtn = swBenchmarkTimeRun(&benchmark);
    swTestLogLine("min = %lu, max = %lu, deviation = %lu, variance = %lu, samples = %u\n",
                  benchmark.min, benchmark.max, benchmark.deviation, benchmark.variance, benchmark.sampleSize);
  }
  return rtn;
}

swTestDeclare(BenchmarkCPUTimeTest, NULL, NULL, swTestRun)
{
  bool rtn = false;
  swBenchmark benchmark = { NULL };
  if (swBenchmarkTimeReset(&benchmark, CLOCK_PROCESS_CPUTIME_ID, 10000000, testFunction))
  {
    rtn = swBenchmarkTimeRun(&benchmark);
    swTestLogLine("min = %lu, max = %lu, deviation = %lu, variance = %lu, samples = %u\n",
                  benchmark.min, benchmark.max, benchmark.deviation, benchmark.variance, benchmark.sampleSize);
  }
  return rtn;
}


swTestSuiteStructDeclare(BenchmarkTest, NULL, NULL, swTestRun,
                         &BenchmarkBasicTest, &BenchmarkRealTimeTest, &BenchmarkMonotonicTest, &BenchmarkMonotonicRawTest, &BenchmarkCPUTimeTest);

