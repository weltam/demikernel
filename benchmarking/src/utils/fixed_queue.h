#ifndef UTILS_FIXED_QUEUE_H_
#define UTILS_FIXED_QUEUE_H_

#include <atomic>
#include <vector>

#include "io_u.h"

namespace benchmark {

// Not thread safe. No safety checks for wrapping, so user must ensure
// pushes/pops will be vaild with IsEmpty()/IsFull() calls. Also don't want to
// deal with templating madness right now, so this will only be for Io_u* for
// now unless I really need it for something else.
class FixedQueue {
 public:
  FixedQueue(unsigned int capacity);
  unsigned int Size();
  void Push(Io_u *obj);
  Io_u * Pop();

 private:
  const unsigned int cap;
  unsigned int head;
  unsigned int tail;
  std::atomic<unsigned int> qd;
  std::vector<Io_u *> q;
};

}  // namespace benchmark.
#endif  // UTILS_FIXED_QUEUE_H_
