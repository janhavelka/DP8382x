// Internal collector arithmetic, separate from MDIO and source decoding.
#pragma once
#include <DP8382x/DP8382x.h>
#include <limits>

namespace DP8382x::detail {
inline void retainSampleCount(EvidenceSample& sample) noexcept {
  if (sample.samples == std::numeric_limits<uint32_t>::max()) { sample.samplesSaturated = true; }
  else { ++sample.samples; }
}

// SNLS647I table8-38: FF saturates in hardware; software must not wrap either.
inline void retainFalseCarriers(EvidenceSample& sample, uint8_t count) noexcept {
  sample.counterSaturated = sample.counterSaturated || count == 0xFFU;
  if (count > std::numeric_limits<uint64_t>::max() - sample.sampledSum) {
    sample.sampledSum = std::numeric_limits<uint64_t>::max();
    sample.sumSaturated = true;
  } else { sample.sampledSum += count; }
  sample.counterLowerBound = sample.counterLowerBound || sample.counterSaturated || sample.sumSaturated;
}
} // namespace DP8382x::detail
