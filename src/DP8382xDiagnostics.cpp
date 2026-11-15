/// @file DP8382xDiagnostics.cpp
/// @brief Exact loopback sessions using the existing operation and restoration owner.
#include <DP8382x/DP8382x.h>
#include <DP8382x/CommandTable.h>

namespace DP8382x {
namespace {
Status rejected(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG, message, static_cast<int32_t>(ConfigDetail::LINK_COMBINATION));
}
Status busyLoop(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY, "Loopback owner unavailable", static_cast<int32_t>(detail));
}
MdioBatch readWord(uint8_t address) noexcept {
  MdioBatch batch; batch.frameCount = 1;
  batch.frames[0] = {FrameKind::READ, address, 0, 0, NO_READ_INDEX};
  return batch;
}
MdioBatch writeWord(uint8_t address, uint16_t value) noexcept {
  auto batch = readWord(address); batch.frames[0].kind = FrameKind::WRITE; batch.frames[0].value = value;
  return batch;
}
MdioBatch updateWord(uint8_t address, uint16_t preserve, uint16_t value) noexcept {
  auto batch = readWord(address); batch.frameCount = 2;
  batch.frames[1] = {FrameKind::WRITE, address, value, preserve, 0};
  return batch;
}
bool nearEnd(LoopbackMode mode) noexcept {
  return mode != LoopbackMode::REVERSE && mode != LoopbackMode::EXTERNAL;
}
uint16_t selector(const LoopbackOptions& options) noexcept {
  switch (options.mode) {
    case LoopbackMode::MII: return options.speed == LinkSpeed::MBPS_10 ? 0x0001 : 0x0004;
    case LoopbackMode::PCS_INPUT: return 0x0001;
    case LoopbackMode::PCS_OUTPUT: return 0x0002;
    case LoopbackMode::DIGITAL: return 0x0004;
    case LoopbackMode::ANALOG: return 0x0008;
    case LoopbackMode::REVERSE: return 0x0010;
    case LoopbackMode::EXTERNAL: return 0;
  }
  return 0;
}
Status validBudget(const OperationBudget& budget) noexcept {
  if (budget.pollIntervalUs == 0U || budget.maxStalledPolls == 0U) {
    return Status::Error(Err::INVALID_PARAM, "Invalid polling budget", static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  return Status::Ok();
}
Status nonzeroBudget(const OperationBudget& budget) noexcept {
  if (budget.io.maxFrames == 0U || budget.io.deadlineUs == 0U) {
    return Status::Error(Err::OPERATION_TIMEOUT, "Empty loopback budget", static_cast<int32_t>(
        budget.io.maxFrames == 0U ? TimeoutDetail::FRAME_BUDGET : TimeoutDetail::DEADLINE));
  }
  return Status::Ok();
}
} // namespace

DP8382x::Traffic DP8382x::_operationTraffic() const noexcept {
  return _operation.kind == OperationKind::LOOPBACK_START || _operation.kind == OperationKind::LOOPBACK_STOP ?
      Traffic::RAW : Traffic::MANAGED;
}
const LinkSettings& DP8382x::_configurationTarget() const noexcept {
  return _operation.kind == OperationKind::LOOPBACK_START ? _loopProfile : _settings.desired;
}
LoopbackSnapshot DP8382x::loopbackSnapshot() const noexcept {
  auto snapshot = _loopback;
  snapshot.temporaryMayBeActive = _operation.temporaryMayBeActive;
  return snapshot;
}

Status DP8382x::startLoopback(const LoopbackOptions& options, const OperationBudget& budget,
                             OperationToken& token) noexcept {
  if (options.mode > LoopbackMode::EXTERNAL ||
      (options.speed != LinkSpeed::MBPS_10 && options.speed != LinkSpeed::MBPS_100)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid loopback enum", static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }
  const auto polling = validBudget(budget); if (!polling.ok()) { return polling; }
  if (!options.maintenanceAdmitted || options.reverseFifoDepth > 3U ||
      (options.mode != LoopbackMode::REVERSE && options.reverseFifoDepth != 0U) ||
      (options.mode == LoopbackMode::PCS_INPUT && options.speed != LinkSpeed::MBPS_10) ||
      (options.mode == LoopbackMode::DIGITAL && (options.speed != LinkSpeed::MBPS_100 || !options.allowHardResetExit)) ||
      (options.mode == LoopbackMode::ANALOG && !options.termination100Ohm) ||
      (options.mode == LoopbackMode::REVERSE && !options.partnerReady) ||
      (options.mode == LoopbackMode::EXTERNAL && !options.externalFixture) ||
      (options.transmitOnLine && (options.mode != LoopbackMode::MII || options.speed != LinkSpeed::MBPS_100))) {
    return rejected("Loopback speed, maintenance or fixture prerequisite missing");
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return busyLoop(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state == OperationState::RESULT_PENDING) { return busyLoop(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return Status::Error(Err::INVALID_CONFIG, "Clock contract failed", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)); }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return busyLoop(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED, "Initialize before loopback"); }
  if (!_settings.desiredValid || _settings.trust != ConfigurationTrust::VERIFIED) { return rejected("Verified persistent profile required"); }
  const auto profile = validateLinkSettings(_settings.desired); if (!profile.ok()) { return profile; }
  if (!nearEnd(options.mode) && _settings.desired.mode == LinkMode::FORCED && _settings.desired.forced.speed != options.speed) {
    return rejected("Fixture speed differs from forced profile");
  }
  if (!nearEnd(options.mode) && _settings.desired.mode == LinkMode::AUTONEGOTIATED &&
      (_settings.desired.advertisement.technologyMask & (options.speed == LinkSpeed::MBPS_100 ? 0x0CU : 0x03U)) == 0U) {
    return rejected("Fixture speed is absent from advertisement");
  }
  const auto nonzero = nonzeroBudget(budget); if (!nonzero.ok()) { return nonzero; }
  _beginOperation(OperationKind::LOOPBACK_START, budget);
  _result.identity = _identity;
  _loopback = {options, _operation.token, false, false};
  _loopSession = true;
  _loopRestoreTail = true;
  _loopSavedBmcr = _loopSavedBiscr = _loopSavedPhyscr = 0;
  _loopProfile = {};
  _loopProfile.mode = LinkMode::FORCED;
  _loopProfile.advertisement.technologyMask = 0;
  _loopProfile.forced = {options.speed, Duplex::FULL};
  _loopProfile.crossover.pairSelection = PairSelection::MDI;
  _loopProfile.crossover.robust = true;
  _result.loopback = {options.mode, options.speed, false, false, options.mode == LoopbackMode::DIGITAL, 0, false};
  _setLoopStep(LoopStep::READ_BMCR);
  token = _operation.token;
  return _operation.status;
}

Status DP8382x::stopLoopback(const OperationBudget& budget, OperationToken& token) noexcept {
  const auto polling = validBudget(budget); if (!polling.ok()) { return polling; }
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return busyLoop(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state == OperationState::RESULT_PENDING) { return busyLoop(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return Status::Error(Err::INVALID_CONFIG, "Clock contract failed", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)); }
  if (!_loopSession || !_operation.temporaryMayBeActive) { return rejected("No retained loopback to stop"); }
  const auto nonzero = nonzeroBudget(budget); if (!nonzero.ok()) { return nonzero; }
  _beginOperation(OperationKind::LOOPBACK_STOP, budget);
  _loopback.controlsVerified = false;
  const auto& options = _loopback.options;
  _result.loopback = {options.mode, options.speed, false, false, options.mode == LoopbackMode::DIGITAL, 0, false};
  _loopRestoreTail = true;
  _setLoopStep(LoopStep::IDENTIFY_STOP);
  token = _operation.token;
  return _operation.status;
}

void DP8382x::_setLoopStep(LoopStep step) noexcept {
  _loopWorking = true;
  _loopStep = step;
  if (step == LoopStep::RESTART) {
    _result.reset = ResetKind::RESTART;
    _result.commandRequired = true;
  }
  _operation.nextPollValid = false;
  _operation.phase = step == LoopStep::WAIT_RESTART ? OperationPhase::WAIT :
      step == LoopStep::RESTART ? OperationPhase::COMMAND : OperationPhase::VERIFY;
  if (step == LoopStep::IDENTIFY_STOP) { _operation.phase = OperationPhase::IDENTIFY; }
  if (step >= LoopStep::READ_BMCR && step <= LoopStep::CHECK_LINK) { _operation.phase = OperationPhase::QUIESCE; }
  if (step >= LoopStep::CLEAR_BMCR && step <= LoopStep::DIGITAL_0130) { _operation.phase = OperationPhase::COMMAND; }
  if (step >= LoopStep::STOP_BMCR && step <= LoopStep::RESTORE_FIFO) { _operation.phase = OperationPhase::RESTORE; }
}
void DP8382x::_beginLoopbackActivation() noexcept {
  if (_loopback.options.mode == LoopbackMode::DIGITAL) { _setLoopStep(LoopStep::DIGITAL_BMCR); }
  else if (_loopback.options.mode == LoopbackMode::REVERSE) { _setLoopStep(LoopStep::SET_FIFO); }
  else { _setLoopStep(LoopStep::SET_BISCR); }
}
void DP8382x::_beginLoopbackRestore() noexcept {
  _loopWorking = false;
  _result.reset = _loopback.options.mode == LoopbackMode::DIGITAL ? ResetKind::HARD : ResetKind::NONE;
  _result.commandRequired = _result.reset == ResetKind::HARD;
  _setInitStep(InitStep::IDENTIFY);
}

MdioBatch DP8382x::_loopbackBatch() const noexcept {
  const auto& options = _loopback.options;
  switch (_loopStep) {
    case LoopStep::IDENTIFY_STOP: {
      auto batch = readWord(2); batch.frameCount = 2; batch.frames[1] = {FrameKind::READ, 3, 0, 0, NO_READ_INDEX}; return batch;
    }
    case LoopStep::READ_BMCR: case LoopStep::VERIFY_BMCR: case LoopStep::VERIFY_STOP_BMCR: return readWord(0);
    case LoopStep::READ_PHYRCR: case LoopStep::WAIT_RESTART: return readWord(0x1F);
    case LoopStep::READ_BISCR: case LoopStep::VERIFY_BISCR: case LoopStep::VERIFY_STOP_BISCR: return readWord(0x16);
    case LoopStep::READ_PHYSCR: case LoopStep::VERIFY_ACTIVE_FIFO: case LoopStep::VERIFY_FIFO: return readWord(0x11);
    case LoopStep::CHECK_LINK: case LoopStep::RELINK: return readWord(0x10);
    // Table8-19 commands neutral0; never echo the power-pin OR bit into software.
    case LoopStep::CLEAR_BMCR: case LoopStep::STOP_BMCR: return updateWord(0, 0x35FF, 0);
    case LoopStep::CLEAR_BISCR: case LoopStep::STOP_BISCR: return updateWord(0x16, 0xFFA0, 0);
    case LoopStep::SET_FIFO: return updateWord(0x11, 0xFCFF, static_cast<uint16_t>(uint16_t{options.reverseFifoDepth} << 8U));
    case LoopStep::DIGITAL_BMCR: return writeWord(0, 0x2100);
    case LoopStep::SET_BISCR:
      if (options.mode == LoopbackMode::DIGITAL) { return writeWord(0x16, 0x0104); }
      return updateWord(0x16, 0xFFA0, static_cast<uint16_t>(selector(options) | (options.transmitOnLine ? 0x0040U : 0U)));
    case LoopStep::SET_MII: return updateWord(0, 0x35FF, 0x4000);
    case LoopStep::DIGITAL_0122: return _mmdBatch(0x0122, FrameKind::WRITE, 0x2000);
    case LoopStep::DIGITAL_0123: return _mmdBatch(0x0123, FrameKind::WRITE, 0x2000);
    case LoopStep::DIGITAL_0130: return _mmdBatch(0x0130, FrameKind::WRITE, 0x47FF);
    case LoopStep::RESTART: return writeWord(0x1F, 0x4000);
    // HARD may reload BASIC isolation straps. Restore only the saved ordinary
    // extras (preflight required zero), with power/reset/restart neutral zero.
    case LoopStep::RESTORE_BMCR: return updateWord(0, 0x317F, static_cast<uint16_t>(_loopSavedBmcr & 0x0480));
    case LoopStep::RESTORE_BISCR: return updateWord(0x16, 0x8FA0, static_cast<uint16_t>(_loopSavedBiscr & 0x705F));
    case LoopStep::RESTORE_FIFO: return updateWord(0x11, 0xFCFF, static_cast<uint16_t>(_loopSavedPhyscr & 0x0300));
  }
  return {};
}

Status DP8382x::_advanceLoopback(Status status, bool called, const MdioBatch& batch) noexcept {
  const auto& receipt = _transport.lastBatch;
  const auto completed = _result.failingBatch.completedFrames;
  const auto& options = _loopback.options;
  const auto value = receipt.frames[0].value;
  bool resetActive = false;
  if (called) {
    // Selector frames are not semantic mutations. All ordinary updates are
    // single owned two-frame RMWs; opaque companions use the common MMD batch.
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& frame = batch.frames[i];
      if (frame.kind != FrameKind::WRITE || (batch.frameCount == 4U && i != 3U)) { continue; }
      const auto& target = receipt.frames[i];
      const bool uncertain = _transport.violation != TransportViolation::NONE;
      const bool attempted = target.attempted || target.completed || target.effectPossible || target.valueValid || uncertain;
      _result.mutationAttempted = _result.mutationAttempted || target.attempted;
      _result.mutationCompleted = _result.mutationCompleted || completed > i;
      _result.mutationPossible = _result.mutationPossible || completed > i || target.effectPossible || uncertain;
      if (attempted && _operation.kind == OperationKind::LOOPBACK_START && _loopStep != LoopStep::RESTART) {
        _operation.temporaryMayBeActive = true;
        _operation.cleanupOwed = true;
        _cleanupOwner = OperationKind::LOOPBACK_START;
      }
      _result.failingBatch.effectPossible = _result.failingBatch.effectPossible || uncertain;
    }
    if (completed != 0U && batch.frames[0].kind == FrameKind::READ) {
      const auto value = receipt.frames[0].value;
      if (_loopStep == LoopStep::READ_BMCR) { _loopSavedBmcr = value; }
      if (_loopStep == LoopStep::READ_BISCR) { _loopSavedBiscr = value; }
      if (_loopStep == LoopStep::READ_PHYSCR) { _loopSavedPhyscr = value; }
      if (batch.frames[0].registerAddress == 0x10) {
        _result.loopback.rawPhysts = value; _result.loopback.phystsValid = true;
      }
      resetActive = (batch.frames[0].registerAddress == 0 && (value & 0x8000U) != 0U) ||
          (batch.frames[0].registerAddress == 0x1F && (value & 0x8000U) != 0U);
      if (resetActive) { _invalidateResetEvidence(true); }
      uint16_t mask = 0, expected = 0;
      switch (_loopStep) {
        case LoopStep::VERIFY_BMCR:
          mask = nearEnd(options.mode) ? 0x7100 : 0x4000;
          expected = nearEnd(options.mode) ? static_cast<uint16_t>(
              (options.speed == LinkSpeed::MBPS_100 ? 0x2100U : 0x0100U) |
              (options.mode == LoopbackMode::MII ? 0x4000U : 0U)) : uint16_t{0};
          break;
        case LoopStep::VERIFY_BISCR:
          mask = 0x305F; expected = static_cast<uint16_t>(selector(options) | (options.transmitOnLine ? 0x0040U : 0U)); break;
        case LoopStep::VERIFY_ACTIVE_FIFO:
          mask = 0x0300; expected = static_cast<uint16_t>(uint16_t{options.reverseFifoDepth} << 8U); break;
        case LoopStep::VERIFY_STOP_BMCR:
          mask = _result.reset == ResetKind::HARD ? 0x7D80 : 0x4000;
          if (_result.reset == ResetKind::HARD) {
            expected = static_cast<uint16_t>((_loopSavedBmcr & 0x0480U) |
                (_settings.desired.mode == LinkMode::AUTONEGOTIATED ? 0x1000U :
                 ((_settings.desired.forced.speed == LinkSpeed::MBPS_100 ? 0x2000U : 0U) |
                  (_settings.desired.forced.duplex == Duplex::FULL ? 0x0100U : 0U))));
          }
          break;
        case LoopStep::VERIFY_STOP_BISCR: mask = 0x705F; expected = static_cast<uint16_t>(_loopSavedBiscr & mask); break;
        case LoopStep::VERIFY_FIFO: mask = 0x0300; expected = static_cast<uint16_t>(_loopSavedPhyscr & mask); break;
        default: break;
      }
      if (!resetActive && mask != 0U && (value & mask) != expected && !_result.mismatch.valid) {
        _result.mismatch = {true, false, 0, batch.frames[0].registerAddress, mask, expected, value};
      }
    }
    if (_loopStep == LoopStep::RESTART) { _result.commandAccepted = completed != 0U; }
  }
  if (!status.ok()) { return _finishOperation(status); }
  if (resetActive) { return _finishOperation(rejected("Unexpected reset during loopback")); }
  if (_result.mismatch.valid) { return _finishOperation(Status::Error(Err::CONFIG_MISMATCH, "Loopback readback mismatch")); }
  switch (_loopStep) {
    case LoopStep::IDENTIFY_STOP: _setLoopStep(LoopStep::STOP_BMCR); break;
    case LoopStep::READ_BMCR:
      if ((value & 0xCC80U) != 0U) { return _finishOperation(rejected("Reset, power, isolation, collision or loopback already active")); }
      _setLoopStep(LoopStep::READ_PHYRCR); break;
    case LoopStep::READ_PHYRCR:
      if ((value & 0xC000U) != 0U) { return _finishOperation(rejected("Reset already active")); }
      _setLoopStep(LoopStep::READ_BISCR); break;
    case LoopStep::READ_BISCR:
      if ((value & 0x305FU) != 0U) { return _finishOperation(rejected("BIST or loopback already active")); }
      _setLoopStep(LoopStep::READ_PHYSCR); break;
    case LoopStep::READ_PHYSCR:
      _setLoopStep(options.mode == LoopbackMode::REVERSE ? LoopStep::CHECK_LINK : LoopStep::CLEAR_BMCR); break;
    case LoopStep::CHECK_LINK: {
      const auto speed = (_loopSavedBmcr & 0x1000U) != 0U ?
          ((value & 0x0002U) != 0U ? LinkSpeed::MBPS_10 : LinkSpeed::MBPS_100) :
          ((_loopSavedBmcr & 0x2000U) != 0U ? LinkSpeed::MBPS_100 : LinkSpeed::MBPS_10);
      if ((value & 1U) == 0U || speed != options.speed) { return _finishOperation(rejected("Reverse requires compatible live link")); }
      _setLoopStep(LoopStep::CLEAR_BMCR); break;
    }
    case LoopStep::CLEAR_BMCR: _setLoopStep(LoopStep::CLEAR_BISCR); break;
    case LoopStep::CLEAR_BISCR:
      if (nearEnd(options.mode)) { _beginConfiguration(false); }
      else { _beginLoopbackActivation(); }
      break;
    case LoopStep::SET_FIFO: _setLoopStep(LoopStep::SET_BISCR); break;
    case LoopStep::DIGITAL_BMCR: _setLoopStep(LoopStep::SET_BISCR); break;
    case LoopStep::SET_BISCR:
      if (options.mode == LoopbackMode::DIGITAL) { _setLoopStep(LoopStep::DIGITAL_0122); }
      else if (options.mode == LoopbackMode::MII) { _setLoopStep(LoopStep::SET_MII); }
      else if (options.mode == LoopbackMode::ANALOG || options.mode == LoopbackMode::REVERSE) { _setLoopStep(LoopStep::RESTART); }
      else { _setLoopStep(LoopStep::VERIFY_BMCR); }
      break;
    case LoopStep::SET_MII: _setLoopStep(LoopStep::VERIFY_BMCR); break;
    case LoopStep::DIGITAL_0122: _setLoopStep(LoopStep::DIGITAL_0123); break;
    case LoopStep::DIGITAL_0123: _setLoopStep(LoopStep::DIGITAL_0130); break;
    case LoopStep::DIGITAL_0130: _setLoopStep(LoopStep::RESTART); break;
    case LoopStep::RESTART:
      _setLoopStep(LoopStep::WAIT_RESTART); return _waitOperation();
    case LoopStep::WAIT_RESTART:
      if ((value & 0x8000U) != 0U) { return _finishOperation(rejected("Unexpected hard reset during loopback")); }
      if ((value & 0x4000U) != 0U) { return _waitOperation(); }
      _result.protocolComplete = true;
      _setLoopStep(LoopStep::VERIFY_BMCR); break;
    case LoopStep::VERIFY_BMCR: _setLoopStep(LoopStep::VERIFY_BISCR); break;
    case LoopStep::VERIFY_BISCR: {
      if (options.mode == LoopbackMode::REVERSE) { _setLoopStep(LoopStep::VERIFY_ACTIVE_FIFO); break; }
      _loopback.controlsVerified = _result.loopback.controlsVerified = true;
      return _finishOperation(Status::Ok());
    }
    case LoopStep::VERIFY_ACTIVE_FIFO: _setLoopStep(LoopStep::RELINK); break;
    case LoopStep::RELINK:
      if ((value & 1U) == 0U) { return _waitOperation(); }
      if ((_loopSavedBmcr & 0x1000U) != 0U &&
          (((value & 2U) != 0U ? LinkSpeed::MBPS_10 : LinkSpeed::MBPS_100) != options.speed)) {
        return _finishOperation(rejected("Reverse relink changed speed"));
      }
      _loopback.controlsVerified = _result.loopback.controlsVerified = true;
      return _finishOperation(Status::Ok());
    case LoopStep::STOP_BMCR: _setLoopStep(LoopStep::STOP_BISCR); break;
    case LoopStep::STOP_BISCR: _beginLoopbackRestore(); break;
    case LoopStep::RESTORE_BMCR: _setLoopStep(LoopStep::RESTORE_BISCR); break;
    case LoopStep::RESTORE_BISCR: _setLoopStep(LoopStep::RESTORE_FIFO); break;
    case LoopStep::RESTORE_FIFO: _setLoopStep(LoopStep::VERIFY_STOP_BMCR); break;
    case LoopStep::VERIFY_STOP_BMCR:
      _setLoopStep(LoopStep::VERIFY_STOP_BISCR); break;
    case LoopStep::VERIFY_STOP_BISCR:
      _setLoopStep(LoopStep::VERIFY_FIFO); break;
    case LoopStep::VERIFY_FIFO:
      _loopRestoreTail = false;
      _result.persistentRestored = true;
      _settings.trust = ConfigurationTrust::VERIFIED;
      _initialized = true;
      return _finishOperation(Status::Ok());
  }
  return _operation.status;
}
} // namespace DP8382x
