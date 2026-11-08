/// @file DP8382xConfiguration.cpp
/// @brief Coupled link profiles and finite Clause 28 next-page service.
#include <DP8382x/DP8382x.h>
#include <DP8382x/CommandTable.h>

#include <limits>

namespace DP8382x {
namespace {
// SNLS647I tables 8-19/23/26: keep unrelated fields while neutralizing
// reset/restart and the prohibited ANAR remote-fault advertisement.
constexpr uint16_t ANAR_PRESERVE = 0x5200;
constexpr uint16_t ANAR_VERIFY = 0xADFF;
constexpr uint16_t BMCR_PRESERVE = 0x4CFF;
constexpr uint16_t BMCR_VERIFY = 0x3100;
constexpr uint16_t ANNPTR_PRESERVE = 0x4800;
// Tables 8-28/29/30/43: only these stable owned fields change. PHYCR14
// is inactive in automatic routing and must be preserved rather than cleared.
constexpr uint16_t MIRROR_MASK = cmd::MASK_CR3_PAIR_SWAP | cmd::MASK_CR3_POLARITY_SWAP;
constexpr uint16_t BASE_CAPABILITIES = cmd::MASK_ANER_LOCAL_NEXT | cmd::MASK_ANER_PARTNER_NEXT |
                                     cmd::MASK_ANER_PARTNER_AN;

Status invalidSettings(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG, message,
                       static_cast<int32_t>(ConfigDetail::LINK_COMBINATION));
}

Status configBusy(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY, "Configuration owner unavailable", static_cast<int32_t>(detail));
}

Status configTimeout(TimeoutDetail detail) noexcept {
  return Status::Error(Err::OPERATION_TIMEOUT, "Configuration budget exhausted", static_cast<int32_t>(detail));
}

uint16_t advertisementWord(const LinkSettings& settings) noexcept {
  // SNLS647I table 8-23: API abilities occupy bits 0..3; wire abilities 5..8.
  return static_cast<uint16_t>(0x0001U | (uint16_t{settings.advertisement.technologyMask} << 5U) |
      (settings.advertisement.symmetricPause ? cmd::MASK_ANAR_SYMMETRIC_PAUSE : 0U) |
      (settings.advertisement.asymmetricPause ? cmd::MASK_ANAR_ASYM_PAUSE : 0U));
}

uint16_t modeWord(const LinkSettings& settings) noexcept {
  if (settings.mode == LinkMode::AUTONEGOTIATED) { return cmd::MASK_BMCR_AN_ENABLE; }
  return static_cast<uint16_t>((settings.forced.speed == LinkSpeed::MBPS_100 ? cmd::MASK_BMCR_SPEED : 0U) |
      (settings.forced.duplex == Duplex::FULL ? cmd::MASK_BMCR_DUPLEX : 0U));
}

uint16_t routingMask(const LinkSettings& settings) noexcept {
  return settings.crossover.pairSelection == PairSelection::AUTOMATIC ? cmd::MASK_PHYCR_AUTO_MDIX :
      static_cast<uint16_t>(cmd::MASK_PHYCR_AUTO_MDIX | cmd::MASK_PHYCR_FORCE_MDIX);
}

uint16_t routingWord(const LinkSettings& settings) noexcept {
  return settings.crossover.pairSelection == PairSelection::AUTOMATIC ? cmd::MASK_PHYCR_AUTO_MDIX :
      (settings.crossover.pairSelection == PairSelection::MDIX ? cmd::MASK_PHYCR_FORCE_MDIX : uint16_t{0});
}

