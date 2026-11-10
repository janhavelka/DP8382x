/// @file DP8382xLink.cpp
/// @brief Bounded live link acquisition and passive change evidence.
#include <DP8382x/DP8382x.h>
#include <DP8382x/CommandTable.h>

#include <limits>

namespace DP8382x {
namespace {
bool sameLink(const LinkSnapshot& a, const LinkSnapshot& b) noexcept {
  return a.linkUp == b.linkUp && a.negotiation == b.negotiation &&
      a.speed == b.speed && a.duplex == b.duplex && a.resolvedModeValid == b.resolvedModeValid &&
      a.pauseValid == b.pauseValid && a.pauseRx == b.pauseRx && a.pauseTx == b.pauseTx &&
      a.temporaryMayBeActive == b.temporaryMayBeActive;
}
Status linkTimeout(TimeoutDetail detail) noexcept {
  return Status::Error(Err::OPERATION_TIMEOUT, "Link observation budget exhausted", static_cast<int32_t>(detail));
}
} // namespace

void DP8382x::_invalidateLink() noexcept {
  if (_link.valid) {
    if (!_link.stale && _link.changes != std::numeric_limits<uint32_t>::max()) { ++_link.changes; }
    _link.change = LinkChange::STALE;
  }
  _link.stale = true;
  _link.lastRefreshStatus = Status::Error(Err::INVALID_CONFIG, "Link evidence invalidated",
                                         static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
}

Status DP8382x::readLinkStatus(LinkSnapshot& snapshot, const IoBudget& budget) noexcept {
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || _polling || (_operation.state == OperationState::RUNNING &&
      ((!_configuring && _operation.phase == OperationPhase::WAIT) || _result.nextPage.requested))) {
    // ANER has one destructive consumer during the typed next-page epoch.
    return Status::Error(Err::BUSY, "Observation owner unavailable", static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
  }
  const auto fail = [&](Status status) noexcept {
    _invalidateLink();
    _link.lastRefreshStatus = status;
    _evidence.lastStatus = status;
    return status;
  };
  if (_transport.clockInvalid) {
    return fail(Status::Error(Err::INVALID_CONFIG, "Clock contract failed", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)));
  }
  const auto quiet = _checkQuiet();
  if (!quiet.ok()) { return _transport.clockInvalid ? fail(quiet) : quiet; }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED, "Initialize before observing link"); }
  if (_stopped) {
    return Status::Error(Err::BUSY, "Controller report fault", static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED));
  }
  if (budget.maxFrames < 10U) { return fail(linkTimeout(TimeoutDetail::FRAME_BUDGET)); }
  // Reserve the sum of both callback bounds before consuming the first latch.
  // _execute rechecks each actual submission against the same absolute deadline.
  uint64_t raw = 0, upper = 0;
  _active = true; // Clock callbacks also obey the no-reentry contract.
  const auto clockStatus = _sampleTime(raw, upper);
  _active = false;
  if (!clockStatus.ok()) { return fail(clockStatus); }
  if (raw >= budget.deadlineUs) { return fail(linkTimeout(TimeoutDetail::DEADLINE)); }
  const auto& bounds = _config.transport.bounds;
  const uint64_t bound = uint64_t{2} * bounds.fixedUs + uint64_t{10} * bounds.frameUs;
  if (upper >= budget.deadlineUs || bound >= budget.deadlineUs - upper) {
    return fail(linkTimeout(TimeoutDetail::BOUND_ADMISSION));
  }

  // SNLS647I tables8-20/34: first BMSR consumes historical latch-low; second
  // establishes current state. PHYSTS precedes the clearing ANER dependency.
  constexpr uint8_t addresses[] = {cmd::REG_BMSR, cmd::REG_BMSR, cmd::REG_PHYSTS,
      cmd::REG_BMCR, cmd::REG_ANAR, cmd::REG_ALNPAR, cmd::REG_ANER, cmd::REG_PHYCR,
      cmd::REG_BMSR, cmd::REG_PHYSTS};
  uint16_t words[10]{};
  IoBudget remaining = budget;
  _evidence.lastRequested = evidenceBit(EvidenceSource::BMSR) | evidenceBit(EvidenceSource::PHYSTS) |
                            evidenceBit(EvidenceSource::ANER);
  for (uint8_t offset = 0; offset < 10U;) {
    MdioBatch batch;
    batch.frameCount = offset == 0U ? uint8_t{8} : uint8_t{2};
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      batch.frames[i] = {FrameKind::READ, addresses[offset + i], 0, 0, NO_READ_INDEX};
    }
    const auto status = _execute(batch, remaining);
    if (!status.ok()) { return fail(status); }
    for (uint8_t i = 0; i < batch.frameCount; ++i) { words[offset + i] = _transport.lastBatch.frames[i].value; }
    remaining.maxFrames -= batch.frameCount;
    offset = static_cast<uint8_t>(offset + batch.frameCount);
  }

  LinkSnapshot next;
  next.rawBmsrFirst = words[0]; next.rawBmsrCurrent = words[1]; next.rawPhystsFirst = words[2];
  next.rawBmcr = words[3]; next.rawAnar = words[4]; next.rawAnlpar = words[5];
  next.rawAner = words[6]; next.rawPhycr = words[7];
  next.rawBmsrFinal = words[8]; next.rawPhystsFinal = words[9];
  constexpr uint16_t basicMask = cmd::MASK_BMSR_LINK | cmd::MASK_BMSR_AN_COMPLETE;
  constexpr uint16_t statusMask = cmd::MASK_PHYSTS_LINK | cmd::MASK_PHYSTS_AN_COMPLETE;
  const auto agrees = [](uint16_t basic, uint16_t status) noexcept {
    return ((basic & cmd::MASK_BMSR_LINK) != 0U) == ((status & cmd::MASK_PHYSTS_LINK) != 0U) &&
        ((basic & cmd::MASK_BMSR_AN_COMPLETE) != 0U) == ((status & cmd::MASK_PHYSTS_AN_COMPLETE) != 0U);
  };
  if (((words[1] ^ words[8]) & basicMask) != 0U || ((words[2] ^ words[9]) & statusMask) != 0U ||
      !agrees(words[1], words[2]) || !agrees(words[8], words[9])) {
    return fail(Status::Error(Err::BUSY, "Link changed during observation", static_cast<int32_t>(BusyDetail::OBSERVATION_CHANGED)));
  }
  const bool an = (next.rawBmcr & cmd::MASK_BMCR_AN_ENABLE) != 0U;
  const bool complete = (next.rawBmsrFinal & cmd::MASK_BMSR_AN_COMPLETE) != 0U;
  next.negotiation = !an ? NegotiationState::DISABLED :
      (complete ? NegotiationState::COMPLETE : NegotiationState::IN_PROGRESS);
  next.linkUp = (next.rawBmsrFinal & cmd::MASK_BMSR_LINK) != 0U;
  next.temporaryMayBeActive = _operation.temporaryMayBeActive;
  const bool overridden = next.temporaryMayBeActive || (_operation.state == OperationState::RUNNING &&
      (_result.mutationAttempted || _result.mutationPossible));
  const bool extensionKnown = _settings.trust == ConfigurationTrust::VERIFIED &&
      _settings.cr2Valid && _settings.desiredValid;
  const bool extension = extensionKnown && (_settings.rawCr2 & cmd::MASK_CR2_EXTENDED_DUPLEX) != 0U;
  if (next.linkUp && !overridden && (next.rawBmcr & cmd::MASK_BMCR_RESET) == 0U) {
    if (!an) {
      // PHYSTS low mode bits are explicitly not a forced-mode oracle.
      next.speed = (next.rawBmcr & cmd::MASK_BMCR_SPEED) != 0U ? LinkSpeed::MBPS_100 : LinkSpeed::MBPS_10;
      next.duplex = (next.rawBmcr & cmd::MASK_BMCR_DUPLEX) != 0U ? Duplex::FULL : Duplex::HALF;
      if (next.speed == LinkSpeed::MBPS_100 && next.duplex == Duplex::HALF &&
          (!extensionKnown || extension || _settings.desired.extendedFullDuplex)) { next.duplex = Duplex::UNKNOWN; }
      next.resolvedModeValid = next.duplex != Duplex::UNKNOWN;
    } else if (complete && (next.rawAner & cmd::MASK_ANER_PARALLEL_FAULT) == 0U) {
      next.speed = (next.rawPhystsFinal & cmd::MASK_PHYSTS_SPEED) != 0U ? LinkSpeed::MBPS_10 : LinkSpeed::MBPS_100;
      next.duplex = (next.rawPhystsFinal & cmd::MASK_PHYSTS_DUPLEX) != 0U ? Duplex::FULL : Duplex::HALF;
      const bool partnerAn = (next.rawAner & cmd::MASK_ANER_PARTNER_AN) != 0U;
      // Table8-29: parallel detection can become 100/full only through extension.
      if (!partnerAn && next.duplex == Duplex::FULL && !(extension && next.speed == LinkSpeed::MBPS_100)) {
        next.duplex = Duplex::UNKNOWN;
      }
      next.resolvedModeValid = next.duplex != Duplex::UNKNOWN;
      const uint16_t common = next.rawAnar & next.rawAnlpar;
      // Table28B-3 applies only when the highest common technology is full duplex.
      // A plausible PHYSTS word alone cannot qualify inconsistent base pages.
      const bool commonFull = (common & cmd::MASK_ANAR_FULL100) != 0U ? next.speed == LinkSpeed::MBPS_100 :
          (common & cmd::MASK_ANAR_HALF100) == 0U && (common & cmd::MASK_ANAR_FULL10) != 0U &&
          next.speed == LinkSpeed::MBPS_10;
      if (next.resolvedModeValid && partnerAn && next.duplex == Duplex::FULL && commonFull &&
          (next.rawAnar & cmd::MASK_ANAR_SELECTOR) == 1U && (next.rawAnlpar & cmd::MASK_ALNPAR_SELECTOR) == 1U) {
        // IEEE802.3 Table28B-3. Asymmetric RX/TX is resolved in local orientation.
        const bool localPause = (next.rawAnar & cmd::MASK_ANAR_SYMMETRIC_PAUSE) != 0U;
        const bool remotePause = (next.rawAnlpar & cmd::MASK_ALNPAR_PAUSE) != 0U;
        const bool asymmetric = (next.rawAnar & cmd::MASK_ANAR_ASYM_PAUSE) != 0U &&
                                (next.rawAnlpar & cmd::MASK_ALNPAR_ASYM_PAUSE) != 0U;
        const bool rx = (localPause && remotePause) || (asymmetric && localPause);
        const bool tx = (localPause && remotePause) || (asymmetric && remotePause);
        next.pauseValid = rx == ((next.rawPhycr & cmd::MASK_PHYCR_PAUSE_RX) != 0U) &&
                          tx == ((next.rawPhycr & cmd::MASK_PHYCR_PAUSE_TX) != 0U);
        if (next.pauseValid) { next.pauseRx = rx; next.pauseTx = tx; }
      }
    }
  }
  if (an && next.resolvedModeValid &&
      ((next.rawPhystsFirst ^ next.rawPhystsFinal) & (cmd::MASK_PHYSTS_SPEED | cmd::MASK_PHYSTS_DUPLEX)) != 0U) {
    return fail(Status::Error(Err::BUSY, "Resolved mode changed during observation", static_cast<int32_t>(BusyDetail::OBSERVATION_CHANGED)));
  }
  next.valid = true;
  next.stale = false;
  next.timeValid = true;
  next.observedSinceUs = raw;
  next.observedAtUs = _lastTimeUs + bounds.clockLagUs;
  next.lastRefreshStatus = Status::Ok();
  next.change = !_link.valid ? LinkChange::FIRST_VALID : _link.stale ? LinkChange::REGAINED :
      sameLink(next, _link) ? LinkChange::REPEATED : LinkChange::CHANGED;
  next.observations = _link.observations;
  if (next.observations != std::numeric_limits<uint32_t>::max()) { ++next.observations; }
  next.changes = _link.changes;
  if (next.change != LinkChange::REPEATED && next.changes != std::numeric_limits<uint32_t>::max()) { ++next.changes; }
  _link = next;
  _evidence.lastStatus = Status::Ok();
  snapshot = next;
  return Status::Ok();
}
} // namespace DP8382x
