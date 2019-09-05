#ifndef UTILS_THREADING_H_
#define UTILS_THREADING_H_

#include <thread>

namespace benchmark {

void pinSelfToCore(unsigned int core);
void pinThreadToCore(unsigned int core, std::thread::native_handle_type nh);

}  // namespace benchmark.
#endif  // UTILS_THREADING_H_
