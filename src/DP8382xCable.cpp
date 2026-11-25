/// @file DP8382xCable.cpp
/// @brief Manual DP83826I TDR, using the shared bounded operation/restore owner.
#include <DP8382x/DP8382x.h>
#include <cmath>

namespace DP8382x {
namespace {
// SNLS647I tables 8-54/56..60; GENCFG 0600 is the named SNLA428 exception.
constexpr uint16_t addresses[] = {0x0170,0x0173,0x0175,0x0176,0x0177,0x0178,0x0456};
constexpr uint16_t masks[] = {0x7770,0xFFFF,0x380F,0x001F,0x1F00,0x0007,0x0600};
constexpr uint8_t recipeFields[] = {0,1,2,5,6};
// Independently transcribed data from SNLA428 table 2-1, not its sample code.
constexpr uint16_t recipeWords[][6] = {
  {0x5C12,0x5C12,0x5C22,0x5E32,0x5E42,0x5E52},
  {0x0D00,0x0D0C,0x0D13,0x1A20,0x343A,0x8F6E},
  {0x1007,0x1007,0x1007,0x1007,0x100A,0x100D},
  {0x0002,0x0002,0x0002,0x0002,0x0002,0x0006},
  {0x1FA0,0x1FA0,0x1FA0,0x1FA0,0x1FA0,0x1F90},
  {0x0813,0x0813,0x0813,0x0815,0x0816,0x0816}
};
Status invalid(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG, message, static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
}
Status busyCable(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY, "Cable operation owner unavailable", static_cast<int32_t>(detail));
}
MdioBatch direct(uint8_t address, FrameKind kind = FrameKind::READ, uint16_t value = 0) noexcept {
  MdioBatch batch; batch.frameCount = 1; batch.frames[0] = {kind,address,value,0,NO_READ_INDEX}; return batch;
}
bool sound(const CableWord& word) noexcept { return word.valid && !word.stale; }
Status nonzeroBudget(const OperationBudget& budget) noexcept {
  if (budget.io.maxFrames == 0U || budget.io.deadlineUs == 0U) {
    return Status::Error(Err::OPERATION_TIMEOUT, "Empty cable budget", static_cast<int32_t>(
        budget.io.maxFrames == 0U ? TimeoutDetail::FRAME_BUDGET : TimeoutDetail::DEADLINE));
  }
  return Status::Ok();
}
} // namespace

