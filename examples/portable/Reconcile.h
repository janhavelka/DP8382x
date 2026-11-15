// Application-owned verification and explicit restoration decisions.
#pragma once
#include "Initialize.h"

struct VerificationAttempt {
  DP8382x::OperationToken token{};
  DP8382x::OperationResult result{};
  bool resultAvailable{false};
};

inline DP8382x::Status beginVerification(DP8382x::DP8382x& phy,
                                        const DP8382x::OperationBudget& budget,
                                        VerificationAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startVerify(budget, token);
  if (status.inProgress()) { attempt = {}; attempt.token = token; }
  return status;
}

// One owner-loop turn, not a busy loop. A short slice yields before the entire
// MMD read. While pending, settingsSnapshot raw fields may be partial/history;
// they cannot replace the stale aggregate tuple with a verified profile.
inline DP8382x::Status pollVerification(DP8382x::DP8382x& phy,
                                       VerificationAttempt& attempt,
                                       uint32_t maxFrames = DP8382x::MAX_BATCH_FRAMES) noexcept {
  if (attempt.resultAvailable) { return attempt.result.status; }
  const auto status = phy.pollOperation(attempt.token, maxFrames);
  if (status.inProgress()) { return status; }
  DP8382x::OperationResult result;
  const auto taken = phy.takeResult(attempt.token, result);
  if (!taken.ok()) { return taken; }
  attempt.result = result;
  attempt.resultAvailable = true;
  return result.status;
}

// If the owner elects to restore unchanged intent, call beginInitialization
// with ResetKind::NONE and a NEW InitializeAttempt/budget. Poll it through the
// existing initialization helper and retain both results. If it fails, only an
// explicit new owner decision admits another attempt; there is no implicit
// reset escalation, retry or replacement of the original failure evidence.
