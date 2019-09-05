#include "iodrivers/spdk_nvme.h"

#include <cstdlib>
#include <iostream>

#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include "io_u.h"
#include "options.h"
#include "utils/parsing.h"

#include "spdk/env.h"
#include "spdk/log.h"
#include "spdk/nvme.h"
#include "spdk/vmd.h"

namespace benchmark {

using std::string;

namespace {

static constexpr char kTrTypeString[] = "trtype=";
static constexpr char kTrAddrString[] = "traddr=";

static constexpr char kOptsString[] = "a:dn:t:v";
static const option long_options[] = {
  {"device-address", required_argument, NULL, 'a'},
  {"delay-doorbell", no_argument, NULL, 'd'},
  {"namespace", required_argument, NULL, 'n'},
  {"transport-type", required_argument, NULL, 't'},
  {"use-vmd", no_argument, NULL, 'v'},
  {0, 0, 0, 0},
};

}

// Non-class functions used for callbacks.
bool probeCb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
    struct spdk_nvme_ctrlr_opts *opts);
void attachCb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
    struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts);
void opCb(void *cb_ctx, const struct spdk_nvme_cpl *cpl);

// Super jank, but use getopt again to avoid manually parsing everything. This
// should only be called once all the other args have been parsed by main.
int SpdkNvme::ParseArgs(string args) {
  unsigned int argc = NumArgs(args);
  // Make 1 larger and ignore the 0th slot because getopt won't let you parse an
  // actual arg in the location since it expects the binary name there.
  char *argv[argc + 1];
  memset(argv, 0, argc + 1);
  argv[0] = 0;
  char cArgs[args.size() + 1];
  memcpy(cArgs, args.c_str(), args.size());
  cArgs[args.size()] = 0;
  ArgsToCArgs(cArgs, &argv[1]);

  // Reset so that we can parse the new args. Must be set to 0 or 1, though
  // setting to 0 will not change the behavior.
  optind = 1;
  int options_idx = 0;
  for (int c = getopt_long(argc + 1, argv, kOptsString, long_options,
        &options_idx);
      c != -1;
      c = getopt_long(argc + 1, argv, kOptsString, long_options,
        &options_idx)) {
    switch (c) {
      case 'a':
        devAddress = string(optarg);
        break;
      case 'd':
        delayDoorbell = true;
        break;
      case 'n':
        namespaceId = std::atoi(optarg);
        break;
      case 't':
        transportType = string(optarg);
        break;
      case 'v':
        useVmd = true;
        break;
      default:
        std::cout << "Unknown option: " << c << std::endl;
    }
  }
  return 0;
}

// Right now only works for PCIe-based NVMe drives where the user specifies the
// address of a single device.
int SpdkNvme::parseTransportId(struct spdk_nvme_transport_id *trid) {
  struct spdk_pci_addr pci_addr;
  string trinfo = string(kTrTypeString) + transportType + " " + kTrAddrString +
      devAddress;
  memset(trid, 0, sizeof(*trid));
  trid->trtype = SPDK_NVME_TRANSPORT_PCIE;
  if (spdk_nvme_transport_id_parse(trid, trinfo.c_str()) < 0) {
    SPDK_ERRLOG("Failed to parse transport type and device %s\n",
        trinfo.c_str());
    return -1;
  }
  if (trid->trtype != SPDK_NVME_TRANSPORT_PCIE) {
    SPDK_ERRLOG("Unsupported transport type and device %s\n",
        trinfo.c_str());
    return -1;
  }
  if (spdk_pci_addr_parse(&pci_addr, trid->traddr) < 0) {
    SPDK_ERRLOG("invalid device address %s\n", devAddress.c_str());
    return -1;
  }
  spdk_pci_addr_fmt(trid->traddr, sizeof(trid->traddr), &pci_addr);
  return 0;
}

int SpdkNvme::Init() {
  struct spdk_env_opts opts;
  struct spdk_nvme_transport_id trid;

  // Coremask in opts defaults to 0x1.
  spdk_env_opts_init(&opts);
  opts.name = "benchmark";
  if (spdk_env_init(&opts) < 0) {
    SPDK_ERRLOG("Unable to initialize SPDK env\n");
    return 1;
  }

  // Calling this should undo the effects of the coremask given above.
  spdk_unaffinitize_thread();

  // VMD is an Intel specific way of interacting with NVMe devices.
  if (useVmd && spdk_vmd_init()) {
    SPDK_ERRLOG(
        "Failed to initialize VMD. Some NMVe devices may be unavailable\n");
  }
  if (parseTransportId(&trid) != 0) {
    return -1;
  }

  // Probe devices until we find the one we want to attach to.
  // Assuming we haven't already probed, so just probe everything here.
  if (spdk_nvme_probe(&trid, this, probeCb, attachCb, nullptr) != 0) {
    SPDK_ERRLOG("spdk_nvme_probe() failed\n");
    return -1;
  }

  return 0;
}

void SpdkNvme::Teardown() {
  if (qpair != nullptr) {
    spdk_nvme_ctrlr_free_io_qpair(qpair);
  }
  if (ctrlr != nullptr) {
    spdk_nvme_detach(ctrlr);
  }
}

