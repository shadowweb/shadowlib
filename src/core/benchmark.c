#include "core/benchmark.h"
#include "core/memory.h"
#include "core/stop-watch.h"

#include <string.h>

bool swBenchmarkInit(swBenchmark *benchmark, uint32_t loopCount, uint32_t sampleSize, void (*func)())
{
  bool rtn = false;
  if (benchmark && loopCount && sampleSize && func)
  {
    memset(benchmark, 0, sizeof(*benchmark));
    if ((benchmark->variances = swMemoryMalloc(sampleSize * sizeof(uint64_t))))
    {
      if ((benchmark->minValues = swMemoryMalloc(sampleSize * sizeof(uint64_t))))
      {
        if ((benchmark->maxDeviations = swMemoryMalloc(sampleSize * sizeof(uint64_t))))
        {
          benchmark->loopCount = loopCount;
          benchmark->sampleSize = sampleSize;
          benchmark->func = func;
          rtn = true;
        }
      }
    }
    if (!rtn)
      swBenchmarkRelease(benchmark);
  }
  return rtn;
}

void swBenchmarkRelease(swBenchmark *benchmark)
{
  if (benchmark)
  {
    if (benchmark->variances)
      swMemoryFree(benchmark->variances);
    if (benchmark->minValues)
      swMemoryFree(benchmark->minValues);
    if (benchmark->maxDeviations)
      swMemoryFree(benchmark->maxDeviations);
  }
}

// static void swBenchmarkMeasure(uint64_t *ticks[], uint32_t loopCount, uint32_t sampleSize, void (*func)())
static void swBenchmarkMeasure(swBenchmark *benchmark)
{
  if (benchmark)
  {
    uint64_t (*ticks)[benchmark->loopCount][benchmark->sampleSize] = benchmark->ticks;
    swStopWatch stopWatch = {0};
    swStopWatchPrepare(&stopWatch);
    for (uint32_t i = 0; i < benchmark->loopCount; i++)
    {
      for (uint32_t j = 0; j < benchmark->sampleSize; j++)
      {
        swStopWatchMeasure(&stopWatch, benchmark->func);
        (*ticks)[ i ][ j ] = stopWatch.timeTicks;
      }
    }
  }
}

// This implements the following formula
// sum = (x1 + x2 + ... + xn)
// sumOfSquares = (x1^2 + x2^2 + ... + xn^2)
// variance = sumOfSquares/n - sum^2 / n^2
// this is an alternative way of calculating variance that significantly reduces the number of mathematical operations
// and eliminates the need of dealing with negative numbers
// this formula can be derived from the original one by simple algebraic transformations

static bool swBenchmarkCalculateVariance(uint64_t *ticks, uint32_t sampleSize, uint64_t *result)
{
  bool rtn = false;
  if (ticks && sampleSize && result)
  {
    uint32_t i = 0;
    uint64_t sum = 0;
    uint64_t previousSum = 0;
    while (i < sampleSize)
    {
      previousSum = sum;
      sum += ticks[i];
      // watch for overflow
      if (sum < previousSum)
        break;
      i++;
    }

    if (i == sampleSize)
    {
      uint64_t sumSquare = sum * sum;
      // watch for overflow again
      if (sumSquare >= previousSum)
      {
        uint64_t sumOfSquares = 0;
        uint64_t previousSumOfSquares = 0;
        for (i = 0; i < sampleSize; i++)
        {
          previousSumOfSquares = sumOfSquares;
          sumOfSquares += (ticks[i] * ticks[i]);
          // watch for overflow again
          if (sumOfSquares < previousSumOfSquares)
            break;
        }
        if (i == sampleSize)
        {
          *result = (sumOfSquares / ((uint64_t)(sampleSize))) - (sumSquare / (((uint64_t)(sampleSize)) * ((uint64_t)(sampleSize))));
          rtn = true;
        }
      }
    }
  }
  return rtn;
}

bool swBenchmarkRun(swBenchmark *benchmark)
{
  bool rtn = false;
  if (benchmark)
  {
    uint64_t ticks[benchmark->loopCount][benchmark->sampleSize];
    benchmark->ticks = (void *)(&ticks);

    // swBenchmarkMeasure(ticks, benchmark->loopCount, benchmark->sampleSize, benchmark->func);
    swBenchmarkMeasure(benchmark);

    uint64_t preveousMin = 0;
    uint32_t j = 0;
    for (; j < benchmark->loopCount; j++)
    {
      uint64_t minTime = ULONG_MAX;
      uint64_t maxTime = 0;

      for (uint32_t i = 0; i < benchmark->sampleSize; i++)
      {
        if (minTime > ticks[j][i])
          minTime = ticks[j][i];
        if (maxTime < ticks[j][i])
          maxTime = ticks[j][i];
      }

      benchmark->maxDeviations[j] = maxTime - minTime;
      benchmark->minValues[j] = minTime;

      if ((preveousMin != 0) && (preveousMin > minTime))
        benchmark->spurious++;
      if (benchmark->maxDeviations[j] > benchmark->maxDeviationAll)
        benchmark->maxDeviationAll = benchmark->maxDeviations[j];

      if (!swBenchmarkCalculateVariance(ticks[j], benchmark->sampleSize, &(benchmark->variances[j])))
        break;
      benchmark->totalVariance += benchmark->variances[j];
      preveousMin = minTime;
    }

    if (j == benchmark->sampleSize)
    {
      if (swBenchmarkCalculateVariance(benchmark->variances, benchmark->sampleSize, &(benchmark->varianceOfVariances)) &&
            swBenchmarkCalculateVariance(benchmark->minValues, benchmark->sampleSize, &(benchmark->varianceOfMins)))
        rtn = true;
    }
    benchmark->ticks = NULL;
  }
  return rtn;
}
