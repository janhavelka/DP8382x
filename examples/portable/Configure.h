// Application-owned negotiated profiles, compiled by the portable consumer.
#pragma once

#include <DP8382x/DP8382x.h>

// Generic library policy: advertise all four documented 10/100 technologies.
// Pause remains disabled unless the application and MAC policy select it.
inline DP8382x::LinkSettings allNegotiatedTechnologies() noexcept {
  DP8382x::LinkSettings settings;
  settings.advertisement.technologyMask = DP8382x::LINK_TECHNOLOGY_MASK;
  return settings;
}

// This project's initial 100/full-only choice is application data, not a
// fallback imposed by the driver on other users of the library.
inline DP8382x::LinkSettings application100Full() noexcept {
  DP8382x::LinkSettings settings;
  settings.advertisement.technologyMask = DP8382x::LINK_100_FULL;
  return settings;
}

struct ConfigureAttempt {
  DP8382x::OperationToken token{};
  DP8382x::OperationResult result{};
  bool resultAvailable{false};
};

// Initialize first: it performs mandatory VOD setup. Admission below is local
// and copies a valid profile into desired intent; it does not issue MDIO.
// The application supplies its own absolute deadline, total frames and polling
// policy, and serializes this call with every other access to the instance.
inline DP8382x::Status beginConfiguration(DP8382x::DP8382x& phy,
                                         const DP8382x::LinkSettings& settings,
                                         const DP8382x::ConfigureOptions& options,
                                         const DP8382x::OperationBudget& budget,
                                         ConfigureAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startConfigure(settings, options, budget, token);
  if (status.is(DP8382x::Err::IN_PROGRESS)) {
    attempt = {};
    attempt.token = token;
  }
  return status;
}

// Optional Clause 28 exchange of one null message. NP is generated for this
// epoch only; it is never part of the persistent Advertisement value.
inline DP8382x::ConfigureOptions nullPageExchange() noexcept {
  DP8382x::ConfigureOptions options;
  options.nextPages.pageCount = 1;
  options.nextPages.pages[0].message = true;
  options.nextPages.pages[0].data = 1;
  options.nextPages.maxPartnerPages = 4;
  return options;
}

// Call once per owner-loop turn, then return to the scheduler. IN_PROGRESS is
// not a failure or proof of link-down. Honor operationSnapshot().nextPollAtUs
// when valid. maxFrames is a slice, so a short slice yields before a whole batch.
inline DP8382x::Status pollConfiguration(DP8382x::DP8382x& phy,
                                        ConfigureAttempt& attempt,
                                        uint32_t maxFrames = DP8382x::MAX_BATCH_FRAMES) noexcept {
  if (attempt.resultAvailable) { return attempt.result.status; }
  const auto status = phy.pollOperation(attempt.token, maxFrames);
  if (status.is(DP8382x::Err::IN_PROGRESS)) { return status; }
  DP8382x::OperationResult result;
  const auto taken = phy.takeResult(attempt.token, result);
  if (!taken.ok()) { return taken; } // A stale token is not this attempt's result.
  attempt.result = result;
  attempt.resultAvailable = true;
  return result.status;
}

// After resultAvailable, inspect result.status before consuming its ordinary
// outcome; partial evidence and nextPage flags remain diagnostic on failure.
// If cleanupOwed is true, retain this ConfigureAttempt and use a separate
// InitializeAttempt from Initialize.h with a fresh budget. Never replace the
// original failure with restoration success. Cancellation itself performs no I/O.