Status validateCableSettings(const CableSettings& settings) noexcept {
  const auto& c = settings.controls;
  if (settings.recipe > CableRecipe::RAW || c.channel > CableChannel::B_TO_B) {
    return Status::Error(Err::INVALID_PARAM, "Invalid cable enum", static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }
  if (!settings.maintenanceAdmitted || !settings.peerQuiet || !settings.allowHardResetExit ||
      settings.runMask == 0U || (settings.runMask & 0xC0U) != 0U ||
      (settings.recipe == CableRecipe::RAW && settings.runMask != 1U) ||
      c.averaging > 6U || c.segment < 1U || c.segment > 5U || c.shadowAverage > 7U ||
      c.forwardShadow > 15U || c.locationThreshold > 31U || c.shortThreshold > 31U || c.pulseWidth > 7U) {
    return invalid("Invalid cable controls or maintenance/peer/HARD admission");
  }
  return Status::Ok();
}

Status interpretCable(const CableResult& result, double delay, CableDiagnosis& diagnosis) noexcept {
  if (!std::isfinite(delay) || delay <= 0.0) { return Status::Error(Err::INVALID_PARAM, "Positive finite propagation delay required"); }
  if (result.autorun || result.recipe != CableRecipe::SNLA428) { return invalid("Derived distance requires the SNLA428 transmit-A recipe"); }
  if (!result.complete || result.stale || result.requestedMask == 0U || (result.requestedMask & 0xC0U) != 0U) {
    return Status::Error(Err::RESULT_NOT_AVAILABLE, "Complete fresh cable samples required");
  }
  // Validate the whole selected plan before publishing any ordinary output.
  for (uint8_t i = 0; i < 6U; ++i) {
    if ((result.requestedMask & (1U << i)) == 0U) { continue; }
    const auto& sample = result.samples[i];
    if (sample.state != CableRunState::COMPLETE || !sample.triggerAccepted || !sample.freshnessObserved || sample.failed ||
        !sound(sample.cdcr) || !sound(sample.index) || !sound(sample.amplitude) || !sound(sample.peak) ||
        (sample.cdcr.raw & 0xC003U) != 2U) {
      return Status::Error(Err::RESULT_NOT_AVAILABLE, "Uncorrelated, failed or incomplete cable row");
    }
  }
  CableDiagnosis staged;
  constexpr uint8_t scan[] = {2,3,4,5,1,0};
  for (const auto i : scan) {
    if ((result.requestedMask & (1U << i)) == 0U) { continue; }
    const auto& sample = result.samples[i];
    const auto amplitude = sample.amplitude.raw & 0x007FU;
    if (amplitude <= (i < 2U ? 24U : 10U)) { continue; }
    const int index = static_cast<int>(sample.index.raw & 0x00FFU);
    staged.metres = std::abs(static_cast<double>(index - 5) * 4.0 / delay);
    if (!std::isfinite(staged.metres)) { return Status::Error(Err::INVALID_PARAM, "Propagation delay overflows distance"); }
    staged.run = static_cast<CableRun>(i);
    staged.conclusion = sample.peak.raw == 0U ? CableConclusion::OPEN : CableConclusion::SHORT;
    staged.distanceValid = true;
    staged.amplitudeAtLimit = amplitude == 0x7FU;
    staged.rawBit11 = (sample.peak.raw & 0x0800U) != 0U;
    staged.rawBit5 = (sample.peak.raw & 0x0020U) != 0U;
    break;
  }
  diagnosis = staged;
  return Status::Ok();
}

Status DP8382x::_admitCable(const OperationBudget& budget, bool stop) const noexcept {
  if (_config.variant == Variant::DP83825I) { return _unsupportedFeature(); }
  if (budget.pollIntervalUs == 0U || budget.maxStalledPolls == 0U) {
    return Status::Error(Err::INVALID_PARAM, "Invalid cable polling budget", static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return busyCable(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state == OperationState::RESULT_PENDING) { return busyCable(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return Status::Error(Err::INVALID_CONFIG, "Clock contract failed", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)); }
  if (stop) {
    if (!_cableSession || !_operation.temporaryMayBeActive) { return invalid("No retained cable effects to restore"); }
  }
  return Status::Ok();
}

Status DP8382x::startCableTest(const CableSettings& settings, const OperationBudget& budget, OperationToken& token) noexcept {
  if (_interface.desiredValid && _interface.desired.isolationOwned && _interface.desired.isolate) {
    return invalid("Release owned MAC isolation before manual cable capture");
  }
  const auto validation = validateCableSettings(settings); if (!validation.ok()) { return validation; }
  const auto admission = _admitCable(budget, false); if (!admission.ok()) { return admission; }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return busyCable(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED, "Fresh initialized identity required"); }
  if (!_settings.desiredValid || _settings.trust != ConfigurationTrust::VERIFIED) { return invalid("Verified persistent profile required"); }
  if (_cableCalibration) { return invalid("Known calibration requires clear-then-HARD recovery"); }
  const auto budgetStatus = nonzeroBudget(budget); if (!budgetStatus.ok()) { return budgetStatus; }
  if (_eee.desiredValid && _eee.desired.enabled) { return invalid("Disable EEE before manual cable isolation"); }
  _beginOperation(OperationKind::CABLE_START, budget);
  _cable = {}; _cable.settings = settings; _cable.settingsValid = true;
  _cable.result.session = _operation.token;
  _cable.result.recipe = settings.recipe; _cable.result.requestedMask = settings.runMask;
  _cableSession = _cableRestoreTail = true; _cableCurrent = false;
  _cableRun = _cableField = 0;
  _cableSaved = {}; _cableSavedBmcr = 0;
  _setCableStep(CableStep::IDENTIFY);
  token = _operation.token; return _operation.status;
}
Status DP8382x::startCableAcquire(const OperationBudget& budget, OperationToken& token) noexcept {
  const auto admission = _admitCable(budget, false); if (!admission.ok()) { return admission; }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED, "Fresh initialized identity required"); }
  if (!_cableSession || !_cableCurrent || !_cable.result.samples[_cableRun].triggerAccepted) {
    return Status::Error(Err::RESULT_NOT_AVAILABLE, "No current accepted cable trigger");
  }
  const auto budgetStatus = nonzeroBudget(budget); if (!budgetStatus.ok()) { return budgetStatus; }
  _beginOperation(OperationKind::CABLE_ACQUIRE, budget);
  auto& sample = _cable.result.samples[_cableRun];
  sample.state = CableRunState::STARTED;
  sample.index.stale = sample.amplitude.stale = sample.peak.stale = true;
  _cable.result.complete = false; _cable.result.stale = true;
  _setCableStep(CableStep::WAIT);
  token = _operation.token; return _operation.status;
}
Status DP8382x::stopCableTest(const OperationBudget& budget, OperationToken& token) noexcept {
  const auto admission = _admitCable(budget, true); if (!admission.ok()) { return admission; }
  const auto budgetStatus = nonzeroBudget(budget); if (!budgetStatus.ok()) { return budgetStatus; }
  _beginOperation(OperationKind::CABLE_STOP, budget);
  _cableRestoreTail = true;
  _setCableStep(CableStep::STOP_IDENTITY);
  token = _operation.token; return _operation.status;
}
CableSnapshot DP8382x::cableSnapshot() const noexcept {
  auto snapshot = _cable; snapshot.temporaryMayBeActive = _cableSession && _operation.temporaryMayBeActive; return snapshot;
}
Status DP8382x::startConfigureCableAutorun(const CableAutorunOptions& options,
    const OperationBudget& budget, OperationToken& token) noexcept {
  if (!options.maintenanceAdmitted) { return invalid("Autorun policy requires maintenance admission"); }
  const auto admission = _admitCable(budget, false); if (!admission.ok()) { return admission; }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return busyCable(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED, "Initialize before autorun policy"); }
  if (_settings.desiredValid && _settings.trust != ConfigurationTrust::VERIFIED) { return invalid("Verify persistent profile before autorun policy"); }
  if (_cableCalibration) { return invalid("Known calibration requires clear-then-HARD recovery"); }
  const auto available = nonzeroBudget(budget); if (!available.ok()) { return available; }
  _beginOperation(OperationKind::CABLE_AUTORUN_CONFIGURE, budget);
  _cable = {}; _cable.result.recipe = CableRecipe::RAW; _cableRun = 0;
  _autorunTailPending = false;
  _autorun.desired = options.enabled; _autorun.desiredValid = true;
  _autorun.trust = ConfigurationTrust::DIRTY; _autorun.observedStale = true;
  _setCableStep(CableStep::IDENTIFY);
  token = _operation.token; return _operation.status;
}
Status DP8382x::startCableAutorunAcquire(const OperationBudget& budget, OperationToken& token) noexcept {
  const auto admission = _admitCable(budget, false); if (!admission.ok()) { return admission; }
  if (_operation.temporaryMayBeActive) { return busyCable(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED, "Initialize before autorun observation"); }
  const auto available = nonzeroBudget(budget); if (!available.ok()) { return available; }
  _beginOperation(OperationKind::CABLE_AUTORUN_ACQUIRE, budget);
  _cable = {}; _cable.settings.recipe = CableRecipe::RAW; _cable.settings.runMask = 1;
  _cable.result.session = _operation.token; _cable.result.recipe = CableRecipe::RAW;
  _cable.result.requestedMask = 1; _cable.result.autorun = true; _cableRun = 0;
  _setCableStep(CableStep::CDCR);
  token = _operation.token; return _operation.status;
}
void DP8382x::_setCableStep(CableStep step) noexcept {
  _cableWorking = true; _configuring = _loopWorking = _bistWorking = false;
  _cableStep = step; _operation.nextPollValid = false;
  _operation.phase = step >= CableStep::STOP_IDENTITY ? OperationPhase::RESTORE : OperationPhase::VERIFY;
  if (step == CableStep::IDENTIFY || step == CableStep::STOP_IDENTITY) { _operation.phase = OperationPhase::IDENTIFY; }
  if (step == CableStep::WAIT) { _operation.phase = OperationPhase::WAIT; }
}
uint16_t DP8382x::_cableValue(uint8_t field) const noexcept {
  if (_cable.settings.recipe == CableRecipe::SNLA428) {
    if (field <= 2U) { return recipeWords[field][_cableRun]; }
    if (field == 5U) { return recipeWords[3][_cableRun]; }
    if (field == 6U) { return 0x0600; }
  }
  const auto& c = _cable.settings.controls;
  switch (field) {
    case 0: return static_cast<uint16_t>((static_cast<uint32_t>(c.channel) << 13U) | (c.dcNoInit ? 0x1000U : 0U) |
                                        (uint32_t{c.averaging} << 8U) | (uint32_t{c.segment} << 4U));
    case 1: return static_cast<uint16_t>((uint32_t{c.duration} << 8U) | c.initialSkip);
    case 2: return static_cast<uint16_t>((uint32_t{c.shadowAverage} << 11U) | c.forwardShadow);
    case 3: return c.locationThreshold;
    case 4: return static_cast<uint16_t>(uint32_t{c.shortThreshold} << 8U);
    case 5: return c.pulseWidth;
    default: return 0;
  }
}
MdioBatch DP8382x::_cableBatch() const noexcept {
  const auto modify = [this](uint16_t address, uint16_t mask, uint16_t value) {
    auto batch = _mmdBatch(address, FrameKind::READ, 0);
    batch.frameCount = 5;
    batch.frames[4] = {FrameKind::WRITE,0x0E,value,static_cast<uint16_t>(~mask),3}; return batch;
  };
  switch (_cableStep) {
    case CableStep::IDENTIFY: case CableStep::STOP_IDENTITY: {
      auto batch = direct(2); batch.frameCount = 2; batch.frames[1] = {FrameKind::READ,3,0,0,NO_READ_INDEX}; return batch;
    }
    case CableStep::BMCR: case CableStep::VERIFY_BMCR: return direct(0);
    case CableStep::PHYRCR: return direct(0x1F);
    case CableStep::CR1: return direct(9);
    case CableStep::BISCR: return direct(0x16);
    case CableStep::LINK: return direct(0x10);
    case CableStep::CDCR: case CableStep::WAIT: case CableStep::BOOKEND:
    case CableStep::STOP_STATUS: case CableStep::STOP_VERIFY: return direct(0x1E);
    case CableStep::SAVE: case CableStep::VERIFY_RESTORE: return _mmdBatch(addresses[_cableField],FrameKind::READ,0);
    case CableStep::APPLY:
      if (_cable.settings.recipe == CableRecipe::RAW) { return modify(addresses[_cableField],masks[_cableField],_cableValue(_cableField)); }
      if (_cableField < 4U) { const auto field = recipeFields[_cableField]; return _mmdBatch(addresses[field],FrameKind::WRITE,_cableValue(field)); }
      if (_cableField < 6U) { return _mmdBatch(_cableField == 4U ? 0x0416 : 0x0411,FrameKind::WRITE,recipeWords[_cableField][_cableRun]); }
      return modify(0x0456,0x0600,0x0600);
    case CableStep::VERIFY: {
      const auto field = _cable.settings.recipe == CableRecipe::RAW ? _cableField : recipeFields[_cableField];
      return _mmdBatch(addresses[field],FrameKind::READ,0);
    }
    // D3's complete command literal is confined to that recipe. Ordinary
    // A commands leave RH completion/failure bits [1:0] neutral.
    case CableStep::TRIGGER: return direct(0x1E,FrameKind::WRITE,_cable.settings.recipe == CableRecipe::SNLA428 ?
        uint16_t{0x8102} : static_cast<uint16_t>((_cable.result.prerequisiteCdcr.raw & (isA(_config.variant)?0x3FFCU:0x3FFFU)) | 0x8000U));
    case CableStep::INDEX: return _mmdBatch(0x0180,FrameKind::READ,0);
    case CableStep::AMPLITUDE: return _mmdBatch(0x0185,FrameKind::READ,0);
    case CableStep::PEAK: return _mmdBatch(0x018A,FrameKind::READ,0);
    case CableStep::STOP_COMMAND: return direct(0x1E,FrameKind::WRITE,static_cast<uint16_t>(_cable.result.prerequisiteCdcr.raw & (isA(_config.variant)?0x3FFCU:0x3FFFU)));
    case CableStep::RESTORE_BMCR: {
      auto batch = direct(0); batch.frameCount = 2;
      batch.frames[1] = {FrameKind::WRITE,0,static_cast<uint16_t>((_cableSavedBmcr & 0x0480U) | _softwarePowerWord()),0x317F,0}; return batch;
    }
    case CableStep::RESTORE: return modify(addresses[_cableField],masks[_cableField],static_cast<uint16_t>(_cableSaved[_cableField] & masks[_cableField]));
    case CableStep::POLICY_WRITE: {
      auto batch = direct(9); batch.frameCount = 2;
      batch.frames[1] = {FrameKind::WRITE,9,static_cast<uint16_t>(_autorun.desired ? 0x0100 : 0),0xFEFF,0};
      return batch;
    }
    case CableStep::POLICY_VERIFY: return direct(9);
  }
  return {};
}