char * SpdkNvme::GetBuffer(unsigned int len) {
  // For now just allocate a buffer on the host.
  char *ret = (char*) spdk_zmalloc(0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY,
      SPDK_MALLOC_DMA);
  if (ret == nullptr) {
    SPDK_ERRLOG("Unable to allocate write buffer\n");
  }
  return ret;
}

void  SpdkNvme::PutBuffer(char *buf) {
  spdk_free(buf);
}

void *SpdkNvme::OpenFile(std::string path) {
  return (void *) 1;
}

int SpdkNvme::QueueIO(Io_u *iou) {
  if (iou->offset % requestSizeMult != 0 ||
        iou->bufSize % requestSizeMult != 0) {
    SPDK_ERRLOG("Requested write that was not aligned or not a multiple of "
        "sector size: offset %lld, len %lld\n", iou->offset, iou->bufSize);
    return -1;
  }
  unsigned long long lba = iou->offset / requestSizeMult;
  unsigned long long lbaCount = iou->bufSize / requestSizeMult;
  int rc = -1;
  switch (iou->ioType) {
    case kIoRead:
      queueMutex.lock();
      rc = spdk_nvme_ns_cmd_read(ns, qpair, iou->buf, lba, lbaCount, opCb,
          iou, 0);
      queueMutex.unlock();
      if (rc != 0) {
        SPDK_ERRLOG("Unable to submit IO to queue\n");
      }
      break;
    case kIoWrite:
      queueMutex.lock();
      rc = spdk_nvme_ns_cmd_write(ns, qpair, iou->buf, lba, lbaCount, opCb,
          iou, 0);
      queueMutex.unlock();
      if (rc != 0) {
        SPDK_ERRLOG("Unable to submit IO to queue\n");
      }
      break;
    case kIoSync:
      queueMutex.lock();
      rc = spdk_nvme_ns_cmd_flush(ns, qpair, opCb, iou);
      queueMutex.unlock();
      if (rc != 0) {
        SPDK_ERRLOG("Unable to submit IO to queue\n");
      }
      break;
    default:
      SPDK_ERRLOG("Unknown IO type\n");
      rc = -1;
  }
  return rc;
}

int SpdkNvme::ProcessCompletions(unsigned int max) {
  queueMutex.lock();
  int rc = spdk_nvme_qpair_process_completions(qpair, max);
  queueMutex.unlock();
  return rc;
}

bool probeCb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
    struct spdk_nvme_ctrlr_opts *opts) {
  // Always say that we would like to attach to the controller since we aren't
  // really looking for anything specific.
  return true;
}

void attachCb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
    struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts) {
  struct spdk_nvme_io_qpair_opts qpopts;
  SpdkNvme *cp = (SpdkNvme *) cb_ctx;

  if (cp->qpair != nullptr) {
    SPDK_ERRLOG("Already attached to a qpair\n");
    return;
  }

  cp->ctrlr_opts = *opts;
  cp->ctrlr = ctrlr;
  cp->tr_id = *trid;

  cp->ns = spdk_nvme_ctrlr_get_ns(cp->ctrlr, cp->namespaceId);

  if (cp->ns == nullptr) {
    SPDK_ERRLOG("Can't get namespace by id %d\n", cp->namespaceId);
    return;
  }

  if (!spdk_nvme_ns_is_active(cp->ns)) {
    SPDK_ERRLOG("Inactive namespace at id %d\n", cp->namespaceId);
    return;
  }

  spdk_nvme_ctrlr_get_default_io_qpair_opts(cp->ctrlr, &qpopts, sizeof(qpopts));
  if (cp->delayDoorbell) {
    qpopts.delay_pcie_doorbell = true;
  } else {
    qpopts.delay_pcie_doorbell = false;
  }
  if (cp->benchmarkOpts->QueueDepth > 32) {
    qpopts.io_queue_size = cp->benchmarkOpts->QueueDepth;
    qpopts.io_queue_requests = cp->benchmarkOpts->QueueDepth * 2;
  } else {
    qpopts.io_queue_size = 32;
    qpopts.io_queue_requests = 64;
  }

  cp->qpair = spdk_nvme_ctrlr_alloc_io_qpair(cp->ctrlr, &qpopts,
      sizeof(qpopts));
  if (!cp->qpair) {
    SPDK_ERRLOG("Unable to allocate nvme qpair\n");
    return;
  }
  cp->namespaceSize = spdk_nvme_ns_get_size(cp->ns);
  if (cp->namespaceSize <= 0) {
    SPDK_ERRLOG("Unable to get namespace size for namespace %d\n",
        cp->namespaceId);
    return;
  }

  cp->requestSizeMult = spdk_nvme_ns_get_sector_size(cp->ns);
}

void opCb(void *cb_ctx, const struct spdk_nvme_cpl *cpl) {
  if (spdk_nvme_cpl_is_error(cpl)) {
    SPDK_NOTICELOG("write IO completed with error %s\n",
        spdk_nvme_cpl_get_status_string(&cpl->status));
  }
}

}  // namespace benchmark
