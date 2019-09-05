#include "iodrivers/iodriver.h"

#include "io_u.h"

namespace benchmark {

char * IODriver::GetBuffer(unsigned int len) {
  return (char *) calloc(len, 1);
}

void IODriver::PutBuffer(char *buf) {
  free(buf);
}

}  // namespace benchmark.
