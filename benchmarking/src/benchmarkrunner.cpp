#include "benchmarkrunner.h"

#include <chrono>
#include <iostream>
#include <ratio>
#include <string>

#include <stdlib.h>

#include "io_u.h"
#include "iodrivers/iodriver.h"
#include "iodrivers/spdk_nvme.h"
#include "iodrivers/sync_read_write.h"
#include "options.h"
#include "utils/threading.h"
#include "workgenerators/workgenerator.h"

namespace benchmark {

using namespace std::chrono;
using std::mutex;
using std::ostream;
using std::string;
using std::thread;

BenchmarkRunner::BenchmarkRunner(BenchmarkOptions &o) : opts(o),
    completionsThreadRun(false), completionsThreadErr(false),
    doneQueuing(false), inFlightIos(opts.QueueDepth) {
  int *tmp = (int *) fillBuf;
  // TODO(t-asmart): Should make this a flag.
  // Fill a buffer with random data that can be used in writes.
  srand(42);
  for (unsigned int i = 0; i < 8; ++i) {
    tmp[i] = rand();
  }
  for (unsigned int i = 1; i < kFillBufSize / (8 * sizeof(int)); ++i) {
    memcpy(tmp + (i * 8), fillBuf, 8 * sizeof(int));
  }
}

void BenchmarkRunner::getIoDriver() {
  switch (opts.IoDriver) {
    case kDriverSpdk:
      ioDriver = new SpdkNvme();
      break;
    case kDriverSync:
      ioDriver = new SyncReadWrite();
      break;
    default:
      std::cerr << "Unsupported IODriver type" << std::endl;
  }
  ioDriver->benchmarkOpts = &opts;
  aioDriver = dynamic_cast<AsyncIODriver *>(ioDriver);
}

void BenchmarkRunner::getWorkloadGenerator() {
  unsigned long blkSize =  opts.BlockSize;
  if (blkSize % ioDriver->requestSizeMult != 0) {
    blkSize = ((blkSize + (ioDriver->requestSizeMult - 1)) /
        ioDriver->requestSizeMult) * ioDriver->requestSizeMult;
    std::cout << "WARNING: Block size is not a multiple of device sector size,"
        " rounding up to " << blkSize << std::endl;
  }
  if (opts.WorkloadType == kIoRandRead) {
    generator = new RandomGenerator(ioDriver->requestSizeMult, blkSize,
        opts.MaxFileSize);
    opts.WorkloadType = kIoRead;
  } else if (opts.WorkloadType == kIoRandWrite) {
    generator = new RandomGenerator(ioDriver->requestSizeMult, blkSize,
        opts.MaxFileSize);
    opts.WorkloadType = kIoWrite;
  } else {
    generator = new SequentialGenerator(blkSize);
  }
}

int BenchmarkRunner::prepareThreads() {
  if (opts.AsyncCompletions && aioDriver != nullptr) {
    return prepareAsyncThreads();
  } else {
    return prepareSyncThreads();
  }
}

int BenchmarkRunner::prepareAsyncThreads() {
  // Get cores to pin the parent and child threads to.
  int parentCore = opts.Cores.GetNextCore();
  int childCore = opts.Cores.GetNextCore();
  if (parentCore == -1 || childCore == -1) {
    std::cerr << "Unable to get cores to pin to" << std::endl;
    return -1;
  }

  completionsThreadRun = false;
  completionsThreadErr = false;
  completionsThread = thread([&] {
      checkCompletions();
    });
  pinSelfToCore(parentCore);
  pinThreadToCore(childCore, completionsThread.native_handle());
  return 0;
}

int BenchmarkRunner::prepareSyncThreads() {
  int parentCore = opts.Cores.GetNextCore();
  if (parentCore == -1) {
    std::cerr << "Unable to get cores to pin to" << std::endl;
    return -1;
  }
  pinSelfToCore(parentCore);

  return 0;
}

int BenchmarkRunner::runWorkload() {
  int rc = 0;
  totalRuntime.Start = steady_clock::now();
  testFile.DriverData = ioDriver->OpenFile(opts.TestPath);
  if (testFile.DriverData == nullptr) {
    std::cerr << "Unable to open test file " << opts.TestPath << std::endl;
    return -1;
  }
  if (opts.AsyncCompletions && aioDriver != nullptr) {
    rc = runAsyncWorkload();
  } else {
    rc = runSyncWorkload();
  }
  ioDriver->CloseFile(testFile.DriverData);
  totalRuntime.End = steady_clock::now();
  return rc;
}

int BenchmarkRunner::runAsyncWorkload() {
  int ret = 0;
  // Actually start the other thread, which we left on hold earlier because
  // setup wasn't finished.
  completionsThreadRun = true;
  unsigned int requestsQd = 0;
  // Submit our requests, this will be interleaved with the other thread getting
  // completions.
  while (requestsQd < opts.NumRequests) {
    if (completionsThreadErr) {
      ret = -1;
      break;
    }
    if (inFlightIos.Size() < opts.QueueDepth) {
      // For now, don't count the time it takes to allocate buffer memory and
      // fill it in if needed.
      // Create new tracking Io_u.
      Io_u *iou = allocateAndFillIou();
      if (iou == nullptr) {
        ret = -1;
        completionsThreadRun = false;
        break;
      }
      ioDriver->PrepareForIo(iou);

      iou->Latency.Start = steady_clock::now();
      // Doesn't really matter if we use aioDriver or ioDriver since they both
      // define QueueIO().
      if (aioDriver->QueueIO(iou) != 0) {
        ret = -1;
        completionsThreadRun = false;
        iou->status = kIoFailed;
        iou->Latency.End = steady_clock::now();
#ifdef SUBMIT_LATENCY
        iou->SubmitLatency.End = iou->Latency.End;
#endif
        break;
      }
#ifdef SUBMIT_LATENCY
      iou->SubmitLatency.End = steady_clock::now();
      iou->SubmitLatency.Start = iou->Latency.Start;
#endif

      iou->status = kIoQueued;
      inFlightIos.Push(iou);
      iou->currentQueueDepth = inFlightIos.Size();
      ++requestsQd;
    }
    // Random sleep so that Optane behaves better on writes < 4k for whatever
    // random reason. Need to compile with -O0 to actually use though.
    //for (unsigned int i = 0; i < 5000; ++i) {}
  }

  // Wait for the other thread to either error out or drain the request queue
  // that we built up earlier so that we don't inadvertently cancel them by
  // calling Teardown().
  doneQueuing = true;
  completionsThread.join();
  return ret;
}

int BenchmarkRunner::runSyncWorkload() {
  int ret = 0;
  unsigned long long requestsQd = 0;
  while (requestsQd < opts.NumRequests) {
    // For now, don't count the time it takes to allocate buffer memory and
    // fill it in if needed.
    Io_u *iou = allocateAndFillIou();
    if (iou == nullptr) {
      ret = -1;
      break;
    }
    ioDriver->PrepareForIo(iou);
    iou->Latency.Start = steady_clock::now();

    if (ioDriver->QueueIO(iou) != 0) {
      ret = -1;
      iou->status = kIoFailed;
      iou->Latency.End = steady_clock::now();
#ifdef SUBMIT_LATENCY
      iou->SubmitLatency = iou->Latency;
#endif
      ioDriver->PutBuffer(iou->buf);
      break;
    }
    iou->Latency.End = steady_clock::now();
#ifdef SUBMIT_LATENCY
    iou->SubmitLatency = iou->Latency;
#endif
#ifdef BUFFER_LATENCY
    iou->BufferFree.Start = steady_clock::now();
#endif
    ioDriver->PutBuffer(iou->buf);
#ifdef BUFFER_LATENCY
    iou->BufferFree.End = steady_clock::now();
#endif

    iou->status = kIoCompleted;
    inFlightIos.Push(iou);
    ++requestsQd;
  }
  return ret;
}

int BenchmarkRunner::Run() {
  checkClockResolution();
  getIoDriver();
  // Get a reference to an IODriver based on options given at construction.
  // Setup IO stuff.
  if (ioDriver == nullptr) {
    std::cerr << "No IODriver given" << std::endl;
    return -1;
  }
  if (aioDriver == nullptr && opts.AsyncCompletions) {
    std::cout << "WARNING: async completions flag given for sync IODriver" <<
        std::endl;
  }
  if (ioDriver->ParseArgs(opts.DriverOpts) != 0) {
    std::cerr << "Error parsing IODriver opts" << std::endl;
    delete ioDriver;
    return -1;
  }
  if (ioDriver->Init() != 0) {
    std::cerr << "Unable to initialize IODriver" << std::endl;
    delete ioDriver;
    return -1;
  }
  getWorkloadGenerator();
  if (generator == nullptr) {
    std::cerr << "Unable to get workload generator" << std::endl;
    ioDriver->Teardown();
    delete ioDriver;
    return -1;
  }
  if (prepareThreads() != 0) {
    ioDriver->Teardown();
    delete generator;
    ioDriver->Teardown();
    delete ioDriver;
    return -1;
  }

  int rc = runWorkload();

  if (rc != 0) {
    std::cout << "Benchmark failed" << std::endl;
  }

  delete generator;
  ioDriver->Teardown();
  delete ioDriver;
  return rc;
}

void BenchmarkRunner::checkCompletions() {
  // We spin up the thread before we have finished setting stuff up, so wait
  // here until setup is done.
  while (!completionsThreadRun) {}
  for (unsigned int batchSize = inFlightIos.Size();
      completionsThreadRun && !(doneQueuing && batchSize == 0);
      batchSize = inFlightIos.Size()) {
    if (batchSize > 5 || doneQueuing) {
      // Make sure we only complete up to the number of requests we saw in the
      // queue when we actually held the mutex so that we don't accidentally do
      // something weird.
      batchSize = (batchSize < opts.CompleteBatch) ?
          batchSize : opts.CompleteBatch;
#ifdef POLL_LATENCY
      steady_clock::time_point sTime = steady_clock::now();
#endif
      int rc = aioDriver->ProcessCompletions(batchSize);
      steady_clock::time_point eTime = steady_clock::now();
      // TODO(t-asmart): Properly handle when the return is < 0. In this case, I
      // guess we can assume that the final request in the set failed, and all
      // the others completed successfully before it.
      if (rc < 0) {
        // Exit on error, will exit on next loop var check.
        completionsThreadErr = true;
        break;
      }
      // Get complete time before we may have to wait on lock since the IO was
      // technically already completed.
#ifdef POLL_LATENCY
      pollTimes.emplace_back(TimeDif(sTime, eTime), rc);
#endif
      for (int i = 0; i < rc; ++i) {
        Io_u *iou = inFlightIos.Pop();
        iou->Latency.End = eTime;
        iou->status = kIoCompleted;
        iou->completionBatchSize = rc;
        // Return the IO buffer used in this operation.
#ifdef BUFFER_LATENCY
        iou->BufferFree.Start = steady_clock::now();
#endif
        aioDriver->PutBuffer(iou->buf);
#ifdef BUFFER_LATENCY
        iou->BufferFree.End = steady_clock::now();
#endif
      }
    }
  }
}

Io_u *BenchmarkRunner::allocateAndFillIou() {
  Io_u tmp;
  if (opts.ForceSync && !lastWasSync) {
    lastWasSync = true;
    tmp.ioType = kIoSync;
  } else {
    tmp.ioType = opts.WorkloadType;
    lastWasSync = false;
  }
  tmp.DriverFd = testFile.DriverData;
  generator->FillOffsetAndLen(&tmp);
#ifdef BUFFER_LATENCY
  char *buf = allocateAndPopulateBuffer(tmp.bufSize, tmp.BufferAlloc);
#else
  char *buf = allocateAndPopulateBuffer(tmp.bufSize);
#endif
  if (buf == nullptr) {
    return nullptr;
  }
  // Don't allocate an Io_u in the list until we know we can get a buffer.
  allIos.push_back(tmp);
  Io_u *iou = &allIos.back();
  iou->buf = buf;
  return iou;
}

#ifndef BUFFER_LATENCY
char *BenchmarkRunner::allocateAndPopulateBuffer(unsigned long long len) {
#else
char *BenchmarkRunner::allocateAndPopulateBuffer(unsigned long long len,
    TimeDif &latency) {
  latency.Start = steady_clock::now();
#endif
  char *buf = ioDriver->GetBuffer(len);
#ifdef BUFFER_LATENCY
  latency.End = steady_clock::now();
#endif
  if (buf == nullptr) {
    return buf;
  }
  populateBuffer(buf, len);
  return buf;
}

void BenchmarkRunner::populateBuffer(char *buf, unsigned int len) {
  unsigned int filled = 0;
  while (filled < len) {
    unsigned int max = (len - filled < kFillBufSize) ?
        len - filled : kFillBufSize;
    memcpy(buf + filled, fillBuf, max);
    filled += max;
  }
}

void BenchmarkRunner::checkClockResolution() {
  if (std::ratio_greater<
        steady_clock::duration::period,
        std::nano>::value) {
    std::cout << "WARNING: clock resolution is less than nanoseconds! " <<
      "Output will be in nanoseconds, but may not be accurate" << std::endl;
  }
}

/*
 * Output files have the following columns:
 *   1. time since start of test run (ns)
 *   2. latency of operation (ns)
 *   3. buffer size
 *   4. current queue depth at submission
 *   5. number of operations completed at the same time as this on
 */
void BenchmarkRunner::WriteResults(ostream &outFile,
    std::chrono::steady_clock::time_point startTime) {
  for (const auto &iou : allIos) {
    outFile <<
      duration_cast<nanoseconds>(
          iou.Latency.Start - startTime).count() <<
      ", " <<
      duration_cast<nanoseconds>(iou.Latency.End - iou.Latency.Start).count() <<
      ", " << iou.bufSize << ", " << iou.currentQueueDepth <<
      ", " << iou.completionBatchSize << std::endl;
  }
}

#ifdef POLL_LATENCY
/*
 * Output files have the following columns:
 *   1. time since start of test run (ns)
 *   2. latency of operation (ns)
 *   3. number of operations completed in call
 */
void BenchmarkRunner::WritePollResults(ostream &outFile,
    std::chrono::steady_clock::time_point startTime) {
  for (const auto &poll : pollTimes) {
    outFile <<
      duration_cast<nanoseconds>(
          poll.first.Start - startTime).count() <<
      ", " <<
        duration_cast<nanoseconds>(poll.first.End - poll.first.Start).count() <<
      ", " << poll.second << std::endl;
  }
}
#endif

#ifdef SUBMIT_LATENCY
/*
 * Output files have the following columns:
 *   1. time since start of test run (ns)
 *   2. latency of operation (ns)
 */
void BenchmarkRunner::WriteSubmitResults(ostream &outFile,
    std::chrono::steady_clock::time_point startTime) {
  for (const auto &iou : allIos) {
    outFile <<
        duration_cast<nanoseconds>(
            iou.SubmitLatency.End - startTime).count() << "," <<
        duration_cast<nanoseconds>(
            iou.SubmitLatency.End - iou.SubmitLatency.Start).count() <<
        std::endl;
  }
}
#endif

#ifdef BUFFER_LATENCY
/*
 * Output files have the following columns:
 *   1. latency of alloc operation (ns)
 *   2. latency of free operation (ns)
 */
void BenchmarkRunner::WriteBufferResults(ostream &outFile) {
  for (const auto &iou : allIos) {
    outFile <<
        duration_cast<nanoseconds>(
            iou.BufferAlloc.End - iou.BufferAlloc.Start).count() <<
        ", " <<
        duration_cast<nanoseconds>(
            iou.BufferFree.End - iou.BufferFree.Start).count() <<
        std::endl;
  }
}
#endif

}  // namespace benchmark.
