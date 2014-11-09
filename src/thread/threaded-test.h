#ifndef SW_THREAD_THREADEDTEST_H
#define SW_THREAD_THREADEDTEST_H

#include "io/edge-async.h"
#include "storage/dynamic-string.h"
#include "thread/thread-manager.h"
#include "unittest/unittest.h"

#define SW_BENCHMARK_MAGIC (0xdeadbef3)

typedef struct swThreadedTestSuite swThreadedTestSuite;

typedef struct swThreadedTestThreadData
{
  uint64_t  executionCPUTime;
  uint64_t  executionTotalTime;
  swThreadedTestSuite *test;
  void     *data;               // per thread data
  uint32_t  id;
  bool      shutdown;
  bool      done;
} swThreadedTestThreadData;

typedef struct swThreadedTestData
{
  // swDynamicString *testName;
  swThreadManager threadManager;
  swEdgeAsync killLoop;
  swThreadedTestThreadData *threadData;
  void *data;                     // per test data
  uint32_t numThreads;
} swThreadedTestData;

typedef void (*swThreadedTestDataSetupFunc)    (swThreadedTestData *data);
typedef void (*swThreadedTestDataTeardownFunc) (swThreadedTestData *data);

typedef void (*swThreadedTestThreadDataSetupFunc)    (swThreadedTestData *data, swThreadedTestThreadData *threadData);
typedef void (*swThreadedTestThreadDataTeardownFunc) (swThreadedTestData *data, swThreadedTestThreadData *threadData);
typedef bool (*swThreadedTestThreadDataRunFunc)      (swThreadedTestData *data, swThreadedTestThreadData *threadData);

struct swThreadedTestSuite
{
  const char     *testName;
  swThreadedTestData testData;    // reset for every tests
  swTest        **tests;          // array of tests to populate swTestSuite with
  swStaticArray   threadCounts;   // defines the number of threads to run per test
  uint32_t        currentTest;
  uint32_t        magic;
  swThreadedTestDataSetupFunc          setupFunc;
  swThreadedTestDataTeardownFunc       teardownFunc;

  swThreadedTestThreadDataSetupFunc    threadSetupFunc;
  swThreadedTestThreadDataTeardownFunc threadTeardownFunc;
  swThreadedTestThreadDataRunFunc      threadRunFunc;

  uint64_t unused1;
  uint64_t unused2;
};

void swThreadedTestDataSet(swThreadedTestData *testData, void *data);
void *swThreadedTestDataGet(swThreadedTestData *testData);
void swThreadedTestThreadDataSet(swThreadedTestThreadData *benchmarkThreadData, void *data);
void *swThreadedTestThreadDataGet(swThreadedTestThreadData *benchmarkThreadData);

#define swThreadedTestDeclare(testNameIn, setup, teardown, threadSetup, threadTeardown, threadRun, tc) \
  static swThreadedTestSuite testNameIn __attribute__ ((unused, section(".thrededtest"))) = \
  { \
    .testName = #testNameIn, \
    .magic = SW_BENCHMARK_MAGIC, \
    .setupFunc = setup, \
    .teardownFunc = teardown, \
    .threadSetupFunc = threadSetup, \
    .threadTeardownFunc = threadTeardown, \
    .threadRunFunc = threadRun, \
    .threadCounts = swStaticArrayDefine(tc, sizeof(uint32_t)) \
  }

#endif  // SW_THREAD_THREADEDTEST_H
