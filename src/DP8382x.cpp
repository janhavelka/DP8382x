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
  return Status::Error(Err::INVALID_PARAM, "Invalid call argument",
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

// Exact-part revision admission, not a family-wide or caller-configurable filter.
// SNLS647I tables 8-21/22 and TI dp83826.c revision filter; new revisions need review.
struct AcceptedRevision { Variant variant; uint8_t revision; };
constexpr AcceptedRevision ACCEPTED_REVISIONS[] = {{Variant::DP83826I, 1}};

Status assessIdentity(const DeviceIdentity& identity, OperatingMode required) noexcept {
  auto reject = [](Err code, IdentityDetail detail) noexcept {
    return Status::Error(code, "Identity not accepted", static_cast<int32_t>(detail));
  };
  if (identity.id1 == 0U || identity.id2 == 0U) {
    return reject(Err::DEVICE_NOT_FOUND, IdentityDetail::ZERO_ID);
  }
  if (identity.id1 == 0xFFFFU || identity.id2 == 0xFFFFU) {
    return reject(Err::DEVICE_NOT_FOUND, IdentityDetail::ONES_ID);
  }
  if (identity.oui != 0x080028U) { return reject(Err::DEVICE_MISMATCH, IdentityDetail::WRONG_OUI); }
  if (identity.mode == OperatingMode::UNKNOWN) { return reject(Err::DEVICE_MISMATCH, IdentityDetail::WRONG_MODEL); }
  bool revisionAccepted = false;
  for (const auto& row : ACCEPTED_REVISIONS) {
    if (row.variant == identity.declaredVariant && row.revision == identity.revision) { revisionAccepted = true; }
  }
  if (!revisionAccepted) { return reject(Err::DEVICE_MISMATCH, IdentityDetail::REVISION_NOT_ACCEPTED); }
  if (required != OperatingMode::UNKNOWN && required != identity.mode) {
    return reject(Err::DEVICE_MISMATCH, IdentityDetail::MODE_MISMATCH);
  }
  return Status::Ok();
}

bool isDefault(const FrameResult& frame) noexcept {
  return !frame.attempted && !frame.completed && !frame.valueValid &&
         frame.value == 0U && !frame.effectPossible && frame.completedAtUs == 0U &&
         !frame.timeValid;
}

// SNLS647I detailed tables 8-19..97; policy only. The ledger owns field facts.
enum class Access : uint8_t { SAFE_READ, CONSUMING, SELECTOR, RESERVED_ONLY, FOLLOW_UP, PROCEDURE };
struct RegisterPolicy {
  uint16_t address;
  Access access;
  uint16_t writableMask;
  uint16_t preserveMask;
};
constexpr RegisterPolicy REGISTER_POLICY[] = {
  {0x0411, Access::PROCEDURE, 0, 0}, // TDR_0411
  {0x0000, Access::SAFE_READ, 0, 0}, // BMCR
  {0x0001, Access::CONSUMING, 0, 0}, // BMSR
  {0x0002, Access::SAFE_READ, 0, 0}, // PHYIDR1
  {0x0003, Access::SAFE_READ, 0, 0}, // PHYIDR2
  {0x0004, Access::SAFE_READ, 0, 0}, // ANAR
  {0x0005, Access::SAFE_READ, 0, 0}, // ALNPAR
  {0x0006, Access::CONSUMING, 0, 0}, // ANER
  {0x0007, Access::SAFE_READ, 0, 0}, // ANNPTR
  {0x0008, Access::SAFE_READ, 0, 0}, // ANLNPTR
  {0x0009, Access::SAFE_READ, 0, 0}, // CR1
  {0x000A, Access::SAFE_READ, 0, 0}, // CR2
  {0x000B, Access::SAFE_READ, 0, 0}, // CR3
  {0x000D, Access::SELECTOR, 0, 0}, // REGCR
  {0x000E, Access::SELECTOR, 0, 0}, // ADDAR
  {0x000F, Access::CONSUMING, 0, 0}, // FLDS
  {0x0010, Access::CONSUMING, 0, 0}, // PHYSTS
  {0x0011, Access::SAFE_READ, 0, 0}, // PHYSCR
  {0x0012, Access::CONSUMING, 0, 0}, // MISR1
  {0x0013, Access::CONSUMING, 0, 0}, // MISR2
  {0x0014, Access::CONSUMING, 0, 0}, // FCSCR
  {0x0015, Access::CONSUMING, 0, 0}, // RECR
  {0x0016, Access::SAFE_READ, 0, 0}, // BISCR
  {0x0017, Access::CONSUMING, 0, 0}, // RCSR
  {0x0018, Access::SAFE_READ, 0x0692, 0x0692}, // LEDCR
  {0x0019, Access::SAFE_READ, 0, 0}, // PHYCR
  {0x001A, Access::CONSUMING, 0, 0}, // 10BTSCR
  {0x001B, Access::SAFE_READ, 0, 0}, // BICSR1
  {0x001C, Access::SAFE_READ, 0, 0}, // BICSR2
  {0x001E, Access::FOLLOW_UP, 0, 0}, // CDCR
  {0x001F, Access::SAFE_READ, 0, 0}, // PHYRCR
  {0x0025, Access::SAFE_READ, 0, 0}, // MLEDCR
  {0x0027, Access::SAFE_READ, 0, 0}, // COMPT
  {0x002A, Access::SAFE_READ, 0, 0}, // 10M_CFG
  {0x0117, Access::SAFE_READ, 0, 0}, // FLD_CFG1
  {0x0131, Access::SAFE_READ, 0, 0}, // FLD_CFG2
  {0x0170, Access::SAFE_READ, 0, 0}, // CDSCR
  {0x0171, Access::RESERVED_ONLY, 0, 0}, // CDSCR2
  {0x0173, Access::SAFE_READ, 0, 0}, // CDSCR3
  {0x0175, Access::SAFE_READ, 0, 0}, // TDR_175
  {0x0176, Access::SAFE_READ, 0, 0}, // TDR_176
  {0x0177, Access::SAFE_READ, 0, 0}, // CDSCR4
  {0x0178, Access::SAFE_READ, 0, 0}, // TDR_178
  {0x0180, Access::SAFE_READ, 0, 0}, // CDLRR1
  {0x0181, Access::RESERVED_ONLY, 0, 0}, // CDLRR2
  {0x0182, Access::RESERVED_ONLY, 0, 0}, // CDLRR3
  {0x0183, Access::RESERVED_ONLY, 0, 0}, // CDLRR4
  {0x0184, Access::RESERVED_ONLY, 0, 0}, // CDLRR5
  {0x0185, Access::SAFE_READ, 0, 0}, // CDLAR1
  {0x0186, Access::RESERVED_ONLY, 0, 0}, // CDLAR2
  {0x0187, Access::RESERVED_ONLY, 0, 0}, // CDLAR3
  {0x0188, Access::RESERVED_ONLY, 0, 0}, // CDLAR4
  {0x0189, Access::RESERVED_ONLY, 0, 0}, // CDLAR5
  {0x018A, Access::SAFE_READ, 0, 0}, // CDLAR6
  {0x0218, Access::SAFE_READ, 0, 0}, // MSE_Val
  {0x0302, Access::SAFE_READ, 0, 0}, // IO_CFG1
  {0x0303, Access::SAFE_READ, 0, 0}, // LED0_GPIO_CFG
  {0x0304, Access::SAFE_READ, 0, 0}, // LED1_GPIO_CFG
  {0x0305, Access::SAFE_READ, 0, 0}, // LED2_GPIO_CFG
  {0x0306, Access::SAFE_READ, 0, 0}, // LED3_GPIO_CFG
  {0x0308, Access::SAFE_READ, 0, 0}, // CLK_OUT_LED_STATUS
  {0x030B, Access::SAFE_READ, 0, 0}, // VOD_CFG1
  {0x030C, Access::SAFE_READ, 0, 0}, // VOD_CFG2
  {0x030E, Access::SAFE_READ, 0, 0}, // VOD_CFG3
  {0x0404, Access::RESERVED_ONLY, 0, 0}, // ANA_LD_PROG_SL
  {0x040D, Access::SAFE_READ, 0, 0}, // ANA_RX10BT_CTRL
  {0x0456, Access::SAFE_READ, 0, 0}, // GENCFG
  {0x0460, Access::SAFE_READ, 0, 0}, // LEDCFG
  {0x0461, Access::SAFE_READ, 0, 0}, // IOCTRL
  {0x0467, Access::SAFE_READ, 0, 0}, // SOR1
  {0x0468, Access::SAFE_READ, 0, 0}, // SOR2
  {0x0469, Access::SAFE_READ, 0, 0}, // LEDCFG2
  {0x04A0, Access::SAFE_READ, 0, 0}, // RXFCFG1
  {0x04A1, Access::CONSUMING, 0, 0}, // RXFS
  {0x04A2, Access::SAFE_READ, 0, 0}, // RXFPMD1
  {0x04A3, Access::SAFE_READ, 0, 0}, // RXFPMD2
  {0x04A4, Access::SAFE_READ, 0, 0}, // RXFPMD3
  {0x04A5, Access::SAFE_READ, 0, 0}, // RXFSOP1
  {0x04A6, Access::SAFE_READ, 0, 0}, // RXFSOP2
  {0x04A7, Access::SAFE_READ, 0, 0}, // RXFSOP3
  {0x04D1, Access::PROCEDURE, 0, 0}, // EEECFG3
  {0x04D3, Access::PROCEDURE, 0, 0}, // EEE_04D3
  {0x04DF, Access::PROCEDURE, 0, 0}, // EEE_04DF
  {0x033E, Access::PROCEDURE, 0, 0}, // EEE_033E
  {0x033F, Access::PROCEDURE, 0, 0}, // EEE_033F
  {0x0123, Access::PROCEDURE, 0, 0}, // RECIPE_0123
  {0x031B, Access::PROCEDURE, 0, 0}, // EEE_031B
  {0x0466, Access::PROCEDURE, 0, 0}, // EEE_0466
  {0x04CF, Access::PROCEDURE, 0, 0}, // EEE_04CF
  {0x0416, Access::PROCEDURE, 0, 0}, // EEE_0416
  {0x04F5, Access::PROCEDURE, 0, 0}, // EEE_04F5
  {0x04E0, Access::PROCEDURE, 0, 0}, // EEE_04E0
  {0x031F, Access::PROCEDURE, 0, 0}, // EEE_031F
  {0x04F4, Access::PROCEDURE, 0, 0}, // EEE_04F4
  {0x0428, Access::PROCEDURE, 0, 0}, // RECIPE_0428
  {0x0122, Access::PROCEDURE, 0, 0}, // LOOP_0122
  {0x0130, Access::PROCEDURE, 0, 0}, // LOOP_0130
};
constexpr bool validPolicy() noexcept {
  for (const auto& row : REGISTER_POLICY) {
    if (row.writableMask != 0U && row.access != Access::SAFE_READ) { return false; }
  }
  return true;
}
static_assert(validPolicy(), "Raw RMW must not consume evidence");
const RegisterPolicy* registerPolicy(uint32_t address) noexcept {
  for (const auto& row : REGISTER_POLICY) {
    if (row.address == address) { return &row; }
  }
  return nullptr;
}
bool readable(uint32_t address) noexcept {
  const auto* row = registerPolicy(address);
  return row != nullptr && row->access == Access::SAFE_READ;
}
uint16_t writableMask(uint32_t address) noexcept {
  const auto* row = registerPolicy(address);
  return row == nullptr ? uint16_t{0} : row->writableMask;
}
Status checkMmdCall(uint32_t devad, uint32_t address, bool widthOk = true) noexcept {
  if (devad > 31U || address > 0xFFFFU) { return paramError(ParamDetail::ADDRESS_RANGE); }
  if (!widthOk) { return paramError(ParamDetail::VALUE_WIDTH); }
  if (devad != 3U && devad != 7U && devad != cmd::DEVAD_VENDOR) { return paramError(ParamDetail::ACCESS_POLICY); }
  return Status::Ok();
}
MdioBatch directBatch(uint8_t address, FrameKind kind, uint16_t value = 0) noexcept {
  MdioBatch batch;
  batch.frameCount = 1;
  batch.frames[0] = {kind, address, value, 0, NO_READ_INDEX};
  return batch;
}

// SNLS647I 8.3.11.1/.4/.5: fresh complete nonincrementing selection per call.
// Whole words construct REGCR, so no reserved/uninitialized SDK bits can leak.
MdioBatch mmdBatch(uint16_t address, FrameKind kind, uint16_t value, uint16_t devad = cmd::DEVAD_VENDOR) noexcept {
  MdioBatch batch;
  batch.frameCount = 4;
  batch.frames[0] = {FrameKind::WRITE, cmd::REG_REGCR, devad, 0, NO_READ_INDEX};
  batch.frames[1] = {FrameKind::WRITE, cmd::REG_ADDAR, address, 0, NO_READ_INDEX};
  batch.frames[2] = {FrameKind::WRITE, cmd::REG_REGCR, static_cast<uint16_t>(0x4000U | devad), 0, NO_READ_INDEX};
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

struct FrameAudit { uint8_t attempted; uint8_t completed; bool valid; };
FrameAudit auditFrames(const MdioBatch& batch, const BatchResult& result,
                       bool clockValid, uint64_t before, uint64_t upper) noexcept {
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
    const auto kind = batch.frames[i].kind;
    const bool completionValid = frame.completed &&
        (kind == FrameKind::READ ? frame.valueValid : !frame.valueValid);
    // Timing is independent evidence: a bad optional stamp cannot erase a
    // separately valid physical completion, though it prevents publication.
    if (completionValid) { ++completed; }
    const bool frameValid = (!frame.completed || completionValid) &&
                 (!frame.valueValid || (frame.completed && kind == FrameKind::READ)) &&
                 (!frame.timeValid || frame.completed) &&
                 (frame.timeValid || frame.completedAtUs == 0U);
    if (!frameValid) { reportValid = false; }
    if (clockValid && frame.timeValid) {
      if (frame.completedAtUs < previousStamp || frame.completedAtUs > upper) { reportValid = false; }
      else { previousStamp = frame.completedAtUs; }
    }
    // A failed/missing read cannot authorize later dependent writes. Retain the
    // raw remainder for diagnosis, but do not invent a valid physical prefix.
    if (!completionValid) { prefixOpen = false; }
  }
  if (result.status.ok() && completed != batch.frameCount) { reportValid = false; }
  if (result.status.code == Err::BUSY && attempted != 0U) { reportValid = false; }
  return {attempted, completed, reportValid};
}
struct ReportAudit { Status primary; Status release; TransportViolation violation; };
ReportAudit classifyReport(const BatchResult& result, const FrameAudit& frames) noexcept {
  ReportAudit out{normalize(result.status, false), normalize(result.releaseStatus, true), TransportViolation::NONE};
  const auto defect = [&](TransportViolation value) { if (out.violation == TransportViolation::NONE) { out.violation = value; } };
  if (!validStatus(result.status, false)) {
    defect(result.status.msg == nullptr ? TransportViolation::NULL_MESSAGE : TransportViolation::STATUS_CODE);
  }
  const bool quarantine = validStatus(result.status, false) && frames.valid &&
      result.status.is(Err::BUSY) && result.status.detail == static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED) && frames.attempted == 0U;
  if (!validStatus(result.releaseStatus, true) ||
      ((!result.releaseStatus.ok() || quarantine) && result.controllerReusable)) {
    defect(TransportViolation::RELEASE_REPORT);
    out.release = Status::Error(Err::MDIO_ERROR, "Invalid release report", result.releaseStatus.detail);
  }
  if (!frames.valid) {
    defect(TransportViolation::FRAME_REPORT);
    out.primary = Status::Error(Err::MDIO_ERROR, "Invalid frame report", result.status.detail);
  }
  if (out.release.ok() && !result.controllerReusable && !quarantine) {
    defect(TransportViolation::CONTROLLER_STATE);
    out.release = Status::Error(Err::MDIO_ERROR, "Controller state uncertain");
  }
  return out;
}
Status admitBudget(const IoBudget& budget, const TransportBounds& bounds,
                   uint8_t frames, uint64_t raw, uint64_t upper) noexcept {
  const uint64_t bound = uint64_t{bounds.fixedUs} + uint64_t{frames} * bounds.frameUs;
  if (raw >= budget.deadlineUs) { return timeout(TimeoutDetail::DEADLINE); }
  if (budget.maxFrames < frames) { return timeout(TimeoutDetail::FRAME_BUDGET); }
  if (upper >= budget.deadlineUs || bound >= budget.deadlineUs - upper) { return timeout(TimeoutDetail::BOUND_ADMISSION); }
  return Status::Ok();
}

