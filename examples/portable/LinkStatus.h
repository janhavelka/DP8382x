// Application policy over a copied observation: no bus, core or clock access.
#pragma once

#include <DP8382x/DP8382x.h>

// The owner supplies a conservative current-time upper bound in the binding
// clock domain (including its clock lag) and decides its maximum observation
// age. Compare against the acquisition start lower bound so clock uncertainty
// and acquisition duration cannot understate age. This asks for 100/full only;
// it does not establish DHCP, IP, EtherCAT or machine readiness. Down and 10/half remain
// valid PHY observations even though this particular application rejects them.
inline bool cached100FullUsable(const DP8382x::LinkSnapshot& link,
                                uint64_t ownerNowUpperUs, uint64_t maxAgeUs) noexcept {
  return link.valid && !link.stale && link.timeValid &&
      link.observedSinceUs <= link.observedAtUs && ownerNowUpperUs >= link.observedAtUs &&
      ownerNowUpperUs - link.observedSinceUs <= maxAgeUs &&
      link.linkUp && !link.temporaryMayBeActive && link.resolvedModeValid &&
      link.speed == DP8382x::LinkSpeed::MBPS_100 && link.duplex == DP8382x::Duplex::FULL;
}

// Call from the serialized PHY owner using its own sampling schedule. Failure
// preserves output, so rendering should copy phy.linkSnapshot() afterwards and
// inspect stale/lastRefreshStatus. Repeated rendering consumes no new evidence.
inline DP8382x::Status sampleLink(DP8382x::DP8382x& phy,
                                 const DP8382x::IoBudget& budget,
                                 DP8382x::LinkSnapshot& output) noexcept {
  return phy.readLinkStatus(output, budget);
}
