#include "options.h"

#include <cstdlib>
#include <iostream>
#include <string>

#include <getopt.h>
#include <string.h>

namespace benchmark {

using std::string;
using std::vector;

namespace {

static constexpr char kOptsString[] = "M:S:ab:c:d:f:i:m:n:o:p:q:st:";

static const option long_options[] = {
  // Mask value must be in hex.
  {"monitor-device", required_argument, NULL, 'M'},
  {"io-bytes", required_argument, NULL, 'I'},
  {"async-completions", no_argument, NULL, 'a'},
  {"complete-batch", required_argument, NULL, 'b'},
  {"core-mask", required_argument, NULL, 'c'},
  {"io-driver", required_argument, NULL, 'd'},
  {"output-file-prefix", required_argument, NULL, 'f'},
  {"io-size", required_argument, NULL, 'i'},
  {"max-file-size", required_argument, NULL, 'm'},
  {"num-requests", required_argument, NULL, 'n'},
  {"io-driver-opts", required_argument, NULL, 'o'},
  {"test-path", required_argument, NULL, 'p'},
  {"queue-depth", required_argument, NULL, 'q'},
  {"force-sync", no_argument, NULL, 's'},
  {"io-type", required_argument, NULL, 't'},
  // Things that only really have longopt form.
  {"latency-results", no_argument, NULL, 1},
  {"poll-results", no_argument, NULL, 2},
  {"submission-results", no_argument, NULL, 3},
  {"buffer-results", no_argument, NULL, 4},
  {0, 0, 0, 0},
};

static constexpr char kIoDriverSpdk[] = "spdk";
static constexpr char kIoDriverSync[] = "sync";
static constexpr char kIoDriverAio[] = "aio";

static constexpr char kIoReadType[] = "read";
static constexpr char kIoWriteType[] = "write";
static constexpr char kIoRandReadType[] = "randread";
static constexpr char kIoRandWriteType[] = "randwrite";

static const unsigned int kMaxQDepth = 8192;

}  // namespace.

// No cores in an empty core mask.
CoreMask::CoreMask() : cores(vector<unsigned int>()), nextCore(cores.end()) {}

CoreMask::CoreMask(const char *m) {
  string rawList(m);

  size_t pos = 0;
  while (pos < rawList.size()) {
    rawList = rawList.substr(pos);
    cores.push_back(std::stoul(rawList, &pos));
    // Assume comma separated list, so this should skip over the comma and get
    // ready for the next core in the list.
    ++pos;
  }
  nextCore = cores.begin();
}

bool CoreMask::HasNextCore() {
  return nextCore != cores.end();
}

int CoreMask::GetNextCore() {
  if (nextCore == cores.end()) {
    return -1;
  }
  unsigned int ret = *nextCore;
  ++nextCore;
  return ret;
}

int BenchmarkOptions::ParseOptions(int argc, char **argv,
    BenchmarkOptions &opts) {
  bool numReqs = false;
  // Reset just in case.
  int options_idx = 0;
  optind = 1;
  for (int c = getopt_long(argc, argv, kOptsString, long_options, &options_idx);
      c != -1;
      c = getopt_long(argc, argv, kOptsString, long_options, &options_idx)) {
    switch (c) {
      case 1:
        opts.WriteIoLatency = true;
        break;
      case 2:
        opts.WritePollLatency = true;
        break;
      case 3:
        opts.WriteSubmissionLatency = true;
        break;
      case 4:
        opts.WriteBufferLatency = true;
        break;
      case 'M':
        opts.MonitorDevice = string(optarg);
        break;
      case 'I':
        if (numReqs) {
          std::cerr << "Only one of io-bytes or num-requests can be set" <<
            std::endl;
          return -1;
        }
        opts.IoLimitBytes = std::stoull(optarg);
        opts.ReqLimit = false;
        break;
      case 'a':
        opts.AsyncCompletions = true;
        break;
      case 'b':
        opts.CompleteBatch = atoi(optarg);
        break;
      case 'c':
        {
        opts.Cores = CoreMask(optarg);
        break;
        }
      case 'd':
        if (strnlen(optarg, strlen(kIoDriverSpdk) + 1) ==
                strlen(kIoDriverSpdk) &&
            strncmp(kIoDriverSpdk, optarg, strlen(kIoDriverSpdk)) == 0) {
          opts.IoDriver = kDriverSpdk;
        } else if (strnlen(optarg, strlen(kIoDriverSync) + 1) ==
                strlen(kIoDriverSync) &&
            strncmp(kIoDriverSync, optarg, strlen(kIoDriverSync)) == 0) {
          opts.IoDriver = kDriverSync;
/*
        } else if (strncmp(kIoDriverAio, optarg, strlen(kIoDriverAio)) == 0) {
          opts.IoDriver = kDriverAio;
*/
        } else {
          std::cerr << "Bad IO type: " << optarg << std::endl;
          return -1;
        }
        break;
      case 'f':
        opts.OutputFilePrefix = string(optarg);
        break;
      case 'i':
        opts.BlockSize = std::stoull(optarg);
        break;
      case 'm':
        opts.MaxFileSize = std::stoull(optarg);
        break;
      case 'n':
        if (!opts.ReqLimit) {
          std::cerr << "Only one of io-bytes or num-requests can be set" <<
            std::endl;
          return -1;
        }
        opts.NumRequests = atoi(optarg);
        numReqs = true;
        break;
      case 'o':
        opts.DriverOpts = string(optarg);
        break;
      case 'p':
        opts.TestPath = string(optarg);
        break;
      case 'q':
        opts.QueueDepth = std::atoi(optarg);
        if (opts.QueueDepth > kMaxQDepth) {
          std::cerr << "Queue depth too large: " << opts.QueueDepth;
          return -1;
        }
        break;
      case 's':
        opts.ForceSync = true;
        break;
      case 't':
        if (strnlen(optarg, strlen(kIoReadType) + 1) ==
                strlen(kIoReadType) &&
            strncmp(optarg, kIoReadType, strlen(kIoReadType)) == 0) {
          opts.WorkloadType = kIoRead;
        } else if (strnlen(optarg, strlen(kIoWriteType) + 1) ==
                strlen(kIoWriteType) &&
            strncmp(optarg, kIoWriteType, strlen(kIoWriteType)) == 0) {
          opts.WorkloadType = kIoWrite;
        } else if (strnlen(optarg, strlen(kIoRandReadType) + 1) ==
                strlen(kIoRandReadType) &&
            strncmp(optarg, kIoRandReadType, strlen(kIoRandReadType)) == 0) {
          opts.WorkloadType = kIoRandRead;
        } else if (strnlen(optarg, strlen(kIoRandWriteType) + 1) ==
                strlen(kIoRandWriteType) &&
            strncmp(optarg, kIoRandWriteType, strlen(kIoRandWriteType)) == 0) {
          opts.WorkloadType = kIoRandWrite;
        } else {
          std::cerr << "Unknown workload type: " << optarg << std::endl;
          return -1;
        }
        break;
      default:
        std::cerr << "Unknown option: " << c << std::endl;
    }
  }
  return 0;
}

}  // namespace benchmark.
