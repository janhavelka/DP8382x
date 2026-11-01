/// @file DP8382x.cpp
/// @brief Local binding and one-frame terminal Clause 22 transactions.
#include <DP8382x/DP8382x.h>
#include <DP8382x/CommandTable.h>

#include <limits>

namespace DP8382x {
namespace {

Status configError(ConfigDetail detail) noexcept {
  return Status::Error(Err::INVALID_CONFIG, "Invalid binding or clock contract",
                       static_cast<int32_t>(detail));
}

Status paramError(ParamDetail detail) noexcept {
  return Status::Error(Err::INVALID_PARAM, "Invalid direct access",
                       static_cast<int32_t>(detail));
}

Status timeout(TimeoutDetail detail) noexcept {
  return Status::Error(Err::OPERATION_TIMEOUT, "Aggregate allowance exhausted",
                       static_cast<int32_t>(detail));
}

Status busy(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY, "Owner admission closed", static_cast<int32_t>(detail));
}

void increment(uint32_t& value) noexcept {
  if (value != std::numeric_limits<uint32_t>::max()) { ++value; }
}

bool isDefault(const FrameResult& frame) noexcept {
  return !frame.attempted && !frame.completed && !frame.valueValid &&
         frame.value == 0U && !frame.effectPossible && frame.completedAtUs == 0U &&
         !frame.timeValid;
}

// SNLS647I detailed tables 8-19..48. This is a read policy, not a decoder.
// Exclude latch/counter consumption, selectors, holes and CDCR's mandatory
// rescal recovery follow-up. Raw command readback does not establish readiness.
bool readable(uint32_t address) noexcept {
  switch (address) {
    case 0x00: case 0x02: case 0x03: case 0x04: case 0x05:
    case 0x07: case 0x08: case 0x09: case 0x0A: case 0x0B:
    case 0x11: case 0x16: case 0x18: case 0x19: case 0x1B:
    case 0x1C: case 0x1F: return true;
    default: return false;
  }
}

// A terminal backend can report only these code-local details. Raw reports
// remain untouched in the snapshot; normalized diagnostics always use static text.
bool validStatus(const Status& status, bool release) noexcept {
  if (status.msg == nullptr) { return false; }
  switch (status.code) {
    case Err::OK: return status.detail == 0;
    case Err::MDIO_ERROR: case Err::MDIO_TIMEOUT: return true;
    case Err::BUSY:
      return !release &&
             (status.detail == static_cast<int32_t>(BusyDetail::OWNER_ADMISSION) ||
              status.detail == static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED));
    case Err::OPERATION_TIMEOUT:
      return !release && status.detail >= static_cast<int32_t>(TimeoutDetail::DEADLINE) &&
             status.detail <= static_cast<int32_t>(TimeoutDetail::POLL_BUDGET);
    default: return false;
  }
}

Status normalize(const Status& status, bool release) noexcept {
  const Err code = validStatus(status, release) ? status.code : Err::MDIO_ERROR;
  return {code, status.detail, errorName(code)};
}

// Guards defensive reentry through either callback, including local teardown.
// It is not a mutex: the application must serialize all real concurrent callers.
struct CallGuard {
  bool& active;
  explicit CallGuard(bool& flag) noexcept : active(flag) { active = true; }
  ~CallGuard() noexcept { active = false; }
};

} // namespace

Status DP8382x::bind(const Config& config) noexcept {
  if (!isValidVariant(config.variant) ||
      (config.interface != MacInterface::UNKNOWN && config.interface != MacInterface::MII &&
       config.interface != MacInterface::RMII && config.interface != MacInterface::RGMII) ||
      (config.requiredMode != OperatingMode::UNKNOWN && config.requiredMode != OperatingMode::BASIC &&
       config.requiredMode != OperatingMode::ENHANCED) ||
      config.revisionPolicy != RevisionPolicy::DP83826I_REV1_ONLY) {
    return configError(ConfigDetail::ENUM_VALUE);
  }
  if (!isValidPhyAddress(config.phyAddress)) { return configError(ConfigDetail::ADDRESS_RANGE); }
  const auto& transport = config.transport;
  if (transport.executeBatch == nullptr || transport.nowUs == nullptr ||
      transport.bounds.frameUs == 0U || transport.bounds.overheadUs == 0U ||
      transport.bounds.clockLagUs == 0U) { return configError(ConfigDetail::TRANSPORT); }
  if (config.offlineThreshold == 0U) { return configError(ConfigDetail::HEALTH_THRESHOLD); }
  if (config.interface == MacInterface::UNKNOWN) { return configError(ConfigDetail::INTERFACE_REQUIRED); }
  if (config.interface == MacInterface::RGMII) {
    return Status::Error(Err::UNSUPPORTED, "DP83826I has no RGMII",
                         static_cast<int32_t>(UnsupportedDetail::INTERFACE));
  }
  if (_active) { return busy(BusyDetail::OPERATION_ACTIVE); }
  _config = config;
  _transport = {};
  _timeValid = false;
  _lastTimeUs = 0;
  _stopped = false;
  _bound = true;
  return Status::Ok();
}

