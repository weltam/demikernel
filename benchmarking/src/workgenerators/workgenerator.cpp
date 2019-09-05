#include "workgenerators/workgenerator.h"

namespace benchmark {
unsigned long long lastOffset;

SequentialGenerator::SequentialGenerator(unsigned long long bufLen) :
    bufSize(bufLen) {}

void SequentialGenerator::FillOffsetAndLen(Io_u *iou) {
  iou->bufSize = bufSize;
  iou->offset = lastOffset;
  lastOffset += bufSize;
}

RandomGenerator::RandomGenerator(unsigned long long align,
    unsigned long long bufLen, unsigned long long maxSizeBytes) :
    alignment(align), bufSize(bufLen), maxFileSizeBytes(maxSizeBytes) {
  // TODO(t-asmart): Make a param?
  rng.seed(42);
  rngDist = std::uniform_int_distribution<unsigned long long>(0,
      maxSizeBytes - bufSize  - 1);
}

void RandomGenerator::FillOffsetAndLen(Io_u *iou) {
  iou->bufSize = bufSize;
  iou->offset = (rngDist(rng) / alignment) * alignment;
}

}  // namespace benchmark.
