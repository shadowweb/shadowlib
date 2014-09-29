#include "core/benchmark.h"
#include "core/memory.h"
#include "core/stop-watch.h"

#include <string.h>

bool swBenchmarkReset(swBenchmark *benchmark, uint32_t sampleSize, void (*func)())
{
  bool rtn = false;
  if (benchmark && sampleSize && func)
  {
    memset(benchmark, 0, sizeof(*benchmark));
    benchmark->sampleSize = sampleSize;
    benchmark->func = func;
    benchmark->min = ULONG_MAX;
    rtn = true;
  }
  return rtn;
}

static bool swBenchmarkMeasure(swBenchmark *benchmark)
{
  bool rtn = false;
  if (benchmark)
  {
    uint32_t i = 0;
    uint32_t previousSum = 0;
    uint32_t previousSumOfSquares = 0;
    swStopWatch stopWatch = {0};
    swStopWatchPrepare(&stopWatch);
    for (; i < benchmark->sampleSize; i++)
    {
      swStopWatchMeasure(&stopWatch, benchmark->func);
      /*
      swStopWatch stopWatch = {0};
      swStopWatchStart(&stopWatch);
      benchmark->func();
      swStopWatchStop(&stopWatch);
      */
      if (benchmark->min > stopWatch.timeTicks)
        benchmark->min = stopWatch.timeTicks;
      if (benchmark->max < stopWatch.timeTicks)
        benchmark->max = stopWatch.timeTicks;
      benchmark->sum += stopWatch.timeTicks;
      if (benchmark->sum >= previousSum)
      {
        previousSum = benchmark->sum;
        benchmark->sumOfSquares += stopWatch.timeTicks * stopWatch.timeTicks;
        if (benchmark->sumOfSquares >= previousSumOfSquares)
          previousSumOfSquares = benchmark->sumOfSquares;
        else
          break;
      }
      else
        break;
    }
    if (i == benchmark->sampleSize)
      rtn = true;
  }
  return rtn;
}

// This implements the following formula
// sum = (x1 + x2 + ... + xn)
// sumOfSquares = (x1^2 + x2^2 + ... + xn^2)
// variance = sumOfSquares/n - sum^2 / n^2
// this is an alternative way of calculating variance that significantly reduces the number of mathematical operations
// and eliminates the need of dealing with negative numbers
// this formula can be derived from the original one by simple algebraic transformations

static bool swBenchmarkCalculateVariance(swBenchmark *benchmark)
{
  bool rtn = false;
  if (benchmark)
  {
    uint64_t previousSum = benchmark->sum;
    uint64_t sumSquare = benchmark->sum * benchmark->sum;
    if (sumSquare >= previousSum)
    {
      benchmark->variance = (benchmark->sumOfSquares / ((uint64_t)(benchmark->sampleSize))) - (sumSquare / (((uint64_t)(benchmark->sampleSize)) * ((uint64_t)(benchmark->sampleSize))));
      rtn = true;
    }
  }
  return rtn;
}

bool swBenchmarkRun(swBenchmark *benchmark)
{
  bool rtn = false;
  if (benchmark)
  {
    if (swBenchmarkMeasure(benchmark))
    {
      if (swBenchmarkCalculateVariance(benchmark))
      {
        benchmark->deviation = (benchmark->max - benchmark->min);
        rtn = true;
      }
    }
  }
  return rtn;
}
