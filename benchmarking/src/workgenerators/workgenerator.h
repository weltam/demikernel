#ifndef WORKGENERATORS_WORKGENERATOR_H_
#define WORKGENERATORS_WORKGENERATOR_H_

#include <random>

#include "io_u.h"

namespace benchmark {

class WorkloadGenerator {
 public:
  virtual ~WorkloadGenerator() {};

  virtual void FillOffsetAndLen(Io_u *iou) = 0;
};

// Not thread safe.
class SequentialGenerator : public WorkloadGenerator {
 public:
  SequentialGenerator(unsigned long long bufLen);
  virtual void FillOffsetAndLen(Io_u *iou);

 private:
  unsigned long long lastOffset = 0;
  unsigned long long bufSize;
};

class RandomGenerator : public WorkloadGenerator {
 public:
  RandomGenerator(unsigned long long align, unsigned long long bufLen,
      unsigned long long maxSizeBytes);
  virtual void FillOffsetAndLen(Io_u *iou);

 private:
  unsigned long long alignment;
  unsigned long long bufSize;
  unsigned long long maxFileSizeBytes;

  std::mt19937 rng;
  std::uniform_int_distribution<unsigned long long> rngDist;
};

}  // namespace benchmark.
#endif  // WORKGENERATORS_WORKGENERATOR_H_