Status DP8382x::_advanceCable(Status status, bool called, const MdioBatch& batch) noexcept {
  auto& capture = _cable.result;
  auto& sample = capture.samples[_cableRun];
  const auto completed = _result.failingBatch.completedFrames;
  const uint8_t last = static_cast<uint8_t>(batch.frameCount - 1U);
  const auto value = _transport.lastBatch.frames[last].value;
  bool resetActive = false;
  CableWord* word = nullptr;
  switch (_cableStep) {
    case CableStep::CDCR: case CableStep::STOP_STATUS: case CableStep::STOP_VERIFY: word = &capture.prerequisiteCdcr; break;
    case CableStep::WAIT: case CableStep::BOOKEND: word = &sample.cdcr; break;
    case CableStep::INDEX: word = &sample.index; break;
    case CableStep::AMPLITUDE: word = &sample.amplitude; break;
    case CableStep::PEAK: word = &sample.peak; break;
    default: break;
  }
  if (called) {
    // The RMW's sampled BMCR can reveal reset even if its neutral write clears
    // that bit or fails. Preserve this observation before handling primary errors.
    if (_cableStep == CableStep::RESTORE_BMCR && completed != 0U &&
        (_transport.lastBatch.frames[0].value & 0x8000U) != 0U) {
      resetActive = true;
      _invalidateResetEvidence(true);
    }
    if (word != nullptr) {
      word->stale = true;
      if (completed == batch.frameCount) {
        word->raw = value; word->valid = true; word->stale = false;
        word->timeValid = !_transport.clockInvalid;
        word->observedAtUs = word->timeValid ? _lastTimeUs + _config.transport.bounds.clockLagUs : 0;
        if (batch.frames[last].registerAddress == 0x1EU) {
          if ((value & 0x4000U) != 0U) { _cableCalibration = true; }
          // A raw zero cannot prove required recovery. Only the owning stop may
          // accept clear here: its temporary-state barrier still mandates HARD.
          else if (_cableStep == CableStep::STOP_STATUS) { _cableCalibration = false; }
        }
      }
    }
    if (batch.frames[last].kind == FrameKind::WRITE) {
      const auto& receipt = _transport.lastBatch.frames[last];
      const bool uncertain = _transport.violation != TransportViolation::NONE;
      const bool attempted = receipt.attempted || receipt.completed || receipt.effectPossible || receipt.valueValid || uncertain;
      _result.mutationAttempted = _result.mutationAttempted || receipt.attempted;
      _result.mutationCompleted = _result.mutationCompleted || completed == batch.frameCount;
      _result.mutationPossible = _result.mutationPossible || receipt.effectPossible || completed == batch.frameCount || uncertain;
      if (attempted && _cableStep != CableStep::POLICY_WRITE) {
        _operation.temporaryMayBeActive = _operation.cleanupOwed = true;
        _cleanupOwner = OperationKind::CABLE_START;
      }
      if (_cableStep == CableStep::TRIGGER) {
        sample.state = CableRunState::STARTED;
        sample.triggerAccepted = completed == batch.frameCount;
        sample.triggerPossible = attempted;
        _cableCurrent = sample.triggerAccepted;
      }
    }
    if (completed == batch.frameCount && batch.frames[last].kind == FrameKind::READ) {
      if (_cableStep == CableStep::POLICY_VERIFY) {
        _autorun.observed = (value & 0x0100U) != 0U; _autorun.observedValid = true;
        _autorun.timeValid = !_transport.clockInvalid;
        _autorun.observedAtUs = _autorun.timeValid ? _lastTimeUs + _config.transport.bounds.clockLagUs : 0;
        if (_autorun.observed != _autorun.desired && !_result.mismatch.valid) {
          _result.mismatch = {true,false,0,9,0x0100,static_cast<uint16_t>(_autorun.desired ? 0x0100 : 0),value};
        }
      }
      if (_cableStep == CableStep::SAVE) { _cableSaved[_cableField] = value; }
      if (_cableStep == CableStep::BMCR) { _cableSavedBmcr = value; }
      resetActive = ((_cableStep == CableStep::BMCR || _cableStep == CableStep::VERIFY_BMCR) && (value & 0x8000U) != 0U) ||
          (_cableStep == CableStep::PHYRCR && (value & 0xC000U) != 0U);
      if (resetActive) { _invalidateResetEvidence((value & 0x8000U) != 0U); }
      if (_cableStep == CableStep::WAIT && sample.triggerAccepted) {
        sample.freshnessObserved = sample.freshnessObserved || (value & 0x8000U) != 0U || (value & 2U) == 0U;
        sample.failed = (value & 1U) != 0U;
      }
      uint16_t mask = 0, expected = 0;
      if (_cableStep == CableStep::VERIFY || _cableStep == CableStep::VERIFY_RESTORE) {
        const auto field = _cableStep == CableStep::VERIFY && _cable.settings.recipe == CableRecipe::SNLA428 ? recipeFields[_cableField] : _cableField;
        mask = masks[field]; expected = static_cast<uint16_t>((_cableStep == CableStep::VERIFY ? _cableValue(field) : _cableSaved[field]) & mask);
      }
      if (_cableStep == CableStep::VERIFY_BMCR) {
        mask = 0x7D80;
        expected = static_cast<uint16_t>((_cableSavedBmcr & 0x0480U) | _softwarePowerWord() |
            (_settings.desired.mode == LinkMode::AUTONEGOTIATED ? 0x1000U :
             ((_settings.desired.forced.speed == LinkSpeed::MBPS_100 ? 0x2000U : 0U) |
              (_settings.desired.forced.duplex == Duplex::FULL ? 0x0100U : 0U))));
      }
      if (mask != 0U && (value & mask) != expected && !_result.mismatch.valid) {
        _result.mismatch = {true,batch.frameCount > 1U,31,batch.frameCount > 1U ? addresses[
            _cableStep == CableStep::VERIFY && _cable.settings.recipe == CableRecipe::SNLA428 ? recipeFields[_cableField] : _cableField] : uint16_t{0},mask,expected,value};
      }
    }
  }
  if (!status.ok()) { return _finishOperation(status); }
  if (resetActive) { return _finishOperation(invalid("Unexpected reset during cable operation")); }
  if (_result.mismatch.valid) { return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Cable owned-field readback mismatch")); }
  const bool recipe = _cable.settings.recipe == CableRecipe::SNLA428;
  switch (_cableStep) {
    case CableStep::IDENTIFY:
      if (!_initialized) { return _finishOperation(invalid("Identity changed since initialization")); }
      _setCableStep(CableStep::BMCR); break;
    case CableStep::BMCR:
      if ((value & 0xCC80U) != 0U) { return _finishOperation(invalid("Reset, power, isolation or loopback active")); }
      _setCableStep(CableStep::PHYRCR); break;
    case CableStep::PHYRCR:
      if ((value & 0xC000U) != 0U) { return _finishOperation(invalid("Reset or restart active")); }
      _setCableStep(CableStep::CR1); break;
    case CableStep::CR1:
      if (_operation.kind != OperationKind::CABLE_AUTORUN_CONFIGURE && (value & 0x0100U) != 0U) { return _finishOperation(invalid("Disable autorun through its owner before manual cable test")); }
      _setCableStep(CableStep::BISCR); break;
    case CableStep::BISCR:
      if ((value & 0x325FU) != 0U) { return _finishOperation(invalid("BIST or loopback active")); }
      _setCableStep(_operation.kind == OperationKind::CABLE_AUTORUN_CONFIGURE ? CableStep::CDCR : CableStep::LINK); break;
    case CableStep::LINK:
      if ((value & 1U) != 0U) { return _finishOperation(invalid("Manual cable capture requires live link down")); }
      _setCableStep(CableStep::CDCR); break;
    case CableStep::CDCR:
      if (_operation.kind == OperationKind::CABLE_AUTORUN_ACQUIRE) {
        // One raw observation, even with busy/initial done. No trigger correlation.
        _setCableStep(CableStep::INDEX); break;
      }
      if ((value & 0xC000U) != 0U) { return _finishOperation(invalid("CDCR calibration or manual command already active")); }
      if (_operation.kind == OperationKind::CABLE_AUTORUN_CONFIGURE) { _setCableStep(CableStep::POLICY_WRITE); break; }
      _cableField = 0; _setCableStep(CableStep::SAVE); break;
    case CableStep::SAVE:
      if (_cableField == 0U && (((value & 0x0700U) == 0x0700U) ||
          (value & 0x0070U) == 0U || (value & 0x0070U) > 0x0050U)) {
        return _finishOperation(invalid("Saved cable controls contain prohibited encodings"));
      }
      if (++_cableField < (recipe ? 7U : 6U)) { break; }
      while ((capture.requestedMask & (1U << _cableRun)) == 0U) { ++_cableRun; }
      _cableField = 0; _setCableStep(CableStep::APPLY); break;
    case CableStep::APPLY:
      if (++_cableField < (recipe ? 7U : 6U)) { break; }
      _cableField = 0; _setCableStep(CableStep::VERIFY); break;
    case CableStep::VERIFY:
      if (++_cableField < (recipe ? 5U : 6U)) { break; }
      _setCableStep(CableStep::TRIGGER); break;
    case CableStep::TRIGGER:
      _setCableStep(CableStep::WAIT); return _waitOperation();
    case CableStep::WAIT:
      if ((value & 0x4000U) != 0U) { return _finishOperation(invalid("CDCR calibration requires separate recovery")); }
      if ((value & 0x8002U) != 2U || !sample.freshnessObserved) { return _waitOperation(); }
      _setCableStep(CableStep::INDEX); break;
    case CableStep::INDEX: _setCableStep(CableStep::AMPLITUDE); break;
    case CableStep::AMPLITUDE: _setCableStep(CableStep::PEAK); break;
    case CableStep::PEAK: _setCableStep(CableStep::BOOKEND); break;
    case CableStep::BOOKEND: {
      if (_operation.kind == OperationKind::CABLE_AUTORUN_ACQUIRE) {
        capture.statusChanged = value != capture.prerequisiteCdcr.raw;
        sample.failed = (value & 1U) != 0U;
        sample.state = CableRunState::COMPLETE;
        capture.complete = true; // Complete software read plan, not a silicon snapshot.
        capture.stale = true;
        return _finishOperation(Status::Ok());
      }
      if ((value & 0xC003U) != (sample.failed ? 3U : 2U)) {
        sample.index.stale = sample.amplitude.stale = sample.peak.stale = true;
        _cableCurrent = false;
        return _finishOperation(invalid("Cable status changed during capture"));
      }
      sample.state = CableRunState::COMPLETE;
      if (_operation.kind == OperationKind::CABLE_START) {
        uint8_t next = static_cast<uint8_t>(_cableRun + 1U);
        while (next < 6U && (capture.requestedMask & (1U << next)) == 0U) { ++next; }
        if (next < 6U) { _cableCurrent = false; _cableRun = next; _cableField = 0; _setCableStep(CableStep::APPLY); break; }
      }
      capture.complete = true;
      for (uint8_t i = 0; i < 6U; ++i) {
        if ((capture.requestedMask & (1U << i)) != 0U && capture.samples[i].state != CableRunState::COMPLETE) { capture.complete = false; }
      }
      capture.stale = !capture.complete;
      return _finishOperation(Status::Ok());
    }
    case CableStep::STOP_IDENTITY: _setCableStep(CableStep::STOP_STATUS); break;
    case CableStep::STOP_STATUS:
      if ((value & 0x4000U) != 0U) { return _finishOperation(invalid("CDCR calibration clear requires prompt35 recovery")); }
      _setCableStep(CableStep::STOP_COMMAND); break;
    case CableStep::STOP_COMMAND: _setCableStep(CableStep::STOP_VERIFY); break;
    case CableStep::STOP_VERIFY:
      if ((value & 0xC000U) != 0U) { return _finishOperation(invalid("Manual stop control could not be verified")); }
      _cableWorking = false; _cableCurrent = false;
      _result.reset = ResetKind::HARD; _result.commandRequired = true;
      _setInitStep(InitStep::IDENTIFY); break;
    case CableStep::RESTORE_BMCR: _setCableStep(CableStep::VERIFY_BMCR); break;
    case CableStep::VERIFY_BMCR: _cableField = 0; _setCableStep(CableStep::RESTORE); break;
    case CableStep::RESTORE: _setCableStep(CableStep::VERIFY_RESTORE); break;
    case CableStep::VERIFY_RESTORE:
      if (++_cableField < (recipe ? 7U : 6U)) { _setCableStep(CableStep::RESTORE); break; }
      _cableRestoreTail = false; capture.stopped = true;
      _result.persistentRestored = true; _settings.trust = ConfigurationTrust::VERIFIED; _initialized = true;
      return _finishOperation(Status::Ok());
    case CableStep::POLICY_WRITE: _setCableStep(CableStep::POLICY_VERIFY); break;
    case CableStep::POLICY_VERIFY:
      _autorun.trust = ConfigurationTrust::VERIFIED; _autorun.observedStale = false;
      _result.persistentRestored = true;
      if (_operation.kind != OperationKind::VERIFY) { _initialized = true; }
      return _finishOperation(Status::Ok());
  }
  return _operation.status;
}
} // namespace DP8382x
