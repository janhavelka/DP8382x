// The application owns traffic quiescence, board routing and actual host wake.
#pragma once
#include "Initialize.h"

using WakeAttempt = InitializeAttempt;

inline DP8382x::Status beginWakePolicy(DP8382x::DP8382x& phy,
                                      const DP8382x::WakeSettings& settings,
                                      const DP8382x::OperationBudget& budget,
                                      WakeAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status=phy.startConfigureWake(settings,budget,token);
  if (status.inProgress()) { attempt={}; attempt.token=token; }
  return status;
}

// Use pollInitialization once per owner turn, then retain attempt.result.
// An IRQ should only schedule this owner; it must not perform MDIO in an ISR.
inline DP8382x::Status acquireWakeView(DP8382x::DP8382x& phy,
                                      DP8382x::WakeSnapshot& view,
                                      const DP8382x::IoBudget& budget) noexcept {
  return phy.readWakeStatus(view,budget);
}

inline DP8382x::Status beginWakeAcknowledgement(DP8382x::DP8382x& phy,
                                               const DP8382x::OperationBudget& budget,
                                               WakeAttempt& acknowledgement) noexcept {
  DP8382x::OperationToken token;
  const auto status=phy.acknowledgeWake(budget,token);
  if (status.inProgress()) { acknowledgement={}; acknowledgement.token=token; }
  return status;
}

// Render as often as wanted without consuming hardware history again. The last
// complete view is historical after a later partial collection. A magic-match
// indication is chip evidence, never proof that an MCU/application awakened.
inline DP8382x::WakeSnapshot cachedWakeView(const DP8382x::DP8382x& phy) noexcept {
  return phy.wakeSnapshot();
}
// On failed configuration/acknowledgement keep the original result and collector
// samples. Explicit startInitialize restores only owned settings with a NEW
// budget/result; it never replays acknowledgement or fabricates lost events.
