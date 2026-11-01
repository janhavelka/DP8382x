#include <DP8382x/DP8382x.h>

#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <type_traits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
int failures = 0;
unsigned checks = 0;
void check(bool condition, const char* expression, int line) {
  ++checks;
  if (!condition) {
    std::fprintf(stderr, "line %d: %s\n", line, expression);
    ++failures;
  }
}
#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

static_assert(!std::is_copy_constructible<Driver>::value, "one owner");
static_assert(!std::is_copy_assignable<Driver>::value, "one owner");
static_assert(!std::is_move_constructible<Driver>::value, "stable owner address");
static_assert(!std::is_move_assignable<Driver>::value, "stable owner address");

constexpr IoBudget BUDGET{1000U, 1U};
constexpr uint16_t SENTINEL = 0xA55A;

// An application controller model, not an emulated PHY or production adapter.
// It records each actual invocation/launch, releases before returning, and owns
// shared quarantine. Deliberate malformed reports test the core's second check.
struct Controller {
  uint64_t now{100};
  uint64_t duration{5};
  unsigned clocks{0};
  unsigned callbacks{0};
  unsigned launches{0};
  unsigned releases{0};
  bool locked{false};
  bool quarantined{false};
  bool custom{false};
  bool regress{false};
  bool reenter{false};
  bool clockReenter{false};
  Driver* active{nullptr};
  Driver* competing{nullptr};
  uint16_t readValue{0x2000};
  uint16_t written{0};
  MdioBatch lastRequest{};
  IoBudget lastBudget{};
  BatchResult response{};

  Config config(uint32_t address = 1) noexcept {
    Config result;
    result.variant = Variant::DP83826I;
    result.phyAddress = address;
    result.interface = MacInterface::RMII;
    result.transport = {this, execute, clock, {3, 5, 2, 4, 2}};
    return result;
  }
  static uint64_t clock(void* context) noexcept {
    Controller& bus = *static_cast<Controller*>(context);
    ++bus.clocks;
    if (bus.clockReenter) {
      uint16_t value = SENTINEL;
      CHECK(bus.active->end().is(Err::BUSY));
      CHECK(bus.active->bind(bus.config()).is(Err::BUSY));
      CHECK(bus.active->readClause22(2, value, BUDGET).is(Err::BUSY));
      CHECK(value == SENTINEL);
    }
    return bus.now;
  }
  static BatchResult complete(FrameKind kind = FrameKind::READ) noexcept {
    BatchResult result;
    result.status = Status::Ok();
    result.releaseStatus = Status::Ok();
    result.controllerReusable = true;
    result.frames[0].attempted = true;
    result.frames[0].completed = true;
    result.frames[0].valueValid = kind == FrameKind::READ;
    result.frames[0].value = kind == FrameKind::READ ? 0x2000 : 0;
    return result;
  }
  static BatchResult admission(BusyDetail reason) noexcept {
    BatchResult result;
    result.status = Status::Error(Err::BUSY, "Owner admission", static_cast<int32_t>(reason));
    result.releaseStatus = Status::Ok();
    result.controllerReusable = reason != BusyDetail::CONTROLLER_QUARANTINED;
    return result;
  }
  static BatchResult execute(void* context, const MdioBatch& batch,
                             const IoBudget& budget) noexcept {
    Controller& bus = *static_cast<Controller*>(context);
    ++bus.callbacks;
    if (bus.quarantined) return admission(BusyDetail::CONTROLLER_QUARANTINED);
    if (bus.locked) return admission(BusyDetail::OWNER_ADMISSION);
    bus.locked = true;
    bus.lastRequest = batch;
    bus.lastBudget = budget;
    CHECK(batch.frameCount == 1U);
    CHECK(batch.phyAddress <= 31U);
    CHECK(batch.frames[0].registerAddress <= 31U);
    CHECK(batch.frames[0].preserveMask == 0U);
    CHECK(batch.frames[0].readIndex == 0xFFU);
    if (!bus.custom || bus.response.frames[0].attempted) ++bus.launches;
    if (bus.reenter) {
      const unsigned before = bus.callbacks;
      uint16_t value = SENTINEL;
      CHECK(bus.active->end().is(Err::BUSY));
      CHECK(bus.active->bind(bus.config()).is(Err::BUSY));
      CHECK(bus.active->readClause22(2, value, budget).is(Err::BUSY));
      CHECK(value == SENTINEL);
      CHECK(bus.active->writeClause22(0x18, 0, budget).is(Err::BUSY));
      CHECK(bus.callbacks == before);
      if (bus.competing != nullptr) {
        for (uint32_t reg : {0x0DU, 0x0EU}) {
          CHECK(bus.competing->readClause22(reg, value, budget).is(Err::INVALID_PARAM));
          CHECK(bus.competing->writeClause22(reg, 0, budget).is(Err::INVALID_PARAM));
        }
        CHECK(bus.callbacks == before);
        CHECK(bus.competing->readClause22(2, value, budget).is(Err::BUSY));
        CHECK(value == SENTINEL);
        CHECK(bus.callbacks == before + 1U);
      }
    }
    BatchResult result = bus.custom ? bus.response : complete(batch.frames[0].kind);
    if (!bus.custom && batch.frames[0].kind == FrameKind::READ)
      result.frames[0].value = bus.readValue;
    if (batch.frames[0].kind == FrameKind::WRITE && result.frames[0].attempted)
      bus.written = batch.frames[0].value; // May apply even when terminal status fails.
    bus.now = bus.regress ? 99U : bus.now + bus.duration;
    if (result.releaseStatus.ok()) {
      bus.locked = false;
      ++bus.releases;
    }
    if (!result.releaseStatus.ok() || !result.controllerReusable)
      bus.quarantined = true;
    return result;
  }
};