// OP-6: reserve both bounded transfers, quantization and one scheduled wait.
Status admitReset(const IoBudget& budget, const TransportBounds& bounds,
                  uint64_t raw, uint32_t interval) noexcept {
  const uint64_t reserve = uint64_t{4} * bounds.clockLagUs +
      uint64_t{2} * (uint64_t{bounds.fixedUs} + bounds.frameUs) + interval;
  if (budget.maxFrames < 2U || raw >= budget.deadlineUs ||
      reserve >= budget.deadlineUs - raw) { return timeout(TimeoutDetail::BOUND_ADMISSION); }
  return Status::Ok();
}

// Acquisition order captures PHYSTS mirrors before their clearing sources.
constexpr uint16_t EVIDENCE_ADDRESSES[] = {
  0x0010, 0x0001, 0x0006, 0x000F, 0x0012, 0x0013,
  0x0014, 0x0015, 0x0017, 0x001A, 0x04A1
};
int evidenceIndex(uint16_t address) noexcept {
  for (uint8_t i = 0; i < static_cast<uint8_t>(EvidenceSource::COUNT); ++i) {
    if (EVIDENCE_ADDRESSES[i] == address) { return i; }
  }
  return -1;
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
       config.requiredMode != OperatingMode::ENHANCED)) {
    return configError(ConfigDetail::ENUM_VALUE);
  }
  if (!isValidPhyAddress(config.phyAddress)) { return configError(ConfigDetail::ADDRESS_RANGE); }
  const auto& transport = config.transport;
  if (transport.executeBatch == nullptr || transport.nowUs == nullptr ||
      transport.bounds.frameUs == 0U || transport.bounds.fixedUs == 0U ||
      transport.bounds.clockLagUs == 0U) { return configError(ConfigDetail::TRANSPORT); }
  if (config.offlineThreshold == 0U) { return configError(ConfigDetail::HEALTH_THRESHOLD); }
  if (config.interface == MacInterface::UNKNOWN) { return configError(ConfigDetail::INTERFACE_REQUIRED); }
  if (config.interface == MacInterface::RGMII) {
    return Status::Error(Err::UNSUPPORTED, "DP83826I has no RGMII",
                         static_cast<int32_t>(UnsupportedDetail::INTERFACE));
  }
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return busy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state == OperationState::RESULT_PENDING) { return busy(BusyDetail::RESULT_PENDING); }
  if (_operation.cleanupOwed) { return busy(BusyDetail::CLEANUP_REQUIRED); }
  _config = config;
  _transport = {};
  _evidence = {};
  _identity = {};
  _identity.declaredVariant = config.variant;
  _timeValid = false;
  _lastTimeUs = 0;
  _stopped = false;
  _bound = true;
  _initialized = false;
  _quietPending = false;
  _operation.busQuietValid = false;
  _operation.busQuietUntilUs = 0;
  return Status::Ok();
}

