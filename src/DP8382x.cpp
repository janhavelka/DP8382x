/// @file DP8382x.cpp
/// @brief Local binding and complete terminal direct/indirect transactions.
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

// SNLS647I detailed tables 8-19..97. One canonical policy for direct/MMD aliases.
// This is a nonconsuming read policy, not a decoder or freshness assertion.
// Exclude latch/counter consumption, selectors, holes and CDCR's mandatory
// rescal recovery follow-up. Raw command readback does not establish readiness.
bool readable(uint32_t address) noexcept {
  switch (address) {
    case 0x00: case 0x02: case 0x03: case 0x04: case 0x05:
    case 0x07: case 0x08: case 0x09: case 0x0A: case 0x0B:
    case 0x11: case 0x16: case 0x18: case 0x19: case 0x1B:
    case 0x1C: case 0x1F:
    case 0x0025: case 0x0027: case 0x002A: case 0x0117: case 0x0131:
    case 0x0170: case 0x0173: case 0x0175: case 0x0176: case 0x0177:
    case 0x0178: case 0x0180: case 0x0185: case 0x018A: case 0x0218:
    case 0x0302: case 0x0303: case 0x0304: case 0x0305: case 0x0306:
    case 0x0308: case 0x030B: case 0x030C: case 0x030E: case 0x040D:
    case 0x0456: case 0x0460: case 0x0461: case 0x0467: case 0x0468:
    case 0x0469: case 0x04A0: case 0x04A2: case 0x04A3: case 0x04A4:
    case 0x04A5: case 0x04A6: case 0x04A7: return true;
    default: return false;
  }
}

// Table 8-42: independent stable LED0 controls only. All other mutations need
// their initialization/collector/typed procedure; aliasing grants no bypass.
uint16_t writableMask(uint32_t address) noexcept {
  return address == cmd::REG_LEDCR ? cmd::MASK_LEDCR_DIRECT : uint16_t{0};
}

Status mmdAddress(uint32_t devad, uint32_t address) noexcept {
  if (devad > 31U || address > 0xFFFFU) { return paramError(ParamDetail::ADDRESS_RANGE); }
  if (devad == 3U || devad == 7U) { return configError(ConfigDetail::SOURCE_GATE); }
  if (devad != cmd::MMD_VENDOR) { return paramError(ParamDetail::ACCESS_POLICY); }
  switch (address) {
    case 0x04D1: case 0x04D3: case 0x04DF: case 0x033E: case 0x033F:
    case 0x031B: case 0x0466: case 0x04CF: case 0x0416: case 0x04F5:
    case 0x04E0: case 0x031F: case 0x04F4: return configError(ConfigDetail::SOURCE_GATE);
    default: return Status::Ok();
  }
}

// SNLS647I 8.3.11.1/.4/.5: fresh complete nonincrementing selection per call.
// Whole words construct REGCR, so no reserved/uninitialized SDK bits can leak.
MdioBatch mmdBatch(uint16_t address, FrameKind kind, uint16_t value) noexcept {
  MdioBatch batch;
  batch.frameCount = 4;
  batch.frames[0] = {FrameKind::WRITE, cmd::REG_REGCR, cmd::MMD_VENDOR, 0, NO_READ_INDEX};
  batch.frames[1] = {FrameKind::WRITE, cmd::REG_ADDAR, address, 0, NO_READ_INDEX};
  batch.frames[2] = {FrameKind::WRITE, cmd::REG_REGCR, cmd::MMD_VENDOR_DATA, 0, NO_READ_INDEX};
  batch.frames[3] = {kind, cmd::REG_ADDAR, value, 0, NO_READ_INDEX};
  return batch;
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
  MdioBatch batch;
  batch.frameCount = 1;
  batch.frames[0].registerAddress = static_cast<uint8_t>(registerAddress);
  const Status status = _execute(batch, budget);
  if (status.ok()) { value = _transport.lastBatch.frames[0].value; }
  return status;
}

Status DP8382x::writeClause22(uint32_t registerAddress, uint32_t value,
                             const IoBudget& budget) noexcept {
  if (registerAddress > 31U) { return paramError(ParamDetail::ADDRESS_RANGE); }
  if (value > 0xFFFFU) { return paramError(ParamDetail::VALUE_WIDTH); }
  const auto allowed = writableMask(registerAddress);
  if (allowed == 0U || (value & ~uint32_t{allowed}) != 0U) {
    return paramError(ParamDetail::ACCESS_POLICY);
  }
  MdioBatch batch;
  batch.frameCount = 1;
  batch.frames[0] = {FrameKind::WRITE, static_cast<uint8_t>(registerAddress),
                     static_cast<uint16_t>(value), 0, NO_READ_INDEX};
  return _execute(batch, budget);
}

Status DP8382x::readMmd(uint32_t devad, uint32_t registerAddress, uint16_t& value,
                       const IoBudget& budget) noexcept {
  const Status addressStatus = mmdAddress(devad, registerAddress);
  if (!addressStatus.ok()) { return addressStatus; }
  if (!readable(registerAddress)) { return paramError(ParamDetail::ACCESS_POLICY); }
  const Status status = _execute(mmdBatch(static_cast<uint16_t>(registerAddress), FrameKind::READ, 0), budget);
  if (status.ok()) { value = _transport.lastBatch.frames[3].value; }
  return status;
}