void expectRejected(Driver& driver, Controller& bus, uint32_t reg, uint32_t word,
                    bool write, ParamDetail reason) {
  const unsigned calls = bus.callbacks;
  const unsigned clocks = bus.clocks;
  uint16_t value = SENTINEL;
  const Status result = write ? driver.writeClause22(reg, word, BUDGET)
                              : driver.readClause22(reg, value, BUDGET);
  CHECK(result.is(Err::INVALID_PARAM));
  CHECK(result.detail == static_cast<int32_t>(reason));
  CHECK(value == SENTINEL);
  CHECK(bus.callbacks == calls);
  CHECK(bus.clocks == clocks);
}

void testBinding() {
  Controller bus;
  Driver driver;
  CHECK(!driver.isBound());
  CHECK(driver.end().ok());
  uint16_t value = SENTINEL;
  CHECK(driver.readClause22(2, value, BUDGET).is(Err::NOT_BOUND));
  CHECK(driver.writeClause22(0x18, 0, BUDGET).is(Err::NOT_BOUND));
  CHECK(value == SENTINEL && bus.callbacks == 0U && bus.clocks == 0U);
  for (uint32_t address = 0; address < 32U; ++address) {
    CHECK(driver.bind(bus.config(address)).ok());
    CHECK(driver.isBound());
    CHECK(bus.clocks == 0U && bus.callbacks == 0U);
  }
  CHECK(driver.bind(bus.config()).ok());
  CHECK(driver.readClause22(2, value, BUDGET).ok());
  const auto saved = driver.transportSnapshot();
  const unsigned clocks = bus.clocks;
  for (unsigned fault = 0; fault < 12U; ++fault) {
    Config invalid = bus.config();
    ConfigDetail detail = ConfigDetail::ENUM_VALUE;
    switch (fault) {
      case 0: invalid.variant = Variant::UNKNOWN; break;
      case 1: invalid.requiredMode = static_cast<OperatingMode>(255); break;
      case 2: invalid.revisionPolicy = static_cast<RevisionPolicy>(255); break;
      case 3: invalid.interface = static_cast<MacInterface>(255); break;
      case 4: invalid.interface = MacInterface::UNKNOWN; detail = ConfigDetail::INTERFACE_REQUIRED; break;
      case 5: invalid.phyAddress = 32; detail = ConfigDetail::ADDRESS_RANGE; break;
      case 6: invalid.transport.executeBatch = nullptr; detail = ConfigDetail::TRANSPORT; break;
      case 7: invalid.transport.nowUs = nullptr; detail = ConfigDetail::TRANSPORT; break;
      case 8: invalid.transport.bounds.frameUs = 0; detail = ConfigDetail::TRANSPORT; break;
      case 9: invalid.transport.bounds.overheadUs = 0; detail = ConfigDetail::TRANSPORT; break;
      case 10: invalid.transport.bounds.clockLagUs = 0; detail = ConfigDetail::TRANSPORT; break;
      default: invalid.offlineThreshold = 0; detail = ConfigDetail::HEALTH_THRESHOLD; break;
    }
    const Status result = driver.bind(invalid);
    CHECK(result.is(Err::INVALID_CONFIG));
    CHECK(result.detail == static_cast<int32_t>(detail));
    CHECK(driver.isBound());
    CHECK(driver.transportSnapshot().totalBatches == saved.totalBatches);
    CHECK(driver.transportSnapshot().lastBatch.frames[0].value == saved.lastBatch.frames[0].value);
    CHECK(bus.clocks == clocks && bus.callbacks == 1U);
  }
  for (uint32_t address : {32U, 255U, 0x8000U, 0xFFFFFFFFU}) {
    CHECK(driver.bind(bus.config(address)).is(Err::INVALID_CONFIG));
    CHECK(bus.clocks == clocks && bus.callbacks == 1U);
  }
  Config rgmii = bus.config(); rgmii.interface = MacInterface::RGMII;
  CHECK(driver.bind(rgmii).is(Err::UNSUPPORTED));
  Config noContext = bus.config(); noContext.transport.context = nullptr;
  CHECK(driver.bind(noContext).ok()); // Binding cannot invoke this borrowed context.
  CHECK(bus.clocks == clocks && bus.callbacks == 1U);
  CHECK(driver.transportSnapshot().totalBatches == 0U);
  CHECK(driver.end().ok());
  CHECK(!driver.isBound());
  CHECK(driver.end().ok());
  CHECK(bus.clocks == clocks && bus.callbacks == 1U);
}

