#ifndef ENGINE_SYNC_READ_WRITE_H_
#define ENGINE_SYNC_READ_WRITE_H_

#include <mutex>
#include <string>

#include "io_u.h"
#include "iodrivers/iodriver.h"

namespace benchmark {

class SyncReadWrite : public IODriver {
 public:
  virtual int ParseArgs(std::string args);
  virtual int Init();
  virtual void Teardown() {};
  virtual void PrepareForIo(Io_u *iou);
  virtual int QueueIO(Io_u *iou);

  virtual void *OpenFile(std::string path);
  virtual void CloseFile(void *driverData);
};

}  // namespace benchmark
#endif  // ENGINE_SYNC_READ_WRITE_H_
