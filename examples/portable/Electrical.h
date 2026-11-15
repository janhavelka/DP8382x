// Application owns termination/probes/reference conditions, MAC exclusion and dwell.
#pragma once
#include <DP8382x/DP8382x.h>

inline DP8382x::ComplianceOptions admittedTx100Fixture() noexcept {
  DP8382x::ComplianceOptions options;
  options.mode=DP8382x::ComplianceMode::TX100;
  options.maintenanceAdmitted=options.macStopped=options.fixtureAdmitted=options.hardResetAdmitted=true;
  return options;
}
inline DP8382x::Status beginElectricalFixture(DP8382x::DP8382x& phy,
    const DP8382x::OperationBudget& budget,DP8382x::OperationToken& token) noexcept {
  // Call only after the actual operator admits the fixture and stops MAC traffic.
  return phy.startCompliance(admittedTx100Fixture(),budget,token);
}
inline DP8382x::Status pollElectricalFixture(DP8382x::DP8382x& phy,
    DP8382x::OperationToken token,DP8382x::OperationResult& retained) noexcept {
  const auto status=phy.pollOperation(token,DP8382x::MAX_BATCH_FRAMES);
  if(status.inProgress()) { return status; }
  const auto taken=phy.takeResult(token,retained);
  return taken.ok()?status:taken;
}
inline DP8382x::Status stopElectricalFixture(DP8382x::DP8382x& phy,
    const DP8382x::OperationBudget& stopBudget,DP8382x::OperationToken& token) noexcept {
  return phy.stopCompliance(stopBudget,token);
}
// The owner schedules polls without holding the MDIO lock, including HARD silence
// for the whole shared bus. After successful start, it owns a finite instrument
// dwell, then admits stop with a NEW budget/token and retains that result too.
// A failed start/stop may leave emission active: keep MAC stopped and perform an
// explicitly admitted recovery. Successful HARD stop still owes persistent-state
// restoration through a later startInitialize. No register result is compliance PASS.