void testDirect() {
  // Literal allowed map reviewed from SNLS647I detailed tables, not production masks.
  constexpr uint32_t allowed =
      (1UL << 0) | (1UL << 2) | (1UL << 3) | (1UL << 4) | (1UL << 5) |
      (1UL << 7) | (1UL << 8) | (1UL << 9) | (1UL << 10) | (1UL << 11) |
      (1UL << 17) | (1UL << 22) | (1UL << 24) | (1UL << 25) |
      (1UL << 27) | (1UL << 28) | (1UL << 31);
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config(31)).ok());
  for (uint32_t reg = 0; reg < 32U; ++reg) {
    if ((allowed & (uint32_t{1} << reg)) == 0U) {
      expectRejected(driver, bus, reg, 0, false, ParamDetail::ACCESS_POLICY);
    } else {
      const unsigned before = bus.callbacks;
      uint16_t value = SENTINEL;
      CHECK(driver.readClause22(reg, value, BUDGET).ok());
      CHECK(value == 0x2000U);
      CHECK(bus.callbacks == before + 1U);
      CHECK(bus.lastRequest.phyAddress == 31U);
      CHECK(bus.lastRequest.frames[0].registerAddress == reg);
      CHECK(bus.lastRequest.frames[0].kind == FrameKind::READ);
      CHECK(bus.lastRequest.frames[0].value == 0U);
    }
    if (reg != 0x18U)
      expectRejected(driver, bus, reg, 0, true, ParamDetail::ACCESS_POLICY);
  }
  for (uint32_t reg : {32U, 255U, 0x8000U, 0xFFFFFFFFU}) {
    expectRejected(driver, bus, reg, 0, false, ParamDetail::ADDRESS_RANGE);
    expectRejected(driver, bus, reg, 0, true, ParamDetail::ADDRESS_RANGE);
  }
  for (uint32_t value : {0x10000U, 0x80000000U, 0xFFFFFFFFU})
    expectRejected(driver, bus, 0x18, value, true, ParamDetail::VALUE_WIDTH);
  // Exhaust legal LEDCR data width: reserved bits must stay zero. Only 32
  // words pass; all other values must reject without even sampling the clock.
  for (uint32_t word = 0; word <= 0xFFFFU; ++word) {
    if ((word & ~uint32_t{0x0692}) != 0U) {
      expectRejected(driver, bus, 0x18, word, true, ParamDetail::ACCESS_POLICY);
    } else {
      const unsigned before = bus.callbacks;
      CHECK(driver.writeClause22(0x18, word, BUDGET).ok());
      CHECK(bus.written == word);
      CHECK(bus.callbacks == before + 1U);
      CHECK(bus.lastRequest.frames[0].kind == FrameKind::WRITE);
    }
  }
  const auto snapshot = driver.transportSnapshot();
  CHECK(snapshot.totalBatches == bus.callbacks);
  CHECK(snapshot.attemptedFrames == bus.launches);
  CHECK(snapshot.completedFrames == bus.launches);
  CHECK(bus.releases == bus.launches && !bus.locked);
  CHECK(driver.end().ok());
  for (uint32_t address : {0U, 31U}) {
    Controller boundary; Driver endpoint;
    CHECK(endpoint.bind(boundary.config(address)).ok());
    uint16_t value = SENTINEL;
    CHECK(endpoint.readClause22(2, value, BUDGET).ok());
    CHECK(boundary.lastRequest.phyAddress == address && value == 0x2000U);
    CHECK(endpoint.writeClause22(0x18, 0x0692, BUDGET).ok());
    CHECK(boundary.lastRequest.phyAddress == address && boundary.written == 0x0692U);
    CHECK(boundary.launches == 2U && endpoint.transportSnapshot().attemptedFrames == 2U);
    CHECK(endpoint.end().ok());
  }
}

