// Application-owned EEE configuration/status; no MAC or scheduler in the core.
#pragma once
#include "Initialize.h"

using EeeAttempt = InitializeAttempt;

// Establish an owned AN profile first. Opt in only after admitting link loss,
// shared-bus HARD silence and the board's clock/LED routing consequences.
inline DP8382x::Status beginEeePolicy(DP8382x::DP8382x& phy,
                                     const DP8382x::EeeSettings& settings,
                                     const DP8382x::OperationBudget& budget,
                                     EeeAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startConfigureEee(settings,budget,token);
  if (status.inProgress()) { attempt={}; attempt.token=token; attempt.reset=DP8382x::ResetKind::HARD; }
  return status;
}

// One scheduler turn calls pollInitialization. Preserve the shared quiet floor
// and every terminal result. Recovery is a new startInitialize operation.
// Complete register application does not mean a partner negotiated EEE.
inline DP8382x::Status acquireEeeView(DP8382x::DP8382x& phy,
                                     DP8382x::EeeSnapshot& view,
                                     const DP8382x::IoBudget& budget) noexcept {
  return phy.readEeeStatus(view,budget);
}
inline bool negotiatedEee(const DP8382x::EeeSnapshot& view) noexcept {
  return view.valid && !view.stale && view.negotiatedValid && view.negotiated;
}
// After a successful acquisition render multiple phy.eeeSnapshot() copies.
// Before any future TX LPI session the MAC owner must stop/drain transmission
// and supply measured latency constraints. Stage29 sends no TX LPI command.