Status DP8382x::writeMmd(uint32_t devad, uint32_t registerAddress, uint32_t value,
                        const IoBudget& budget) noexcept {
  if (devad > 31U || registerAddress > 0xFFFFU) { return paramError(ParamDetail::ADDRESS_RANGE); }
  if (value > 0xFFFFU) { return paramError(ParamDetail::VALUE_WIDTH); }
  const Status addressStatus = mmdAddress(devad, registerAddress);
  if (!addressStatus.ok()) { return addressStatus; }
  const auto allowed = writableMask(registerAddress);
  if (allowed == 0U || (value & ~uint32_t{allowed}) != 0U) { return paramError(ParamDetail::ACCESS_POLICY); }
  return _execute(mmdBatch(static_cast<uint16_t>(registerAddress), FrameKind::WRITE,
                           static_cast<uint16_t>(value)), budget);
}

Status DP8382x::modifyMmd(uint32_t devad, uint32_t registerAddress, uint32_t mask,
                         uint32_t value, const IoBudget& budget) noexcept {
  if (devad > 31U || registerAddress > 0xFFFFU) { return paramError(ParamDetail::ADDRESS_RANGE); }
  if (mask > 0xFFFFU || value > 0xFFFFU) { return paramError(ParamDetail::VALUE_WIDTH); }
  const Status addressStatus = mmdAddress(devad, registerAddress);
  if (!addressStatus.ok()) { return addressStatus; }
  const auto allowed = writableMask(registerAddress);
  if (!readable(registerAddress) || mask == 0U || (mask & ~uint32_t{allowed}) != 0U ||
      (value & ~mask) != 0U) { return paramError(ParamDetail::ACCESS_POLICY); }
  MdioBatch batch = mmdBatch(static_cast<uint16_t>(registerAddress), FrameKind::READ, 0);
  batch.frameCount = 5;
  // No postincrement: selection remains valid inside this same ownership scope.
  // Never preserve reserved bits, even if a backend returns poisoned read storage.
  batch.frames[4] = {FrameKind::WRITE, cmd::REG_ADDAR, static_cast<uint16_t>(value),
                     static_cast<uint16_t>(uint32_t{allowed} & ~mask), 3};
  return _execute(batch, budget);
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

Status DP8382x::_execute(MdioBatch batch, const IoBudget& budget) noexcept {
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
    // At most MAX_BATCH_FRAMES uint32_t frame bounds fit the uint64_t sum.
    const uint64_t bound = uint64_t{bounds.acquireUs} + uint64_t{batch.frameCount} * bounds.frameUs +
                           bounds.releaseUs + bounds.overheadUs;
    if (before >= budget.deadlineUs) { status = timeout(TimeoutDetail::DEADLINE); }
    else if (budget.maxFrames < batch.frameCount) { status = timeout(TimeoutDetail::FRAME_BUDGET); }
    else if (upper >= budget.deadlineUs || bound >= budget.deadlineUs - upper) {
      status = timeout(TimeoutDetail::BOUND_ADMISSION);
    }
  }
  if (!status.ok()) {
    _transport.lastStatus = status;
    return status;
  }

  batch.phyAddress = static_cast<uint8_t>(_config.phyAddress);
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

  bool reportValid = true;
  bool prefixOpen = true;
  uint8_t attempted = 0;
  uint8_t completed = 0;
  uint64_t previousStamp = before;
  for (uint8_t i = 0; i < MAX_BATCH_FRAMES; ++i) {
    const auto& frame = result.frames[i];
    if (i >= batch.frameCount || !prefixOpen || !frame.attempted) {
      if (!isDefault(frame)) { reportValid = false; }
      prefixOpen = false;
      continue;
    }
    ++attempted;
    increment(_transport.attemptedFrames);
    const auto kind = batch.frames[i].kind;
    const bool completionValid = frame.completed &&
        (kind == FrameKind::READ ? frame.valueValid : !frame.valueValid);
    // Timing is independent evidence: a bad optional stamp cannot erase a
    // separately valid physical completion, though it prevents publication.
    if (completionValid) { ++completed; increment(_transport.completedFrames); }
    const bool frameValid = (!frame.completed || completionValid) &&
                 (!frame.valueValid || (frame.completed && kind == FrameKind::READ)) &&
                 (!frame.timeValid || frame.completed) &&
                 (frame.timeValid || frame.completedAtUs == 0U);
    if (!frameValid) { reportValid = false; }
    if (clockStatus.ok() && frame.timeValid) {
      if (frame.completedAtUs < previousStamp || frame.completedAtUs > upper) { reportValid = false; }
      else { previousStamp = frame.completedAtUs; }
    }
    // A failed/missing read cannot authorize later dependent writes. Retain the
    // raw remainder for diagnosis, but do not invent a valid physical prefix.
    if (!completionValid) { prefixOpen = false; }
  }
  if (result.status.ok() && completed != batch.frameCount) { reportValid = false; }
  if (result.status.code == Err::BUSY && attempted != 0U) { reportValid = false; }
  if (!reportValid) {
    violation(TransportViolation::FRAME_REPORT);
    status = Status::Error(Err::MDIO_ERROR, "Invalid frame report", result.status.detail);
  }

  const bool preexistingQuarantine = validStatus(result.status, false) && reportValid &&
      result.status.code == Err::BUSY &&
      result.status.detail == static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED) && attempted == 0U;
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
  return status;
}

} // namespace DP8382x
