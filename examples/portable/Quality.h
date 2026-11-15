// Application owns MAC drain, scheduling and any later recovery decision.
#pragma once
#include <DP8382x/DP8382x.h>

struct QualityViews {
  DP8382x::SignalQualitySnapshot mse{};
  DP8382x::EvidenceSnapshot events{};
  DP8382x::LinkSnapshot link{};
};

// Acquire only raw MSE. ANER page, jabber/polarity and error history were already
// consumed by the single collector. Rendering either view never repeats reads.
inline QualityViews cachedQualityViews(const DP8382x::DP8382x& phy) noexcept {
  return {phy.signalQualitySnapshot(),phy.evidenceSnapshot(),phy.linkSnapshot()};
}
inline DP8382x::Status acquireQualityViews(DP8382x::DP8382x& phy,QualityViews& views,
                                         const DP8382x::IoBudget& budget) noexcept {
  DP8382x::SignalQualitySnapshot mse;
  const auto status=phy.readSignalQuality(mse,budget);
  if(status.ok()) { views=cachedQualityViews(phy); }
  return status; // On failure the caller's ordinary views remain unchanged.
}

// Use only after the application stopped MAC traffic on an identified MII PHY.
// The profile enables energy-loss and RX-error detection. Host readback proves
// configuration only; a fixture must measure the specified physical latency.
inline DP8382x::QualitySettings qualityMiiProfile() noexcept {
  DP8382x::QualitySettings settings;
  settings.maintenanceAdmitted=settings.macStopped=true;
  settings.fastDrop.owned=true;settings.fastDrop.causeMask=0x0009;
  return settings;
}
inline DP8382x::Status startMiiFastDrop(DP8382x::DP8382x& phy,
                                      const DP8382x::OperationBudget& budget,
                                      DP8382x::OperationToken& token) noexcept {
  return phy.startConfigureQuality(qualityMiiProfile(),budget,token);
}
// The owner calls pollOperation with finite slices and takeResult after terminal
// completion. On failure/cancel retain the result; cleanupOwed requires a later
// explicitly budgeted startInitialize. Neither this example nor FLD resets or
// retries the PHY automatically. Raw MSE is not SNR or a calibrated cable grade.
