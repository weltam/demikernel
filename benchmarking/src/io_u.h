#ifndef IO_U_H_
#define IO_U_H_

#include <chrono>
#include <string>

#include "utils/time_tracking.h"

namespace benchmark {

enum IoStatus {
  kIoQueued,
  kIoCompleted,
  kIoFailed,
};

enum IoType {
  kIoRead,
  kIoWrite,
  kIoSync,
  // Values that don't really belong here, but are used at startup to pick a
  // workload generator.
  kIoRandRead,
  kIoRandWrite,
};

class BenchmarkFile {
 public:
  // Opaque pointer the driver can use as a handle to this file. Could be
  // something like a Unix fd or something else.
  void *DriverData;
  std::string Path;
  unsigned long long TotalLen;
};

class Io_u {
 public:
  char *buf = nullptr;
  unsigned long long int bufSize = 0;
  unsigned long long int offset = 0;
  IoType ioType;
  // Handle to the file that is being written.
  void *DriverFd = nullptr;
  TimeDif Latency;
#ifdef SUBMIT_LATENCY
  TimeDif SubmitLatency;
#endif
#ifdef BUFFER_LATENCY
  TimeDif BufferAlloc;
  TimeDif BufferFree;
#endif
  IoStatus status;
  unsigned int currentQueueDepth = 0;
  unsigned int completionBatchSize = 0;
};

}  // namespace benchmark.
#endif  // IO_U_H_
