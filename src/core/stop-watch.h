#ifndef SW_CORE_STOPWATCH_H
#define SW_CORE_STOPWATCH_H

#include <stdint.h>
#include <limits.h>

typedef struct swStopWatch
{
  uint64_t timeTicks;
  uint32_t startLow;
  uint32_t startHigh;
  uint32_t finishLow;
  uint32_t finishHigh;
} swStopWatch;

static inline void swStopWatchStart(swStopWatch *stopWatch)
{
  if (stopWatch)
  {
    // warm up instruction and data cache
    asm volatile (
      "CPUID\n\t"
      "RDTSC\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t" : "=r" (stopWatch->startHigh), "=r" (stopWatch->startLow) :: "%rax", "%rbx", "%rcx", "%rdx"
    );
    asm volatile(
      "RDTSCP\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t"
      "CPUID\n\t" : "=r" (stopWatch->finishHigh), "=r" (stopWatch->finishLow) :: "%rax", "%rbx", "%rcx", "%rdx"
    );
    asm volatile (
      "CPUID\n\t"
      "RDTSC\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t" : "=r" (stopWatch->startHigh), "=r" (stopWatch->startLow) :: "%rax", "%rbx", "%rcx", "%rdx"
    );
    asm volatile(
      "RDTSCP\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t"
      "CPUID\n\t" : "=r" (stopWatch->finishHigh), "=r" (stopWatch->finishLow) :: "%rax", "%rbx", "%rcx", "%rdx"
    );

    // collect start data
    asm volatile (
      "CPUID\n\t"
      "RDTSC\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t" : "=r" (stopWatch->startHigh), "=r" (stopWatch->startLow) :: "%rax", "%rbx", "%rcx", "%rdx"
    );
  }
}

static inline void swStopWatchStop(swStopWatch *stopWatch)
{
  // collect finish data
  asm volatile (
    "RDTSCP\n\t"
    "mov %%edx, %0\n\t"
    "mov %%eax, %1\n\t"
    "CPUID\n\t" : "=r" (stopWatch->finishHigh), "=r" (stopWatch->finishLow) :: "%rax", "%rbx", "%rcx", "%rdx"
  );
  uint64_t start = ( ((uint64_t)stopWatch->startHigh << 32) | stopWatch->startLow );
  uint64_t finish = ( ((uint64_t)stopWatch->finishHigh << 32) | stopWatch->finishLow );
  stopWatch->timeTicks = (finish > start) ? (finish - start) : (ULONG_MAX - start + finish);
}

static inline void swStopWatchPrepare(swStopWatch *stopWatch)
{
  if (stopWatch)
  {
    // warm up instruction and data cache
    asm volatile (
      "CPUID\n\t"
      "RDTSC\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t" : "=r" (stopWatch->startHigh), "=r" (stopWatch->startLow) :: "%rax", "%rbx", "%rcx", "%rdx"
    );
    asm volatile(
      "RDTSCP\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t"
      "CPUID\n\t" : "=r" (stopWatch->finishHigh), "=r" (stopWatch->finishLow) :: "%rax", "%rbx", "%rcx", "%rdx"
    );
    asm volatile (
      "CPUID\n\t"
      "RDTSC\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t" : "=r" (stopWatch->startHigh), "=r" (stopWatch->startLow) :: "%rax", "%rbx", "%rcx", "%rdx"
    );
    asm volatile(
      "RDTSCP\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t"
      "CPUID\n\t" : "=r" (stopWatch->finishHigh), "=r" (stopWatch->finishLow) :: "%rax", "%rbx", "%rcx", "%rdx"
    );
  }
}

static inline void swStopWatchMeasure(swStopWatch *stopWatch, void (*func)())
{
  // collect start data
  asm volatile (
    "CPUID\n\t"
    "RDTSC\n\t"
    "mov %%edx, %0\n\t"
    "mov %%eax, %1\n\t" : "=r" (stopWatch->startHigh), "=r" (stopWatch->startLow) :: "%rax", "%rbx", "%rcx", "%rdx"
  );

  func();

  // collect finish data
  asm volatile (
    "RDTSCP\n\t"
    "mov %%edx, %0\n\t"
    "mov %%eax, %1\n\t"
    "CPUID\n\t" : "=r" (stopWatch->finishHigh), "=r" (stopWatch->finishLow) :: "%rax", "%rbx", "%rcx", "%rdx"
  );
  uint64_t start = ( ((uint64_t)stopWatch->startHigh << 32) | stopWatch->startLow );
  uint64_t finish = ( ((uint64_t)stopWatch->finishHigh << 32) | stopWatch->finishLow );
  stopWatch->timeTicks = (finish > start) ? (finish - start) : (ULONG_MAX - start + finish);
}

#endif // SW_CORE_STOPWATCH_H