Status validatePages(const NextPageExchange& exchange) noexcept {
  if (exchange.pageCount > MAX_NEXT_PAGES || exchange.maxPartnerPages == 0U ||
      exchange.maxPartnerPages > MAX_NEXT_PAGES) {
    return Status::Error(Err::INVALID_PARAM, "Invalid next-page bound",
                         static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  // IEEE 802.3-2012 Annex 28C: these messages have defined 16-bit encoding
  // without advertising a technology absent from this exact 10/100 PHY.
  uint8_t remaining = 0;
  bool remoteFault = false;
  for (uint8_t i = 0; i < exchange.pageCount; ++i) {
    const auto& page = exchange.pages[i];
    if (page.data > cmd::MASK_ANNPTR_CODE) {
      return Status::Error(Err::INVALID_PARAM, "Next-page payload exceeds eleven bits",
                           static_cast<int32_t>(ParamDetail::VALUE_WIDTH));
    }
    if (remaining != 0U) {
      if (page.message || (remoteFault && page.data > 3U)) {
        return invalidSettings("Invalid ordered unformatted next page");
      }
      --remaining;
      continue;
    }
    if (!page.message) { return invalidSettings("Unformatted page has no defining message"); }
    remoteFault = false;
    switch (page.data) {
      case 1:
        // Clause 28.2.3.4/Annex 28C.2 defines null as end of information.
        // Infer local admission: a later substantive message would contradict it.
        if (i + 1U < exchange.pageCount) { return invalidSettings("Null message must terminate local information"); }
        break;
      case 4: remaining = 1; remoteFault = true; break;
      case 5: case 6: remaining = 4; break;
      default: return invalidSettings("Message is reserved or outside the admitted page semantics");
    }
  }
  return remaining == 0U ? Status::Ok() : invalidSettings("Message lacks its following unformatted pages");
}

MdioBatch readRegister(uint8_t address) noexcept {
  MdioBatch batch;
  batch.frameCount = 1;
  batch.frames[0] = {FrameKind::READ, address, 0, 0, NO_READ_INDEX};
  return batch;
}

MdioBatch updateRegister(uint8_t address, uint16_t preserve, uint16_t value) noexcept {
  auto batch = readRegister(address);
  batch.frameCount = 2;
  batch.frames[1] = {FrameKind::WRITE, address, value, preserve, 0};
  return batch;
}

Status mismatchStatus() noexcept {
  return Status::Error(Err::CONFIG_MISMATCH, "Link profile verification failed");
}
} // namespace

Status DP8382x::startConfigure(const LinkSettings& settings, const ConfigureOptions& options,
                              const OperationBudget& budget, OperationToken& token) noexcept {
  auto status = validateLinkSettings(settings);
  if (!status.ok()) { return status; }
  status = validatePages(options.nextPages);
  if (!status.ok()) { return status; }
  if (settings.mode == LinkMode::FORCED &&
      (options.waitForCompletion || options.nextPages.pageCount != 0U)) {
    return invalidSettings("Forced mode has no negotiation wait or next pages");
  }
  if (budget.pollIntervalUs == 0U || budget.maxStalledPolls == 0U) {
    return Status::Error(Err::INVALID_PARAM, "Invalid polling budget",
                         static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return configBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state == OperationState::RESULT_PENDING) { return configBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) {
    return Status::Error(Err::INVALID_CONFIG, "Clock contract failed", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  }
  if (_operation.cleanupOwed && _cleanupOwner != OperationKind::CONFIGURE) { return configBusy(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized) { return Status::Error(Err::NOT_INITIALIZED, "Initialize before configuring"); }
  if (!capabilities().valid) {
    return Status::Error(Err::NOT_INITIALIZED, "Fresh accepted identity is required");
  }
  if (budget.io.maxFrames == 0U) { return configTimeout(TimeoutDetail::FRAME_BUDGET); }
  if (budget.io.deadlineUs == 0U) { return configTimeout(TimeoutDetail::DEADLINE); }

  const bool owed = _operation.cleanupOwed;
  const auto quietUntil = _operation.busQuietUntilUs;
  const bool quietValid = _operation.busQuietValid;
  _operation = {};
  _operation.cleanupOwed = owed;
  _operation.busQuietUntilUs = quietUntil;
  _operation.busQuietValid = quietValid;
  _operation.token = {++_tokenCounter};
  _operation.kind = OperationKind::CONFIGURE;
  _operation.state = OperationState::RUNNING;
  _operation.status = Status::Error(Err::IN_PROGRESS, "Link configuration pending");
  _operation.remaining = budget.io;
  _operationBudget = budget;
  _result = {};
  _result.token = _operation.token;
  _result.kind = _operation.kind;
  _result.identity = _identity;
  _settings.desired = settings;
  _settings.desiredValid = true;
  _settings.trust = ConfigurationTrust::DIRTY;
  _settings.observedStale = true;
  _configureOptions = options;
  _pollTimeValid = false;
  _beginConfiguration(options.nextPages.pageCount != 0U);
  token = _operation.token;
  return _operation.status;
}

void DP8382x::_beginConfiguration(bool exchange) noexcept {
  if (_operation.kind == OperationKind::INITIALIZE) { _configureOptions = {}; }
  _configuring = true;
  _pageIndex = 0;
  _partnerToggle = false;
  _localMore = false;
  _partnerMore = false;
  _pageEventPending = false;
  _pageEventRaw = 0;
  _result.nextPage.requested = exchange;
  _setConfigStep(exchange ? ConfigStep::DRAIN :
      (_settings.desired.mode == LinkMode::FORCED ? ConfigStep::READ_BMCR : ConfigStep::READ_ANAR));
}

void DP8382x::_setConfigStep(ConfigStep step) noexcept {
  _configStep = step;
  _operation.nextPollValid = false;
  switch (step) {
    case ConfigStep::DRAIN: case ConfigStep::READ_ANAR: case ConfigStep::READ_BMCR:
    case ConfigStep::READ_CR1: case ConfigStep::READ_PHYCR: case ConfigStep::READ_CR3: case ConfigStep::READ_CR2:
      _operation.phase = OperationPhase::PREFLIGHT; break;
    case ConfigStep::VERIFY_ANAR: case ConfigStep::VERIFY_BMCR: case ConfigStep::VERIFY_CLEAR:
    case ConfigStep::VERIFY_CR1: case ConfigStep::VERIFY_PHYCR: case ConfigStep::VERIFY_CR3: case ConfigStep::VERIFY_CR2:
      _operation.phase = OperationPhase::VERIFY; break;
    case ConfigStep::WAIT_BASE: case ConfigStep::READ_BASE: case ConfigStep::WAIT_PAGE:
    case ConfigStep::READ_PAGE: case ConfigStep::WAIT_COMPLETE:
      _operation.phase = OperationPhase::WAIT; break;
    case ConfigStep::APPLY_BMCR: _operation.phase = OperationPhase::COMMAND; break;
    case ConfigStep::APPLY_ANAR: case ConfigStep::LOAD_PAGE: case ConfigStep::CLEAR_NP:
    case ConfigStep::APPLY_CR1: case ConfigStep::APPLY_PHYCR: case ConfigStep::APPLY_CR3: case ConfigStep::APPLY_CR2:
      _operation.phase = OperationPhase::RESTORE; break;
  }
}

MdioBatch DP8382x::_configurationBatch() const noexcept {
  // Every RMW is one terminal controller transaction; no selector/read word
  // survives a batch. Table 8-23 reserves 14/12/9 and prohibits local RF13.
  switch (_configStep) {
    case ConfigStep::DRAIN: case ConfigStep::WAIT_BASE: case ConfigStep::WAIT_PAGE: return readRegister(cmd::REG_ANER);
    case ConfigStep::READ_ANAR: case ConfigStep::VERIFY_ANAR: case ConfigStep::VERIFY_CLEAR: return readRegister(cmd::REG_ANAR);
    case ConfigStep::READ_BMCR: case ConfigStep::VERIFY_BMCR: return readRegister(cmd::REG_BMCR);
    case ConfigStep::READ_CR1: case ConfigStep::VERIFY_CR1: return readRegister(cmd::REG_CR1);
    case ConfigStep::READ_PHYCR: case ConfigStep::VERIFY_PHYCR: return readRegister(cmd::REG_PHYCR);
    case ConfigStep::READ_CR3: case ConfigStep::VERIFY_CR3: return readRegister(cmd::REG_CR3);
    case ConfigStep::READ_CR2: case ConfigStep::VERIFY_CR2: return readRegister(cmd::REG_CR2);
    case ConfigStep::APPLY_CR1:
      return updateRegister(cmd::REG_CR1, static_cast<uint16_t>(~cmd::MASK_CR1_ROBUST_MDIX),
          _settings.desired.crossover.robust ? cmd::MASK_CR1_ROBUST_MDIX : 0);
    case ConfigStep::APPLY_PHYCR:
      return updateRegister(cmd::REG_PHYCR, static_cast<uint16_t>(~routingMask(_settings.desired)), routingWord(_settings.desired));
    case ConfigStep::APPLY_CR3:
      return updateRegister(cmd::REG_CR3, static_cast<uint16_t>(~MIRROR_MASK), _settings.desired.crossover.swapPair ? MIRROR_MASK : 0);
    case ConfigStep::APPLY_CR2:
      return updateRegister(cmd::REG_CR2, static_cast<uint16_t>(~cmd::MASK_CR2_EXTENDED_DUPLEX),
          _settings.desired.extendedFullDuplex ? cmd::MASK_CR2_EXTENDED_DUPLEX : 0);
    case ConfigStep::APPLY_ANAR:
      return updateRegister(cmd::REG_ANAR, ANAR_PRESERVE, static_cast<uint16_t>(advertisementWord(_settings.desired) |
          (_result.nextPage.requested ? cmd::MASK_ANAR_NEXT_PAGE : 0U)));
    case ConfigStep::APPLY_BMCR:
      // Activate only after all coupled controls. Forced mode neutralizes both
      // commands; AN alone requests restart. Neither branch touches VOD.
      return updateRegister(cmd::REG_BMCR, BMCR_PRESERVE, static_cast<uint16_t>(modeWord(_settings.desired) |
          (_settings.desired.mode == LinkMode::AUTONEGOTIATED ? cmd::MASK_BMCR_AN_RESTART : 0U)));
    case ConfigStep::READ_BASE: return readRegister(cmd::REG_ALNPAR);
    case ConfigStep::READ_PAGE: return readRegister(cmd::REG_ANLNPTR);
    case ConfigStep::WAIT_COMPLETE: return readRegister(cmd::REG_BMSR);
    case ConfigStep::CLEAR_NP: return updateRegister(cmd::REG_ANAR, ANAR_PRESERVE, advertisementWord(_settings.desired));
    case ConfigStep::LOAD_PAGE: {
      const auto count = _configureOptions.nextPages.pageCount;
      const NextPage page = _pageIndex < count ? _configureOptions.nextPages.pages[_pageIndex] : NextPage{1, true, false};
      const bool more = _pageIndex + 1U < count;
      // Table 8-26: hardware controls toggle11; bit14 is read-only reserved.
      return updateRegister(cmd::REG_ANNPTR, ANNPTR_PRESERVE, static_cast<uint16_t>(page.data |
          (page.message ? cmd::MASK_ANNPTR_MESSAGE : 0U) |
          (page.acknowledge2 ? cmd::MASK_ANNPTR_ACK2 : 0U) | (more ? cmd::MASK_ANNPTR_NEXT_PAGE : 0U)));
    }
  }
  return {};
}

void DP8382x::_retainPageEvidence(const MdioBatch& batch, const BatchResult& receipt,
                                 uint8_t completed, bool clockValid, uint64_t upper) noexcept {
  if (!_configuring || !_result.nextPage.requested || _operation.state != OperationState::RUNNING) { return; }
  auto& next = _result.nextPage;
  for (uint8_t i = 0; i < batch.frameCount; ++i) {
    if (batch.frames[i].kind != FrameKind::READ) { continue; }
    const auto address = batch.frames[i].registerAddress;
    const auto& frame = receipt.frames[i];
    if (i >= completed) {
      if ((address == cmd::REG_ANER || (_pageEventPending &&
          (address == cmd::REG_ALNPAR || address == cmd::REG_ANLNPTR))) &&
          (frame.attempted || frame.effectPossible || frame.completed || frame.valueValid ||
           _transport.violation != TransportViolation::NONE)) {
        next.pagePayloadMissing = true;
      }
      continue;
    }
    if (address == cmd::REG_ANER && (_configStep == ConfigStep::WAIT_BASE || _configStep == ConfigStep::WAIT_PAGE) &&
        (frame.value & cmd::MASK_ANER_PAGE) != 0U && !_pageEventPending) {
      _pageEventPending = true;
      _pageEventRaw = frame.value;
      next.pagePayloadMissing = true;
      if (_configStep == ConfigStep::WAIT_BASE) { next.baseReceived = true; }
      else if (next.receivedPages < MAX_NEXT_PAGES) { next.pages[next.receivedPages++] = {}; }
    }
    if (address == cmd::REG_ALNPAR && _configStep == ConfigStep::READ_BASE && _pageEventPending) {
      next.basePage = frame.value;
      next.basePageValid = true;
      next.pagePayloadMissing = false;
    }
    if (address == cmd::REG_ANLNPTR && _configStep == ConfigStep::READ_PAGE && _pageEventPending && next.receivedPages != 0U) {
      auto& page = next.pages[next.receivedPages - 1U];
      page.raw = frame.value;
      page.valid = true;
      page.observedAtUs = upper;
      page.timeValid = clockValid;
      next.pagePayloadMissing = false;
    }
  }
}

void DP8382x::_observeConfiguration() noexcept {
  // Called only after the complete stable readback pass. Decode hardware,
  // never copy intent over a valid mismatch. A forced request did not acquire
  // ANAR, so an unexpectedly enabled AN mode cannot form a complete AN tuple.
  LinkSettings observed;
  observed.mode = (_settings.rawBmcr & cmd::MASK_BMCR_AN_ENABLE) != 0U ? LinkMode::AUTONEGOTIATED : LinkMode::FORCED;
  if (observed.mode == LinkMode::AUTONEGOTIATED) {
    if (_settings.desired.mode == LinkMode::FORCED || (_settings.rawAnar & cmd::MASK_ANAR_SELECTOR) != 1U) {
      // Without a fresh IEEE-selector advertisement, retain any older tuple
      // and timestamp as history; raw current words still diagnose the mismatch.
      _settings.observedStale = true;
      return;
    }
    observed.advertisement.technologyMask = static_cast<uint8_t>((_settings.rawAnar & cmd::MASK_ANAR_TECHNOLOGY) >> 5U);
    observed.advertisement.symmetricPause = (_settings.rawAnar & cmd::MASK_ANAR_SYMMETRIC_PAUSE) != 0U;
    observed.advertisement.asymmetricPause = (_settings.rawAnar & cmd::MASK_ANAR_ASYM_PAUSE) != 0U;
  } else {
    observed.forced.speed = (_settings.rawBmcr & cmd::MASK_BMCR_SPEED) != 0U ? LinkSpeed::MBPS_100 : LinkSpeed::MBPS_10;
    observed.forced.duplex = (_settings.rawBmcr & cmd::MASK_BMCR_DUPLEX) != 0U ? Duplex::FULL : Duplex::HALF;
  }
  observed.crossover.robust = (_settings.rawCr1 & cmd::MASK_CR1_ROBUST_MDIX) != 0U;
  observed.crossover.pairSelection = (_settings.rawPhycr & cmd::MASK_PHYCR_AUTO_MDIX) != 0U ? PairSelection::AUTOMATIC :
      ((_settings.rawPhycr & cmd::MASK_PHYCR_FORCE_MDIX) != 0U ? PairSelection::MDIX : PairSelection::MDI);
  observed.crossover.swapPair = (_settings.rawCr3 & cmd::MASK_CR3_PAIR_SWAP) != 0U;
  observed.crossover.swapPolarity = (_settings.rawCr3 & cmd::MASK_CR3_POLARITY_SWAP) != 0U;
  observed.extendedFullDuplex = (_settings.rawCr2 & cmd::MASK_CR2_EXTENDED_DUPLEX) != 0U;
  _settings.observed = observed;
  _settings.observedValid = true;
  _settings.observedStale = false;
  _settings.observedAtUs = _lastTimeUs + _config.transport.bounds.clockLagUs;
  _settings.timeValid = true;
}

Status DP8382x::_advanceConfiguration(Status status, bool called, const MdioBatch& batch) noexcept {
  const auto completedStep = _configStep;
  const auto& receipt = _transport.lastBatch;
  const uint16_t value = receipt.frames[0].value;
  const bool readValid = called && _result.failingBatch.completedFrames != 0U;
  // A completed page load remains evidence if release or the deadline fails.
  if (called && _configStep == ConfigStep::LOAD_PAGE && _result.failingBatch.completedFrames == 2U) {
    ++_result.nextPage.transmittedPages;
  }
  if (called && _configStep == ConfigStep::APPLY_ANAR && _result.nextPage.requested &&
      (receipt.frames[1].attempted || receipt.frames[1].effectPossible || receipt.frames[1].completed ||
       receipt.frames[1].valueValid || _transport.violation != TransportViolation::NONE)) {
    _result.nextPage.npMayRemainSet = true;
  }
  // Preserve each independently returned word and the first mismatch even if
  // release, the next read, or the post-callback deadline check subsequently fails.
  if (readValid) {
    if (_configStep == ConfigStep::READ_ANAR) { _result.savedAnar = value; _result.savedAnarValid = true; }
    if (_configStep == ConfigStep::READ_BMCR) { _result.savedBmcr = value; _result.savedBmcrValid = true; }
    if (_configStep == ConfigStep::READ_CR1) { _result.savedCr1 = value; _result.savedCr1Valid = true; }
    if (_configStep == ConfigStep::READ_PHYCR) { _result.savedPhycr = value; _result.savedPhycrValid = true; }
    if (_configStep == ConfigStep::READ_CR3) { _result.savedCr3 = value; _result.savedCr3Valid = true; }
    if (_configStep == ConfigStep::READ_CR2) { _result.savedCr2 = value; _result.savedCr2Valid = true; }
    const auto compare = [&](uint16_t address, uint16_t mask, uint16_t expected,
                             uint16_t& observed, bool& valid) noexcept {
      observed = value;
      valid = true;
      if ((value & mask) != expected && !_result.mismatch.valid) {
        _result.mismatch = {true, false, 0, address, mask, expected, value};
      }
    };
    if (_configStep == ConfigStep::VERIFY_CR1) {
      compare(cmd::REG_CR1, cmd::MASK_CR1_ROBUST_MDIX,
          _settings.desired.crossover.robust ? cmd::MASK_CR1_ROBUST_MDIX : 0, _settings.rawCr1, _settings.cr1Valid);
    }
    if (_configStep == ConfigStep::VERIFY_PHYCR) {
      compare(cmd::REG_PHYCR, routingMask(_settings.desired), routingWord(_settings.desired), _settings.rawPhycr, _settings.phycrValid);
    }
    if (_configStep == ConfigStep::VERIFY_CR3) {
      compare(cmd::REG_CR3, MIRROR_MASK, _settings.desired.crossover.swapPair ? MIRROR_MASK : 0, _settings.rawCr3, _settings.cr3Valid);
    }
    if (_configStep == ConfigStep::VERIFY_CR2) {
      compare(cmd::REG_CR2, cmd::MASK_CR2_EXTENDED_DUPLEX,
          _settings.desired.extendedFullDuplex ? cmd::MASK_CR2_EXTENDED_DUPLEX : 0, _settings.rawCr2, _settings.cr2Valid);
    }
    if (_configStep == ConfigStep::VERIFY_ANAR || _configStep == ConfigStep::VERIFY_CLEAR) {
      _settings.rawAnar = value;
      _settings.anarValid = true;
      const uint16_t expected = static_cast<uint16_t>(advertisementWord(_settings.desired) |
          ((_configStep == ConfigStep::VERIFY_ANAR && _result.nextPage.requested) ? cmd::MASK_ANAR_NEXT_PAGE : 0U));
      if ((value & ANAR_VERIFY) != expected && !_result.mismatch.valid) {
        _result.mismatch = {true, false, 0, cmd::REG_ANAR, ANAR_VERIFY, expected, value};
      }
    }
    if (_configStep == ConfigStep::VERIFY_BMCR) {
      _settings.rawBmcr = value;
      _settings.bmcrValid = true;
      if ((value & BMCR_VERIFY) != modeWord(_settings.desired) && !_result.mismatch.valid) {
        _result.mismatch = {true, false, 0, cmd::REG_BMCR, BMCR_VERIFY, modeWord(_settings.desired), value};
      }
    }
  }
  if (!status.ok()) {
    return _finishOperation(status);
  }
  switch (_configStep) {
    case ConfigStep::DRAIN: _setConfigStep(ConfigStep::READ_ANAR); break;
    case ConfigStep::READ_ANAR: _setConfigStep(ConfigStep::READ_BMCR); break;
    case ConfigStep::READ_BMCR: _setConfigStep(ConfigStep::READ_CR1); break;
    case ConfigStep::READ_CR1: _setConfigStep(ConfigStep::READ_PHYCR); break;
    case ConfigStep::READ_PHYCR: _setConfigStep(ConfigStep::READ_CR3); break;
    case ConfigStep::READ_CR3: _setConfigStep(ConfigStep::READ_CR2); break;
    case ConfigStep::READ_CR2:
      // A previous forced profile requires robust routing until AN is enabled.
      // Defer disabling it regardless of the saved hardware image's trust.
      _setConfigStep(_settings.desired.crossover.robust ? ConfigStep::APPLY_CR1 : ConfigStep::APPLY_PHYCR); break;
    case ConfigStep::APPLY_CR1:
      _setConfigStep(_settings.desired.crossover.robust ? ConfigStep::APPLY_PHYCR : ConfigStep::VERIFY_ANAR); break;
    case ConfigStep::APPLY_PHYCR: _setConfigStep(ConfigStep::APPLY_CR3); break;
    case ConfigStep::APPLY_CR3: _setConfigStep(ConfigStep::APPLY_CR2); break;
    case ConfigStep::APPLY_CR2:
      _setConfigStep(_settings.desired.mode == LinkMode::AUTONEGOTIATED ? ConfigStep::APPLY_ANAR : ConfigStep::APPLY_BMCR); break;
    case ConfigStep::APPLY_ANAR:
      _result.nextPage.npMayRemainSet = _result.nextPage.requested;
      _setConfigStep(ConfigStep::APPLY_BMCR); break;
    case ConfigStep::APPLY_BMCR:
      _setConfigStep(_settings.desired.mode == LinkMode::FORCED ? ConfigStep::VERIFY_CR1 :
          (_settings.desired.crossover.robust ? ConfigStep::VERIFY_ANAR : ConfigStep::APPLY_CR1)); break;
    case ConfigStep::VERIFY_ANAR: _setConfigStep(ConfigStep::VERIFY_CR1); break;
    case ConfigStep::VERIFY_CR1: _setConfigStep(ConfigStep::VERIFY_PHYCR); break;
    case ConfigStep::VERIFY_PHYCR: _setConfigStep(ConfigStep::VERIFY_CR3); break;
    case ConfigStep::VERIFY_CR3: _setConfigStep(ConfigStep::VERIFY_CR2); break;
    case ConfigStep::VERIFY_CR2: _setConfigStep(ConfigStep::VERIFY_BMCR); break;
    case ConfigStep::VERIFY_BMCR:
      _observeConfiguration();
      if (_result.mismatch.valid) {
        _settings.lastVerificationStatus = mismatchStatus();
        return _finishOperation(mismatchStatus(), !_settings.observedStale);
      }
      if (_settings.desired.mode == LinkMode::AUTONEGOTIATED) {
        if ((value & cmd::MASK_BMCR_AN_RESTART) != 0U) { return _waitOperation(); }
      }
      if (_result.nextPage.requested) { _setConfigStep(ConfigStep::WAIT_BASE); break; }
      if (_configureOptions.waitForCompletion) { _setConfigStep(ConfigStep::WAIT_COMPLETE); break; }
      break;
    case ConfigStep::WAIT_BASE:
      if (!_pageEventPending) { return _waitOperation(); }
      _setConfigStep(ConfigStep::READ_BASE); break;
    case ConfigStep::READ_BASE:
      _pageEventPending = false;
      _partnerToggle = (value & cmd::MASK_ANNPTR_TOGGLE) != 0U;
      if ((_pageEventRaw & BASE_CAPABILITIES) != BASE_CAPABILITIES || (value & cmd::MASK_ANAR_NEXT_PAGE) == 0U) {
        _result.nextPage.partnerRefused = true;
        _setConfigStep(ConfigStep::WAIT_COMPLETE);
      } else { _setConfigStep(ConfigStep::LOAD_PAGE); }
      break;
    case ConfigStep::LOAD_PAGE:
      _localMore = _pageIndex + 1U < _configureOptions.nextPages.pageCount;
      _setConfigStep(ConfigStep::WAIT_PAGE); break;
    case ConfigStep::WAIT_PAGE:
      if (!_pageEventPending) { return _waitOperation(); }
      _setConfigStep(ConfigStep::READ_PAGE); break;
    case ConfigStep::READ_PAGE: {
      _pageEventPending = false;
      const bool toggle = (value & cmd::MASK_ANNPTR_TOGGLE) != 0U;
      const bool duplicate = toggle == _partnerToggle;
      if (duplicate) {
        if (_result.nextPage.duplicatePages != std::numeric_limits<uint32_t>::max()) { ++_result.nextPage.duplicatePages; }
      } else {
        _partnerToggle = toggle;
        _partnerMore = (value & cmd::MASK_ANAR_NEXT_PAGE) != 0U;
        if (!_localMore && !_partnerMore) {
          _setConfigStep(ConfigStep::WAIT_COMPLETE);
          break;
        }
        if (_localMore) { ++_pageIndex; }
        else { _pageIndex = _configureOptions.nextPages.pageCount; }
      }
      if (_result.nextPage.receivedPages >= _configureOptions.nextPages.maxPartnerPages) {
        return _finishOperation(configTimeout(TimeoutDetail::FRAME_BUDGET));
      }
      _setConfigStep(duplicate ? ConfigStep::WAIT_PAGE : ConfigStep::LOAD_PAGE);
      break;
    }
    case ConfigStep::WAIT_COMPLETE:
      if ((value & cmd::MASK_BMSR_AN_COMPLETE) == 0U) { return _waitOperation(); }
      _result.negotiationComplete = true;
      _result.nextPage.complete = _result.nextPage.requested && !_result.nextPage.partnerRefused;
      if (_result.nextPage.requested) { _setConfigStep(ConfigStep::CLEAR_NP); break; }
      break;
    case ConfigStep::CLEAR_NP: _setConfigStep(ConfigStep::VERIFY_CLEAR); break;
    case ConfigStep::VERIFY_CLEAR:
      _observeConfiguration();
      if (_result.mismatch.valid) {
        _settings.lastVerificationStatus = mismatchStatus();
        return _finishOperation(mismatchStatus(), !_settings.observedStale);
      }
      _result.nextPage.npMayRemainSet = false;
      break;
  }
  const bool finished = (completedStep == ConfigStep::VERIFY_BMCR && !_result.nextPage.requested &&
                         !_configureOptions.waitForCompletion && (_settings.desired.mode == LinkMode::FORCED ||
                         (value & cmd::MASK_BMCR_AN_RESTART) == 0U)) ||
      (completedStep == ConfigStep::WAIT_COMPLETE && !_result.nextPage.requested && (value & cmd::MASK_BMSR_AN_COMPLETE) != 0U) ||
      (completedStep == ConfigStep::VERIFY_CLEAR && batch.frames[0].registerAddress == cmd::REG_ANAR &&
       batch.frameCount == 1U && !_result.nextPage.npMayRemainSet);
  if (!finished) { return _operation.status; }
  _settings.trust = ConfigurationTrust::VERIFIED;
  _settings.lastVerificationStatus = Status::Ok();
  _result.persistentRestored = true;
  _initialized = true;
  _configuring = false;
  return _finishOperation(Status::Ok());
}

} // namespace DP8382x
