// Application sequencing: stop MAC/DMA, establish the declared clock and
// release conflicting pin consumers before attesting these permissions.
#pragma once
#include <DP8382x/DP8382x.h>

inline DP8382x::InterfaceSettings rmiiFollowerProfile() noexcept {
  DP8382x::InterfaceSettings s;
  s.interface=DP8382x::MacInterface::RMII;
  s.rmiiClock=DP8382x::RmiiClock::FOLLOWER_50MHZ;
  s.referenceClockMHz=50;
  s.maintenanceAdmitted=s.macStopped=s.pinsAvailable=true;
  s.referenceClockStable=s.managementClockAvailable=s.clockChangeApproved=true;
  s.elasticityCode=1; // Table 8-41: code 1 selects the two-bit tolerance.
  s.maxFrameBytes=1518;s.clockTolerancePpm=50;
  return s;
}

inline DP8382x::Status configureStoppedRmii(DP8382x::DP8382x& phy,
                                          DP8382x::InterfaceResult& result,
                                          const DP8382x::IoBudget& budget) noexcept {
  return phy.configureInterface(rmiiFollowerProfile(),result,budget);
}

// A running MAC is deliberately rejected before I/O. Do not change this flag
// merely to make the call succeed: it reports actual application preparation.
inline DP8382x::Status rejectLiveInterfaceChange(DP8382x::DP8382x& phy,
                                               DP8382x::InterfaceResult& result,
                                               const DP8382x::IoBudget& budget) noexcept {
  auto s=rmiiFollowerProfile();s.macStopped=false;
  return phy.configureInterface(s,result,budget);
}
// Check Status before trusting result. On accepted/uncertain failure retain the
// result and keep MAC consumers stopped; separately admit startInitialize for
// restoration. Readback does not qualify a clock waveform or a repeater pair.
