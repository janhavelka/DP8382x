// Application-owned MAC drain, dwell and post-exit coordination.
#pragma once
#include <DP8382x/DP8382x.h>

struct LpiOwnerAttempt {
  DP8382x::LpiResult entry{};
  DP8382x::LpiResult exit{};
  bool keepMacStopped{true};
};

// First configure EEE, then acquire link/power/EEE observations. Stop and drain
// the MAC outside this library before passing explicit admission. On any failure
// keep the MAC stopped and inspect phy.lpiSnapshot() and the retained entry.
inline DP8382x::Status enterOwnerLpi(DP8382x::DP8382x& phy,
                                    const DP8382x::LpiOptions& admission,
                                    const DP8382x::IoBudget& budget,
                                    LpiOwnerAttempt& attempt) noexcept {
  attempt={};
  return phy.startLpi(admission,attempt.entry,budget);
}

// Return to the application's scheduler for dwell. To cancel dwell, or on its
// normal completion, call this with a NEW budget. It neither sleeps nor retries.
inline DP8382x::Status exitOwnerLpi(DP8382x::DP8382x& phy,
                                   const DP8382x::IoBudget& budget,
                                   LpiOwnerAttempt& attempt) noexcept {
  attempt.keepMacStopped=true;
  return phy.stopLpi(attempt.exit,budget);
}

// Call only after the application has separately established post-exit readiness.
// The nominal36us wake time alone is not that evidence. Failed exit remains visible
// in attempt.exit; explicit HARD initialization is an alternative restoration,
// with its own operation result, never an automatic action in this example.
inline bool permitMacAfterLpi(const DP8382x::DP8382x& phy,
                              bool applicationPostExitReady,
                              LpiOwnerAttempt& attempt) noexcept {
  const bool safe=applicationPostExitReady && attempt.exit.status.ok() &&
      phy.lpiSnapshot().forcedLpi==DP8382x::ForcedLpi::OFF &&
      !phy.operationSnapshot().cleanupOwed;
  attempt.keepMacStopped=!safe;
  return safe;
}
