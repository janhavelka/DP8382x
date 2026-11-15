// Application mapping of one passive copy. No clock, bus or recovery action.
#pragma once

#include <DP8382x/DP8382x.h>

inline const char* cachedHealthAdvice(const DP8382x::HealthSnapshot& health) noexcept {
  switch (health.state) {
    case DP8382x::DriverState::UNINIT: return "Initialization not established";
    case DP8382x::DriverState::READY: return "Check link and application policy separately";
    case DP8382x::DriverState::DEGRADED: return "Retain the error; owner decides the next attempt";
    case DP8382x::DriverState::OFFLINE: return "Repeated managed failure; explicit attempts remain allowed";
  }
  return "Unknown health state";
}

// Copy under the instance owner, then pass the value to display/log consumers.
// This does not sample hardware. lastError may describe an earlier failure even
// when READY; use the validity flags for timestamps, never a nonzero test.
inline DP8382x::HealthSnapshot copyHealth(const DP8382x::DP8382x& phy) noexcept {
  return phy.healthSnapshot();
}