void testReports() {
  // Every numeric status byte: only five primary classifications are legal.
  for (unsigned raw = 0; raw < 256U; ++raw) {
    Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
    bus.custom = true; bus.response = Controller::complete();
    bus.response.status = {static_cast<Err>(raw), -2147483647, "backend"};
    const Err code = static_cast<Err>(raw);
    bool legal = code == Err::OK || code == Err::MDIO_ERROR || code == Err::MDIO_TIMEOUT ||
                 code == Err::BUSY || code == Err::OPERATION_TIMEOUT;
    if (code == Err::OK) bus.response.status.detail = 0;
    if (code == Err::BUSY || code == Err::OPERATION_TIMEOUT) {
      bus.response.frames[0] = {};
      bus.response.status.detail = code == Err::BUSY
          ? static_cast<int32_t>(BusyDetail::OWNER_ADMISSION)
          : static_cast<int32_t>(TimeoutDetail::DEADLINE);
    }
    uint16_t value = SENTINEL;
    const Status status = driver.readClause22(2, value, BUDGET);
    CHECK(status.code == (legal ? code : Err::MDIO_ERROR));
    CHECK(status.detail == bus.response.status.detail);
    CHECK(value == (code == Err::OK ? 0x2000U : SENTINEL));
    CHECK(driver.transportSnapshot().lastBatch.status.code == code);
    CHECK(driver.transportSnapshot().lastBatch.status.detail == bus.response.status.detail);
    CHECK(bus.callbacks == 1U && !bus.locked);
    if (!legal) {
      CHECK(driver.transportSnapshot().violation == TransportViolation::STATUS_CODE);
      CHECK(driver.readClause22(2, value, BUDGET).is(Err::BUSY));
      CHECK(bus.callbacks == 1U);
    }
    CHECK(driver.end().ok());
  }
  // The complete int32 domain endpoints must survive callback normalization.
  for (int32_t detail : {std::numeric_limits<int32_t>::min(), int32_t{-1},
                         std::numeric_limits<int32_t>::max()}) {
    Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
    bus.custom = true; bus.response = Controller::complete(FrameKind::WRITE);
    bus.response.status = Status::Error(Err::MDIO_TIMEOUT, "accepted then failed", detail);
    bus.response.frames[0].completed = false;
    bus.response.frames[0].effectPossible = true;
    const Status status = driver.writeClause22(0x18, 0x0692, BUDGET);
    CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail == detail);
    CHECK(bus.written == 0x0692U && bus.callbacks == 1U);
    CHECK(driver.transportSnapshot().lastBatch.frames[0].effectPossible);
    CHECK(driver.transportSnapshot().attemptedFrames == 1U);
    CHECK(driver.transportSnapshot().completedFrames == 0U);
    CHECK(!bus.locked && bus.releases == 1U);
    CHECK(driver.end().ok());
  }
  // Each contradiction is independent; default unattempted unused slots matter.
  for (unsigned fault = 0; fault < 15U; ++fault) {
    Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
    bus.custom = true; bus.response = Controller::complete();
    TransportViolation violation = TransportViolation::FRAME_REPORT;
    switch (fault) {
      case 0: bus.response.frames[0] = {}; break; // Short successful response.
      case 1: bus.response.frames[0].attempted = false; break;
      case 2: bus.response.frames[0].completed = false; break;
      case 3: bus.response.frames[0].valueValid = false; break;
      case 4: bus.response.frames[1].attempted = true; break;
      case 5: bus.response.frames[7].value = 1; break;
      case 6: bus.response.frames[0].completedAtUs = 104; break;
      case 7: bus.response.frames[0].timeValid = true; bus.response.frames[0].completedAtUs = 99; break;
      case 8: bus.response.frames[0].timeValid = true; bus.response.frames[0].completedAtUs = 108; break;
      case 9: bus.response.status.msg = nullptr; violation = TransportViolation::NULL_MESSAGE; break;
      case 10: bus.response.status.detail = 7; violation = TransportViolation::STATUS_CODE; break;
      case 11: bus.response.status = {Err::BUSY, 0, "invalid admission"}; violation = TransportViolation::STATUS_CODE; break;
      case 12: bus.response.status = {Err::OPERATION_TIMEOUT, 99, "invalid deadline"}; violation = TransportViolation::STATUS_CODE; break;
      case 13: bus.response.frames[0].completed = false; bus.response.frames[0].valueValid = false; break;
      default: bus.response.frames[1].effectPossible = true; break;
    }
    uint16_t value = SENTINEL;
    CHECK(driver.readClause22(2, value, BUDGET).is(Err::MDIO_ERROR));
    CHECK(value == SENTINEL);
    const auto result = driver.transportSnapshot();
    CHECK(result.violation == violation);
    CHECK(result.totalBatches == 1U);
    CHECK(result.attemptedFrames == ((fault == 0U || fault == 1U) ? 0U : 1U));
    // Timestamp/unused-slot defects cannot erase a structurally complete first
    // frame, but contradictory READ completion/value flags cannot establish one.
    CHECK(result.completedFrames == ((fault <= 3U || fault == 13U) ? 0U : 1U));
    CHECK(driver.readClause22(2, value, BUDGET).is(Err::BUSY));
    CHECK(bus.callbacks == 1U);
    CHECK(driver.end().ok());
  }
  for (uint64_t timestamp : {uint64_t{100}, uint64_t{107}}) {
    Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
    bus.custom = true; bus.response = Controller::complete();
    bus.response.frames[0].timeValid = true;
    bus.response.frames[0].completedAtUs = timestamp;
    uint16_t value = SENTINEL;
    CHECK(driver.readClause22(2, value, BUDGET).ok());
    CHECK(driver.transportSnapshot().lastBatch.frames[0].completedAtUs == timestamp);
    CHECK(driver.end().ok());
  }
  // A write may not masquerade as a completed read.
  Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
  bus.custom = true; bus.response = Controller::complete(FrameKind::READ);
  CHECK(driver.writeClause22(0x18, 0, BUDGET).is(Err::MDIO_ERROR));
  CHECK(driver.transportSnapshot().violation == TransportViolation::FRAME_REPORT);
  CHECK(driver.end().ok());

  // A failed attempted read can contain scratch data without a valid word.
  // It is not a malformed report, nor an ordinary output or a completed frame.
  CHECK(driver.bind(bus.config()).ok());
  bus.response = Controller::complete();
  bus.response.status = {Err::MDIO_TIMEOUT, -37, "read failed"};
  bus.response.frames[0].completed = false;
  bus.response.frames[0].valueValid = false;
  bus.response.frames[0].value = 0xBEEF;
  bus.response.frames[0].effectPossible = true;
  uint16_t value = SENTINEL;
  const Status scratch = driver.readClause22(2, value, BUDGET);
  CHECK(scratch.is(Err::MDIO_TIMEOUT) && scratch.detail == -37);
  CHECK(value == SENTINEL);
  CHECK(driver.transportSnapshot().violation == TransportViolation::NONE);
  CHECK(driver.transportSnapshot().lastBatch.frames[0].value == 0xBEEFU);
  CHECK(driver.transportSnapshot().attemptedFrames == 1U);
  CHECK(driver.transportSnapshot().completedFrames == 0U);
  bus.custom = false;
  CHECK(driver.readClause22(2, value, BUDGET).ok());
  CHECK(driver.end().ok());

  // Valid completed write evidence survives a later terminal primary failure.
  CHECK(driver.bind(bus.config()).ok());
  bus.custom = true; bus.response = Controller::complete(FrameKind::WRITE);
  bus.response.status = {Err::MDIO_ERROR, -39, "error after completion"};
  CHECK(driver.writeClause22(0x18, 0x0692, BUDGET).is(Err::MDIO_ERROR));
  CHECK(driver.transportSnapshot().completedFrames == 1U);
  CHECK(driver.transportSnapshot().lastBatch.frames[0].completed);
  CHECK(bus.written == 0x0692U);
  CHECK(driver.end().ok());

  // Controller-lock timeout is one callback, zero physical frame attempts.
  Controller contended; Driver waiting;
  CHECK(waiting.bind(contended.config()).ok());
  contended.custom = true;
  contended.response.status = {Err::MDIO_TIMEOUT, -83, "lock timeout"};
  contended.response.releaseStatus = Status::Ok();
  contended.response.controllerReusable = true;
  value = SENTINEL;
  const Status lockTimeout = waiting.readClause22(2, value, BUDGET);
  CHECK(lockTimeout.is(Err::MDIO_TIMEOUT) && lockTimeout.detail == -83);
  CHECK(value == SENTINEL);
  CHECK(contended.callbacks == 1U && contended.launches == 0U && !contended.locked);
  const auto receipt = waiting.transportSnapshot();
  CHECK(receipt.totalBatches == 1U && receipt.attemptedFrames == 0U && receipt.completedFrames == 0U);
  CHECK(receipt.violation == TransportViolation::NONE);
  contended.custom = false;
  CHECK(waiting.readClause22(2, value, BUDGET).ok());
  CHECK(contended.callbacks == 2U && contended.launches == 1U);
  CHECK(waiting.end().ok());
}