Status DP8382x::end() noexcept {
  if (_active) { return busy(BusyDetail::OPERATION_ACTIVE); }
  _config = {};
  _transport = {};
  _timeValid = false;
  _lastTimeUs = 0;
  _stopped = false;
  _bound = false;
  return Status::Ok();
}

Status DP8382x::readClause22(uint32_t registerAddress, uint16_t& value,
                            const IoBudget& budget) noexcept {
  if (registerAddress > 31U) { return paramError(ParamDetail::ADDRESS_RANGE); }
  if (!readable(registerAddress)) { return paramError(ParamDetail::ACCESS_POLICY); }
  return _transfer(FrameKind::READ, static_cast<uint8_t>(registerAddress), 0, value, budget);
}

Status DP8382x::writeClause22(uint32_t registerAddress, uint32_t value,
                             const IoBudget& budget) noexcept {
  if (registerAddress > 31U) { return paramError(ParamDetail::ADDRESS_RANGE); }
  if (value > 0xFFFFU) { return paramError(ParamDetail::VALUE_WIDTH); }
  // Table 8-42 permits independent LED0 control; other direct mutations need
  // initialization/collector/typed sequencing and are not bootstrap escape hatches.
  if (registerAddress != cmd::REG_LEDCR || (value & ~uint32_t{cmd::MASK_LEDCR_DIRECT}) != 0U) {
    return paramError(ParamDetail::ACCESS_POLICY);
  }
  uint16_t unused = 0;
  return _transfer(FrameKind::WRITE, static_cast<uint8_t>(registerAddress),
                   static_cast<uint16_t>(value), unused, budget);
}

Status DP8382x::_sampleTime(uint64_t& raw, uint64_t& upper) noexcept {
  raw = _config.transport.nowUs(_config.transport.context);
  const auto lag = _config.transport.bounds.clockLagUs;
  if ((_timeValid && raw < _lastTimeUs) || raw > std::numeric_limits<uint64_t>::max() - lag) {
    _transport.clockInvalid = true;
    return configError(ConfigDetail::CLOCK_CONTRACT);
  }
  _timeValid = true;
  _lastTimeUs = raw;
  upper = raw + lag;
  return Status::Ok();
}

