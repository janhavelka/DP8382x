// Application-owned manual cable capture; compiled and run by test/consumer.
#pragma once
#include "Initialize.h"

using CableAttempt = InitializeAttempt;

// The owner first quiesces MAC/EtherCAT traffic and disconnects or silences the
// peer. These settings record that decision; they do not operate the fixture.
inline DP8382x::Status beginCableCapture(DP8382x::DP8382x& phy,
                                        const DP8382x::CableSettings& settings,
                                        const DP8382x::OperationBudget& budget,
                                        CableAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startCableTest(settings, budget, token);
  if (status.inProgress()) { attempt = {}; attempt.token = token; }
  return status;
}

// A separately admitted reread never retriggers the current measurement.
inline DP8382x::Status reacquireCableCapture(DP8382x::DP8382x& phy,
                                            const DP8382x::OperationBudget& budget,
                                            CableAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startCableAcquire(budget, token);
  if (status.inProgress()) { attempt = {}; attempt.token = token; }
  return status;
}

// Retain the capture and restoration attempts separately. Restoration can fail
// even when the raw capture succeeded. HARD requires the shared bus quiet floor.
inline DP8382x::Status stopCableCapture(DP8382x::DP8382x& phy,
                                       const DP8382x::OperationBudget& budget,
                                       CableAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.stopCableTest(budget, token);
  if (status.inProgress()) {
    attempt = {}; attempt.token = token; attempt.reset = DP8382x::ResetKind::HARD;
  }
  return status;
}

// One scheduled owner turn, then return to the application's scheduler. Honor
// nextPollAtUs and SharedMdioQuiet across every PHY on this controller. On error,
// retain partial result.cable and admit restoration with its own finite budget.
// Rendering uses that retained value or cableSnapshot(), never additional reads.
inline DP8382x::Status pollCableCapture(DP8382x::DP8382x& phy, CableAttempt& attempt,
                                       SharedMdioQuiet& quiet,
                                       uint32_t maxFrames = DP8382x::MAX_BATCH_FRAMES) noexcept {
  return pollInitialization(phy, attempt, quiet, maxFrames);
}
