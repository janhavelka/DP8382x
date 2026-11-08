#include "support/Check.h"
#include <DP8382x/DP8382x.h>

#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;

// The independent model follows the literal table 8-19 reset and table 8-31/32
// selector protocol. Its timings exercise software contracts, not PHY timing.
struct Wire {
  FrameKind kind;
  uint8_t phy;
  uint8_t reg;
  uint16_t value;
};
enum class Fault { NONE, NONTERMINAL, SHORT, MISSING_VALUE };
struct Controller {
  struct Phy {
    uint16_t id1{0x2000}, id2{0xA131}, bmcr{0x3100};
    uint16_t sor1{0x1234}, sor2{0x5678}, vod{0x8400}, led{0};
    uint16_t selector{0}, address{0};
    unsigned resetReads{0};
  };
  std::array<Phy, 32> phys{};
  std::array<Wire, 512> trace{};
  std::array<uint8_t, 128> batchSizes{};
  std::array<IoBudget, 128> budgets{};
  uint64_t now{100}, frameTime{2};
  TransportBounds bounds{8, 4, 1};
  unsigned clocks{0}, calls{0}, acquisitions{0}, releases{0}, frames{0};
  unsigned jumpClockCall{0};
  uint64_t jumpClockTo{0};
  bool locked{false}, quarantined{false}, stickyReset{false};
  bool acceptedFailure{true}, failedEffectPossible{true};
  int failFrame{-1}, releaseBatch{-1}, reusableFalseBatch{-1}, faultyBatch{-1}, lateBatch{-1}, regressBatch{-1};
  int overrideVod{-1};
  Fault fault{Fault::NONE};
  Driver* active{nullptr};
  Driver* competitor{nullptr};
  OperationToken activeToken{};
  bool exerciseReentry{false};
  unsigned excluded{0};

  Config config(uint32_t address = 1) noexcept {
    Config value;
    value.variant = Variant::DP83826I;
    value.interface = MacInterface::RMII;
    value.phyAddress = address;
    value.transport = {this, execute, clock, bounds};
    return value;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    ++bus.clocks;
    if (bus.clocks == bus.jumpClockCall) bus.now = bus.jumpClockTo;
    return bus.now;
  }
  uint16_t read(Phy& phy, uint8_t reg) noexcept {
    if (reg == 0x00) {
      if ((phy.bmcr & 0x8000U) && !stickyReset) {
        if (phy.resetReads == 0) phy.bmcr = 0x3100;
        else --phy.resetReads;
      }
      return phy.bmcr;
    }
    if (reg == 0x1F) return 0; // No external PHYRCR command in this PCS fixture.
    if (reg == 0x02) return phy.id1;
    if (reg == 0x03) return phy.id2;
    if (reg == 0x18) return phy.led;
    if (reg == 0x0E && phy.selector == 0x401F) {
      if (phy.address == 0x0467) return phy.sor1;
      if (phy.address == 0x0468) return phy.sor2;
      if (phy.address == 0x030E) return overrideVod < 0 ? phy.vod : static_cast<uint16_t>(overrideVod);
      if (phy.address == 0x0018) return phy.led;
    }
    return 0; // Cable/link down is intentional; initialization must still work.
  }
  void write(Phy& phy, uint8_t reg, uint16_t word) noexcept {
    if (reg == 0x00) phy.bmcr = word;
    else if (reg == 0x0D) phy.selector = word;
    else if (reg == 0x0E && phy.selector == 0x001F) phy.address = word;
    else if (reg == 0x0E && phy.selector == 0x401F) {
      if (phy.address == 0x030E) phy.vod = word;
      if (phy.address == 0x0018) phy.led = word;
    } else if (reg == 0x18) phy.led = word;
  }
  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget& budget) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    const unsigned call = bus.calls++;
    BatchResult result;
    result.releaseStatus = Status::Ok();
    result.controllerReusable = !bus.quarantined;
    if (bus.locked || bus.quarantined) {
      result.status = {Err::BUSY, static_cast<int32_t>(bus.quarantined ? BusyDetail::CONTROLLER_QUARANTINED : BusyDetail::OWNER_ADMISSION), "Controller busy"};
      return result;
    }
    CHECK(call < bus.batchSizes.size());
    bus.batchSizes[call] = batch.frameCount;
    bus.budgets[call] = budget;
    CHECK(batch.frameCount > 0 && batch.frameCount <= 8);
    CHECK(budget.maxFrames >= batch.frameCount);
    bus.locked = true;
    ++bus.acquisitions;
    result.status = Status::Ok();
    auto& phy = bus.phys[batch.phyAddress];
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& frame = batch.frames[i];
      auto& receipt = result.frames[i];
      CHECK(frame.registerAddress < 32);
      CHECK(frame.readIndex == 0xFF); // Initialization has no RMW command.
      CHECK(frame.preserveMask == 0);
      CHECK(bus.frames < bus.trace.size());
      bus.trace[bus.frames] = {frame.kind, batch.phyAddress, frame.registerAddress, frame.value};
      const bool failed = static_cast<int>(bus.frames) == bus.failFrame;
      ++bus.frames;
      receipt.attempted = true;
      if (frame.kind == FrameKind::WRITE && (!failed || bus.acceptedFailure))
        bus.write(phy, frame.registerAddress, frame.value);
      if (failed) {
        receipt.effectPossible = bus.failedEffectPossible;
        result.status = {Err::MDIO_TIMEOUT, -1701, "Injected frame timeout"};
      } else {
        receipt.completed = true;
        if (frame.kind == FrameKind::READ) {
          receipt.valueValid = true;
          receipt.value = bus.read(phy, frame.registerAddress);
        }
      }
      bus.now += bus.frameTime;
      if (bus.exerciseReentry) {
        uint16_t output = 0xBEEF;
        if (bus.active) {
          const auto status = bus.active->readClause22(2, output, budget);
          CHECK(status.is(Err::BUSY));
          CHECK(status.detail == static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
          CHECK(bus.active->pollOperation(bus.activeToken).is(Err::BUSY));
          CHECK(bus.active->cancel(bus.activeToken).is(Err::BUSY));
          CHECK(bus.active->end().is(Err::BUSY));
          CHECK(bus.active->abandonCleanup().is(Err::BUSY));
          CHECK(output == 0xBEEF);
        }
        if (bus.competitor) {
          const auto status = bus.competitor->readMmd(31, 0x0467, output, budget);
          CHECK(status.is(Err::BUSY));
          CHECK(status.detail == static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
          CHECK(output == 0xBEEF);
        }
        ++bus.excluded;
      }
      if (failed) break;
    }
    if (static_cast<int>(call) == bus.faultyBatch) {
      if (bus.fault == Fault::NONTERMINAL) result.status = {Err::IN_PROGRESS, -2147483647, "Nonterminal callback"};
      if (bus.fault == Fault::SHORT) result.frames[batch.frameCount - 1] = {};
      if (bus.fault == Fault::MISSING_VALUE) result.frames[batch.frameCount - 1].valueValid = false;
    }
    if (static_cast<int>(call) == bus.lateBatch) bus.now = budget.deadlineUs;
    if (static_cast<int>(call) == bus.regressBatch) bus.now = 1;
    if (static_cast<int>(call) == bus.releaseBatch) {
      result.releaseStatus = {Err::MDIO_ERROR, -901, "Injected release failure"};
      result.controllerReusable = false;
      bus.quarantined = true;
    } else {
      ++bus.releases;
      bus.locked = false;
    }
    if (static_cast<int>(call) == bus.reusableFalseBatch) {
      result.controllerReusable = false;
      bus.quarantined = true;
    }
    return result;
  }
};

