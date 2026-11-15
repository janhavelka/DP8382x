// Private managed-health accumulation. No transport, clock or policy ownership.
#pragma once
#include <DP8382x/DP8382x.h>
#include <limits>

namespace DP8382x::detail {
inline void retainHealth(HealthSnapshot& health, Status status,
                         uint64_t timeUs, bool timeValid) noexcept {
  const auto increment = [](uint32_t& value) noexcept {
    if (value != std::numeric_limits<uint32_t>::max()) { ++value; }
  };
  if (status.ok()) {
    increment(health.totalSuccess);
    health.consecutiveFailures = 0;
    health.lastOkUs = timeValid ? timeUs : 0;
    health.lastOkTimeValid = timeValid;
  } else {
    increment(health.totalFailures);
    increment(health.consecutiveFailures);
    health.lastError = status;
    health.lastErrorUs = timeValid ? timeUs : 0;
    health.lastErrorTimeValid = timeValid;
  }
}
} // namespace DP8382x::detail
