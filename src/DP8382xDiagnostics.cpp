/// @file DP8382xDiagnostics.cpp
/// @brief Exact loopback sessions using the existing operation and restoration owner.
#include <DP8382x/DP8382x.h>
#include <DP8382x/CommandTable.h>
#include <limits>

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
  return _operation.kind == OperationKind::LOOPBACK_START || _operation.kind == OperationKind::LOOPBACK_STOP ||
      _operation.kind == OperationKind::BIST_START || _operation.kind == OperationKind::BIST_ACQUIRE || _operation.kind == OperationKind::BIST_STOP ?
      Traffic::RAW : Traffic::MANAGED;
}
const LinkSettings& DP8382x::_configurationTarget() const noexcept {
  return _startingLoopback() ? _loopProfile : _settings.desired;
}
LoopbackSnapshot DP8382x::loopbackSnapshot() const noexcept {
  auto snapshot = _loopback;
  snapshot.temporaryMayBeActive = _operation.temporaryMayBeActive;
  return snapshot;
}

bool DP8382x::_startingLoopback() const noexcept {
  return _operation.kind == OperationKind::LOOPBACK_START || _operation.kind == OperationKind::BIST_START;
}
Status DP8382x::startLoopback(const LoopbackOptions& options, const OperationBudget& budget,
                             OperationToken& token) noexcept {
  return _startLoopback(options, budget, token, false);
}
Status DP8382x::_startLoopback(const LoopbackOptions& options, const OperationBudget& budget,
                              OperationToken& token, bool lineBist) noexcept {
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
      (options.mode == LoopbackMode::EXTERNAL && !options.externalFixture && !lineBist) ||
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
  if (_bistSession) { return busyLoop(BusyDetail::CLEANUP_REQUIRED); }
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
  else if (_loopback.options.mode == LoopbackMode::MII) { _setLoopStep(LoopStep::SET_MII); }
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
      if (attempted && _startingLoopback() && _loopStep != LoopStep::RESTART) {
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
    case LoopStep::IDENTIFY_STOP:
      if (_operation.kind == OperationKind::BIST_STOP) { _setBistStep(BistStep::DISABLE); }
      else { _setLoopStep(LoopStep::STOP_BISCR); }
      break;
    case LoopStep::READ_BMCR:
      if ((value & 0xCC80U) != 0U) { return _finishOperation(rejected("Reset, power, isolation, collision or loopback already active")); }
      _setLoopStep(LoopStep::READ_PHYRCR); break;
    case LoopStep::READ_PHYRCR:
      if ((value & 0xC000U) != 0U) { return _finishOperation(rejected("Reset already active")); }
      _setLoopStep(LoopStep::READ_BISCR); break;
    case LoopStep::READ_BISCR:
      if ((value & 0x325FU) != 0U) { return _finishOperation(rejected("BIST or loopback already active")); }
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
      else if (options.mode == LoopbackMode::ANALOG || options.mode == LoopbackMode::REVERSE) { _setLoopStep(LoopStep::RESTART); }
      else { _setLoopStep(LoopStep::VERIFY_BMCR); }
      break;
    // Table 8-40 permits BISCR6 only while BMCR14 is set. Establish MII
    // loopback before enabling line transmission, and remove it in reverse order.
    case LoopStep::SET_MII: _setLoopStep(LoopStep::SET_BISCR); break;
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
    case LoopStep::STOP_BISCR: _setLoopStep(LoopStep::STOP_BMCR); break;
    case LoopStep::STOP_BMCR: _beginLoopbackRestore(); break;
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

BistSnapshot DP8382x::bistSnapshot() const noexcept {
  auto snapshot = _bist;
  snapshot.temporaryMayBeActive = _bistSession && _operation.temporaryMayBeActive;
  return snapshot;
}
Status DP8382x::startBist(const BistOptions& options, const OperationBudget& budget,
                         OperationToken& token) noexcept {
  if (options.mode > BistMode::GENERATOR_CHECKER || options.counterMode > BistCounterMode::WRAP ||
      options.loopback.mode > LoopbackMode::EXTERNAL) {
    return Status::Error(Err::INVALID_PARAM, "Invalid BIST mode", static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }
  if (options.packetBytes > 2047U || options.interPacketGapBytes > 1020U ||
      (options.interPacketGapBytes % 4U) != 0U || options.observationUs == 0U) {
    return rejected("BIST packet, gap or observation interval invalid");
  }
  if (options.useLoopback && options.loopback.mode != LoopbackMode::DIGITAL &&
      options.loopback.mode != LoopbackMode::ANALOG && options.loopback.mode != LoopbackMode::EXTERNAL) {
    return rejected("BIST loopback interaction lacks exact-part evidence");
  }
  auto path = options.loopback;
  if (!options.useLoopback) { path.mode = LoopbackMode::EXTERNAL; }
  const auto status = _startLoopback(path, budget, token, !options.useLoopback);
  if (!status.inProgress()) { return status; }
  _operation.kind = _result.kind = OperationKind::BIST_START;
  _bist = {};
  _bist.options = options;
  _bist.token = token;
  _bistSession = _bistRestoreTail = true;
  _bistWorking = _bistSavedIpgValid = _bistSavedPacketValid = false;
  _bistActivity = _bistLoss = _bistCapturePending = false;
  _result.bist.session = token;
  _result.bist.mode = options.mode;
  // Save before any companion mutation: even a failed digital setup needs HARD,
  // which reloads packet/IPG defaults. Its owning exit must retain these values.
  _setBistStep(BistStep::SAVE_IPG);
  return status;
}
Status DP8382x::_admitBistOperation(const OperationBudget& budget, bool stop) const noexcept {
  const auto polling = validBudget(budget); if (!polling.ok()) { return polling; }
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return busyLoop(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state == OperationState::RESULT_PENDING) { return busyLoop(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return Status::Error(Err::INVALID_CONFIG, "Clock contract failed", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)); }
  if (!_bistSession || !_operation.temporaryMayBeActive) { return rejected("No retained BIST session"); }
  if (!stop && (!_initialized || !capabilities().valid)) { return Status::Error(Err::NOT_INITIALIZED, "Fresh initialized BIST required"); }
  if (!stop && !_bist.active) { return rejected("BIST setup was not verified"); }
  return nonzeroBudget(budget);
}
Status DP8382x::startBistAcquire(const BistObservation& observation, const OperationBudget& budget,
                                OperationToken& token) noexcept {
  const auto status = _admitBistOperation(budget, false); if (!status.ok()) { return status; }
  _beginOperation(OperationKind::BIST_ACQUIRE, budget);
  _bistObservation = observation;
  _result.bist.session = _bist.token;
  _result.bist.mode = _bist.options.mode;
  _result.bist.historyLost = _bist.measurement.historyLost;
  _bist.stale = true;
  _setBistStep(BistStep::BEFORE);
  _operation.nextPollAtUs = _bistDueUs;
  _operation.nextPollValid = true;
  token = _operation.token;
  return _operation.status;
}
Status DP8382x::stopBist(const OperationBudget& budget, OperationToken& token) noexcept {
  const auto status = _admitBistOperation(budget, true); if (!status.ok()) { return status; }
  _beginOperation(OperationKind::BIST_STOP, budget);
  _result.bist = _bist.measurement;
  _result.bist.session = _bist.token;
  _result.bist.mode = _bist.options.mode;
  _result.loopback = {_loopback.options.mode, _loopback.options.speed, false, false,
                     _loopback.options.mode == LoopbackMode::DIGITAL, 0, false};
  _bist.active = _loopback.controlsVerified = false;
  _bist.stale = true;
  _loopRestoreTail = _bistRestoreTail = true;
  _setLoopStep(LoopStep::IDENTIFY_STOP);
  token = _operation.token;
  return _operation.status;
}
void DP8382x::_setBistStep(BistStep step) noexcept {
  _bistWorking = true;
  _loopWorking = _configuring = false;
  _bistStep = step;
  _operation.nextPollValid = false;
  _operation.phase = step >= BistStep::DISABLE ? OperationPhase::RESTORE : OperationPhase::VERIFY;
}
MdioBatch DP8382x::_bistBatch() const noexcept {
  const auto ipg = static_cast<uint16_t>(_bist.options.interPacketGapBytes / 4U);
  const auto controls = static_cast<uint16_t>((static_cast<uint32_t>(_bist.options.mode) << 12U) |
      (_bist.options.counterMode == BistCounterMode::WRAP ? 0x4000U : 0U));
  switch (_bistStep) {
    case BistStep::SAVE_IPG: case BistStep::VERIFY_IPG: case BistStep::VERIFY_RESTORE_IPG: return readWord(0x1B);
    case BistStep::SAVE_PACKET: case BistStep::VERIFY_PACKET: case BistStep::VERIFY_RESTORE_PACKET: return readWord(0x1C);
    case BistStep::APPLY_IPG: return writeWord(0x1B, ipg); // High bits neutral; never echo counter MSB as command.
    case BistStep::APPLY_PACKET: return updateWord(0x1C, 0xF800, _bist.options.packetBytes);
    case BistStep::BASELINE: case BistStep::CAPTURE: {
      // Table8-45 capture/clear + successive read, indivisible under the bus owner.
      auto batch = writeWord(0x1B, static_cast<uint16_t>(0x8000U | ipg));
      batch.frameCount = 2; batch.frames[1] = {FrameKind::READ, 0x1B, 0, 0, NO_READ_INDEX}; return batch;
    }
    case BistStep::LINK: return readWord(0x10);
    case BistStep::ENABLE: return updateWord(0x16, 0x8FFF, controls);
    case BistStep::VERIFY_ENABLE: case BistStep::BEFORE: case BistStep::AFTER: case BistStep::VERIFY_DISABLE: return readWord(0x16);
    case BistStep::DISABLE: return updateWord(0x16, 0x8FFF, 0);
    case BistStep::RESTORE_IPG: return writeWord(0x1B, _bistSavedIpg);
    case BistStep::RESTORE_PACKET: return updateWord(0x1C, 0xF800, _bistSavedPacket);
  }
  return {};
}
Status DP8382x::_advanceBist(Status status, bool called, const MdioBatch& batch) noexcept {
  auto& measured = _result.bist;
  const auto& receipt = _transport.lastBatch;
  const auto completed = _result.failingBatch.completedFrames;
  if (called) {
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      if (batch.frames[i].kind != FrameKind::WRITE) { continue; }
      const auto& frame = receipt.frames[i];
      const bool uncertain = _transport.violation != TransportViolation::NONE;
      const bool attempted = frame.attempted || frame.completed || frame.effectPossible || frame.valueValid || uncertain;
      _result.mutationAttempted = _result.mutationAttempted || frame.attempted;
      _result.mutationCompleted = _result.mutationCompleted || completed > i;
      _result.mutationPossible = _result.mutationPossible || frame.effectPossible || completed > i || uncertain;
      if (attempted) { _operation.cleanupOwed = _operation.temporaryMayBeActive = true; _cleanupOwner = OperationKind::BIST_START; }
    }
    if (_bistCapturePending) { measured.historyLost = true; }
    for (uint8_t i = 0; i < completed; ++i) {
      if (batch.frames[i].kind != FrameKind::READ) { continue; }
      const auto value = receipt.frames[i].value;
      if (_bistStep == BistStep::SAVE_IPG) { _bistSavedIpg = value & 0x00FFU; _bistSavedIpgValid = true; }
      if (_bistStep == BistStep::SAVE_PACKET) { _bistSavedPacket = value & 0x07FFU; _bistSavedPacketValid = true; }
      if (_bistStep == BistStep::BEFORE) { measured.rawBefore = value; measured.beforeValid = true; }
      if (_bistStep == BistStep::AFTER) { measured.rawAfter = value; measured.afterValid = true; }
      if (_bistStep == BistStep::CAPTURE && i == 1U) {
        measured.rawCounter = value; measured.counterValid = true;
        measured.errorBytes = static_cast<uint8_t>(value >> 8U);
        const auto& sample = _evidence.samples[static_cast<unsigned>(EvidenceSource::BICSR1)];
        measured.timeValid = sample.timeValid; measured.observedAtUs = sample.observedAtUs;
        measured.wrapPossible = _bist.options.counterMode == BistCounterMode::WRAP;
        measured.saturated = !measured.wrapPossible && measured.errorBytes == 0xFFU;
      }
      uint16_t mask = 0, expected = 0;
      switch (_bistStep) {
        case BistStep::VERIFY_IPG: mask = 0x00FF; expected = static_cast<uint16_t>(_bist.options.interPacketGapBytes / 4U); break;
        case BistStep::VERIFY_PACKET: mask = 0x07FF; expected = _bist.options.packetBytes; break;
        case BistStep::VERIFY_ENABLE: case BistStep::BEFORE: case BistStep::AFTER:
          mask = 0x7000; expected = static_cast<uint16_t>((static_cast<uint32_t>(_bist.options.mode) << 12U) |
              (_bist.options.counterMode == BistCounterMode::WRAP ? 0x4000U : 0U)); break;
        case BistStep::VERIFY_DISABLE: mask = 0x7000; break;
        case BistStep::VERIFY_RESTORE_IPG: mask = 0x00FF; expected = _bistSavedIpg; break;
        case BistStep::VERIFY_RESTORE_PACKET: mask = 0x07FF; expected = _bistSavedPacket; break;
        default: break;
      }
      if (mask != 0U && (value & mask) != expected && !_result.mismatch.valid) {
        _result.mismatch = {true, false, 0, batch.frames[i].registerAddress, mask, expected, value};
      }
    }
  }
  if (!status.ok()) { return _finishOperation(status); }
  if (_result.mismatch.valid) { return _finishOperation(Status::Error(Err::CONFIG_MISMATCH, "BIST readback mismatch")); }
  switch (_bistStep) {
    case BistStep::SAVE_IPG: _setBistStep(BistStep::SAVE_PACKET); break;
    case BistStep::SAVE_PACKET:
      _bistWorking = false; _setLoopStep(LoopStep::READ_BMCR); break;
    case BistStep::APPLY_IPG: _setBistStep(BistStep::APPLY_PACKET); break;
    case BistStep::APPLY_PACKET: _setBistStep(BistStep::VERIFY_IPG); break;
    case BistStep::VERIFY_IPG: _setBistStep(BistStep::VERIFY_PACKET); break;
    case BistStep::VERIFY_PACKET: _setBistStep(BistStep::BASELINE); break;
    case BistStep::BASELINE: _setBistStep(BistStep::LINK); break;
    case BistStep::LINK:
      _result.loopback.rawPhysts = receipt.frames[0].value; _result.loopback.phystsValid = true;
      if (_bist.options.useLoopback && _loopback.options.mode != LoopbackMode::EXTERNAL &&
          (receipt.frames[0].value & 1U) == 0U) { return _waitOperation(); }
      _setBistStep(BistStep::ENABLE); break;
    case BistStep::ENABLE: _setBistStep(BistStep::VERIFY_ENABLE); break;
    case BistStep::VERIFY_ENABLE: case BistStep::AFTER: {
      const auto anchor = _lastTimeUs + _config.transport.bounds.clockLagUs;
      if (_bist.options.observationUs > std::numeric_limits<uint64_t>::max() - anchor) {
        return _finishOperation(Status::Error(Err::OPERATION_TIMEOUT, "BIST observation time overflow", static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION)));
      }
      _bistDueUs = anchor + _bist.options.observationUs;
      if (_bistStep == BistStep::VERIFY_ENABLE) { _bist.active = true; return _finishOperation(Status::Ok()); }
      measured.measurementValid = true;
      measured.activityObserved = _bistActivity;
      measured.lockObserved = (measured.rawBefore & measured.rawAfter & 0x0800U) != 0U;
      measured.syncLossObserved = _bistLoss;
      // RH does not prove an uninterrupted lock window. Repeat-latch semantics
      // remain gated: zero never becomes PASS, even with apparently good traffic.
      if (_bist.options.mode == BistMode::OFF) { measured.outcome = BistOutcome::DISABLED; }
      else if (_bist.options.mode == BistMode::SINGLE_PACKET) {
        if (_bistObservation.singlePacketObserved) { measured.outcome = BistOutcome::SINGLE_PACKET_OBSERVED; }
      } else if (measured.lockObserved && !measured.syncLossObserved && measured.errorBytes != 0U &&
          (_bist.options.mode == BistMode::CHECKER ? _bistObservation.externalStimulusObserved : measured.activityObserved)) {
        measured.outcome = BistOutcome::ERRORS_OBSERVED;
      }
      return _finishOperation(Status::Ok());
    }
    case BistStep::BEFORE: _setBistStep(BistStep::CAPTURE); break;
    case BistStep::CAPTURE: _setBistStep(BistStep::AFTER); break;
    case BistStep::DISABLE: _setBistStep(BistStep::VERIFY_DISABLE); break;
    case BistStep::VERIFY_DISABLE:
      if ((receipt.frames[0].value & 0x0200U) != 0U) { return _waitOperation(); }
      _bistWorking = false; _setLoopStep(LoopStep::STOP_BISCR); break;
    case BistStep::RESTORE_IPG:
      _setBistStep(_bistSavedPacketValid ? BistStep::RESTORE_PACKET : BistStep::VERIFY_RESTORE_IPG); break;
    case BistStep::RESTORE_PACKET:
      _setBistStep(_bistSavedIpgValid ? BistStep::VERIFY_RESTORE_IPG : BistStep::VERIFY_RESTORE_PACKET); break;
    case BistStep::VERIFY_RESTORE_IPG:
      if (_bistSavedPacketValid) { _setBistStep(BistStep::VERIFY_RESTORE_PACKET); break; }
      [[fallthrough]];
    case BistStep::VERIFY_RESTORE_PACKET:
      _bistRestoreTail = false;
      _result.persistentRestored = true; _settings.trust = ConfigurationTrust::VERIFIED; _initialized = true;
      return _finishOperation(Status::Ok());
  }
  return _operation.status;
}
} // namespace DP8382x