OperationBudget budget(uint32_t frames = 128, uint64_t deadline = 100000) {
  return {{deadline, frames}, 10, 8};
}
OperationToken start(Driver& driver, ResetKind reset = ResetKind::NONE, OperationBudget allowance = budget()) {
  OperationToken token{0xDEAD};
  CHECK(driver.startInitialize({reset}, allowance, token).is(Err::IN_PROGRESS));
  CHECK(token.value != 0 && token.value != 0xDEAD);
  return token;
}
Status drain(Driver& driver, Controller& bus, OperationToken token, unsigned limit = 100) {
  Status status{Err::IN_PROGRESS, 0, "Pending"};
  for (unsigned i = 0; i < limit && status.is(Err::IN_PROGRESS); ++i) {
    const auto snapshot = driver.operationSnapshot();
    if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
    const unsigned previous = bus.acquisitions;
    status = driver.pollOperation(token);
    CHECK(bus.acquisitions - previous <= 1);
    CHECK(!bus.locked || bus.quarantined);
  }
  CHECK(!status.is(Err::IN_PROGRESS));
  return status;
}
OperationResult take(Driver& driver, OperationToken token) {
  OperationResult result;
  CHECK(driver.takeResult(token, result).ok());
  CHECK(result.token.value == token.value);
  return result;
}
void advanceBatches(Driver& driver, Controller& bus, OperationToken token, unsigned count) {
  for (unsigned i = 0; i < count; ++i) {
    const auto snapshot = driver.operationSnapshot();
    if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
    CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
  }
}
void wire(const Controller& bus, unsigned index, FrameKind kind, uint8_t reg, uint16_t value = 0, uint8_t phy = 1) {
  CHECK(index < bus.frames);
  CHECK(bus.trace[index].kind == kind);
  CHECK(bus.trace[index].phy == phy);
  CHECK(bus.trace[index].reg == reg);
  CHECK(bus.trace[index].value == value);
}
void indirect(const Controller& bus, unsigned first, uint16_t address, bool write, uint16_t word = 0) {
  wire(bus, first, FrameKind::WRITE, 0x0D, 0x001F);
  wire(bus, first + 1, FrameKind::WRITE, 0x0E, address);
  wire(bus, first + 2, FrameKind::WRITE, 0x0D, 0x401F);
  wire(bus, first + 3, write ? FrameKind::WRITE : FrameKind::READ, 0x0E, word);
}

