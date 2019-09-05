#include "utils/time_tracking.h"

namespace benchmark {

using namespace std::chrono;

TimeDif::TimeDif() : Start(steady_clock::time_point()),
    End(steady_clock::time_point()) {}

TimeDif::TimeDif(steady_clock::time_point sTime,
    steady_clock::time_point eTime) :
    Start(sTime), End(eTime) {}

}  // namespace benchmark.