Status DP8382x::end() noexcept {
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return busy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state == OperationState::RESULT_PENDING) { return busy(BusyDetail::RESULT_PENDING); }
  if (_operation.cleanupOwed) { return busy(BusyDetail::CLEANUP_REQUIRED); }
  _config = {};
  _transport = {};
  _evidence = {};
  _identity = {};
  _timeValid = false;
  _lastTimeUs = 0;
  _stopped = false;
  _bound = false;
  _initialized = false;
  _quietPending = false;
  _operation.busQuietValid = false;
  _operation.busQuietUntilUs = 0;
  return Status::Ok();
}

Status DP8382x::readClause22(uint32_t registerAddress, uint16_t& value,
                            const IoBudget& budget) noexcept {
  if (registerAddress > 31U) { return paramError(ParamDetail::ADDRESS_RANGE); }
  if (!readable(registerAddress)) { return paramError(ParamDetail::ACCESS_POLICY); }
  const auto batch = directBatch(static_cast<uint8_t>(registerAddress), FrameKind::READ);
  const Status status = _execute(batch, budget);
  if (status.ok()) { value = _transport.lastBatch.frames[0].value; }
  return status;
}

Status DP8382x::readIdentity(DeviceIdentity& identity, const IoBudget& budget) noexcept {
  return _readIdentity(identity, budget, false);
}

