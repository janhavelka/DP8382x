// Application-owned silicon autorun policy; compiled by test/consumer/cable.cpp.
#pragma once
#include "CableCapture.h"

// The application explicitly admits disruptive maintenance. Enabling this one
// flag neither triggers a capture nor schedules recovery or future sampling.
inline DP8382x::Status configureCableAutorun(DP8382x::DP8382x& phy,
                                            const DP8382x::CableAutorunOptions& options,
                                            const DP8382x::OperationBudget& budget,
                                            CableAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startConfigureCableAutorun(options, budget, token);
  if (status.inProgress()) { attempt = {}; attempt.token = token; }
  return status;
}

// Drive this later under its own finite budget. No software trigger or polling
// until link-down occurs is implied. Even a complete result has unknown freshness
// and coherence; inspect retained raw words and per-word validity, not distance.
inline DP8382x::Status acquireCableAutorun(DP8382x::DP8382x& phy,
                                          const DP8382x::OperationBudget& budget,
                                          CableAttempt& attempt) noexcept {
  DP8382x::OperationToken token;
  const auto status = phy.startCableAutorunAcquire(budget, token);
  if (status.inProgress()) { attempt = {}; attempt.token = token; }
  return status;
}

// Use pollCableCapture for one scheduled owner turn. To hand off to manual
// capture, configure enabled=false with maintenance admission and consume that
// separate result first. Verified disable does not establish engine quiescence;
// manual capture still performs its own bounded live preflight.
