// One owner acquisition followed by any number of copied diagnostic views.
#pragma once
#include <DP8382x/DP8382x.h>
#include <cstddef>
#include <cstdio>

// Call under the application's instance serialization. This consumes hardware
// events once; on failure output is unchanged and evidenceSnapshot() retains
// successful partial samples and possible history loss. Six physical reads.
inline DP8382x::Status sampleEvents(DP8382x::DP8382x& phy,
                                   const DP8382x::IoBudget& budget,
                                   DP8382x::CounterSnapshot& output) noexcept {
  return phy.collectEvents(output, budget);
}

// Format a COPY after releasing the owner/controller lock. The caller owns the
// bounded buffer and output sink. RECR remains raw because its width/saturation
// conflict prevents qualified arithmetic. No bus, driver or clock access.
inline bool formatCachedEvents(const DP8382x::CounterSnapshot& snapshot,
                                char* output, std::size_t capacity) noexcept {
  if (output == nullptr || capacity == 0U) return false;
  const int count = std::snprintf(output, capacity,
      "valid=%u stale=%u MISR1=%04X MISR2=%04X FLDS=%04X FCSCR=%llu lower-bound=%u RECR-raw=%04X",
      snapshot.valid ? 1U : 0U, snapshot.stale ? 1U : 0U,
      static_cast<unsigned>(snapshot.misr1Events),
      static_cast<unsigned>(snapshot.misr2Events),
      static_cast<unsigned>(snapshot.fldCauses),
      static_cast<unsigned long long>(snapshot.falseCarrierTotal),
      snapshot.falseCarrierLowerBound ? 1U : 0U,
      static_cast<unsigned>(snapshot.rawRecr));
  return count >= 0 && static_cast<std::size_t>(count) < capacity;
}