Status DP8382x::_readIdentity(DeviceIdentity& identity, const IoBudget& budget, bool operation, bool* invoked) noexcept {
  auto batch = directBatch(cmd::REG_PHYIDR1, FrameKind::READ);
  batch.frameCount = 2;
  batch.frames[1] = {FrameKind::READ, cmd::REG_PHYIDR2, 0, 0, NO_READ_INDEX};
  bool called = false;
  Status status = _execute(batch, budget, &called, operation);
  if (invoked != nullptr) { *invoked = called; }
  if (!called) { return status; } // Local admission never reuses an old receipt.
  const auto& result = _transport.lastBatch;
  if ((status.code == Err::BUSY || status.code == Err::OPERATION_TIMEOUT) &&
      _transport.violation == TransportViolation::NONE &&
      result.releaseStatus.ok() &&
      !result.frames[0].attempted) { return status; }

  DeviceIdentity observed;
  observed.declaredVariant = _config.variant;
  const auto& first = result.frames[0];
  const auto& second = result.frames[1];
  observed.id1Valid = first.attempted && first.completed && first.valueValid;
  observed.id2Valid = observed.id1Valid && second.attempted && second.completed && second.valueValid;
  if (observed.id1Valid) { observed.id1 = first.value; }
  if (observed.id2Valid) { observed.id2 = second.value; }
  if (observed.id2Valid) {
    // Numeric namespaces from SNLS647I tables 8-21/22; suffix is never decoded.
    observed.oui = (uint32_t{observed.id1} << 6U) | ((observed.id2 & 0xFC00U) >> 10U);
    observed.model = static_cast<uint8_t>((observed.id2 & 0x03F0U) >> 4U);
    observed.revision = static_cast<uint8_t>(observed.id2 & 0x000FU);
    observed.mode = observed.model == 0x11U ? OperatingMode::BASIC :
                    observed.model == 0x13U ? OperatingMode::ENHANCED : OperatingMode::UNKNOWN;
    observed.decodedValid = true;
    if (!_transport.clockInvalid && _transport.violation == TransportViolation::NONE) {
      observed.observedAtUs = _lastTimeUs + _config.transport.bounds.clockLagUs;
      observed.timeValid = true;
    }
  }
  if (status.ok()) {
    status = assessIdentity(observed, _config.requiredMode);
    observed.acceptance = status.ok() ? IdentityAcceptance::ACCEPTED : IdentityAcceptance::REJECTED;
    observed.stale = false;
  }
  observed.admissionStatus = status;
  if (!status.ok() || (_identity.decodedValid && observed.decodedValid &&
      (_identity.id1 != observed.id1 || _identity.id2 != observed.id2))) {
    _invalidateEvidence(); // Old device-state observations cannot describe a different identity.
  }
  _identity = observed;
  if (!operation && _operation.state == OperationState::RUNNING &&
      (!status.ok() || (_result.identity.decodedValid &&
       (_result.identity.id1 != observed.id1 || _result.identity.id2 != observed.id2)))) {
    _result.identity.stale = true;
    _finishOperation(configError(ConfigDetail::DEVICE_INVALIDATED));
  }
  if (status.ok()) { identity = observed; }
  return status;
}

Capabilities DP8382x::capabilities() const noexcept {
  if (!_bound || _identity.stale || _identity.acceptance != IdentityAcceptance::ACCEPTED) { return {}; }
  // SNLS647I table 4-3 and 8.4.1: both identity modes support MII and RMII.
  // No fixture availability or incomplete typed-feature implementation is inferred.
  return {true, LINK_TECHNOLOGY_MASK, INTERFACE_MII | INTERFACE_RMII, true, false, false};
}

Status DP8382x::_checkMutation() noexcept {
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return busy(BusyDetail::OPERATION_ACTIVE); }
  const auto quiet = _checkQuiet();
  if (!quiet.ok()) { return quiet; }
  if (_operation.state == OperationState::RESULT_PENDING) { return busy(BusyDetail::RESULT_PENDING); }
  if (_operation.cleanupOwed) { return busy(BusyDetail::CLEANUP_REQUIRED); }
  // Existing maintenance exceptions allow owner-attested use before probing.
  // They must not override a failed/rejected probe. Stale successful identity
  // still permits maintenance, but never supplies usable typed capabilities.
  const auto status = _identity.admissionStatus;
  return status.code == Err::NOT_INITIALIZED ? Status::Ok() : status;
}

Status DP8382x::writeClause22(uint32_t registerAddress, uint32_t value,
                             const IoBudget& budget) noexcept {
  if (registerAddress > 31U) { return paramError(ParamDetail::ADDRESS_RANGE); }
  if (value > 0xFFFFU) { return paramError(ParamDetail::VALUE_WIDTH); }
  const auto allowed = writableMask(registerAddress);
  if (allowed == 0U || (value & ~uint32_t{allowed}) != 0U) {
    return paramError(ParamDetail::ACCESS_POLICY);
  }
  const auto admission = _checkMutation();
  if (!admission.ok()) { return admission; }
  const auto batch = directBatch(static_cast<uint8_t>(registerAddress), FrameKind::WRITE,
                                 static_cast<uint16_t>(value));
  return _execute(batch, budget);
}

Status DP8382x::readMmd(uint32_t devad, uint32_t registerAddress, uint16_t& value,
                       const IoBudget& budget) noexcept {
  const Status addressStatus = checkMmdCall(devad, registerAddress);
  if (!addressStatus.ok()) { return addressStatus; }
  bool admitted = devad == cmd::DEVAD_VENDOR && readable(registerAddress);
  for (const auto& row : cmd::EEE_REGISTERS) {
    if (row.devad == devad && row.address == registerAddress && row.safeRead) { admitted = true; }
  }
  if (!admitted) { return paramError(ParamDetail::ACCESS_POLICY); }
  const Status status = _execute(mmdBatch(static_cast<uint16_t>(registerAddress), FrameKind::READ, 0, static_cast<uint16_t>(devad)), budget);
  if (status.ok()) { value = _transport.lastBatch.frames[3].value; }
  return status;
}

Status DP8382x::writeMmd(uint32_t devad, uint32_t registerAddress, uint32_t value,
                        const IoBudget& budget) noexcept {
  const Status addressStatus = checkMmdCall(devad, registerAddress, value <= 0xFFFFU);
  if (!addressStatus.ok()) { return addressStatus; }
  const auto allowed = devad == cmd::DEVAD_VENDOR ? writableMask(registerAddress) : uint16_t{0};
  if (allowed == 0U || (value & ~uint32_t{allowed}) != 0U) { return paramError(ParamDetail::ACCESS_POLICY); }
  const auto admission = _checkMutation();
  if (!admission.ok()) { return admission; }
  return _execute(mmdBatch(static_cast<uint16_t>(registerAddress), FrameKind::WRITE,
                           static_cast<uint16_t>(value)), budget);
}

Status DP8382x::modifyMmd(uint32_t devad, uint32_t registerAddress, uint32_t mask,
                         uint32_t value, const IoBudget& budget) noexcept {
  const Status addressStatus = checkMmdCall(devad, registerAddress, mask <= 0xFFFFU && value <= 0xFFFFU);
  if (!addressStatus.ok()) { return addressStatus; }
  const auto allowed = devad == cmd::DEVAD_VENDOR ? writableMask(registerAddress) : uint16_t{0};
  if (!readable(registerAddress) || mask == 0U || (mask & ~uint32_t{allowed}) != 0U ||
      (value & ~mask) != 0U) { return paramError(ParamDetail::ACCESS_POLICY); }
  const auto admission = _checkMutation();
  if (!admission.ok()) { return admission; }
  MdioBatch batch = mmdBatch(static_cast<uint16_t>(registerAddress), FrameKind::READ, 0);
  batch.frameCount = 5;
  // Five frames inferred from 8.3.11.1 state retention; physical check pending.
  // LEDCR forces reserved zero; other targets use their per-register preserve policy.
  batch.frames[4] = {FrameKind::WRITE, cmd::REG_ADDAR, static_cast<uint16_t>(value),
                     static_cast<uint16_t>(uint32_t{registerPolicy(registerAddress)->preserveMask} & ~mask), 3};
  return _execute(batch, budget);
}

void DP8382x::_invalidateEvidence() noexcept {
  _initialized = false;
  _evidence.configurationDirty = true;
  _identity.stale = true;
  for (auto& sample : _evidence.samples) { sample.stale = true; }
}

Status DP8382x::invalidateDeviceState(InvalidationReason reason) noexcept {
  if (reason > InvalidationReason::AMBIGUOUS_MUTATION) { return paramError(ParamDetail::ENUM_VALUE); }
  if (_active || _polling) { return busy(BusyDetail::OPERATION_ACTIVE); }
  _invalidateEvidence();
  if (_operation.state == OperationState::RUNNING) {
    _result.identity.stale = true;
    _result.straps.stale = true;
    _result.straps.timeValid = false;
    _finishOperation(configError(ConfigDetail::DEVICE_INVALIDATED));
    if (reason == InvalidationReason::EXTERNAL_RESET || reason == InvalidationReason::EXTERNAL_POWER_ON) {
      _operation.cleanupOwed = false;
      _result.cleanupOwed = false;
    }
  } else if (reason == InvalidationReason::EXTERNAL_RESET || reason == InvalidationReason::EXTERNAL_POWER_ON) {
    _operation.cleanupOwed = false;
  }
  return Status::Ok();
}

