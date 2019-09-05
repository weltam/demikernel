#include <chrono>
#include <fstream>
#include <iostream>

#include "benchmarkrunner.h"
#include "iodrivers/iodriver.h"
#include "pcm/pcie_monitor.h"
#include "options.h"
#include "utils/time_tracking.h"

int main(int argc, char **argv) {
  benchmark::BenchmarkOptions opts;

  // Some sanity checks for options.
  if (benchmark::BenchmarkOptions::ParseOptions(argc, argv, opts) != 0) {
    return -1;
  }
  if (!opts.Cores.HasNextCore()) {
    std::cerr << "No core mask assigned" << std::endl;
    return -1;
  }

  benchmark::PcieMonitor *monitor = nullptr;
  if (!opts.MonitorDevice.empty()) {
    monitor = new benchmark::PcieMonitor(opts.MonitorDevice);
    monitor->Init();
  }

  benchmark::BenchmarkRunner r(opts);


  // Track total runtime here so that results output to files can all use the
  // same starting time point.
  benchmark::TimeDif totalTime;
  totalTime.Start = std::chrono::steady_clock::now();

  if (monitor != nullptr) {
    monitor->Run();
  }

  int rc = r.Run();
  if (rc != 0) {
    if (monitor != nullptr) {
      monitor->Exit();
      delete monitor;
    }
    return rc;
  }
  totalTime.End = std::chrono::steady_clock::now();

  if (!opts.OutputFilePrefix.empty()) {
    if (opts.WriteIoLatency) {
      std::ofstream outFile(opts.OutputFilePrefix + "_lat.log");
      if (!outFile.good()) {
        std::cerr << "Failed to open output file: " <<
            opts.OutputFilePrefix + "_lat.log" << std::endl;
        return -1;
      }
      r.WriteResults(outFile, totalTime.Start);
      outFile.close();
    }

#ifdef POLL_LATENCY
    if (opts.WritePollLatency) {
      std::ofstream outFile(opts.OutputFilePrefix + "_poll.log");
      if (!outFile.good()) {
        std::cerr << "Failed to open output file: " <<
            opts.OutputFilePrefix + "_poll.log" << std::endl;
        return -1;
      }
      r.WritePollResults(outFile, totalTime.Start);
      outFile.close();
    }
#endif

#ifdef SUBMIT_LATENCY
    if (opts.WriteSubmissionLatency) {
      std::ofstream outFile(opts.OutputFilePrefix + "_submit.log");
      if (!outFile.good()) {
        std::cerr << "Failed to open output file: " <<
            opts.OutputFilePrefix + "_submit.log" << std::endl;
        return -1;
      }
      r.WriteSubmitResults(outFile);
      outFile.close();
    }
#endif

#ifdef BUFFER_LATENCY
    if (opts.WriteBufferLatency) {
      std::ofstream outFile(opts.OutputFilePrefix + "_buffer.log");
      if (!outFile.good()) {
        std::cerr << "Failed to open output file: " <<
            opts.OutputFilePrefix + "_buffer.log" << std::endl;
        return -1;
      }
      r.WriteBufferResults(outFile);
      outFile.close();
    }
#endif

    if (monitor != nullptr) {
      std::ofstream outFile(opts.OutputFilePrefix + "_pcie-mon.log");
      if (!outFile.good()) {
        std::cerr << "Failed to open output file: " <<
            opts.OutputFilePrefix + "_pcie-mon.log" << std::endl;
        return -1;
      }
      monitor->WriteResults(outFile, totalTime.Start);
      outFile.close();
    }
  }

  if (monitor != nullptr) {
    monitor->Exit();
    delete monitor;
  }

  std::cout << "Total benchmark runtime was " <<
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        totalTime.End - totalTime.Start).count() <<
    " ns" << std::endl;
  return rc;
}
