// Application-owned initialization example, compiled and executed by test/consumer.
#pragma once

#include <DP8382x/DP8382x.h>

// Keep a separate value for the original operation and each restoration attempt.
// A successful restoration must never replace the original failure evidence.
struct InitializeAttempt {
  DP8382x::OperationToken token{};
  DP8382x::OperationResult result{};
  bool resultAvailable{false};
  DP8382x::ResetKind reset{DP8382x::ResetKind::NONE};
  bool hardFloorCaptured{false};
};

// This belongs to the shared controller owner, not to one PHY. It outlives
// cancel/take/abandon/end/rebind. Every bus participant consults the same value.
// A missing anchor requires trusted external timing/disposition before the
// owner explicitly replaces this state; elapsed time on a broken clock is no proof.
struct SharedMdioQuiet {
  uint64_t untilUs{0};
  bool unanchored{false};

  bool canIssue(uint64_t trustedNowUs) const noexcept {
    return !unanchored && trustedNowUs >= untilUs;
  }
};

// Before calling, the application establishes supply/clock/RESET_N readiness,
// obtains exclusive driver ownership and supplies an absolute finite budget.
inline DP8382x::Status beginInitialization(DP8382x::DP8382x& phy,
                                           DP8382x::ResetKind reset,
                                           const DP8382x::OperationBudget& budget,
                                           InitializeAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startInitialize({reset}, budget, token);
  if (status.is(DP8382x::Err::IN_PROGRESS)) {
    attempt = {};
    attempt.token = token;
    attempt.reset = reset;
  }
  return status;
}

// Call once from each owner-loop turn. Never spin here. If still IN_PROGRESS,
// return to the application's scheduler; operationSnapshot().nextPollAtUs, when
// valid, is the earliest next action, not a promise that the owner runs then.
// maxFrames is a slice: the driver yields before any batch that cannot fit.
inline DP8382x::Status pollInitialization(DP8382x::DP8382x& phy,
                                          InitializeAttempt& attempt,
                                          SharedMdioQuiet& sharedQuiet,
                                          uint32_t maxFrames = DP8382x::MAX_BATCH_FRAMES) noexcept {
  if (attempt.resultAvailable) { return attempt.result.status; }
  if (attempt.reset == DP8382x::ResetKind::HARD &&
      phy.operationSnapshot().phase == DP8382x::OperationPhase::COMMAND) {
    attempt.hardFloorCaptured = false; // A new command needs its own completion anchor.
  }
  const auto status = phy.pollOperation(attempt.token, maxFrames);
  const auto snapshot = phy.operationSnapshot();
  if (snapshot.busQuietValid) {
    if (snapshot.busQuietUntilUs > sharedQuiet.untilUs) sharedQuiet.untilUs = snapshot.busQuietUntilUs;
    if (snapshot.phase == DP8382x::OperationPhase::WAIT) attempt.hardFloorCaptured = true;
  }
  if (phy.transportSnapshot().clockInvalid) sharedQuiet.unanchored = true;
  if (status.is(DP8382x::Err::IN_PROGRESS)) { return status; }
  // A token/lifetime rejection must not be presented as this operation's result.
  DP8382x::OperationResult result;
  const auto taken = phy.takeResult(attempt.token, result);
  if (!taken.ok()) { return taken; }
  attempt.result = result;
  attempt.resultAvailable = true;
  if (result.reset == DP8382x::ResetKind::HARD &&
      (result.mutationAttempted || result.mutationPossible)) {
    if (snapshot.busQuietValid) attempt.hardFloorCaptured = true;
    if (!attempt.hardFloorCaptured) sharedQuiet.unanchored = true;
  }
  return result.status;
}

// When restoration fails, keep both InitializeAttempt values. The owner closes
// admission, drains users, and establishes external reset, isolation, power
// removal or transfer to another owner BEFORE passing true. This bool records
// that external decision; it neither performs nor proves a physical action.
// Keep SharedMdioQuiet after this call. end() discards the core floor, but the
// shared owner still holds all PHYs until its retained floor or trusted external
// recovery requirement is satisfied. Never clear it merely because teardown succeeded.
inline DP8382x::Status abandonInitialization(DP8382x::DP8382x& phy,
                                             bool externalDispositionEstablished) noexcept {
  if (!externalDispositionEstablished) {
    return DP8382x::Status::Error(DP8382x::Err::INVALID_PARAM,
                                  "Owner must establish external disposition");
  }
  const auto abandoned = phy.abandonCleanup();
  if (!abandoned.ok()) { return abandoned; }
  return phy.end(); // Local teardown; original and restoration results stay in the owner.
}