void testTiming() {
  // 100 raw + 2 lag + (3 acquire + 5 frame + 2 release + 4 overhead).
  for (const IoBudget budget : {IoBudget{100, 1}, IoBudget{101, 1},
                                IoBudget{1000, 0}, IoBudget{116, 1}, IoBudget{117, 1}}) {
    Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
    uint16_t value = SENTINEL;
    const Status status = driver.readClause22(2, value, budget);
    const bool admitted = budget.deadlineUs == 117U;
    CHECK(status.is(admitted ? Err::OK : Err::OPERATION_TIMEOUT));
    CHECK(bus.callbacks == (admitted ? 1U : 0U));
    CHECK(value == (admitted ? 0x2000U : SENTINEL));
    if (!admitted) {
      const TimeoutDetail reason = budget.deadlineUs == 100U ? TimeoutDetail::DEADLINE :
          budget.maxFrames == 0U ? TimeoutDetail::FRAME_BUDGET : TimeoutDetail::BOUND_ADMISSION;
      CHECK(status.detail == static_cast<int32_t>(reason));
    } else {
      CHECK(bus.lastBudget.deadlineUs == budget.deadlineUs && bus.lastBudget.maxFrames == budget.maxFrames);
    }
    CHECK(driver.end().ok());
  }
  // Successful callback can still violate aggregate publication allowance.
  for (uint64_t duration : {uint64_t{11}, uint64_t{17}, uint64_t{20}}) {
    Controller bus; bus.duration = duration;
    Driver driver; CHECK(driver.bind(bus.config()).ok());
    uint16_t value = SENTINEL;
    const Status status = driver.readClause22(2, value, {117, 1});
    CHECK(status.is(Err::OPERATION_TIMEOUT) && value == SENTINEL);
    CHECK(status.detail == static_cast<int32_t>(duration >= 17U
        ? TimeoutDetail::DEADLINE : TimeoutDetail::BOUND_ADMISSION));
    CHECK(driver.transportSnapshot().late == (duration >= 17U));
    CHECK(driver.transportSnapshot().completedFrames == 1U);
    CHECK(bus.callbacks == 1U && !bus.locked);
    CHECK(driver.end().ok());
  }
  {
    Controller bus; bus.duration = 20;
    Driver driver; CHECK(driver.bind(bus.config()).ok());
    CHECK(driver.writeClause22(0x18, 0x0010, {117, 1}).is(Err::OPERATION_TIMEOUT));
    CHECK(bus.written == 0x0010U && bus.callbacks == 1U);
    CHECK(driver.transportSnapshot().lastBatch.frames[0].completed);
    CHECK(driver.transportSnapshot().late);
    CHECK(driver.end().ok());
  }
  for (bool primaryFailure : {false, true}) {
    Controller bus; bus.regress = true;
    Driver driver; CHECK(driver.bind(bus.config()).ok());
    if (primaryFailure) {
      bus.custom = true; bus.response = Controller::complete();
      bus.response.status = {Err::MDIO_TIMEOUT, -31, "primary"};
    }
    uint16_t value = SENTINEL;
    const Status status = driver.readClause22(2, value, BUDGET);
    CHECK(status.is(primaryFailure ? Err::MDIO_TIMEOUT : Err::INVALID_CONFIG));
    CHECK(value == SENTINEL && driver.transportSnapshot().clockInvalid);
    bus.now = 500; bus.regress = false;
    const unsigned clocks = bus.clocks;
    CHECK(driver.readClause22(2, value, BUDGET).is(Err::INVALID_CONFIG));
    CHECK(bus.callbacks == 1U && bus.clocks == clocks);
    CHECK(driver.end().ok());
    bus.custom = false;
    CHECK(driver.bind(bus.config()).ok());
    CHECK(driver.readClause22(2, value, BUDGET).ok());
    CHECK(driver.end().ok());
  }
  {
    Controller bus; bus.now = std::numeric_limits<uint64_t>::max() - 1U;
    Driver driver; CHECK(driver.bind(bus.config()).ok());
    uint16_t value = SENTINEL;
    CHECK(driver.readClause22(2, value, {std::numeric_limits<uint64_t>::max(), 1}).is(Err::INVALID_CONFIG));
    CHECK(value == SENTINEL && bus.callbacks == 0U);
    CHECK(driver.transportSnapshot().clockInvalid);
    CHECK(driver.end().ok());
  }
  {
    Controller bus; bus.now = 0; bus.duration = 0;
    Driver driver; CHECK(driver.bind(bus.config()).ok());
    uint16_t value = SENTINEL;
    CHECK(driver.readClause22(2, value, BUDGET).ok()); // Zero/frozen time cannot cause loops.
    CHECK(driver.readClause22(2, value, BUDGET).ok());
    CHECK(bus.callbacks == 2U && !driver.transportSnapshot().clockInvalid);
    CHECK(driver.end().ok());
  }
  {
    Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
    bus.custom = true; bus.response = Controller::complete();
    bus.response.frames[1].attempted = true;
    bus.regress = true;
    uint16_t value = SENTINEL;
    CHECK(driver.readClause22(2, value, BUDGET).is(Err::MDIO_ERROR));
    CHECK(value == SENTINEL && driver.transportSnapshot().clockInvalid);
    const unsigned clocks = bus.clocks;
    const Status later = driver.readClause22(2, value, BUDGET);
    CHECK(later.is(Err::INVALID_CONFIG));
    CHECK(later.detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    CHECK(bus.callbacks == 1U && bus.clocks == clocks);
    CHECK(driver.end().ok());
  }
  // No modular wrap or narrowing near the nonwrapping uint64 boundary.
  {
    Controller bus; bus.now = std::numeric_limits<uint64_t>::max() - 20U;
    Driver driver; CHECK(driver.bind(bus.config()).ok());
    uint16_t value = SENTINEL;
    CHECK(driver.readClause22(2, value, {std::numeric_limits<uint64_t>::max(), 1}).ok());
    CHECK(value == 0x2000U && !driver.transportSnapshot().clockInvalid);
    CHECK(driver.end().ok());
  }
  // Lower bound proves lateness, but primary transport failure remains primary.
  {
    Controller bus; bus.duration = 20; bus.custom = true;
    bus.response = Controller::complete();
    bus.response.status = {Err::MDIO_TIMEOUT, -71, "late primary"};
    Driver driver; CHECK(driver.bind(bus.config()).ok());
    uint16_t value = SENTINEL;
    const Status status = driver.readClause22(2, value, {117, 1});
    CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail == -71);
    CHECK(value == SENTINEL && driver.transportSnapshot().late);
    CHECK(driver.end().ok());
  }
}

