#ifndef UTTILS_TIME_TRACKING_H_
#define UTTILS_TIME_TRACKING_H_

#include <chrono>

namespace benchmark {

class TimeDif {
 public:
  TimeDif();
  TimeDif(std::chrono::steady_clock::time_point sTime,
      std::chrono::steady_clock::time_point eTime);

  std::chrono::steady_clock::time_point Start;
  std::chrono::steady_clock::time_point End;
};

}  // namespace benchmark.
#endif  // UTTILS_TIME_TRACKING_H_