Status DP8382x::acquireEvidence(uint32_t sourceMask, EvidenceSnapshot& evidence,
                               const IoBudget& budget) noexcept {
  if (sourceMask == 0U || (sourceMask & ~ALL_EVIDENCE) != 0U) { return paramError(ParamDetail::ACCESS_POLICY); }
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || _polling || (_operation.state == OperationState::RUNNING &&
      _operation.phase == OperationPhase::WAIT)) { return busy(BusyDetail::OPERATION_ACTIVE); }
  uint32_t required = 0;
  for (uint8_t i = 0; i < static_cast<uint8_t>(EvidenceSource::COUNT); ++i) {
    if ((sourceMask & (uint32_t{1} << i)) != 0U) { required += EVIDENCE_ADDRESSES[i] > 31U ? 4U : 1U; }
  }
  if (budget.maxFrames < required) { return timeout(TimeoutDetail::FRAME_BUDGET); }
  // Do not mark cached samples stale when shared hard-reset silence denies I/O.
  const auto quiet = _checkQuiet();
  if (!quiet.ok()) { return quiet; }
  _evidence.lastRequested = sourceMask;
  // Mark selected old samples historical before any partial refresh.
  for (uint8_t i = 0; i < static_cast<uint8_t>(EvidenceSource::COUNT); ++i) {
    if ((sourceMask & (uint32_t{1} << i)) != 0U) { _evidence.samples[i].stale = true; }
  }
  IoBudget remaining = budget;
  MdioBatch batch;
  auto run = [&]() noexcept {
    const uint32_t count = batch.frameCount;
    const Status result = _execute(batch, remaining);
    if (result.ok()) { remaining.maxFrames -= count; }
    batch = {};
    return result;
  };
  for (uint8_t i = 0; i < static_cast<uint8_t>(EvidenceSource::COUNT); ++i) {
    if ((sourceMask & (uint32_t{1} << i)) == 0U) { continue; }
    const auto address = EVIDENCE_ADDRESSES[i];
    const auto next = address > 31U ? mmdBatch(address, FrameKind::READ, 0) :
                                     directBatch(static_cast<uint8_t>(address), FrameKind::READ);
    if (batch.frameCount + next.frameCount > MAX_BATCH_FRAMES) {
      const auto status = run();
      if (!status.ok()) { _evidence.lastStatus = status; return status; }
    }
    for (uint8_t n = 0; n < next.frameCount; ++n) { batch.frames[batch.frameCount++] = next.frames[n]; }
  }
  const auto status = run();
  _evidence.lastStatus = status;
  if (status.ok()) { evidence = _evidence; }
  return status;
}

Status DP8382x::modifyInterruptEnables(uint32_t address, uint32_t mask, uint32_t value,
                                      const IoBudget& budget) noexcept {
  if (address != 0x12U && address != 0x13U) { return paramError(ParamDetail::ACCESS_POLICY); }
  if (mask > 0xFFFFU || value > 0xFFFFU) { return paramError(ParamDetail::VALUE_WIDTH); }
  if (mask == 0U || (mask & ~uint32_t{0x00FF}) != 0U || (value & ~mask) != 0U) {
    return paramError(ParamDetail::ACCESS_POLICY);
  }
  const auto admission = _checkMutation();
  if (!admission.ok()) { return admission; }
  auto batch = directBatch(static_cast<uint8_t>(address), FrameKind::READ);
  batch.frameCount = 2;
  batch.frames[1] = {FrameKind::WRITE, static_cast<uint8_t>(address), static_cast<uint16_t>(value),
                     static_cast<uint16_t>(0x00FFU & ~mask), 0};
  const auto status = _execute(batch, budget);
  _evidence.lastStatus = status;
  return status;
}

Status DP8382x::modifyWakeControl(uint32_t mask, uint32_t value, bool acknowledge,
                                 const IoBudget& budget) noexcept {
  if (mask > 0xFFFFU || value > 0xFFFFU) { return paramError(ParamDetail::VALUE_WIDTH); }
  if ((mask == 0U && !acknowledge) || (mask & ~uint32_t{0x17A1}) != 0U || (value & ~mask) != 0U ||
      ((mask & 0x0600U) != 0U && (mask & 0x0600U) != 0x0600U)) {
    return paramError(ParamDetail::ACCESS_POLICY);
  }
  // No-change skipping needs a stable comparison; the semantic owner excludes
  // external configuration writers through the whole call. Actual RMW is atomic.
  const auto admission = _checkMutation();
  if (!admission.ok()) { return admission; }
  if (budget.maxFrames < 14U) { return timeout(TimeoutDetail::FRAME_BUDGET); }
  uint16_t sampled = 0;
  auto status = readMmd(31, 0x04A0, sampled, budget);
  if (!status.ok()) { return status; }
  if (!acknowledge && (sampled & mask) == value) { return Status::Ok(); }
  IoBudget remaining{budget.deadlineUs, budget.maxFrames - 4U};
  EvidenceSnapshot captured;
  status = acquireEvidence(evidenceBit(EvidenceSource::MISR2) | evidenceBit(EvidenceSource::RXFS), captured, remaining);
  if (!status.ok()) { return status; }
  remaining.maxFrames -= 5U;
  auto batch = mmdBatch(0x04A0, FrameKind::READ, 0);
  batch.frameCount = 5;
  batch.frames[4] = {FrameKind::WRITE, cmd::REG_ADDAR,
                     static_cast<uint16_t>(value | (acknowledge ? 0U : 0x0800U)),
                     static_cast<uint16_t>(~(mask | uint32_t{0x0800})), 3};
  status = _execute(batch, remaining);
  _evidence.lastStatus = status;
  return status;
}

