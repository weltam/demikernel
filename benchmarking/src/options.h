#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <string>
#include <vector>

#include "io_u.h"
#include "iodrivers/driver_types.h"

namespace benchmark {

class CoreMask {
 public:
  CoreMask();
  CoreMask(const char *m);
  bool HasNextCore();
  int GetNextCore();

 private:
  std::vector<unsigned int> cores;
  std::vector<unsigned int>::iterator nextCore;
};

class BenchmarkOptions {
 public:
  bool AsyncCompletions = false;
  bool ForceSync = false;
  // Cores should be in a comma separated list in the following order:
  // 1. PCIe monitoring core (if monitoring is enabled)
  // 2. Core for submission thread
  // 3. Core for completion thread
  // Note that you can repeat the final 2 options for multi-threaded tests.
  CoreMask Cores;
  IODriverType IoDriver;
  std::string DriverOpts;
  unsigned int QueueDepth = 1;
  unsigned int CompleteBatch = 1;
  unsigned int NumRequests = 100;
  unsigned long long IoLimitBytes = 0;
  bool ReqLimit = true;
  unsigned long long BlockSize = 4096;
  unsigned long long MaxFileSize = 0;
  std::string TestPath;
  std::string OutputFilePrefix;
  IoType WorkloadType = kIoWrite;

  std::string MonitorDevice;

  bool WriteIoLatency = false;
  bool WritePollLatency = false;
  bool WriteSubmissionLatency = false;
  bool WriteBufferLatency = false;

  static int ParseOptions(int argc, char **argv, BenchmarkOptions &options);
};

}  // namespace benchmark.

#endif  // OPTIONS_H_
