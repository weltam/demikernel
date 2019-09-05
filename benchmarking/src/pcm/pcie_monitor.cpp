#include "pcm/pcie_monitor.h"

#include <iostream>
#include <vector>

namespace benchmark {

namespace {

static const unsigned int kMaxStacks = 6;
static const unsigned int kMaxParts = 4;
static const unsigned int kMaxDevs = 32;
static const unsigned int kMaxFuncs = 8;
static const unsigned int kMaxCounters = 4;

static const unsigned long long kMonitorSleepMs = 100;

struct PmuEventFields {
  unsigned long long EventCode;
  unsigned long long Umask;
  unsigned long long FcMask;
  unsigned long long ChMaskBase;
  unsigned int Multiplier;
  EventType Event;
};

// Outbound.
static constexpr PmuEventFields kTxnByCpuMemWrite {
  0xc1,
  0x1,
  0x7,
  0x1,
  1,
  kEventTxnByCpuMemWrite,
};

static constexpr PmuEventFields kTxnByCpuMemRead {
  0xc1,
  0x4,
  0x7,
  0x1,
  1,
  kEventTxnByCpuMemRead,
};

static constexpr PmuEventFields kTxnByCpuIoWrite {
  0xc1,
  0x20,
  0x7,
  0x1,
  1,
  kEventTxnByCpuIoWrite,
};

static constexpr PmuEventFields kTxnByCpuIoRead {
  0xc1,
  0x80,
  0x7,
  1,
  kEventTxnByCpuIoRead,
};

// Inbound.
static constexpr PmuEventFields kTxnOfCpuMemWrite {
  0x84,
  0x1,
  0x7,
  0x1,
  64,
  kEventTxnOfCpuMemWrite,
};

static constexpr PmuEventFields kTxnOfCpuMemRead {
  0x84,
  0x4,
  0x7,
  0x1,
  64,
  kEventTxnOfCpuMemRead,
};

}  // namespace.

using std::string;
using std::thread;
using std::vector;

PcieMonitor::PcieMonitor(string targetDevice) : devStack(0), devSocket(0),
    devPart(0) {
  size_t p = 0;
  // Remove the group from the front of the address.
  std::stoul(targetDevice, &p, 16);
  targetDevice = targetDevice.substr(p + 1);
  target.busno = (unsigned char) std::stoul(targetDevice, &p, 16);
  targetDevice = targetDevice.substr(p + 1);
  target.devno = (unsigned char) std::stoul(targetDevice, &p, 16);
  targetDevice = targetDevice.substr(p + 1);
  target.funcno = (unsigned char) std::stoul(targetDevice, &p, 16);
}

PcieMonitor::~PcieMonitor() {
  if (pcm != nullptr) {
    pcm->cleanup();
    // TODO(t-asmart): Memory leak, but I can't actually delete the PCM without
    // causeing the program to abort because they don't join threads the spin
    // off.
    //delete pcm;
  }
}

int PcieMonitor::Init() {
  int ret = 0;
  pcm = PCM::getInstance();
  PCM::ErrorCode status = pcm->program();
  switch (status) {
    case PCM::Success:
      break;
    case PCM::MSRAccessDenied:
      std::cerr << "No MSR or PCI CFG space access" << std::endl;
      ret = -1;
      break;
    case PCM::PMUBusy:
      std::cerr << "PMU is being used by another program" << std::endl;
      ret = -1;
      break;
    default:
      std::cerr << "Unknown error" << std::endl;
      ret = -1;
      break;
  }

  numSockets = pcm->getNumSockets();
  if (numSockets > 4) {
    std::cerr << "systems with more than 4 sockets aren't supported" <<
        std::endl;
    ret = -1;
  }

  // Find the PCIe device we want to monitor.
  ret = findDeviceInTree(target, &devStack, &devSocket, &devPart);
  if (ret != 0) {
    std::cerr << "unable to find device" << std::endl;
  }
  return ret;
}

void PcieMonitor::Run() {
  worker = thread([&] {
    // Can hold up to 4 events.
    vector<PmuEventFields> events {kTxnByCpuMemWrite, kTxnByCpuMemRead,
        kTxnOfCpuMemWrite, kTxnOfCpuMemRead};
    IIOPMUCNTCTLRegister rawCounterRegs[kMaxCounters];
    for (unsigned int i = 0; i < events.size(); ++i) {
      rawCounterRegs[i].fields.event_select = events[i].EventCode;
      rawCounterRegs[i].fields.umask = events[i].Umask;
      rawCounterRegs[i].fields.fc_mask = events[i].FcMask;
      // Shift up enough to get to the right part.
      rawCounterRegs[i].fields.ch_mask = events[i].ChMaskBase << devPart;
    }

    while (1) {
      if (exitVar) {
        break;
      }
      // Do some fun stuff with PCM's IIO here.
      // Reset counters for the stack we're looking at.
      pcm->programIIOCounters(rawCounterRegs, devStack);

      IIOCounterState before[events.size()];
      TimeDif times[events.size()];
      for (unsigned int counter = 0; counter < events.size(); ++counter) {
        // Pull starting values from the counters that we are actually using
        // to monitor events.
        times[counter].Start = std::chrono::steady_clock::now();
        before[counter] = pcm->getIIOCounterState(devSocket, devStack, counter);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(kMonitorSleepMs));
      for (unsigned int counter = 0; counter < events.size(); ++counter) {
        // Pull ending values from the counters that we are actually using
        // to monitor events.
        IIOCounterState after =
            pcm->getIIOCounterState(devSocket, devStack, counter);
        times[counter].End = std::chrono::steady_clock::now();
        data.push_back({
            times[counter],
            getNumberOfEvents(before[counter], after),
            events[counter].Event,
        });
      }
    }
  });
}

void PcieMonitor::Exit() {
  exitVar = true;
  worker.join();
}

int PcieMonitor::findDeviceInTree(struct bdf &target, unsigned int *devStack,
    unsigned int *devSocket, unsigned int *devPart) {
  vector<unsigned int> startingBusses;
  switch (numSockets) {
    case 1:
      // Fallthrough.
    case 2:
      startingBusses.push_back(0x0);
      startingBusses.push_back(0x80);
      break;
    case 4:
      startingBusses.push_back(0x0);
      startingBusses.push_back(0x40);
      startingBusses.push_back(0x80);
      startingBusses.push_back(0xc0);
      break;
    default:
      return -1;
  }

  for (unsigned int i = 0; i < startingBusses.size(); ++i) {
    struct iio_skx iio_skx;

    unsigned int cpubusno = 0;
    // Get the Ubox register.
    if (PciHandleType::exists(0, startingBusses[i], 8, 2)) {
      iio_skx.socket_id = i;
      PciHandleType h(0, startingBusses[i], 8, 2);
      // Read the CPUBUSNO registers from Ubox.
      h.read32(0xcc, &cpubusno);
      iio_skx.stacks[0].busno = cpubusno & 0xff;
      iio_skx.stacks[1].busno = (cpubusno >> 8) & 0xff;
      iio_skx.stacks[2].busno = (cpubusno >> 16) & 0xff;
      iio_skx.stacks[3].busno = (cpubusno >> 24) & 0xff;
      h.read32(0xd0, &cpubusno);
      iio_skx.stacks[4].busno = cpubusno & 0xff;
      iio_skx.stacks[5].busno = (cpubusno >> 8) & 0xff;

      // See if the PCIe bridges at the busses read above exist.
      for (unsigned int stack = 0; stack < kMaxStacks; ++stack) {
        unsigned int bus = iio_skx.stacks[stack].busno;
        // Check for different ports on the bridges for the busses we found.
        for (unsigned int part = 0; part < kMaxParts; ++part) {
          struct pci *pci = &iio_skx.stacks[stack].parts[part].root_pci_dev;
          pci->bdf.busno = bus;
          pci->bdf.devno = part;
          pci->bdf.funcno = 0;
          if (stack != 0 && bus == 0) {
            // Workaround for some IIO stack that may not exist.
            pci->exist = false;
          } else {
            // Call into PCM.
            probe_pci(pci);
          }
        }
      }

      // Find devices connected to the bridges from above.
      for (unsigned int stack = 0; stack < kMaxStacks; ++stack) {
        for (unsigned int part = 0; part < kMaxParts; ++part) {
          struct pci p = iio_skx.stacks[stack].parts[part].root_pci_dev;
          if (!p.exist) {
            continue;
          } else if (bdfsEqual(p.bdf, target)) {
            // If for some reason one of the bridges is the device we're looking
            // for, record that and return.
            *devStack = stack;
            *devSocket = i;
            *devPart = part;
            return 0;
          }

          // Scan for devices on the other side of the bridge.
          for (unsigned int otherBus = p.secondary_bus_number;
              otherBus <= p.subordinate_bus_number; ++otherBus) {
            for (unsigned int dev = 0; dev < kMaxDevs; ++dev) {
              for (unsigned int func = 0; func < kMaxFuncs; ++func) {
                struct pci pci;
                pci.exist = false;
                pci.bdf.busno = otherBus;
                pci.bdf.devno = dev;
                pci.bdf.funcno = func;
                probe_pci(&pci);
                if (pci.exist && bdfsEqual(pci.bdf, target)) {
                  *devStack = stack;
                  *devSocket = i;
                  *devPart = part;
                  return 0;
                }
              }
            }
          }
        }
      }
    }
  }
  return -1;
}

bool PcieMonitor::bdfsEqual(struct bdf &bdf1, struct bdf &bdf2) {
  return bdf1.busno == bdf2.busno &&
    bdf1.devno == bdf2.devno &&
    bdf1.funcno == bdf2.funcno;
}

/*
 * Output files have the following columns:
 *   1. time since start of test run (ns)
 *   2. latency of operation (ns)
 *   3. number of events
 *   4. event type
 */
void PcieMonitor::WriteResults(std::ostream &outFile,
      std::chrono::steady_clock::time_point startTime) {
  for (const auto &reading : data) {
    outFile <<
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          reading.Time.Start - startTime).count() <<
      ", " <<
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          reading.Time.End - reading.Time.Start).count() <<
      ", " << reading.Data << ", " << reading.Event << std::endl;
  }
}

}  // namespace benchmark.
