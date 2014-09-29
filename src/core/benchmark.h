#ifndef SW_CORE_BENCHMARK_H
#define SW_CORE_BENCHMARK_H

#include <stdbool.h>
#include <stdint.h>

typedef struct swBenchmark
{
  void (*func)(void);
  uint64_t max;
  uint64_t min;
  uint64_t deviation;
  uint64_t variance;

  uint64_t sum;
  uint64_t sumOfSquares;

  uint32_t sampleSize;
} swBenchmark;

bool swBenchmarkReset(swBenchmark *benchmark, uint32_t sampleSize, void (*func)());
bool swBenchmarkRun(swBenchmark *benchmark);

#endif // SW_CORE_BENCHMARK_H
