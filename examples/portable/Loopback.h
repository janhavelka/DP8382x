// Application-owned maintenance session. Compiled/executed by test/consumer.
#pragma once
#include "Initialize.h"

// Reuse the owner value and shared hard-reset-floor handling. This does not
// call initialization: the core's loopback operation owns its own procedure.
using LoopbackAttempt = InitializeAttempt;

inline DP8382x::Status beginLoopback(DP8382x::DP8382x& phy,
                                    const DP8382x::LoopbackOptions& options,
                                    const DP8382x::OperationBudget& budget,
                                    LoopbackAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startLoopback(options, budget, token);
  if (status.inProgress()) {
    attempt = {};
    attempt.token = token;
    attempt.reset = options.mode == DP8382x::LoopbackMode::ANALOG ||
        options.mode == DP8382x::LoopbackMode::DIGITAL || options.mode == DP8382x::LoopbackMode::REVERSE ?
        DP8382x::ResetKind::RESTART : DP8382x::ResetKind::NONE;
  }
  return status;
}

inline DP8382x::Status beginLoopbackStop(DP8382x::DP8382x& phy,
                                        const DP8382x::OperationBudget& budget,
                                        LoopbackAttempt& attempt) noexcept {
  const auto session = phy.loopbackSnapshot(); // Cached; preserve before admission.
  DP8382x::OperationToken token;
  const auto status = phy.stopLoopback(budget, token);
  if (status.inProgress()) {
    attempt = {};
    attempt.token = token;
    attempt.reset = session.options.mode == DP8382x::LoopbackMode::DIGITAL ?
        DP8382x::ResetKind::HARD : DP8382x::ResetKind::NONE;
  }
  return status;
}

// One owner turn. Before each call the shared owner excludes all other bus
// users; afterward it honors SharedMdioQuiet and schedules the next turn.
inline DP8382x::Status pollLoopback(DP8382x::DP8382x& phy, LoopbackAttempt& attempt,
                                   SharedMdioQuiet& quiet, uint32_t maxFrames = DP8382x::MAX_BATCH_FRAMES) noexcept {
  return pollInitialization(phy, attempt, quiet, maxFrames);
}

// After a successful start and result consumption, the operation slot is free.
// The application chooses finite dwell and supplies/correlates MAC or partner
// traffic. Configuration success is not a packet test. Stop traffic before
// beginLoopbackStop; retain start and each stop attempt separately. Failed stop
// requires an explicit new owner decision/budget, never an automatic retry.
