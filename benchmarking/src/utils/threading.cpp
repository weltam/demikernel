#include "utils/threading.h"

#include <pthread.h>
#include <sched.h>

namespace benchmark {

using std::thread;

void pinSelfToCore(unsigned int core) {
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(core, &mask);
  sched_setaffinity(0, sizeof(mask), &mask);
}

void pinThreadToCore(unsigned int core, thread::native_handle_type nh) {
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(core, &mask);
  pthread_setaffinity_np(nh, sizeof(mask), &mask);
}

}
