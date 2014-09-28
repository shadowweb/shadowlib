#ifndef SW_CORE_BENCHMARK_H
#define SW_CORE_BENCHMARK_H

#include <stdbool.h>
#include <stdint.h>

typedef struct swBenchmark
{
  uint64_t *variances;
  uint64_t *minValues;
  uint64_t *maxDeviations;
  void     *ticks;
  void (*func)(void);
  uint64_t maxDeviationAll;
  uint64_t totalVariance;
  uint64_t varianceOfVariances;
  uint64_t varianceOfMins;
  uint32_t loopCount;
  uint32_t sampleSize;
  uint32_t spurious;
} swBenchmark;

bool swBenchmarkInit(swBenchmark *benchmark, uint32_t loopCount, uint32_t sampleSize, void (*func)());
void swBenchmarkRelease(swBenchmark *benchmark);
bool swBenchmarkRun(swBenchmark *benchmark);

#endif // SW_CORE_BENCHMARK_H
