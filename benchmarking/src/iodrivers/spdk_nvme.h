#ifndef ENGINE_SPDK_NVME_H_
#define ENGINE_SPDK_NVME_H_

#include <mutex>
#include <string>

#include "io_u.h"
#include "iodrivers/iodriver.h"

#include "spdk/nvme.h"

namespace benchmark {

class SpdkNvme : public AsyncIODriver {
 public:
  virtual int ParseArgs(std::string args);
  virtual int Init();
  virtual void Teardown();
  virtual void PrepareForIo(Io_u *iou) {};
  virtual int QueueIO(Io_u *iou);
  int ProcessCompletions(unsigned int max);

  // Spdk's NVMe driver has no concept of files.
  virtual void *OpenFile(std::string path);
  virtual void CloseFile(void *driverData) {};

  virtual char *GetBuffer(unsigned int len);
  virtual void PutBuffer(char *buf);

  std::string transportType;
  std::string devAddress;
  unsigned int namespaceId = 0;
  bool delayDoorbell = false;
  bool useVmd = false;

  // Spdk specific stuff.
  struct spdk_nvme_ctrlr *ctrlr = nullptr;
  struct spdk_nvme_ctrlr_opts ctrlr_opts;
  struct spdk_nvme_transport_id tr_id;
  struct spdk_nvme_qpair *qpair = nullptr;
  struct spdk_nvme_ns *ns = nullptr;
  unsigned long long namespaceSize = 0;

 private:
  int parseTransportId(struct spdk_nvme_transport_id *trid);

  std::mutex queueMutex;
};

}  // namespace benchmark
#endif  // ENGINE_SPDK_NVME_H_