void DP8382x::_retainEvidence(const MdioBatch& batch, const BatchResult& result,
                              bool clockValid, uint64_t before, uint64_t upper) noexcept {
  uint16_t selected = 0;
  uint16_t control = 0;
  bool prefix = true;
  uint64_t previous = before;
  for (uint8_t i = 0; i < batch.frameCount; ++i) {
    const auto& frame = batch.frames[i];
    const auto& receipt = result.frames[i];
    // Resolve from the complete requested descriptor, never from cached selectors.
    uint16_t address = frame.registerAddress;
    bool target = frame.registerAddress != cmd::REG_REGCR && frame.registerAddress != cmd::REG_ADDAR;
    if (frame.registerAddress == cmd::REG_REGCR && frame.kind == FrameKind::WRITE) { control = frame.value; }
    else if (frame.registerAddress == cmd::REG_ADDAR && frame.kind == FrameKind::WRITE && control == 0x001FU) { selected = frame.value; }
    else if (frame.registerAddress == cmd::REG_ADDAR && control == 0x401FU) { address = selected; target = true; }
    const bool completed = prefix && receipt.attempted && receipt.completed &&
        (frame.kind == FrameKind::READ ? receipt.valueValid : !receipt.valueValid);
    bool stampValid = clockValid;
    uint64_t stamp = upper;
    if (receipt.timeValid) {
      stampValid = stampValid && receipt.completed && receipt.completedAtUs >= previous && receipt.completedAtUs <= upper;
      stamp = receipt.completedAtUs;
      if (stampValid) { previous = stamp; }
    } else if (receipt.completedAtUs != 0U) { stampValid = false; }
    const int index = target ? evidenceIndex(address) : -1;
    if (frame.kind == FrameKind::READ && index >= 0) {
      auto& sample = _evidence.samples[static_cast<unsigned>(index)];
      if (completed) {
        sample.raw = receipt.value;
        sample.observedOnes |= receipt.value;
        sample.observedZeros |= static_cast<uint16_t>(~receipt.value);
        increment(sample.samples);
        if (address == 0x0014U || address == 0x0015U) {
          const uint64_t count = address == 0x0014U ? receipt.value & 0x00FFU : receipt.value;
          if (count > std::numeric_limits<uint64_t>::max() - sample.sampledSum) {
            sample.sampledSum = std::numeric_limits<uint64_t>::max();
            sample.sumSaturated = true;
          } else { sample.sampledSum += count; }
        }
        sample.valid = true;
        sample.stale = false;
        sample.timeValid = stampValid;
        sample.observedAtUs = stampValid ? stamp : 0;
      } else if (receipt.attempted || receipt.completed || receipt.valueValid || receipt.effectPossible) {
        if (receipt.effectPossible || receipt.completed || receipt.valueValid) { sample.historyLost = true; }
        sample.stale = true;
      }
    }
    // A raw target write cannot establish configuration trust, even on success.
    if (target && frame.kind == FrameKind::WRITE &&
        (receipt.attempted || receipt.completed || receipt.valueValid || receipt.effectPossible)) {
      const bool initialized = _initialized;
      if (address == 0x001FU && frame.value == 0x4000U) {
        // Table8-48 circuitry restart keeps registers, identity and initialized state.
        for (auto& sample : _evidence.samples) { sample.stale = true; }
      } else { _invalidateEvidence(); }
      // OP-13 preserves readiness on a proved effect-free non-reset failure.
      if (address != 0U && !receipt.completed && !receipt.valueValid && !receipt.effectPossible) {
        _initialized = initialized;
      }
    }
    if (!completed) { prefix = false; }
  }
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

Status DP8382x::_execute(MdioBatch batch, const IoBudget& budget, bool* called, bool operation) noexcept {
  if (called != nullptr) { *called = false; }
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || (!operation && (_polling || (_operation.state == OperationState::RUNNING &&
      _operation.phase == OperationPhase::WAIT)))) { return busy(BusyDetail::OPERATION_ACTIVE); }
  if (_transport.clockInvalid) { return configError(ConfigDetail::CLOCK_CONTRACT); }
  if (_stopped) { return busy(BusyDetail::CONTROLLER_QUARANTINED); }
  CallGuard guard(_active);
  uint64_t before = 0;
  uint64_t upper = 0;
  Status status = _sampleTime(before, upper);
  if (status.ok() && _quietAt(before)) { return busy(BusyDetail::OPERATION_ACTIVE); }
  if (status.ok()) {
    status = admitBudget(budget, _config.transport.bounds, batch.frameCount, before, upper);
  }
  if (status.ok() && operation && _initStep == InitStep::COMMAND) {
    status = admitReset(budget, _config.transport.bounds, before, _commandWaitUs());
  }
  if (!status.ok()) {
    return status;
  }

  batch.phyAddress = static_cast<uint8_t>(_config.phyAddress);
  increment(_transport.totalBatches);
  if (called != nullptr) { *called = true; }
  const BatchResult result = _config.transport.executeBatch(_config.transport.context, batch, budget);
  _transport.lastBatch = result;
  _transport.violation = TransportViolation::NONE;
  _transport.late = false;
  uint64_t after = 0;
  const Status clockStatus = _sampleTime(after, upper);
  if (clockStatus.ok()) { _transport.late = after >= budget.deadlineUs; }

  _retainEvidence(batch, result, clockStatus.ok(), before, upper);
  const auto frames = auditFrames(batch, result, clockStatus.ok(), before, upper);
  for (uint8_t i = 0; i < frames.attempted; ++i) { increment(_transport.attemptedFrames); }
  for (uint8_t i = 0; i < frames.completed; ++i) { increment(_transport.completedFrames); }
  const auto report = classifyReport(result, frames);
  _transport.violation = report.violation;
  if (report.violation != TransportViolation::NONE && report.violation != TransportViolation::CONTROLLER_STATE) { _stopped = true; }
  _transport.stopped = _stopped;
  _transport.controllerReusable = result.controllerReusable && report.release.ok() && !_stopped;
  status = report.primary.ok() ? report.release : report.primary;
  if (status.ok()) { status = clockStatus; }
  if (status.ok()) {
    if (_transport.late) { status = timeout(TimeoutDetail::DEADLINE); }
  }
  return status;
}

Status DP8382x::startInitialize(const InitializeOptions& options, const OperationBudget& budget,
                                OperationToken& token) noexcept {
  if (options.reset > ResetKind::RESTART) { return paramError(ParamDetail::ENUM_VALUE); }
  if (budget.pollIntervalUs == 0U || budget.maxStalledPolls == 0U) { return paramError(ParamDetail::BUDGET_RANGE); }
  if (!_bound) { return Status::Error(Err::NOT_BOUND, "No transport binding"); }
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return busy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state == OperationState::RESULT_PENDING) { return busy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return configError(ConfigDetail::CLOCK_CONTRACT); }
  if (budget.io.maxFrames == 0U) { return timeout(TimeoutDetail::FRAME_BUDGET); }
  if (budget.io.deadlineUs == 0U) { return timeout(TimeoutDetail::DEADLINE); }
  const bool owed = _operation.cleanupOwed;
  const auto quietUntil = _operation.busQuietUntilUs;
  const bool quietValid = _operation.busQuietValid;
  _operation = {};
  _operation.cleanupOwed = owed;
  _operation.busQuietUntilUs = quietUntil;
  _operation.busQuietValid = quietValid;
  _operation.token = {++_tokenCounter};
  _operation.state = OperationState::RUNNING;
  _operation.kind = OperationKind::INITIALIZE;
  _operation.status = Status::Error(Err::IN_PROGRESS, "Initialization pending");
  _operation.remaining = budget.io;
  _operationBudget = budget;
  _result = {};
  _result.token = _operation.token;
  _result.kind = _operation.kind;
  _result.reset = options.reset;
  _result.commandRequired = options.reset != ResetKind::NONE;
  _pollTimeValid = false;
  _setInitStep(InitStep::IDENTIFY);
  token = _operation.token;
  return _operation.status;
}

Status DP8382x::_checkToken(OperationToken token) const noexcept {
  if (token.value == 0U || token.value > _tokenCounter ||
      (_operation.state != OperationState::IDLE && token.value != _operation.token.value)) {
    return Status::Error(Err::TOKEN_MISMATCH, "Wrong operation token");
  }
  if (_operation.state == OperationState::IDLE) { return Status::Error(Err::RESULT_NOT_AVAILABLE, "Empty operation slot"); }
  return Status::Ok();
}

Status DP8382x::_finishOperation(Status status) noexcept {
  _operation.state = OperationState::RESULT_PENDING;
  _operation.phase = OperationPhase::COMPLETE;
  _operation.nextPollValid = false;
  _operation.status = status;
  if (_result.persistentRestored) { _operation.cleanupOwed = false; }
  else if (_result.mutationCompleted || _result.mutationPossible) { _operation.cleanupOwed = true; }
  _result.status = status;
  _result.cleanupOwed = _operation.cleanupOwed;
  _result.clockInvalid = _transport.clockInvalid;
  return status;
}

