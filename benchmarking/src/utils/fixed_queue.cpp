#include "utils/fixed_queue.h"

#include <vector>

namespace benchmark {

using std::vector;

FixedQueue::FixedQueue(unsigned int capacity) :
    cap(capacity), head(0), tail(0), qd(0),
    q(vector<Io_u *>(capacity, nullptr)) {}

unsigned int FixedQueue::Size() {
  return qd.load(std::memory_order_acquire);
}

void FixedQueue::Push(Io_u * obj) {
  q[head] = obj;
  head = (head + 1 >= cap) ? 0 : head + 1;
  qd.fetch_add(1, std::memory_order_acq_rel);
}

Io_u * FixedQueue::Pop() {
  Io_u * ret = q[tail];
  tail = (tail + 1 >= cap) ? 0 : tail + 1;
  qd.fetch_sub(1, std::memory_order_acq_rel);
  return ret;
}

}  // namespace benchmark.