Status DP8382x::_transfer(FrameKind kind, uint8_t address, uint16_t writeValue,
                         uint16_t& readValue, const IoBudget& budget) noexcept {
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active) { return busy(BusyDetail::OPERATION_ACTIVE); }
  if (_transport.clockInvalid) { return configError(ConfigDetail::CLOCK_CONTRACT); }
  if (_stopped) { return busy(BusyDetail::CONTROLLER_QUARANTINED); }
  CallGuard guard(_active);
  uint64_t before = 0;
  uint64_t upper = 0;
  Status status = _sampleTime(before, upper);
  if (status.ok()) {
    const auto& bounds = _config.transport.bounds;
    // All four addends are uint32_t; promotion before addition cannot overflow uint64_t.
    const uint64_t bound = uint64_t{bounds.acquireUs} + bounds.frameUs +
                           bounds.releaseUs + bounds.overheadUs;
    if (before >= budget.deadlineUs) { status = timeout(TimeoutDetail::DEADLINE); }
    else if (budget.maxFrames < 1U) { status = timeout(TimeoutDetail::FRAME_BUDGET); }
    else if (upper >= budget.deadlineUs || bound >= budget.deadlineUs - upper) {
      status = timeout(TimeoutDetail::BOUND_ADMISSION);
    }
  }
  if (!status.ok()) {
    _transport.lastStatus = status;
    return status;
  }

  MdioBatch batch;
  batch.phyAddress = static_cast<uint8_t>(_config.phyAddress);
  batch.frameCount = 1;
  batch.frames[0].kind = kind;
  batch.frames[0].registerAddress = address;
  batch.frames[0].value = writeValue;
  increment(_transport.totalBatches);
  const BatchResult result = _config.transport.executeBatch(_config.transport.context, batch, budget);
  _transport.lastBatch = result;
  _transport.lastBatchValid = true;
  _transport.violation = TransportViolation::NONE;
  _transport.late = false;
  uint64_t after = 0;
  const Status clockStatus = _sampleTime(after, upper);
  if (clockStatus.ok()) { _transport.late = after >= budget.deadlineUs; }

  auto violation = [this](TransportViolation reason) noexcept {
    if (_transport.violation == TransportViolation::NONE) { _transport.violation = reason; }
  };
  status = normalize(result.status, false);
  if (!validStatus(result.status, false)) {
    violation(result.status.msg == nullptr ? TransportViolation::NULL_MESSAGE : TransportViolation::STATUS_CODE);
  }
  Status release = normalize(result.releaseStatus, true);
  if (!validStatus(result.releaseStatus, true) ||
      (!result.releaseStatus.ok() && result.controllerReusable)) {
    violation(TransportViolation::RELEASE_REPORT);
    release = Status::Error(Err::MDIO_ERROR, "Invalid release report", result.releaseStatus.detail);
  }

  const auto& frame = result.frames[0];
  bool frameValid = true;
  if (!frame.attempted) { frameValid = isDefault(frame); }
  else {
    increment(_transport.attemptedFrames);
    const bool completionValid = frame.completed &&
        (kind == FrameKind::READ ? frame.valueValid : !frame.valueValid);
    // Timing is independent evidence: a bad optional stamp cannot erase a
    // separately valid physical completion, though it prevents publication.
    if (completionValid) { increment(_transport.completedFrames); }
    frameValid = (!frame.completed || completionValid) &&
                 (!frame.valueValid || (frame.completed && kind == FrameKind::READ)) &&
                 (!frame.timeValid || frame.completed) &&
                 (frame.timeValid || frame.completedAtUs == 0U);
    if (clockStatus.ok() && frame.timeValid &&
        (frame.completedAtUs < before || frame.completedAtUs > upper)) { frameValid = false; }
  }
  bool reportValid = frameValid;
  for (uint8_t i = 1; i < MAX_BATCH_FRAMES; ++i) {
    if (!isDefault(result.frames[i])) { reportValid = false; }
  }
  if (result.status.ok() && (!frame.attempted || !frame.completed)) { reportValid = false; }
  if (result.status.code == Err::BUSY && frame.attempted) { reportValid = false; }
  if (!reportValid) {
    violation(TransportViolation::FRAME_REPORT);
    status = Status::Error(Err::MDIO_ERROR, "Invalid frame report", result.status.detail);
  }

  const bool preexistingQuarantine = validStatus(result.status, false) && reportValid &&
      result.status.code == Err::BUSY &&
      result.status.detail == static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED) && !frame.attempted;
  if (release.ok() && !result.controllerReusable && !preexistingQuarantine) {
    violation(TransportViolation::CONTROLLER_STATE);
    release = Status::Error(Err::MDIO_ERROR, "Controller state uncertain");
  }
  // A malformed report escaping the trusted backend requires owner-wide shutdown.
  // A valid failure/release report instead uses that backend's shared quarantine.
  if (!validStatus(result.status, false) || !validStatus(result.releaseStatus, true) ||
      !reportValid || (!result.releaseStatus.ok() && result.controllerReusable)) { _stopped = true; }
  _transport.controllerReusable = result.controllerReusable && release.ok() && !_stopped;
  if (status.ok()) { status = release; }
  if (status.ok()) { status = clockStatus; }
  if (status.ok()) {
    if (_transport.late) { status = timeout(TimeoutDetail::DEADLINE); }
    else if (upper >= budget.deadlineUs ||
             _config.transport.bounds.overheadUs >= budget.deadlineUs - upper) {
      status = timeout(TimeoutDetail::BOUND_ADMISSION);
    }
  }
  _transport.lastStatus = status;
  if (status.ok() && kind == FrameKind::READ) { readValue = frame.value; }
  return status;
}

} // namespace DP8382x