void initialization() {
  for (const auto reset : {ResetKind::NONE, ResetKind::PCS}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto token = start(driver, reset);
    CHECK(bus.clocks == 0 && bus.calls == 0);
    CHECK(!driver.isInitialized());
    CHECK(drain(driver, bus, token).ok());
    CHECK(driver.isInitialized());
    CHECK(bus.frames == (reset == ResetKind::NONE ? 20U : 32U));
    CHECK(bus.calls == (reset == ResetKind::NONE ? 7U : 12U));
    CHECK(bus.acquisitions == bus.releases);
    wire(bus, 0, FrameKind::READ, 0x02);
    wire(bus, 1, FrameKind::READ, 0x03);
    indirect(bus, 2, 0x0467, false);
    indirect(bus, 6, 0x0468, false);
    wire(bus, 10, FrameKind::READ, 0x00);
    wire(bus, 11, FrameKind::READ, 0x1F);
    unsigned vod = 12;
    if (reset == ResetKind::PCS) {
      wire(bus, 12, FrameKind::WRITE, 0x00, 0x8000);
      wire(bus, 13, FrameKind::READ, 0x00);
      wire(bus, 14, FrameKind::READ, 0x02);
      wire(bus, 15, FrameKind::READ, 0x03);
      indirect(bus, 16, 0x0467, false);
      indirect(bus, 20, 0x0468, false);
      vod = 24;
    }
    indirect(bus, vod, 0x030E, true, 0x4A40);
    indirect(bus, vod + 4, 0x030E, false);
    const auto result = take(driver, token);
    CHECK(result.status.ok() && result.persistentRestored && !result.cleanupOwed);
    CHECK(result.mutationAttempted && result.mutationCompleted);
    CHECK(!result.temporaryMayBeActive && !result.clockInvalid && !result.late);
    CHECK(result.commandRequired == (reset == ResetKind::PCS));
    CHECK(result.commandAccepted == (reset == ResetKind::PCS));
    CHECK(result.protocolComplete == (reset == ResetKind::PCS));
    CHECK(result.identity.id1 == 0x2000 && result.identity.id2 == 0xA131);
    CHECK(result.identity.acceptance == IdentityAcceptance::ACCEPTED && !result.identity.stale);
    CHECK(result.straps.sor1 == 0x1234 && result.straps.sor2 == 0x5678);
    CHECK(result.straps.sor1Valid && result.straps.sor2Valid && result.straps.timeValid && !result.straps.stale);
    CHECK(!result.mismatch.valid);
    CHECK(bus.phys[1].vod == 0x4A40);
    for (unsigned i = 0; i < bus.frames; ++i) CHECK(bus.trace[i].reg != 0x17 && bus.trace[i].reg != 0x1E);
    for (unsigned i = 1; i < bus.calls; ++i) {
      CHECK(bus.budgets[i].deadlineUs == bus.budgets[0].deadlineUs);
      CHECK(bus.budgets[i].maxFrames < bus.budgets[i - 1].maxFrames);
    }
  }
  // The lower six reserved VOD bits do not participate in stable verification.
  for (const auto observed : {0x4A7F, 0x4A00, 0x8400}) {
    Controller bus;
    bus.overrideVod = observed;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto token = start(driver);
    const auto status = drain(driver, bus, token);
    const auto result = take(driver, token);
    CHECK(status.ok() == (observed == 0x4A7F));
    if (observed != 0x4A7F) {
      CHECK(status.is(Err::CONFIG_MISMATCH));
      CHECK(result.cleanupOwed && !result.persistentRestored);
      CHECK(result.mismatch.valid && result.mismatch.mmd && result.mismatch.devad == 31);
      CHECK(result.mismatch.registerAddress == 0x030E && result.mismatch.mask == 0xFFC0);
      CHECK(result.mismatch.expected == 0x4A40 && result.mismatch.observed == observed);
      CHECK(!driver.isInitialized());
    }
  }
  // NONE must wait for an externally active PCS reset and recapture evidence.
  Controller bus;
  bus.phys[1].bmcr = 0x8000;
  bus.phys[1].resetReads = 1;
  Driver driver;
  CHECK(driver.bind(bus.config()).ok());
  const auto token = start(driver);
  CHECK(drain(driver, bus, token).ok());
  CHECK(bus.frames == 31);
  const auto result = take(driver, token);
  CHECK(!result.commandAccepted && !result.protocolComplete);
  for (unsigned i = 0; i < bus.frames; ++i)
    CHECK(!(bus.trace[i].kind == FrameKind::WRITE && bus.trace[i].reg == 0));
}

