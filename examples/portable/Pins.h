// The application has stopped the MAC and released the selected board pin.
#pragma once
#include <DP8382x/DP8382x.h>

inline DP8382x::PinSettings linkLedProfile() noexcept {
  DP8382x::PinSettings profile;
  profile.pins[0].function=DP8382x::PinFunction::LED;
  profile.pins[0].source=DP8382x::LedSource::LINK_ACTIVITY;
  profile.availableMask=1;
  profile.maintenanceAdmitted=true;
  profile.macStopped=true;
  profile.outputsQuiesced=true;
  profile.blinkOwned=true;
  profile.blinkCode=2; // Datasheet 200ms selection, not a measured waveform.
  return profile;
}

inline DP8382x::Status applyLinkLed(DP8382x::DP8382x& phy,
                                  DP8382x::PinResult& result,
                                  const DP8382x::IoBudget& budget) noexcept {
  return phy.configurePins(linkLedProfile(),result,budget);
}

// With an existing LED2 IRQ/WoL owner this request rejects before I/O. The
// application must release/reconfigure that owner explicitly, then try again.
inline DP8382x::Status tryLed2(DP8382x::DP8382x& phy,
                             DP8382x::PinResult& result,
                             const DP8382x::IoBudget& budget) noexcept {
  auto profile=linkLedProfile();
  profile.pins[0]={};profile.blinkOwned=false;
  profile.pins[2].function=DP8382x::PinFunction::LED;
  profile.availableMask=4;profile.colUnused=true;
  return phy.configurePins(profile,result,budget);
}
// Check Status before using a verified result. On a failed accepted mutation,
// retain PinResult and use separately budgeted startInitialize for restoration.
// Cached pinsSnapshot rendering performs no I/O or destructive acknowledgement.