void DP8382x::_setInitStep(InitStep step) noexcept {
  _initStep = step;
  _operation.nextPollValid = false;
  switch (step) {
    case InitStep::IDENTIFY: case InitStep::SOR1: case InitStep::SOR2:
    case InitStep::REIDENTIFY: case InitStep::RESOR1: case InitStep::RESOR2:
      _operation.phase = OperationPhase::IDENTIFY; break;
    case InitStep::CHECK_RESET: case InitStep::CHECK_PHYRCR: _operation.phase = OperationPhase::QUIESCE; break;
    case InitStep::COMMAND: _operation.phase = OperationPhase::COMMAND; break;
    case InitStep::WAIT_EXISTING: case InitStep::WAIT_PHYRCR: case InitStep::WAIT_COMMAND:
      _operation.phase = OperationPhase::WAIT; break;
    case InitStep::APPLY: _operation.phase = OperationPhase::RESTORE; break;
    case InitStep::VERIFY: _operation.phase = OperationPhase::VERIFY; break;
  }
}

Status DP8382x::_waitOperation() noexcept {
  // The latest post-callback raw sample plus lag bounds physical completion.
  const uint64_t upper = _lastTimeUs + _config.transport.bounds.clockLagUs;
  if (_operationBudget.pollIntervalUs > std::numeric_limits<uint64_t>::max() - upper) {
    return _finishOperation(timeout(TimeoutDetail::BOUND_ADMISSION));
  }
  _operation.nextPollAtUs = upper + _operationBudget.pollIntervalUs;
  if (_quietPending && _operation.busQuietValid &&
      _operation.nextPollAtUs < _operation.busQuietUntilUs) {
    _operation.nextPollAtUs = _operation.busQuietUntilUs;
  }
  _operation.nextPollValid = true;
  return _operation.status;
}

uint32_t DP8382x::_commandWaitUs() const noexcept {
  const auto interval = _operationBudget.pollIntervalUs;
  return _result.reset == ResetKind::HARD && interval < 2000U ? 2000U : interval;
}

bool DP8382x::_quietAt(uint64_t raw) noexcept {
  if (!_quietPending) { return false; }
  if (!_operation.busQuietValid || raw < _operation.busQuietUntilUs) { return true; }
  _quietPending = false;
  _operation.busQuietValid = false;
  return false;
}

Status DP8382x::_checkQuiet() noexcept {
  if (!_quietPending) { return Status::Ok(); }
  if (_transport.clockInvalid) { return configError(ConfigDetail::CLOCK_CONTRACT); }
  CallGuard guard(_active);
  uint64_t raw = 0, upper = 0;
  const auto status = _sampleTime(raw, upper);
  if (!status.ok()) { return status; }
  return _quietAt(raw) ? busy(BusyDetail::OPERATION_ACTIVE) : Status::Ok();
}

Status DP8382x::_anchorQuiet() noexcept {
  _quietPending = true;
  _operation.busQuietValid = false;
  _operation.busQuietUntilUs = 0;
  if (_transport.clockInvalid) { return configError(ConfigDetail::CLOCK_CONTRACT); }
  const uint64_t upper = _lastTimeUs + _config.transport.bounds.clockLagUs;
  const auto wait = _commandWaitUs();
  if (wait > std::numeric_limits<uint64_t>::max() - upper) {
    // No representable future sample can prove this mandatory interval elapsed.
    _transport.clockInvalid = true;
    return timeout(TimeoutDetail::BOUND_ADMISSION);
  }
  _operation.busQuietUntilUs = upper + wait;
  _operation.busQuietValid = true;
  return Status::Ok();
}

void DP8382x::_invalidateResetEvidence(bool configurationLost) noexcept {
  if (configurationLost) {
    _invalidateEvidence();
    _result.identity.stale = true;
    _result.straps.stale = true;
    _result.straps.timeValid = false;
  } else {
    for (auto& sample : _evidence.samples) { sample.stale = true; }
  }
}

Status DP8382x::_yieldOperation(uint64_t raw) noexcept {
  if (_pollTimeValid && raw == _pollTimeUs) { increment(_operation.stalledPolls); }
  else { _operation.stalledPolls = 0; }
  _pollTimeUs = raw;
  _pollTimeValid = true;
  if (_operation.stalledPolls >= _operationBudget.maxStalledPolls) {
    return _finishOperation(timeout(TimeoutDetail::POLL_BUDGET));
  }
  return _operation.status;
}

