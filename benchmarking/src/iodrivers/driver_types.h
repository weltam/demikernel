#ifndef IODRIVERS_DRIVER_TYPES_H_
#define IODRIVERS_DRIVER_TYPES_H_

namespace benchmark {

enum IODriverType {
  kDriverSync,
  kDriverAio,
  kDriverSpdk,
};

}  // namespace benchmark.
#endif  // IODRIVERS_DRIVER_TYPES_H_
