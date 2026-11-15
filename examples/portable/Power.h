// Application-owned policy and transition example; executed by the portable consumer.
#pragma once
#include "Initialize.h"

using PowerAttempt = InitializeAttempt;

// Board code supplies maintenance/clock permissions. Poll once per scheduled
// owner turn with pollInitialization; keep each terminal result separately.
inline DP8382x::Status beginPowerPolicy(DP8382x::DP8382x& phy,
                                       const DP8382x::PowerSettings& settings,
                                       const DP8382x::OperationBudget& budget,
                                       PowerAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startConfigurePower(settings, budget, token);
  if (status.inProgress()) { attempt = {}; attempt.token = token; }
  return status;
}

// Acquire explicitly once, then render any number of copied passive views. Check
// status before using output, and check observedStale/timeValid in cached views.
inline DP8382x::Status acquirePowerView(DP8382x::DP8382x& phy,
                                       DP8382x::PowerSnapshot& view,
                                       const DP8382x::IoBudget& budget) noexcept {
  return phy.readPowerStatus(view, budget);
}

// For an explicit wake, submit a complete NORMAL request with disablePll=false
// and the same board clock permission. CONFIG_MISMATCH with
// WAKE_BLOCKED_OR_UNCONFIRMED is terminal: retain it, inspect the external pin/
// clock situation, then separately admit startInitialize if restoration is owed.
// Do not retry automatically or equate control readback with measured clock lock.
