#include "iodrivers/sync_read_write.h"

#include <cstdlib>
#include <iostream>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "io_u.h"

namespace benchmark {

using std::string;

// Ignore arguments.
int SyncReadWrite::ParseArgs(string args) {
  if (!args.empty()) {
    return -1;
  }
  return 0;
}

int SyncReadWrite::Init() {
  return 0;
}

void *SyncReadWrite::OpenFile(std::string path) {
  int tmp = open(path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  long long fd = tmp;
  //long long fd = open(path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    return nullptr;
  }
  return (void *) fd;
}

void SyncReadWrite::CloseFile(void *driverData) {
  close((unsigned long long) driverData);
}

void SyncReadWrite::PrepareForIo(Io_u *iou) {
  lseek((unsigned long long) iou->DriverFd, iou->offset, SEEK_SET);
}

int SyncReadWrite::QueueIO(Io_u *iou) {
  switch (iou->ioType) {
    case kIoRead:
      {
      unsigned int bytesRead = 0;
      while (bytesRead < iou->bufSize) {
        int rc = read((unsigned long long) iou->DriverFd, iou->buf,
            iou->bufSize - bytesRead);
        if (rc < 0) {
          return -1;
        }
        bytesRead += rc;
      }
      break;
      }
    case kIoWrite:
      {
      unsigned int written = 0;
      while (written < iou->bufSize) {
        int rc = write((unsigned long long) iou->DriverFd, iou->buf,
            iou->bufSize - written);
        if (rc < 0) {
          return -1;
        }
        written += rc;
      }
      break;
      }
    case kIoSync:
      if(fsync((unsigned long long) iou->DriverFd) != 0) {
        return -1;
      }
      break;
    default:
    return -1;
  }
  return 0;
}

}  // namespace benchmark
