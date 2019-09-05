#ifndef DRIVER_IODRIVER_H_
#define DRIVER_IODRIVER_H_

#include <string>

#include "io_u.h"
#include "iodrivers/driver_types.h"
#include "options.h"

namespace benchmark {

class IODriver {
 public:
  BenchmarkOptions *benchmarkOpts;

  virtual ~IODriver() {};
  virtual int ParseArgs(std::string args) = 0;
  virtual int Init() = 0;
  virtual void Teardown() = 0;
  // Used to set file offsets etc. before IO is performed.
  virtual void PrepareForIo(Io_u *iou) = 0;
  virtual int QueueIO(Io_u *iou) = 0;

  virtual void *OpenFile(std::string path) = 0;
  virtual void CloseFile(void *driverData) = 0;

  // Used to get memory buffers that will be in IO request. Newly allocated
  // buffers should be zeroed.
  virtual char *GetBuffer(unsigned int len);
  // Return memory buffer used in IO request.
  virtual void PutBuffer(char *buf);

  // Some devices can only handle full sectors. This carries the sector size
  // that all requests must be a multiple of.
  unsigned int requestSizeMult = 1;
};

class AsyncIODriver : public IODriver {
 public:
  virtual ~AsyncIODriver() {};
  virtual int ParseArgs(std::string args) = 0;
  virtual int Init() = 0;
  virtual void Teardown() = 0;
  virtual void PrepareForIo(Io_u *iou) = 0;
  virtual int QueueIO(Io_u *iou) = 0;
  virtual int ProcessCompletions(unsigned int max) = 0;

  virtual void *OpenFile(std::string path) = 0;
  virtual void CloseFile(void *driverData) = 0;
};

}  // namespace benchmark.
#endif  // DRIVER_IODRIVER_H_