void admission() {
  Controller bus;
  Driver driver;
  OperationToken token{0xCAFE};
  auto allowance = budget();
  allowance.pollIntervalUs = 0;
  CHECK(driver.startInitialize({}, allowance, token).is(Err::INVALID_PARAM));
  CHECK(token.value == 0xCAFE);
  allowance = budget();
  allowance.maxStalledPolls = 0;
  CHECK(driver.startInitialize({}, allowance, token).is(Err::INVALID_PARAM));
  CHECK(driver.startInitialize({static_cast<ResetKind>(255)}, budget(), token).is(Err::INVALID_PARAM));
  CHECK(driver.startInitialize({}, budget(), token).is(Err::NOT_BOUND));
  CHECK(driver.bind(bus.config()).ok());
  CHECK(driver.startInitialize({}, budget(0), token).is(Err::OPERATION_TIMEOUT));
  CHECK(driver.startInitialize({}, budget(1, 0), token).is(Err::OPERATION_TIMEOUT));
  CHECK(bus.clocks == 0 && bus.calls == 0 && token.value == 0xCAFE);
  const auto active = start(driver);
  CHECK(driver.startInitialize({}, allowance, token).is(Err::INVALID_PARAM));
  const auto busy = driver.startInitialize({}, budget(), token);
  CHECK(busy.is(Err::BUSY) && busy.detail == static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
  OperationResult output;
  output.token.value = 0xFACE;
  CHECK(driver.takeResult(active, output).is(Err::RESULT_NOT_AVAILABLE));
  CHECK(output.token.value == 0xFACE);
  CHECK(driver.pollOperation({0}).is(Err::TOKEN_MISMATCH));
  CHECK(driver.cancel({active.value + 1}).is(Err::TOKEN_MISMATCH));
  CHECK(bus.clocks == 0 && bus.calls == 0);
  CHECK(driver.cancel(active).ok());
  CHECK(driver.startInitialize({}, budget(), token).detail == static_cast<int32_t>(BusyDetail::RESULT_PENDING));
  take(driver, active);

  // Poll slices yield without splitting the four-frame indirect batch.
  const auto sliced = start(driver);
  CHECK(driver.pollOperation(sliced, 0).is(Err::IN_PROGRESS));
  CHECK(driver.pollOperation(sliced, 1).is(Err::IN_PROGRESS));
  CHECK(bus.calls == 0);
  CHECK(driver.pollOperation(sliced, 2).is(Err::IN_PROGRESS));
  CHECK(bus.calls == 1 && bus.frames == 2);
  for (uint32_t slice : {0U, 1U, 2U, 3U}) {
    CHECK(driver.pollOperation(sliced, slice).is(Err::IN_PROGRESS));
    CHECK(bus.calls == 1 && bus.frames == 2);
  }
  CHECK(driver.pollOperation(sliced, 4).is(Err::IN_PROGRESS));
  CHECK(bus.calls == 2 && bus.frames == 6);
  CHECK(drain(driver, bus, sliced).ok());
  take(driver, sliced);

  // Total budget and bound admission precede the cooperative slice.
  for (uint32_t total : {1U, 5U, 18U}) {
    Controller limited;
    Driver subject;
    CHECK(subject.bind(limited.config()).ok());
    const auto limitedToken = start(subject, ResetKind::NONE, budget(total));
    const auto status = drain(subject, limited, limitedToken);
    CHECK(status.is(Err::OPERATION_TIMEOUT));
    CHECK(status.detail == static_cast<int32_t>(TimeoutDetail::FRAME_BUDGET));
    CHECK(limited.frames <= total);
    take(subject, limitedToken);
  }
  Controller tiny;
  Driver subject;
  CHECK(subject.bind(tiny.config()).ok());
  const auto tinyToken = start(subject, ResetKind::NONE, budget(1));
  const auto status = subject.pollOperation(tinyToken, 0);
  CHECK(status.is(Err::OPERATION_TIMEOUT) && status.detail == static_cast<int32_t>(TimeoutDetail::FRAME_BUDGET));
  CHECK(tiny.calls == 0);
}

void timing() {
  // Independent OP-6 arithmetic: B(1)=150, 4*lag=40000, wait=1000.
  // Equality at 41300 is rejected; 41301 permits the literal command.
  for (uint64_t deadline : {41300ULL, 41301ULL}) {
    Controller bus;
    bus.now = 0;
    bus.frameTime = 0;
    bus.bounds = {50, 100, 10000};
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    auto allowance = budget(128, deadline);
    allowance.pollIntervalUs = 1000;
    const auto token = start(driver, ResetKind::PCS, allowance);
    advanceBatches(driver, bus, token, 5);
    const auto status = driver.pollOperation(token);
    if (deadline == 41300) {
      CHECK(status.is(Err::OPERATION_TIMEOUT));
      CHECK(status.detail == static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
      CHECK(bus.frames == 12);
      const auto result = take(driver, token);
      CHECK(!result.mutationAttempted && !result.cleanupOwed);
    } else {
      CHECK(status.is(Err::IN_PROGRESS));
      CHECK(bus.frames == 13);
      CHECK(driver.operationSnapshot().nextPollAtUs == 11000);
      const unsigned calls = bus.calls;
      bus.now = 10000;
      CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
      CHECK(bus.calls == calls);
      bus.now = 20000;
      CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
      CHECK(bus.calls == calls + 1 && bus.frames == 14);
    }
  }
  // Only one remaining command frame is insufficient even though its own bound fits.
  Controller frameLimited;
  Driver frameDriver;
  CHECK(frameDriver.bind(frameLimited.config()).ok());
  const auto frameToken = start(frameDriver, ResetKind::PCS, budget(13));
  advanceBatches(frameDriver, frameLimited, frameToken, 5);
  CHECK(frameDriver.pollOperation(frameToken).detail == static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
  CHECK(frameLimited.frames == 12);

  Controller overflowing;
  overflowing.frameTime = 0;
  overflowing.now = std::numeric_limits<uint64_t>::max() - 30;
  Driver overflowDriver;
  CHECK(overflowDriver.bind(overflowing.config()).ok());
  const auto overflowToken = start(overflowDriver, ResetKind::PCS,
      budget(128, std::numeric_limits<uint64_t>::max()));
  advanceBatches(overflowDriver, overflowing, overflowToken, 5);
  CHECK(overflowDriver.pollOperation(overflowToken).detail == static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
  CHECK(overflowing.frames == 12);
  CHECK(!take(overflowDriver, overflowToken).mutationAttempted);

  // Scheduling between the poll sample and callback admission cannot bypass
  // reset look-ahead. A single write would fit at 99980; write/wait/read cannot.
  Controller preempted;
  Driver preemptedDriver;
  CHECK(preemptedDriver.bind(preempted.config()).ok());
  const auto preemptedToken = start(preemptedDriver, ResetKind::PCS);
  advanceBatches(preemptedDriver, preempted, preemptedToken, 5);
  preempted.jumpClockCall = preempted.clocks + 2;
  preempted.jumpClockTo = 99980;
  CHECK(preemptedDriver.pollOperation(preemptedToken).detail == static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
  CHECK(preempted.frames == 12 && preempted.calls == 5);
  CHECK(!take(preemptedDriver, preemptedToken).mutationAttempted);

  // Completion-based anchor: actual callback consumes time before the interval starts.
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config()).ok());
  const auto token = start(driver, ResetKind::PCS);
  advanceBatches(driver, bus, token, 5);
  const auto before = bus.now;
  CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
  const auto due = driver.operationSnapshot().nextPollAtUs;
  CHECK(due == before + 2 + 1 + 10);
  const auto calls = bus.calls;
  bus.now = due - 1;
  CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS) && bus.calls == calls);
  bus.now = due;
  CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS) && bus.calls == calls + 1);

  // Frozen time is bounded even when no physical frames consume time.
  Controller frozen;
  frozen.frameTime = 0;
  Driver stopped;
  CHECK(stopped.bind(frozen.config()).ok());
  auto finite = budget();
  finite.maxStalledPolls = 3;
  const auto frozenToken = start(stopped, ResetKind::PCS, finite);
  advanceBatches(stopped, frozen, frozenToken, 6);
  CHECK(stopped.pollOperation(frozenToken).is(Err::IN_PROGRESS));
  CHECK(stopped.operationSnapshot().stalledPolls == 1);
  CHECK(stopped.pollOperation(frozenToken).is(Err::IN_PROGRESS));
  CHECK(stopped.pollOperation(frozenToken).detail == static_cast<int32_t>(TimeoutDetail::POLL_BUDGET));
  CHECK(frozen.frames == 13 && !frozen.locked);
  CHECK(take(stopped, frozenToken).cleanupOwed);

  Controller slices;
  Driver noWork;
  CHECK(noWork.bind(slices.config()).ok());
  const auto noWorkToken = start(noWork, ResetKind::NONE, finite);
  CHECK(noWork.pollOperation(noWorkToken, 0).is(Err::IN_PROGRESS));
  CHECK(noWork.operationSnapshot().stalledPolls == 0);
  CHECK(noWork.pollOperation(noWorkToken, 0).is(Err::IN_PROGRESS));
  ++slices.now;
  CHECK(noWork.pollOperation(noWorkToken, 0).is(Err::IN_PROGRESS));
  CHECK(noWork.operationSnapshot().stalledPolls == 0);
  CHECK(noWork.pollOperation(noWorkToken, 0).is(Err::IN_PROGRESS));
  CHECK(noWork.pollOperation(noWorkToken, 0).is(Err::IN_PROGRESS));
  CHECK(noWork.pollOperation(noWorkToken, 0).detail == static_cast<int32_t>(TimeoutDetail::POLL_BUDGET));
  CHECK(slices.calls == 0);

  // Exact raw expiry wins over slice admission and leaves ordinary I/O untouched.
  Controller expired;
  Driver timed;
  CHECK(timed.bind(expired.config()).ok());
  const auto expiredToken = start(timed, ResetKind::NONE, budget(128, expired.now));
  CHECK(timed.pollOperation(expiredToken, 0).detail == static_cast<int32_t>(TimeoutDetail::DEADLINE));
  CHECK(expired.calls == 0);
  // Clock overflow and regression latch until an explicit end/rebind.
  for (bool overflow : {false, true}) {
    Controller broken;
    Driver subject;
    CHECK(subject.bind(broken.config()).ok());
    const auto brokenToken = start(subject, ResetKind::NONE, budget(128, std::numeric_limits<uint64_t>::max()));
    if (overflow) broken.now = std::numeric_limits<uint64_t>::max();
    else { CHECK(subject.pollOperation(brokenToken).is(Err::IN_PROGRESS)); broken.now = 1; }
    const auto fault = subject.pollOperation(brokenToken);
    CHECK(fault.is(Err::INVALID_CONFIG) && fault.detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    CHECK(take(subject, brokenToken).clockInvalid);
    OperationToken rejected{0xFACE};
    const unsigned clocks = broken.clocks;
    CHECK(subject.startInitialize({}, budget(), rejected).detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    CHECK(rejected.value == 0xFACE && broken.clocks == clocks);
    CHECK(subject.abandonCleanup().ok());
    CHECK(subject.end().ok());
    broken.now = 100;
    CHECK(subject.bind(broken.config()).ok());
    CHECK(drain(subject, broken, start(subject)).ok());
  }
  // Sticky reset has no invented silicon maximum; finite caller budgets end it.
  Controller sticky;
  sticky.stickyReset = true;
  Driver subject;
  CHECK(subject.bind(sticky.config()).ok());
  const auto stickyToken = start(subject, ResetKind::PCS, budget(16));
  CHECK(drain(subject, sticky, stickyToken).is(Err::OPERATION_TIMEOUT));
  const auto result = take(subject, stickyToken);
  CHECK(result.commandAccepted && !result.protocolComplete && result.cleanupOwed);
  CHECK(sticky.frames == 16 && sticky.phys[1].vod == 0x8400);
}

void failures() {
  for (const auto reset : {ResetKind::NONE, ResetKind::PCS}) {
    const int total = reset == ResetKind::NONE ? 20 : 32;
    const int target = reset == ResetKind::NONE ? 15 : 27;
    for (int failed = 0; failed < total; ++failed) {
      Controller bus;
      bus.failFrame = failed;
      Driver driver;
      CHECK(driver.bind(bus.config()).ok());
      const auto token = start(driver, reset);
      const auto status = drain(driver, bus, token);
      CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail == -1701);
      CHECK(bus.frames == static_cast<unsigned>(failed + 1));
      CHECK(driver.transportSnapshot().attemptedFrames == static_cast<unsigned>(failed + 1));
      CHECK(driver.transportSnapshot().completedFrames == static_cast<unsigned>(failed));
      CHECK(bus.acquisitions == bus.releases && !bus.locked);
      const auto result = take(driver, token);
      CHECK(result.status.is(Err::MDIO_TIMEOUT) && result.status.detail == -1701);
      CHECK(result.failingBatch.normalized.is(Err::MDIO_TIMEOUT));
      CHECK(!result.persistentRestored && !driver.isInitialized());
      const bool semantic = failed >= target || (reset == ResetKind::PCS && failed >= 12);
      CHECK(result.cleanupOwed == semantic);
      CHECK(result.mutationAttempted == semantic);
      if (failed == target || (reset == ResetKind::PCS && failed == 12)) CHECK(result.mutationPossible);
      if (failed == 1) CHECK(result.identity.id1Valid && !result.identity.id2Valid && result.identity.id1 == 0x2000);
      if (reset == ResetKind::PCS && failed >= 13 && failed < 24) {
        CHECK(result.straps.sor1Valid && result.straps.sor2Valid && result.straps.stale);
      }
      // Selector failures do not authorize a target effect or automatic recovery.
      CHECK(driver.operationSnapshot().cleanupOwed == semantic);
      const auto calls = bus.calls;
      CHECK(driver.pollOperation(token).is(Err::RESULT_NOT_AVAILABLE));
      CHECK(bus.calls == calls);
    }
  }
  // Explicit proof that an unaccepted, effect-free failed command owes no replay.
  Controller noEffect;
  noEffect.failFrame = 12;
  noEffect.acceptedFailure = false;
  noEffect.failedEffectPossible = false;
  Driver clean;
  CHECK(clean.bind(noEffect.config()).ok());
  const auto noEffectToken = start(clean, ResetKind::PCS);
  CHECK(drain(clean, noEffect, noEffectToken).is(Err::MDIO_TIMEOUT));
  const auto cleanResult = take(clean, noEffectToken);
  CHECK(cleanResult.mutationAttempted && !cleanResult.mutationCompleted && !cleanResult.mutationPossible && !cleanResult.cleanupOwed);

  for (const auto reset : {ResetKind::NONE, ResetKind::PCS}) {
    for (int batch = 0; batch < (reset == ResetKind::NONE ? 7 : 12); ++batch) {
      Controller bus;
      bus.releaseBatch = batch;
      Driver driver;
      CHECK(driver.bind(bus.config()).ok());
      const auto token = start(driver, reset);
      CHECK(drain(driver, bus, token).is(Err::MDIO_ERROR));
      const auto result = take(driver, token);
      CHECK(result.failingBatch.release.detail == -901);
      CHECK(bus.acquisitions == static_cast<unsigned>(batch + 1) && bus.releases == static_cast<unsigned>(batch));
      CHECK(result.cleanupOwed == (batch >= 5));
      if (batch == 1 || batch == 2 || (reset == ResetKind::PCS && (batch == 8 || batch == 9))) {
        CHECK(result.straps.sor1Valid && result.straps.sor1 == 0x1234);
        if (batch == 2 || batch >= 8) CHECK(result.straps.sor2Valid && result.straps.sor2 == 0x5678);
        CHECK(result.straps.stale && !result.straps.timeValid);
      }
      uint16_t output = 0xCAFE;
      CHECK(driver.readClause22(2, output, {100000, 1}).is(Err::BUSY));
      CHECK(output == 0xCAFE);
    }
  }
  Controller unreusable;
  unreusable.reusableFalseBatch = 0;
  Driver stopped;
  CHECK(stopped.bind(unreusable.config()).ok());
  const auto stoppedToken = start(stopped);
  CHECK(drain(stopped, unreusable, stoppedToken).is(Err::MDIO_ERROR));
  const auto stoppedResult = take(stopped, stoppedToken);
  CHECK(stoppedResult.failingBatch.release.is(Err::MDIO_ERROR));
  CHECK(stoppedResult.failingBatch.violation == TransportViolation::CONTROLLER_STATE);
  // A primary frame error survives a separate failed release.
  Controller both;
  both.failFrame = 15;
  both.releaseBatch = 5;
  Driver driver;
  CHECK(driver.bind(both.config()).ok());
  const auto token = start(driver);
  CHECK(drain(driver, both, token).detail == -1701);
  const auto result = take(driver, token);
  CHECK(result.status.detail == -1701 && result.failingBatch.release.detail == -901 && result.cleanupOwed);

  for (const auto fault : {Fault::NONTERMINAL, Fault::SHORT, Fault::MISSING_VALUE}) {
    Controller bus;
    bus.faultyBatch = 6;
    bus.fault = fault;
    Driver subject;
    CHECK(subject.bind(bus.config()).ok());
    const auto badToken = start(subject);
    CHECK(drain(subject, bus, badToken).is(Err::MDIO_ERROR));
    const auto badResult = take(subject, badToken);
    CHECK(badResult.cleanupOwed && !badResult.persistentRestored);
    CHECK(badResult.failingBatch.violation != TransportViolation::NONE);
    if (fault == Fault::NONTERMINAL) CHECK(badResult.status.detail == -2147483647);
    CHECK(bus.frames == 20);
  }
  for (int late : {5, 6}) {
    Controller bus;
    bus.lateBatch = late;
    Driver subject;
    CHECK(subject.bind(bus.config()).ok());
    const auto lateToken = start(subject);
    CHECK(drain(subject, bus, lateToken).detail == static_cast<int32_t>(TimeoutDetail::DEADLINE));
    const auto lateResult = take(subject, lateToken);
    CHECK(lateResult.late && lateResult.cleanupOwed && !lateResult.persistentRestored);
    CHECK(lateResult.mutationCompleted);
  }
  Controller regressed;
  regressed.regressBatch = 5;
  Driver clockDriver;
  CHECK(clockDriver.bind(regressed.config()).ok());
  const auto clockToken = start(clockDriver);
  CHECK(drain(clockDriver, regressed, clockToken).detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  CHECK(take(clockDriver, clockToken).cleanupOwed);
}

void lifecycle() {
  // Cancellation at every externally reachable batch boundary is silent and frozen.
  for (const auto reset : {ResetKind::NONE, ResetKind::PCS}) {
    const unsigned batches = reset == ResetKind::NONE ? 7 : 12;
    for (unsigned completed = 0; completed < batches; ++completed) {
      Controller bus;
      Driver driver;
      CHECK(driver.bind(bus.config()).ok());
      const auto token = start(driver, reset);
      advanceBatches(driver, bus, token, completed);
      const unsigned calls = bus.calls, clocks = bus.clocks;
      CHECK(driver.end().is(Err::BUSY) && driver.bind(bus.config()).is(Err::BUSY));
      CHECK(driver.cancel(token).ok());
      CHECK(driver.pollOperation(token).is(Err::CANCELLED));
      CHECK(driver.cancel(token).ok());
      CHECK(driver.end().detail == static_cast<int32_t>(BusyDetail::RESULT_PENDING));
      CHECK(bus.calls == calls && bus.clocks == clocks);
      const auto result = take(driver, token);
      CHECK(result.status.is(Err::CANCELLED));
      const bool effects = completed >= 6;
      CHECK(result.cleanupOwed == effects);
      CHECK(!result.temporaryMayBeActive);
      if (effects) {
        CHECK(driver.end().detail == static_cast<int32_t>(BusyDetail::CLEANUP_REQUIRED));
        CHECK(driver.abandonCleanup().ok());
      }
      CHECK(driver.end().ok());
      CHECK(bus.calls == calls && bus.clocks == clocks);
      OperationResult sentinel;
      sentinel.token.value = 0xCAFE;
      CHECK(driver.takeResult(token, sentinel).is(Err::RESULT_NOT_AVAILABLE));
      CHECK(sentinel.token.value == 0xCAFE);
      CHECK(driver.bind(bus.config()).ok());
      const auto newer = start(driver);
      CHECK(newer.value > token.value);
      CHECK(driver.pollOperation(token).is(Err::TOKEN_MISMATCH));
      CHECK(driver.cancel(newer).ok());
    }
  }
  // Failed restoration keeps the barrier and its own result; explicit restoration
  // uses a new token and never rewrites an earlier cancelled result.
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config()).ok());
  const auto original = start(driver, ResetKind::PCS);
  advanceBatches(driver, bus, original, 6);
  CHECK(driver.cancel(original).ok());
  const auto cancelled = take(driver, original);
  CHECK(cancelled.cleanupOwed && cancelled.commandAccepted && !cancelled.protocolComplete);
  bus.phys[1].bmcr = 0x3100;
  const auto failed = start(driver);
  bus.failFrame = static_cast<int>(bus.frames + 15);
  CHECK(drain(driver, bus, failed).is(Err::MDIO_TIMEOUT));
  const auto failedResult = take(driver, failed);
  CHECK(failedResult.cleanupOwed && failedResult.status.is(Err::MDIO_TIMEOUT));
  CHECK(driver.writeClause22(0x18, 0, {100000, 1}).detail == static_cast<int32_t>(BusyDetail::CLEANUP_REQUIRED));
  bus.failFrame = -1;
  const auto restored = start(driver);
  CHECK(drain(driver, bus, restored).ok());
  CHECK(take(driver, restored).persistentRestored);
  CHECK(!driver.operationSnapshot().cleanupOwed && driver.isInitialized());
  CHECK(cancelled.status.is(Err::CANCELLED) && failedResult.status.is(Err::MDIO_TIMEOUT));
  CHECK(original.value < failed.value && failed.value < restored.value);

  // Reapply NONE keeps initialized until mandatory mutation is unverified.
  const auto reapply = start(driver);
  CHECK(driver.isInitialized());
  advanceBatches(driver, bus, reapply, 5);
  CHECK(driver.isInitialized());
  advanceBatches(driver, bus, reapply, 1);
  CHECK(!driver.isInitialized());
  CHECK(drain(driver, bus, reapply).ok());
  CHECK(driver.isInitialized());
  take(driver, reapply);

  const auto unchanged = start(driver);
  bus.failFrame = static_cast<int>(bus.frames + 15);
  bus.acceptedFailure = false;
  bus.failedEffectPossible = false;
  CHECK(drain(driver, bus, unchanged).is(Err::MDIO_TIMEOUT));
  CHECK(driver.isInitialized());
  const auto unchangedResult = take(driver, unchanged);
  CHECK(unchangedResult.mutationAttempted && !unchangedResult.mutationPossible && !unchangedResult.cleanupOwed);
  bus.failFrame = -1;
  bus.acceptedFailure = true;
  bus.failedEffectPossible = true;

  const auto historicalSuccess = start(driver);
  CHECK(drain(driver, bus, historicalSuccess).ok());
  CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION).ok());
  CHECK(driver.identitySnapshot().stale);
  CHECK(!take(driver, historicalSuccess).identity.stale);

  const auto runningIdentity = start(driver);
  advanceBatches(driver, bus, runningIdentity, 1);
  CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION).ok());
  CHECK(take(driver, runningIdentity).identity.stale);

  // A pending terminal copy remains historical after external reset clears the
  // current barrier. It is not rewritten to look like successful restoration.
  const auto pending = start(driver, ResetKind::PCS);
  advanceBatches(driver, bus, pending, 6);
  CHECK(driver.cancel(pending).ok());
  CHECK(driver.operationSnapshot().cleanupOwed);
  CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
  CHECK(!driver.operationSnapshot().cleanupOwed);
  const auto historical = take(driver, pending);
  CHECK(historical.status.is(Err::CANCELLED) && historical.cleanupOwed);
  bus.phys[1].bmcr = 0x3100;

  for (const auto reason : {InvalidationReason::EXTERNAL_MUTATION, InvalidationReason::EXTERNAL_RESET, InvalidationReason::EXTERNAL_POWER_ON}) {
    const auto invalidated = start(driver, ResetKind::PCS);
    advanceBatches(driver, bus, invalidated, 6);
    const auto calls = bus.calls, clocks = bus.clocks;
    CHECK(driver.invalidateDeviceState(reason).ok());
    CHECK(bus.calls == calls && bus.clocks == clocks && !driver.isInitialized());
    const auto retained = take(driver, invalidated);
    CHECK(retained.status.is(Err::INVALID_CONFIG) && retained.status.detail == static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
    CHECK(retained.cleanupOwed == (reason == InvalidationReason::EXTERNAL_MUTATION));
    CHECK(driver.abandonCleanup().ok());
    bus.phys[1].bmcr = 0x3100;
  }

  // An external live identity observation cannot leave an operation authorized
  // by its earlier, now contradictory identity. The ordinary read keeps its own
  // status; the separately retained operation reports invalidation.
  for (const auto replacement : {0xA111, 0xA132}) {
    Controller changed;
    Driver subject;
    CHECK(subject.bind(changed.config()).ok());
    const auto changing = start(subject);
    advanceBatches(subject, changed, changing, 1);
    changed.phys[1].id2 = static_cast<uint16_t>(replacement);
    DeviceIdentity observed;
    const auto read = subject.readIdentity(observed, {100000, 2});
    CHECK(read.ok() == (replacement == 0xA111));
    CHECK(subject.operationSnapshot().state == OperationState::RESULT_PENDING);
    const auto changedResult = take(subject, changing);
    CHECK(changedResult.status.is(Err::INVALID_CONFIG));
    CHECK(changedResult.status.detail == static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
    CHECK(changedResult.identity.stale);
    CHECK(!changedResult.mutationAttempted && changed.frames == 4);
  }
}

void ownership() {
  Controller bus;
  Driver driver, other;
  CHECK(driver.bind(bus.config()).ok());
  CHECK(other.bind(bus.config(2)).ok());
  bus.active = &driver;
  bus.competitor = &other;
  const auto token = start(driver, ResetKind::PCS);
  bus.activeToken = token;
  bus.exerciseReentry = true;
  CHECK(drain(driver, bus, token).ok());
  CHECK(bus.excluded == 32 && bus.acquisitions == 12 && bus.releases == 12);
  CHECK(bus.phys[1].vod == 0x4A40 && bus.phys[2].vod == 0x8400);
  CHECK(!other.isInitialized());
  bus.exerciseReentry = false;
  take(driver, token);
  const auto second = start(other);
  CHECK(drain(other, bus, second).ok());
  CHECK(bus.phys[2].vod == 0x4A40);
  CHECK(take(other, second).status.ok());

  Controller interleaved;
  Driver subject;
  CHECK(subject.bind(interleaved.config()).ok());
  const auto running = start(subject, ResetKind::PCS);
  uint16_t output = 0xCAFE;
  CHECK(subject.readClause22(2, output, {100000, 1}).ok() && output == 0x2000);
  CHECK(subject.writeClause22(0x18, 0, {100000, 1}).detail == static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
  advanceBatches(subject, interleaved, running, 6);
  const auto calls = interleaved.calls, clocks = interleaved.clocks;
  output = 0xCAFE;
  CHECK(subject.readClause22(2, output, {100000, 1}).is(Err::BUSY));
  DeviceIdentity identity;
  identity.id1 = 0xDEAD;
  CHECK(subject.readIdentity(identity, {100000, 2}).is(Err::BUSY) && identity.id1 == 0xDEAD);
  CHECK(subject.readMmd(31, 0x0467, output, {100000, 4}).is(Err::BUSY));
  CHECK(output == 0xCAFE && interleaved.calls == calls && interleaved.clocks == clocks);
  interleaved.now = subject.operationSnapshot().nextPollAtUs;
  CHECK(subject.readClause22(2, output, {100000, 1}).is(Err::BUSY));
  CHECK(interleaved.calls == calls); // Due time alone does not leave the wait phase.
  CHECK(subject.cancel(running).ok());
  CHECK(subject.readClause22(2, output, {100000, 1}).ok());
  CHECK(subject.writeClause22(0x18, 0, {100000, 1}).detail == static_cast<int32_t>(BusyDetail::RESULT_PENDING));
  const auto afterReadCalls = interleaved.calls, afterReadClocks = interleaved.clocks;
  (void)subject.operationSnapshot();
  (void)subject.identitySnapshot();
  (void)subject.capabilities();
  (void)subject.evidenceSnapshot();
  (void)subject.transportSnapshot();
  (void)subject.isInitialized();
  CHECK(interleaved.calls == afterReadCalls && interleaved.clocks == afterReadClocks);
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"initialization", initialization}, {"admission", admission}, {"timing", timing},
    {"failures", failures}, {"lifecycle", lifecycle}, {"ownership", ownership}
  };
  return test::runSuites(argc, argv, suites);
}
