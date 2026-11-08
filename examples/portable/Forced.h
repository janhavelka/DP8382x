// Application-owned forced profile; compiled and linked by the portable consumer.
#pragma once

#include "Configure.h"

// For an explicitly compatible fixed-speed partner. This does not replace the
// initial board's application100Full() autonegotiated advertisement.
inline DP8382x::LinkSettings forced100FullMdi() noexcept {
  DP8382x::LinkSettings settings;
  settings.mode = DP8382x::LinkMode::FORCED;
  settings.forced.speed = DP8382x::LinkSpeed::MBPS_100;
  settings.forced.duplex = DP8382x::Duplex::FULL;
  settings.crossover.pairSelection = DP8382x::PairSelection::MDI;
  settings.crossover.robust = true; // Required even with manual MDI/MDIX.
  return settings; // No Advertisement, next pages or AN-completion wait.
}

// The caller first initializes the PHY, owns the controller and supplies a
// finite absolute budget. Check IN_PROGRESS, then call pollConfiguration once
// per scheduler turn until its retained result is available. Do not busy-spin.
inline DP8382x::Status beginForced100Full(DP8382x::DP8382x& phy,
                                         const DP8382x::OperationBudget& budget,
                                         ConfigureAttempt& attempt) noexcept {
  return beginConfiguration(phy, forced100FullMdi(), {}, budget, attempt);
}

// A terminal OK establishes verified local controls, not link-up, remote duplex
// or electrical qualification. Preserve failed results with cleanupOwed and
// admit a separate startInitialize restoration using a new budget/result.
// To return to AN, submit a new complete application100Full() or other valid
// Advertisement through beginConfiguration; do not merely change BMCR.
