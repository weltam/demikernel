#ifndef BENCHMARKRUNNER_H_
#define BENCHMARKRUNNER_H_

#include <atomic>
#include <list>
#include <mutex>
#include <ostream>
#include <thread>
#include <utility>

#include "io_u.h"
#include "iodrivers/iodriver.h"
#include "options.h"
#include "utils/fixed_queue.h"
#include "utils/time_tracking.h"
#include "workgenerators/workgenerator.h"

namespace benchmark {

namespace {

static const unsigned int kFillBufSize = 4096;

}

class BenchmarkRunner {
 public:
  BenchmarkRunner(BenchmarkOptions &o);
  int Run();
  void WriteResults(std::ostream &outFile,
      std::chrono::steady_clock::time_point startTime);
#ifdef POLL_LATENCY
  void WritePollResults(std::ostream &outFile,
      std::chrono::steady_clock::time_point startTime);
#endif
#ifdef SUBMIT_LATENCY
  void WriteSubmitResults(std::ostream &outFile,
      std::chrono::steady_clock::time_point startTime);
#endif
#ifdef BUFFER_LATENCY
  void WriteBufferResults(std::ostream &outFile);
#endif

 private:
  BenchmarkOptions opts;
  std::atomic<bool> completionsThreadRun;
  std::atomic<bool> completionsThreadErr;
  std::atomic<bool> doneQueuing;
  WorkloadGenerator *generator;
  IODriver *ioDriver;
  AsyncIODriver *aioDriver;
  std::thread completionsThread;
  char fillBuf[kFillBufSize];

  BenchmarkFile testFile;

  std::list<Io_u> allIos;
  // Assuming requests are completed in order for the moment.
  FixedQueue inFlightIos;
#ifdef POLL_LATENCY
  std::list<std::pair<TimeDif, int>> pollTimes;
#endif
  TimeDif totalRuntime;

  bool lastWasSync = false;

  void getIoDriver();
  void getWorkloadGenerator();
  int prepareThreads();
  int prepareAsyncThreads();
  int prepareSyncThreads();
  int runWorkload();
  int runAsyncWorkload();
  int runSyncWorkload();
  void checkCompletions();
  void checkClockResolution();
  bool spaceInQueue();
  Io_u *allocateAndFillIou();
#ifdef BUFFER_LATENCY
  char *allocateAndPopulateBuffer(unsigned long long len, TimeDif &latency);
#else
  char *allocateAndPopulateBuffer(unsigned long long len);
#endif
  void populateBuffer(char *buf, unsigned int len);
};

}  // namespace benchmark.
#endif  // BENCHMARKRUNNER_H_
