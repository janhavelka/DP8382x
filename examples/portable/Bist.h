// Application-owned bounded BIST; compiled and executed by test/consumer.
#pragma once
#include "Loopback.h"

using BistAttempt = InitializeAttempt;

inline DP8382x::Status beginBist(DP8382x::DP8382x& phy, const DP8382x::BistOptions& options,
                                const DP8382x::OperationBudget& budget, BistAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startBist(options, budget, token);
  if (status.inProgress()) { attempt = {}; attempt.token = token; }
  return status;
}

inline DP8382x::Status acquireBist(DP8382x::DP8382x& phy, const DP8382x::BistObservation& observed,
                                  const DP8382x::OperationBudget& budget, BistAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startBistAcquire(observed, budget, token);
  if (status.inProgress()) { attempt = {}; attempt.token = token; }
  return status;
}

inline DP8382x::Status endBist(DP8382x::DP8382x& phy, const DP8382x::OperationBudget& budget,
                              BistAttempt& attempt) noexcept {
  const auto saved = phy.bistSnapshot();
  DP8382x::OperationToken token;
  const auto status = phy.stopBist(budget, token);
  if (status.inProgress()) {
    attempt = {}; attempt.token = token;
    attempt.reset = saved.options.useLoopback && saved.options.loopback.mode == DP8382x::LoopbackMode::DIGITAL ?
        DP8382x::ResetKind::HARD : DP8382x::ResetKind::NONE;
  }
  return status;
}

// Call once per scheduled owner turn, then return to the application. The owner
// honors nextPollAtUs and the shared-bus HARD floor. It keeps each start/acquire/
// stop result separately, stops external stimulus before endBist, and explicitly
// admits any new restoration attempt after failure. It never prints by rereading.
inline DP8382x::Status pollBist(DP8382x::DP8382x& phy, BistAttempt& attempt,
                               SharedMdioQuiet& quiet, uint32_t maxFrames = DP8382x::MAX_BATCH_FRAMES) noexcept {
  return pollInitialization(phy, attempt, quiet, maxFrames);
}
