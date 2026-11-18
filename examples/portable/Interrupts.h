// Deferred application owner example; executed by test/consumer/cable.cpp.
#pragma once
#include "Initialize.h"

using InterruptAttempt = InitializeAttempt;

// Pin permissions must come from the application board plan. Alternate LED
// routing does not disconnect pin 21. The initial board has no assumed wired IRQ.
inline DP8382x::Status beginInterruptPolicy(DP8382x::DP8382x& phy,
                                            const DP8382x::InterruptSettings& settings,
                                            const DP8382x::OperationBudget& budget,
                                            InterruptAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startConfigureInterrupts(settings, budget, token);
  if (status.inProgress()) { attempt = {}; attempt.token = token; }
  return status;
}

// Poll once per scheduled owner turn using pollInitialization's existing token/
// result helper. Configuration and test start/stop have separate attempts and
// budgets. Retain the primary result when a later stop or initialization restores.
// An ISR only posts an application-provided bounded signal; it never calls these
// functions. The owner atomically takes that signal using its platform primitives.
inline DP8382x::Status serviceDeferredInterrupt(DP8382x::DP8382x& phy,
                                               const DP8382x::InterruptServiceBudget& budget,
                                               DP8382x::InterruptServiceSnapshot& view,
                                               bool& reschedule) noexcept {
  const auto status = phy.serviceInterrupts(view, budget);
  reschedule = !status.ok() || view.morePossiblyPending;
  // Return to the scheduler even if reschedule=true. Use level/pin sampling or
  // bounded periodic service; an already asserted pin need not produce a new edge.
  return status;
}

// Any number of readers can render a copied view without MDIO. The owner decides
// notification delivery once per acquisition; cached calls never consume events.
inline DP8382x::InterruptServiceSnapshot cachedInterruptView(const DP8382x::DP8382x& phy) noexcept {
  return phy.interruptServiceSnapshot();
}
