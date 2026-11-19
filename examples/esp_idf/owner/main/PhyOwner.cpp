#include "PhyOwner.h"
#include <limits>

namespace owner_example {
using namespace DP8382x;
namespace {
Status denied(const char* reason) noexcept {
  return Status::Error(Err::BUSY, reason, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
}
bool matches(const LinkSettings& s, const LinkSettings& profile) noexcept {
  return s.mode == LinkMode::AUTONEGOTIATED && s.advertisement.technologyMask == LINK_100_FULL &&
      s.advertisement.symmetricPause == profile.advertisement.symmetricPause &&
      s.advertisement.asymmetricPause == profile.advertisement.asymmetricPause &&
      s.extendedFullDuplex == profile.extendedFullDuplex &&
      s.crossover.pairSelection == profile.crossover.pairSelection &&
      s.crossover.robust == profile.crossover.robust &&
      s.crossover.swapPair == profile.crossover.swapPair &&
      s.crossover.swapPolarity == profile.crossover.swapPolarity;
}
bool fresh(const LinkSnapshot& link, uint64_t nowUs) noexcept {
  return link.timeValid && link.observedSinceUs <= link.observedAtUs &&
      link.observedAtUs <= nowUs && nowUs - link.observedSinceUs <= MAX_LINK_AGE_US;
}
} // namespace

Status PhyOwner::begin(const LinkSettings& profile) noexcept {
  if (_state.phase != OwnerPhase::UNCONFIGURED) return denied("Owner already configured");
  const auto valid = validateLinkSettings(profile);
  if (!valid.ok()) return valid;
  if (profile.mode != LinkMode::AUTONEGOTIATED || profile.advertisement.technologyMask != LINK_100_FULL)
    return Status::Error(Err::INVALID_CONFIG, "Require 100/full-only AN");
  _profile = profile;
  _state.phase = OwnerPhase::WAITING;
  return Status::Ok();
}

void PhyOwner::withdraw() noexcept {
  if (_state.phase == OwnerPhase::ACTIVE) _state.phase = OwnerPhase::WITHDRAWN;
  else if (_state.phase == OwnerPhase::READY) _state.phase = OwnerPhase::WAITING;
}

bool PhyOwner::checkTime(uint64_t nowUs) noexcept {
  if (nowUs < _lastClockUs) _clockInvalid = true;
  _lastClockUs = nowUs;
  const bool inTime = !_clockInvalid && nowUs - _lastServiceUs <= MAX_SERVICE_GAP_US;
  if (!inTime) withdraw();
  return inTime;
}

void PhyOwner::observe(uint64_t nowUs, const HealthSnapshot& health, const SettingsSnapshot& settings,
                       const LinkSnapshot& link, bool frameworkReady) noexcept {
  checkTime(nowUs);
  const auto& old = _state.link;
  if (_state.phase == OwnerPhase::ACTIVE &&
      (link.changes != old.changes || link.observations < old.observations ||
       link.observations - old.observations > 1U ||
       (link.observations != old.observations && (link.rawBmsrFirst & 4U) == 0U))) withdraw();

  _lastServiceUs = nowUs;
  _state.evidenceAvailable = true;
  _state.frameworkReady = frameworkReady && !_state.frameworkRestartRequired;
  _state.health = health;
  _state.settings = settings;
  _state.link = link;
  const bool usable = !_clockInvalid && _state.frameworkReady && health.initialized && health.state == DriverState::READY &&
      settings.trust == ConfigurationTrust::VERIFIED && settings.desiredValid && settings.observedValid &&
      !settings.observedStale && settings.lastVerificationStatus.ok() &&
      matches(settings.desired, _profile) && matches(settings.observed, _profile) &&
      link.valid && !link.stale && link.lastRefreshStatus.ok() && link.linkUp &&
      !link.temporaryMayBeActive && link.negotiation == NegotiationState::COMPLETE &&
      link.resolvedModeValid && link.speed == LinkSpeed::MBPS_100 && link.duplex == Duplex::FULL &&
      link.observations != std::numeric_limits<uint32_t>::max() &&
      link.changes != std::numeric_limits<uint32_t>::max() && fresh(link, nowUs) &&
      (!_maintenanceEnded || link.observedSinceUs > _maintenanceEndUs);
  if (!usable) withdraw();
  else if (_state.phase == OwnerPhase::WAITING) _state.phase = OwnerPhase::READY;
}

void PhyOwner::missingEvidence(uint64_t nowUs) noexcept {
  checkTime(nowUs);
  _lastServiceUs = nowUs;
  _state.evidenceAvailable = false;
  withdraw();
}

void PhyOwner::cachedLinkDown(uint64_t nowUs) noexcept {
  missingEvidence(nowUs);
  // A later identical PHY UP can leave the driver's cached link DOWN: the
  // adapter suppresses duplicate UP publications. Require a framework restart.
  _state.frameworkReady = false;
  _state.frameworkRestartRequired = true;
}

void PhyOwner::frameworkRestarted() noexcept {
  _state.frameworkRestartRequired = false;
}

Status PhyOwner::admit(uint64_t nowUs, uint64_t& generation) noexcept {
  if (!checkTime(nowUs) || !fresh(_state.link, nowUs)) withdraw();
  if (_state.phase != OwnerPhase::READY || _state.generation == std::numeric_limits<uint64_t>::max())
    return denied("Session requires current verified 100/full");
  generation = ++_state.generation;
  _state.phase = OwnerPhase::ACTIVE;
  return Status::Ok();
}
Status PhyOwner::allowFrame(uint64_t nowUs, uint64_t generation) noexcept {
  if (!checkTime(nowUs) || !fresh(_state.link, nowUs)) withdraw();
  if (_state.phase != OwnerPhase::ACTIVE || generation != _state.generation)
    return denied("Application generation withdrawn");
  return Status::Ok();
}
Status PhyOwner::release(uint64_t generation) noexcept {
  if ((_state.phase != OwnerPhase::ACTIVE && _state.phase != OwnerPhase::WITHDRAWN) ||
      generation != _state.generation) return denied("No matching application session");
  _state.phase = OwnerPhase::WAITING;
  return Status::Ok();
}
Status PhyOwner::beginMaintenance(bool framesQuiescent) noexcept {
  if ((_state.phase != OwnerPhase::WAITING && _state.phase != OwnerPhase::READY) ||
      _state.resultPending || !framesQuiescent) return denied("Maintenance requires release and quiescence");
  _state.phase = OwnerPhase::MAINTENANCE;
  return Status::Ok();
}
void PhyOwner::finishMaintenance(uint64_t nowUs) noexcept {
  if (_state.phase != OwnerPhase::MAINTENANCE) return;
  missingEvidence(nowUs);
  _maintenanceEndUs = nowUs;
  _maintenanceEnded = true;
  _state.phase = OwnerPhase::WAITING;
}
Status PhyOwner::retainResult(const OperationResult& result) noexcept {
  if (_state.resultPending) return denied("Deliver pending operation result first");
  if (result.token.value == 0 || result.status.inProgress()) return Status::Error(Err::INVALID_PARAM,"Expected terminal result");
  _result = result;
  _state.resultPending = true;
  return Status::Ok();
}
Status PhyOwner::takeResult(OperationResult& result) noexcept {
  if (!_state.resultPending) return Status::Error(Err::RESULT_NOT_AVAILABLE,"No retained operation result");
  result = _result;
  _state.resultPending = false;
  return Status::Ok();
}
} // namespace owner_example
