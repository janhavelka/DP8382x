// Internal collector arithmetic, separate from MDIO and source decoding.
#pragma once
#include <DP8382x/DP8382x.h>
#include <limits>

namespace DP8382x::detail {
inline void retainSampleCount(EvidenceSample& sample) noexcept {
  if (sample.samples == std::numeric_limits<uint32_t>::max()) { sample.samplesSaturated = true; }
  else { ++sample.samples; }
}

// SNLS505H tables 8-20/21: the 8- and 16-bit counters stop at different limits.
inline void retainCounter(EvidenceSample& sample, uint16_t count, uint16_t limit) noexcept {
  sample.counterSaturated = sample.counterSaturated || count == limit;
  if (count > std::numeric_limits<uint64_t>::max() - sample.sampledSum) {
    sample.sampledSum = std::numeric_limits<uint64_t>::max();
    sample.sumSaturated = true;
  } else { sample.sampledSum += count; }
  sample.counterLowerBound = sample.counterLowerBound || sample.counterSaturated || sample.sumSaturated;
}
inline void retainFalseCarriers(EvidenceSample& sample, uint8_t count) noexcept {
  retainCounter(sample, count, 0x00FFU);
}
} // namespace DP8382x::detail
