// Application-owned exact-address probe, compiled and linked by test/consumer.
#pragma once

#include <DP8382x/DP8382x.h>

// Owner establishes board readiness and truthful transport bounds; no address scan.
// Outputs remain unchanged on failure. The returned capabilities describe silicon,
// not completed initialization, link-up, SDK support or physical qualification.
inline DP8382x::Status probeExactAddress(const DP8382x::Config& config,
                                        const DP8382x::IoBudget& budget,
                                        DP8382x::DeviceIdentity& identity,
                                        DP8382x::Capabilities& capabilities) noexcept {
  DP8382x::DP8382x phy;
  auto status = phy.bind(config);
  if (!status.ok()) { return status; }
  DP8382x::DeviceIdentity observed;
  status = phy.readIdentity(observed, budget);
  // Applications needing failure diagnostics copy identitySnapshot before end.
  const auto supported = phy.capabilities();
  const auto detach = phy.end();
  if (!status.ok()) { return status; }
  if (!detach.ok()) { return detach; }
  identity = observed; // Use decoded fields only after complete accepted success.
  capabilities = supported;
  return status;
}