void testOwnership() {
  Controller bus;
  Driver first, second;
  CHECK(first.bind(bus.config(0)).ok());
  CHECK(second.bind(bus.config(31)).ok());
  bus.reenter = true; bus.active = &first; bus.competing = &second;
  uint16_t value = SENTINEL;
  CHECK(first.readClause22(2, value, BUDGET).ok());
  CHECK(value == 0x2000U && bus.callbacks == 2U && bus.launches == 1U);
  CHECK(!bus.locked && bus.releases == 1U);
  CHECK(first.transportSnapshot().totalBatches == 1U);
  CHECK(second.transportSnapshot().totalBatches == 1U);
  CHECK(second.transportSnapshot().attemptedFrames == 0U);
  bus.reenter = false;
  CHECK(second.readClause22(2, value, BUDGET).ok());
  CHECK(bus.lastRequest.phyAddress == 31U && bus.launches == 2U);
  CHECK(first.end().ok()); CHECK(second.end().ok());

  bus.clockReenter = true; bus.active = &first;
  CHECK(first.bind(bus.config()).ok());
  CHECK(first.readClause22(2, value, BUDGET).ok());
  CHECK(first.end().ok());
  bus.clockReenter = false;

  // Independent contexts have independent clocks, receipts and results.
  Controller other; other.readValue = 0x2211;
  CHECK(first.bind(bus.config()).ok());
  CHECK(second.bind(other.config()).ok());
  CHECK(first.readClause22(2, value, BUDGET).ok() && value == 0x2000U);
  CHECK(second.readClause22(3, value, BUDGET).ok() && value == 0x2211U);
  CHECK(first.transportSnapshot().lastBatch.frames[0].value == 0x2000U);
  CHECK(second.transportSnapshot().lastBatch.frames[0].value == 0x2211U);
  CHECK(first.end().ok()); CHECK(second.end().ok());

  for (bool primaryFailure : {false, true}) {
    Controller failed; Driver driver;
    CHECK(driver.bind(failed.config()).ok());
    failed.custom = true; failed.response = Controller::complete();
    failed.response.releaseStatus = {Err::MDIO_ERROR, -37, "release failed"};
    failed.response.controllerReusable = false;
    if (primaryFailure) failed.response.status = {Err::MDIO_TIMEOUT, -29, "primary failed"};
    value = SENTINEL;
    const Status status = driver.readClause22(2, value, BUDGET);
    CHECK(status.is(primaryFailure ? Err::MDIO_TIMEOUT : Err::MDIO_ERROR));
    CHECK(status.detail == (primaryFailure ? -29 : -37));
    CHECK(value == SENTINEL && failed.quarantined);
    const auto receipt = driver.transportSnapshot();
    CHECK(receipt.lastBatch.releaseStatus.detail == -37);
    CHECK(!receipt.controllerReusable);
    CHECK(driver.readClause22(2, value, BUDGET).is(Err::BUSY));
    CHECK(failed.launches == 1U);
    CHECK(driver.end().ok());
    CHECK(driver.bind(failed.config()).ok());
    CHECK(driver.readClause22(2, value, BUDGET).is(Err::BUSY)); // Local lifecycle cannot repair bus.
    CHECK(failed.launches == 1U);
    failed.locked = false; failed.quarantined = false; failed.custom = false; // Owner repair.
    CHECK(driver.readClause22(2, value, BUDGET).ok());
    CHECK(failed.launches == 2U);
    CHECK(driver.end().ok());
  }
  // Valid release but newly uncertain controller remains failure; a primary
  // error retains precedence while the additional evidence remains visible.
  for (bool primaryFailure : {false, true}) {
    Controller failed; Driver driver; CHECK(driver.bind(failed.config()).ok());
    failed.custom = true; failed.response = Controller::complete();
    failed.response.controllerReusable = false;
    if (primaryFailure) failed.response.status = {Err::MDIO_TIMEOUT, -89, "primary failed"};
    CHECK(driver.readClause22(2, value, BUDGET).is(primaryFailure ? Err::MDIO_TIMEOUT : Err::MDIO_ERROR));
    CHECK(driver.transportSnapshot().violation == TransportViolation::CONTROLLER_STATE);
    CHECK(driver.end().ok());
  }
  // Illegal release codes and details are retained but cannot authorize reuse.
  for (Status release : {Status{Err::IN_PROGRESS, -5, "bad release"},
                         Status{static_cast<Err>(255), -6, "bad release"},
                         Status{Err::OK, 1, "bad detail"},
                         Status{Err::OK, 0, nullptr}}) {
    Controller failed; Driver driver; CHECK(driver.bind(failed.config()).ok());
    failed.custom = true; failed.response = Controller::complete();
    failed.response.releaseStatus = release;
    value = SENTINEL;
    CHECK(driver.readClause22(2, value, BUDGET).is(Err::MDIO_ERROR));
    CHECK(value == SENTINEL);
    CHECK(driver.transportSnapshot().lastBatch.releaseStatus.detail == release.detail);
    CHECK(driver.transportSnapshot().violation == TransportViolation::RELEASE_REPORT);
    CHECK(driver.end().ok());
  }
  {
    Controller failed; Driver driver; CHECK(driver.bind(failed.config()).ok());
    failed.custom = true;
    failed.response = Controller::admission(BusyDetail::OWNER_ADMISSION);
    failed.response.releaseStatus = {Err::MDIO_TIMEOUT, -57, "release timeout"};
    failed.response.controllerReusable = false;
    value = SENTINEL;
    const Status status = driver.readClause22(2, value, BUDGET);
    CHECK(status.is(Err::BUSY));
    CHECK(status.detail == static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
    CHECK(value == SENTINEL);
    CHECK(driver.transportSnapshot().lastBatch.releaseStatus.detail == -57);
    CHECK(driver.transportSnapshot().attemptedFrames == 0U);
    CHECK(!driver.transportSnapshot().controllerReusable);
    CHECK(driver.end().ok());
  }
  {
    Controller failed; Driver driver; CHECK(driver.bind(failed.config()).ok());
    failed.custom = true; failed.response = Controller::complete();
    failed.response.releaseStatus = {Err::MDIO_ERROR, -60, "release failed"};
    // Contradictory reuse must not erase the raw release or authorize another call.
    failed.response.controllerReusable = true;
    value = SENTINEL;
    CHECK(driver.readClause22(2, value, BUDGET).is(Err::MDIO_ERROR));
    CHECK(value == SENTINEL && !driver.transportSnapshot().controllerReusable);
    CHECK(driver.transportSnapshot().lastBatch.releaseStatus.detail == -60);
    CHECK(driver.transportSnapshot().lastBatch.controllerReusable);
    CHECK(driver.readClause22(2, value, BUDGET).is(Err::BUSY));
    CHECK(failed.callbacks == 1U);
    CHECK(driver.end().ok());
  }
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  if (std::strcmp(argv[1], "binding") == 0) testBinding();
  else if (std::strcmp(argv[1], "direct") == 0) testDirect();
  else if (std::strcmp(argv[1], "reports") == 0) testReports();
  else if (std::strcmp(argv[1], "timing") == 0) testTiming();
  else if (std::strcmp(argv[1], "ownership") == 0) testOwnership();
  else if (std::strcmp(argv[1], "--verify-failure-path") == 0) CHECK(false);
  else return 2;
  std::printf("%u checks; %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