Status DP8382x::pollOperation(OperationToken token, uint32_t maxFrames) noexcept {
  if (_active || _polling) { return busy(BusyDetail::OPERATION_ACTIVE); }
  const auto valid = _checkToken(token);
  if (!valid.ok()) { return valid; }
  if (_operation.state == OperationState::RESULT_PENDING) { return _operation.status; }
  CallGuard guard(_polling);
  if (_transport.clockInvalid) { return _finishOperation(configError(ConfigDetail::CLOCK_CONTRACT)); }
  uint64_t raw = 0, upper = 0;
  auto status = _sampleTime(raw, upper);
  if (!status.ok()) { return _finishOperation(status); }
  if (raw >= _operation.remaining.deadlineUs) { return _finishOperation(timeout(TimeoutDetail::DEADLINE)); }
  if (_quietAt(raw)) { return _yieldOperation(raw); }
  if (_operation.nextPollValid && raw < _operation.nextPollAtUs) { return _yieldOperation(raw); }

  MdioBatch batch;
  switch (_initStep) {
    case InitStep::IDENTIFY: case InitStep::REIDENTIFY:
      batch = directBatch(2, FrameKind::READ);
      batch.frameCount = 2;
      batch.frames[1] = {FrameKind::READ, 3, 0, 0, NO_READ_INDEX}; break;
    case InitStep::SOR1: case InitStep::RESOR1: batch = mmdBatch(0x0467, FrameKind::READ, 0); break;
    case InitStep::SOR2: case InitStep::RESOR2: batch = mmdBatch(0x0468, FrameKind::READ, 0); break;
    case InitStep::CHECK_RESET: case InitStep::WAIT_EXISTING:
      batch = directBatch(0, FrameKind::READ); break;
    case InitStep::CHECK_PHYRCR: case InitStep::WAIT_PHYRCR:
      batch = directBatch(0x1F, FrameKind::READ); break;
    case InitStep::WAIT_COMMAND:
      batch = directBatch(_result.reset == ResetKind::PCS ? 0 : 0x1F, FrameKind::READ); break;
    case InitStep::COMMAND:
      batch = directBatch(_result.reset == ResetKind::PCS ? 0 : 0x1F, FrameKind::WRITE,
                          _result.reset == ResetKind::RESTART ? 0x4000 : 0x8000); break;
    case InitStep::APPLY: batch = mmdBatch(0x030E, FrameKind::WRITE, 0x4A40); break;
    case InitStep::VERIFY: batch = mmdBatch(0x030E, FrameKind::READ, 0); break;
  }
  status = admitBudget(_operation.remaining, _config.transport.bounds, batch.frameCount, raw, upper);
  if (!status.ok()) { return _finishOperation(status); }
  if (batch.frameCount > maxFrames) { return _yieldOperation(raw); }
  if (_initStep == InitStep::COMMAND) {
    status = admitReset(_operation.remaining, _config.transport.bounds, raw, _commandWaitUs());
    if (!status.ok()) { return _finishOperation(status); }
  }
  bool called = false;
  if (_initStep == InitStep::IDENTIFY || _initStep == InitStep::REIDENTIFY) {
    DeviceIdentity accepted;
    status = _readIdentity(accepted, _operation.remaining, true, &called);
    if (called) { _result.identity = _identity; }
  } else { status = _execute(batch, _operation.remaining, &called, true); }
  _pollTimeUs = _lastTimeUs;
  _pollTimeValid = true;
  if (called) {
    _operation.stalledPolls = 0;
    const auto& receipt = _transport.lastBatch;
    const auto frames = auditFrames(batch, receipt, false, 0, 0);
    _operation.remaining.maxFrames -= frames.attempted;
    bool effect = false;
    for (const auto& frame : receipt.frames) { effect = effect || frame.effectPossible; }
    _result.failingBatch = {_operation.phase, frames.attempted, frames.completed, effect,
                           status, classifyReport(receipt, frames).release, _transport.violation, _transport.late};
    _result.late = _result.late || _transport.late;
    if ((_initStep == InitStep::SOR1 || _initStep == InitStep::SOR2 ||
         _initStep == InitStep::RESOR1 || _initStep == InitStep::RESOR2) && frames.completed == 4U) {
      if (_initStep == InitStep::SOR1 || _initStep == InitStep::RESOR1) {
        _result.straps.sor1 = receipt.frames[3].value;
        _result.straps.sor1Valid = true;
      } else {
        _result.straps.sor2 = receipt.frames[3].value;
        _result.straps.sor2Valid = true;
      }
      _result.straps.stale = true;
      _result.straps.timeValid = false;
    }
    if (_initStep == InitStep::COMMAND || _initStep == InitStep::APPLY) {
      const uint8_t index = _initStep == InitStep::COMMAND ? uint8_t{0} : uint8_t{3};
      const auto& target = receipt.frames[index];
      // A contradictory callback cannot prove that an omitted target was never
      // attempted. Keep valid zero-attempt denials effect-free, but quarantine
      // alone cannot protect other PHYs from an unreported hard-reset write.
      const bool uncertain = _transport.violation != TransportViolation::NONE;
      _result.mutationAttempted = _result.mutationAttempted || target.attempted;
      _result.mutationCompleted = _result.mutationCompleted || frames.completed > index;
      _result.mutationPossible = _result.mutationPossible || uncertain || target.effectPossible || target.completed || target.valueValid;
      _result.failingBatch.effectPossible = _result.failingBatch.effectPossible || uncertain;
      if (_initStep == InitStep::APPLY && uncertain) { _invalidateEvidence(); }
      if (_initStep == InitStep::COMMAND) {
        _result.commandAccepted = frames.completed > index;
        if (uncertain || target.attempted || target.effectPossible || target.completed || target.valueValid) {
          _invalidateResetEvidence(_result.reset != ResetKind::RESTART);
          if (_result.reset == ResetKind::HARD) {
            const auto anchored = _anchorQuiet();
            if (status.ok()) { status = anchored; }
          }
        }
      }
    }
  }
  if (!status.ok()) { return _finishOperation(status); }
  const uint16_t value = _transport.lastBatch.frames[batch.frameCount - 1U].value;
  switch (_initStep) {
    case InitStep::IDENTIFY: _setInitStep(InitStep::SOR1); break;
    case InitStep::REIDENTIFY: _setInitStep(InitStep::RESOR1); break;
    case InitStep::SOR1: case InitStep::RESOR1:
      _setInitStep(_initStep == InitStep::SOR1 ? InitStep::SOR2 : InitStep::RESOR2); break;
    case InitStep::SOR2: case InitStep::RESOR2:
      _result.straps.stale = false;
      _result.straps.timeValid = true;
      _result.straps.observedAtUs = _lastTimeUs + _config.transport.bounds.clockLagUs;
      _setInitStep(_initStep == InitStep::SOR2 ? InitStep::CHECK_RESET :
          (_result.commandRequired && !_result.commandAccepted ? InitStep::COMMAND : InitStep::APPLY)); break;
    case InitStep::CHECK_RESET:
      if ((value & 0x8000U) != 0U) {
        _invalidateResetEvidence(true);
        _setInitStep(InitStep::WAIT_EXISTING);
        return _waitOperation();
      }
      _setInitStep(InitStep::CHECK_PHYRCR); break;
    case InitStep::CHECK_PHYRCR: case InitStep::WAIT_PHYRCR:
      if ((value & 0xC000U) != 0U) {
        _invalidateResetEvidence((value & 0x8000U) != 0U);
        _setInitStep(InitStep::WAIT_PHYRCR);
        return _waitOperation();
      }
      // An observed reset may change mode/reachability. Accept refreshed identity
      // before issuing even the requested reset; do not write using stale admission.
      _setInitStep(_result.identity.stale ? InitStep::REIDENTIFY :
          (_result.commandRequired ? InitStep::COMMAND : InitStep::APPLY)); break;
    case InitStep::COMMAND:
      _setInitStep(InitStep::WAIT_COMMAND);
      return _waitOperation();
    case InitStep::WAIT_EXISTING:
      if ((value & 0x8000U) != 0U) { return _waitOperation(); }
      _setInitStep(InitStep::CHECK_PHYRCR); break;
    case InitStep::WAIT_COMMAND:
      if ((value & (_result.reset == ResetKind::PCS ? 0x8000U : 0xC000U)) != 0U) {
        if (_result.reset != ResetKind::PCS && (value & 0x8000U) != 0U) { _invalidateResetEvidence(true); }
        return _waitOperation();
      }
      _result.protocolComplete = true;
      _setInitStep(InitStep::REIDENTIFY); break;
    case InitStep::APPLY: _setInitStep(InitStep::VERIFY); break;
    case InitStep::VERIFY:
      if ((value & 0xFFC0U) != 0x4A40U) {
        _initialized = false;
        _result.mismatch = {true, true, 31, 0x030E, 0xFFC0, 0x4A40, value};
        return _finishOperation(Status::Error(Err::CONFIG_MISMATCH, "VOD_CFG3 verification failed"));
      }
      _result.persistentRestored = true;
      _identity = _result.identity; // VOD programming cannot alter the accepted identity.
      _initialized = true;
      return _finishOperation(Status::Ok());
  }
  return _operation.status;
}

Status DP8382x::takeResult(OperationToken token, OperationResult& result) noexcept {
  if (_active || _polling) { return busy(BusyDetail::OPERATION_ACTIVE); }
  const auto valid = _checkToken(token);
  if (!valid.ok()) { return valid; }
  if (_operation.state != OperationState::RESULT_PENDING) { return Status::Error(Err::RESULT_NOT_AVAILABLE, "Operation running"); }
  result = _result;
  const bool owed = _operation.cleanupOwed;
  const auto quietUntil = _operation.busQuietUntilUs;
  const bool quietValid = _operation.busQuietValid;
  _operation = {};
  _operation.cleanupOwed = owed;
  _operation.busQuietUntilUs = quietUntil;
  _operation.busQuietValid = quietValid;
  return Status::Ok();
}

Status DP8382x::cancel(OperationToken token) noexcept {
  if (_active || _polling) { return busy(BusyDetail::OPERATION_ACTIVE); }
  const auto valid = _checkToken(token);
  if (!valid.ok()) { return valid; }
  if (_operation.state == OperationState::RUNNING) { _finishOperation(Status::Error(Err::CANCELLED, "Owner cancelled")); }
  return Status::Ok();
}

Status DP8382x::abandonCleanup() noexcept {
  if (_active || _polling || _operation.state == OperationState::RUNNING) { return busy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.cleanupOwed) {
    _operation.cleanupOwed = false;
    _invalidateEvidence();
  }
  return Status::Ok();
}

} // namespace DP8382x
