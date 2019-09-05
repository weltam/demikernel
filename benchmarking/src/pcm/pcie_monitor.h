#ifndef PCM_PCIE_MONITOR_H_
#define PCM_PCIE_MONITOR_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>

#include "utils/time_tracking.h"

#include "cpucounters.h"
#include "lspci.h"

namespace benchmark {

enum EventType {
  kEventTxnByCpuMemRead,
  kEventTxnByCpuMemWrite,
  kEventTxnByCpuIoWrite,
  kEventTxnByCpuIoRead,
  kEventTxnOfCpuMemWrite,
  kEventTxnOfCpuMemRead,
};

struct MonitorData {
 public:
  TimeDif Time;
  unsigned long long Data;
  EventType Event;
};

class PcieMonitor {
 public:
  PcieMonitor(std::string targetDevice, unsigned int tcore);
  ~PcieMonitor();
  int Init();
  void Run();
  void Exit();
  void WriteResults(std::ostream &outFile,
      std::chrono::steady_clock::time_point startTime);

 private:
  unsigned int core;
  std::thread worker;
  PCM *pcm = nullptr;
  struct bdf target;
  std::atomic<bool> exitVar;
  std::list<MonitorData> data;

  unsigned int numSockets;
  unsigned int devStack;
  unsigned int devSocket;
  unsigned int devPart;

  int findDeviceInTree(struct bdf &target, unsigned int *devStack,
      unsigned int *devSocket, unsigned int *devPart);
  bool bdfsEqual(struct bdf &bdf1, struct bdf &bdf2);
};

}  // namespace benchmark.
#endif  // PCM_PCIE_MONITOR_H_
