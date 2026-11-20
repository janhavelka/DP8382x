#pragma once
#include <DP8382x/DP8382x.h>

namespace owner_example {
// esp_timer monotonic microseconds. The application schedules checks every 100 ms.
inline constexpr uint32_t LINK_POLL_MS = 1000;
inline constexpr uint64_t MAX_LINK_AGE_US = 3ULL * LINK_POLL_MS * 1000;
inline constexpr uint64_t MAX_SERVICE_GAP_US = 250000;

enum class OwnerPhase { UNCONFIGURED, WAITING, READY, ACTIVE, WITHDRAWN, MAINTENANCE };

struct OwnerSnapshot {
  OwnerPhase phase{OwnerPhase::UNCONFIGURED};
  uint64_t generation{0};  // Identifies the session until its explicit release.
  bool evidenceAvailable{false};
  bool frameworkReady{false};
  bool frameworkRestartRequired{false}; // SDK TX contradicted the retained MAC-UP publication.
  bool resultPending{false};
  DP8382x::HealthSnapshot health{};
  DP8382x::SettingsSnapshot settings{};
  DP8382x::LinkSnapshot link{};
};

// Single application task owns this object and its result storage. The caller
// holds the native evidence gate across observe + admit/allowFrame + handoff.
class PhyOwner {
public:
  PhyOwner() = default;
  PhyOwner(const PhyOwner&) = delete;
  PhyOwner& operator=(const PhyOwner&) = delete;

  DP8382x::Status begin(const DP8382x::LinkSettings& profile) noexcept;
  /// @brief Check copied settings against the admitted 100/full AN profile.
  /// @param[in] settings Retained desired and observed evidence from the native owner.
  /// @return True for a verified, current match of both values. Performs no I/O.
  bool verifiedProfile(const DP8382x::SettingsSnapshot& settings) const noexcept;
  void observe(uint64_t nowUs, const DP8382x::HealthSnapshot& health,
               const DP8382x::SettingsSnapshot& settings,
               const DP8382x::LinkSnapshot& link, bool frameworkReady) noexcept;
  void missingEvidence(uint64_t nowUs) noexcept;
  void cachedLinkDown(uint64_t nowUs) noexcept;
  // Called only after the owner reconciles and successfully starts the framework.
  void frameworkRestarted() noexcept;
  DP8382x::Status admit(uint64_t nowUs, uint64_t& generation) noexcept;
  DP8382x::Status allowFrame(uint64_t nowUs, uint64_t generation) noexcept;
  DP8382x::Status release(uint64_t generation) noexcept;
  DP8382x::Status beginMaintenance(bool framesQuiescent) noexcept;
  void finishMaintenance(uint64_t nowUs) noexcept;
  DP8382x::Status retainResult(const DP8382x::OperationResult& result) noexcept;
  DP8382x::Status takeResult(DP8382x::OperationResult& result) noexcept;
  const OwnerSnapshot& snapshot() const noexcept { return _state; }

private:
  void withdraw() noexcept;
  bool checkTime(uint64_t nowUs) noexcept;
  OwnerSnapshot _state{};
  DP8382x::LinkSettings _profile{};
  DP8382x::OperationResult _result{};
  uint64_t _lastClockUs{0};   // Includes the final check immediately before TX.
  uint64_t _lastServiceUs{0}; // Only evidence service advances the allowed service gap.
  uint64_t _maintenanceEndUs{0};
  bool _maintenanceEnded{false};
  bool _clockInvalid{false};
};
} // namespace owner_example
