#include "support/Check.h"
#include <DP8382x/DP8382x.h>

#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;

// The independent model follows the literal tables 8-19/48 resets and tables 8-31/32
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
    uint16_t id1{0x2000}, id2{0xA131}, bmcr{0x3100}, phyrcr{0};
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
  bool resampleOnHard{false};
  uint64_t ownerQuietUntil{0};
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
    if (reg == 0x1F) {
      if (!stickyReset) phy.phyrcr = 0;
      return phy.phyrcr;
    }
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
    else if (reg == 0x1F) {
      phy.phyrcr = word;
      if ((word & 0x8000U) != 0U) {
        phy.vod = 0x8400;
        phy.led = 0;
        phy.bmcr = 0x3100;
        if (resampleOnHard) {
          phy.id2 = 0xA111;
          phy.sor1 = 0x00A5;
          phy.sor2 = 0x005A;
        }
      }
    }
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
    if (bus.locked || bus.quarantined || bus.now < bus.ownerQuietUntil) {
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
  return {{deadline, frames}, 1000, 8};
}
OperationToken start(Driver& driver, ResetKind reset = ResetKind::HARD, OperationBudget allowance = budget()) {
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
    if (snapshot.busQuietValid && bus.now < snapshot.busQuietUntilUs) bus.now = snapshot.busQuietUntilUs;
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

void toCommand(Driver& driver, Controller& bus, OperationToken token) {
  for (unsigned i = 0; i < 20 && driver.operationSnapshot().phase != OperationPhase::COMMAND; ++i) {
    const auto snapshot = driver.operationSnapshot();
    if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
    CHECK(driver.pollOperation(token).inProgress());
  }
  CHECK(driver.operationSnapshot().phase == OperationPhase::COMMAND);
}

void traces() {
  for (const auto reset : {ResetKind::HARD, ResetKind::RESTART}) {
    Controller bus;
    bus.resampleOnHard = true; // A permitted outcome, not proof of actual resampling.
    bus.phys[1].led = 0x0692;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto token = start(driver, reset);
    CHECK(bus.clocks == 0 && bus.calls == 0);
    toCommand(driver, bus, token);
    CHECK(bus.frames == 12);
    wire(bus, 0, FrameKind::READ, 2);
    wire(bus, 1, FrameKind::READ, 3);
    indirect(bus, 2, 0x0467, false);
    indirect(bus, 6, 0x0468, false);
    wire(bus, 10, FrameKind::READ, 0);
    wire(bus, 11, FrameKind::READ, 0x1F);
    CHECK(driver.pollOperation(token).inProgress());
    wire(bus, 12, FrameKind::WRITE, 0x1F, reset == ResetKind::HARD ? uint16_t{0x8000} : uint16_t{0x4000});
    CHECK(!bus.locked);
    const auto waiting = driver.operationSnapshot();
    CHECK(waiting.nextPollValid);
    CHECK(waiting.busQuietValid == (reset == ResetKind::HARD));
    CHECK(waiting.nextPollAtUs == bus.now + bus.bounds.clockLagUs + (reset == ResetKind::HARD ? 2000U : 1000U));
    if (reset == ResetKind::HARD) {
      CHECK(waiting.busQuietUntilUs == waiting.nextPollAtUs);
      CHECK(!driver.capabilities().valid && driver.identitySnapshot().stale);
      CHECK(bus.phys[1].vod == 0x8400 && bus.phys[1].led == 0);
    } else { CHECK(bus.phys[1].led == 0x0692); }
    CHECK(drain(driver, bus, token).ok());
    wire(bus, 13, FrameKind::READ, 0x1F);
    CHECK(bus.phys[1].vod == 0x4A40 && driver.isInitialized());
    const auto result = take(driver, token);
    CHECK(result.status.ok() && result.commandAccepted && result.protocolComplete && result.persistentRestored);
    CHECK(!result.cleanupOwed && !result.temporaryMayBeActive);
    CHECK(result.identity.mode == (reset == ResetKind::HARD ? OperatingMode::BASIC : OperatingMode::ENHANCED));
    CHECK(result.straps.sor1 == (reset == ResetKind::HARD ? 0x00A5 : 0x1234));
    CHECK(result.straps.sor2 == (reset == ResetKind::HARD ? 0x005A : 0x5678));
    CHECK(!result.straps.stale && result.straps.timeValid);
    CHECK(driver.capabilities().valid);
    // Independent full protocol expectations; renewed addressing after reset.
    wire(bus, 14, FrameKind::READ, 2);
    wire(bus, 15, FrameKind::READ, 3);
    indirect(bus, 16, 0x0467, false);
    indirect(bus, 20, 0x0468, false);
    indirect(bus, 24, 0x030E, true, 0x4A40);
    indirect(bus, 28, 0x030E, false);
    CHECK(bus.frames == 32 && bus.acquisitions == 12 && bus.releases == 12);
    CHECK(driver.transportSnapshot().attemptedFrames == 32);
  }
  // A cleared requested bit is insufficient if the other PHYRCR command remains
  // active. Do not refresh/replay into an overlapping reset or restart.
  for (const auto reset : {ResetKind::HARD, ResetKind::RESTART}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto token = start(driver, reset);
    toCommand(driver, bus, token);
    CHECK(driver.pollOperation(token).inProgress());
    bus.phys[1].phyrcr = reset == ResetKind::HARD ? uint16_t{0x4000} : uint16_t{0x8000};
    bus.stickyReset = true;
    bus.now = driver.operationSnapshot().nextPollAtUs;
    const auto frames = bus.frames;
    CHECK(driver.pollOperation(token).inProgress());
    CHECK(bus.frames == frames + 1 && driver.operationSnapshot().phase == OperationPhase::WAIT);
    CHECK(bus.phys[1].vod == 0x8400);
    bus.stickyReset = false;
    CHECK(drain(driver, bus, token).ok());
    CHECK(take(driver, token).protocolComplete);
  }
  // A previously active PHYRCR command prevents NONE from trusting old settings.
  // Both command bits must clear, even when the requested operation writes none.
  for (const uint16_t active : {uint16_t{0x8000}, uint16_t{0x4000}, uint16_t{0xC000}}) {
    Controller bus;
    bus.phys[1].phyrcr = active;
    bus.stickyReset = true;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto token = start(driver, ResetKind::NONE);
    for (unsigned i = 0; i < 5; ++i) CHECK(driver.pollOperation(token).inProgress());
    CHECK(driver.operationSnapshot().phase == OperationPhase::WAIT);
    CHECK(!driver.isInitialized());
    CHECK(driver.capabilities().valid == (active == 0x4000));
    CHECK(bus.phys[1].vod == 0x8400);
    bus.stickyReset = false;
    CHECK(drain(driver, bus, token).ok());
    const auto result = take(driver, token);
    CHECK(result.persistentRestored && !result.commandAccepted && !result.protocolComplete);
    for (unsigned i = 0; i < bus.frames; ++i)
      CHECK(!(bus.trace[i].reg == 0x1F && bus.trace[i].kind == FrameKind::WRITE));
  }
  // An already active HARD may resample mode. Reaccept that mode before issuing
  // the requested disruptive command, not only after the requested command.
  for (const auto reset : {ResetKind::HARD, ResetKind::RESTART}) {
    Controller bus;
    bus.phys[1].phyrcr = 0x8000;
    bus.stickyReset = true;
    Driver driver;
    auto config = bus.config();
    config.requiredMode = OperatingMode::ENHANCED;
    CHECK(driver.bind(config).ok());
    const auto token = start(driver, reset);
    for (unsigned i = 0; i < 5; ++i) CHECK(driver.pollOperation(token).inProgress());
    CHECK(driver.operationSnapshot().phase == OperationPhase::WAIT);
    bus.phys[1].id2 = 0xA111;
    bus.stickyReset = false;
    const auto status = drain(driver, bus, token);
    CHECK(status.is(Err::DEVICE_MISMATCH) && status.detail == static_cast<int32_t>(IdentityDetail::MODE_MISMATCH));
    const auto result = take(driver, token);
    CHECK(!result.commandAccepted && !result.mutationAttempted && !result.cleanupOwed);
    CHECK(result.identity.mode == OperatingMode::BASIC && !driver.capabilities().valid);
    CHECK(bus.phys[1].vod == 0x8400);
    for (unsigned i = 0; i < bus.frames; ++i)
      CHECK(!(bus.trace[i].kind == FrameKind::WRITE && (bus.trace[i].reg == 0 || bus.trace[i].reg == 0x1F)));
  }
}

void floor() {
  for (const uint32_t interval : {1000U, 2000U, 7000U}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    auto allowance = budget();
    allowance.pollIntervalUs = interval;
    const auto token = start(driver, ResetKind::HARD, allowance);
    toCommand(driver, bus, token);
    CHECK(driver.pollOperation(token).inProgress());
    const auto quiet = driver.operationSnapshot();
    const uint64_t due = bus.now + 1U + (interval < 2000U ? 2000U : interval);
    CHECK(quiet.busQuietValid && quiet.busQuietUntilUs == due);
    const auto frames = bus.frames;
    bus.now = due - 1;
    CHECK(driver.pollOperation(token).inProgress());
    CHECK(bus.frames == frames && !bus.locked);
    CHECK(driver.cancel(token).ok());
    const auto original = take(driver, token);
    CHECK(original.cleanupOwed && original.commandAccepted && !original.protocolComplete);
    CHECK(driver.operationSnapshot().busQuietValid && driver.operationSnapshot().busQuietUntilUs == due);
    uint16_t sentinel = 0xCAFE;
    DeviceIdentity identity; identity.id1 = 0xBEEF;
    EvidenceSnapshot history; history.lastRequested = 0xFACE;
    CHECK(driver.readClause22(2, sentinel, {100000, 1}).is(Err::BUSY));
    CHECK(driver.readMmd(31, 0x0467, sentinel, {100000, 4}).is(Err::BUSY));
    CHECK(driver.readIdentity(identity, {100000, 2}).is(Err::BUSY));
    CHECK(driver.acquireEvidence(evidenceBit(EvidenceSource::BMSR), history, {100000, 1}).is(Err::BUSY));
    CHECK(sentinel == 0xCAFE && identity.id1 == 0xBEEF && history.lastRequested == 0xFACE && bus.frames == frames);
    CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_POWER_ON).ok());
    CHECK(!driver.operationSnapshot().cleanupOwed && driver.operationSnapshot().busQuietUntilUs == due);
    CHECK(driver.abandonCleanup().ok());
    CHECK(driver.operationSnapshot().busQuietValid);
    const auto restoration = start(driver, ResetKind::NONE);
    CHECK(driver.pollOperation(restoration).inProgress() && bus.frames == frames);
    CHECK(driver.operationSnapshot().busQuietUntilUs == due);
    bus.now = due;
    CHECK(driver.pollOperation(restoration).inProgress());
    CHECK(bus.frames == frames + 2 && !driver.operationSnapshot().busQuietValid);
    CHECK(drain(driver, bus, restoration).ok());
    CHECK(take(driver, restoration).persistentRestored);
    CHECK(original.status.is(Err::CANCELLED) && original.cleanupOwed);
  }
  // End/rebind cannot clear an obligation; after explicit transfer the owner
  // retains the physical floor across the local binding lifetime.
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config()).ok());
  const auto token = start(driver);
  toCommand(driver, bus, token);
  CHECK(driver.pollOperation(token).inProgress());
  const auto floorEnd = driver.operationSnapshot().busQuietUntilUs;
  CHECK(driver.cancel(token).ok());
  CHECK(driver.end().detail == static_cast<int32_t>(BusyDetail::RESULT_PENDING));
  take(driver, token);
  CHECK(driver.end().detail == static_cast<int32_t>(BusyDetail::CLEANUP_REQUIRED));
  const auto clocks = bus.clocks, calls = bus.calls;
  CHECK(driver.abandonCleanup().ok() && driver.end().ok() && driver.bind(bus.config()).ok());
  CHECK(bus.clocks == clocks && bus.calls == calls);
  CHECK(!driver.operationSnapshot().busQuietValid);
  bus.now = floorEnd; // Owner explicitly enforces OP-12 before new frames.
  CHECK(drain(driver, bus, start(driver, ResetKind::NONE)).ok());
}

void failures() {
  // A malformed receipt cannot prove that an issued semantic command did not
  // reach hardware. This model executes it, then hides the attempted frame.
  for (const auto reset : {ResetKind::HARD, ResetKind::RESTART}) {
    Controller bus;
    bus.faultyBatch = 5;
    bus.fault = Fault::SHORT;
    Driver driver, other;
    CHECK(driver.bind(bus.config()).ok() && other.bind(bus.config(2)).ok());
    const auto token = start(driver, reset);
    CHECK(drain(driver, bus, token).is(Err::MDIO_ERROR));
    const auto result = take(driver, token);
    CHECK(!result.commandAccepted && !result.mutationAttempted && !result.mutationCompleted);
    CHECK(result.mutationPossible && result.cleanupOwed && result.failingBatch.effectPossible);
    CHECK(result.failingBatch.violation == TransportViolation::FRAME_REPORT);
    CHECK(result.failingBatch.attemptedFrames == 0 && result.failingBatch.completedFrames == 0);
    CHECK(driver.transportSnapshot().stopped && !driver.transportSnapshot().controllerReusable);
    CHECK(bus.phys[1].phyrcr == (reset == ResetKind::HARD ? 0x8000 : 0x4000));
    CHECK(bus.frames == 13 && bus.acquisitions == bus.releases && !bus.locked);
    const auto snapshot = driver.operationSnapshot();
    CHECK(snapshot.busQuietValid == (reset == ResetKind::HARD));
    uint16_t output = 0xCAFE;
    if (reset == ResetKind::HARD) {
      CHECK(snapshot.busQuietUntilUs == bus.now + 1U + 2000U);
      const auto beforeFrames = bus.frames, beforeClocks = bus.clocks;
      CHECK(driver.readClause22(2, output, {100000, 1}).detail == static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED));
      CHECK(output == 0xCAFE && bus.frames == beforeFrames && bus.clocks == beforeClocks);
      bus.ownerQuietUntil = snapshot.busQuietUntilUs;
      CHECK(other.readClause22(2, output, {100000, 1}).detail == static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
      CHECK(output == 0xCAFE);
      bus.now = snapshot.busQuietUntilUs;
      CHECK(other.readClause22(2, output, {100000, 1}).ok() && output == 0x2000);
    }
    CHECK(driver.readClause22(2, output, {100000, 1}).detail == static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED));
    CHECK(result.status.is(Err::MDIO_ERROR) && result.cleanupOwed);
  }
  // Malformed selector/read receipts before the semantic command do not invent
  // a reset or quiet floor. Valid owner denials likewise prove no invocation.
  for (const bool malformed : {false, true}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto token = start(driver);
    if (malformed) {
      bus.faultyBatch = 1;
      bus.fault = Fault::SHORT;
      CHECK(drain(driver, bus, token).is(Err::MDIO_ERROR));
    } else {
      toCommand(driver, bus, token);
      bus.locked = true; // Another owner refuses the complete command batch.
      const auto count = bus.frames;
      CHECK(driver.pollOperation(token).is(Err::BUSY));
      CHECK(bus.frames == count);
      bus.locked = false;
    }
    const auto result = take(driver, token);
    CHECK(!result.mutationAttempted && !result.mutationPossible && !result.cleanupOwed);
    CHECK(!driver.operationSnapshot().busQuietValid && bus.phys[1].phyrcr == 0);
  }
  for (const auto reset : {ResetKind::NONE, ResetKind::HARD, ResetKind::RESTART}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto initial = start(driver, ResetKind::NONE);
    CHECK(drain(driver, bus, initial).ok());
    take(driver, initial);
    CHECK(driver.isInitialized());
    const unsigned base = bus.calls;
    bus.faultyBatch = static_cast<int>(base + (reset == ResetKind::NONE ? 5U : 10U));
    bus.fault = Fault::SHORT; // Execute VOD target, then remove its receipt.
    const auto token = start(driver, reset);
    CHECK(drain(driver, bus, token).is(Err::MDIO_ERROR));
    const auto result = take(driver, token);
    CHECK(result.mutationPossible && result.cleanupOwed && result.failingBatch.effectPossible);
    CHECK(result.failingBatch.violation == TransportViolation::FRAME_REPORT);
    CHECK(result.failingBatch.attemptedFrames == 3 && result.failingBatch.completedFrames == 3);
    CHECK(!driver.isInitialized() && !result.persistentRestored && bus.phys[1].vod == 0x4A40);
    if (reset == ResetKind::NONE) CHECK(!result.mutationAttempted && !result.mutationCompleted);
  }
  for (const auto reset : {ResetKind::HARD, ResetKind::RESTART}) {
    for (int fail = 0; fail < 32; ++fail) {
      Controller bus;
      bus.failFrame = fail;
      Driver driver;
      CHECK(driver.bind(bus.config()).ok());
      const auto token = start(driver, reset);
      const auto status = drain(driver, bus, token);
      CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail == -1701);
      CHECK(bus.frames == static_cast<unsigned>(fail + 1));
      CHECK(driver.transportSnapshot().attemptedFrames == static_cast<unsigned>(fail + 1));
      CHECK(driver.transportSnapshot().completedFrames == static_cast<unsigned>(fail));
      CHECK(!bus.locked && bus.acquisitions == bus.releases);
      const auto result = take(driver, token);
      CHECK(result.cleanupOwed == (fail >= 12));
      CHECK(result.mutationAttempted == (fail >= 12));
      CHECK(!result.persistentRestored && !driver.isInitialized());
      if (fail == 12 && reset == ResetKind::HARD) {
        CHECK(result.mutationPossible && !result.commandAccepted);
        CHECK(driver.operationSnapshot().busQuietValid);
        const auto count = bus.frames;
        uint16_t word = 0xCAFE;
        CHECK(driver.readClause22(2, word, {100000, 1}).is(Err::BUSY));
        CHECK(bus.frames == count && word == 0xCAFE);
      }
    }
    for (int phase = 0; phase < 12; ++phase) {
      Controller bus;
      bus.releaseBatch = phase;
      Driver driver;
      CHECK(driver.bind(bus.config()).ok());
      const auto token = start(driver, reset);
      CHECK(drain(driver, bus, token).is(Err::MDIO_ERROR));
      const auto result = take(driver, token);
      CHECK(result.failingBatch.release.detail == -901);
      CHECK(result.cleanupOwed == (phase >= 5));
      if (phase == 5 && reset == ResetKind::HARD) CHECK(driver.operationSnapshot().busQuietValid);
    }
  }
  // Even a proved effect-free HARD attempt requires silence, but no restoration.
  Controller noEffect;
  noEffect.failFrame = 12;
  noEffect.acceptedFailure = false;
  noEffect.failedEffectPossible = false;
  Driver clean;
  CHECK(clean.bind(noEffect.config()).ok());
  const auto token = start(clean);
  CHECK(drain(clean, noEffect, token).is(Err::MDIO_TIMEOUT));
  CHECK(!take(clean, token).cleanupOwed);
  CHECK(clean.operationSnapshot().busQuietValid);

  for (const auto reset : {ResetKind::HARD, ResetKind::RESTART}) {
    Controller bus;
    bus.overrideVod = 0x8400;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto mismatch = start(driver, reset);
    CHECK(drain(driver, bus, mismatch).is(Err::CONFIG_MISMATCH));
    const auto result = take(driver, mismatch);
    CHECK(result.mismatch.valid && result.mismatch.registerAddress == 0x030E);
    CHECK(result.mismatch.mask == 0xFFC0 && result.mismatch.expected == 0x4A40 && result.mismatch.observed == 0x8400);
    CHECK(result.cleanupOwed && !result.persistentRestored);
    bus.overrideVod = -1;
    const auto restore = start(driver, ResetKind::NONE);
    CHECK(drain(driver, bus, restore).ok());
    CHECK(take(driver, restore).persistentRestored && !driver.operationSnapshot().cleanupOwed);
    CHECK(result.status.is(Err::CONFIG_MISMATCH));
  }
}

void timing() {
  for (const auto reset : {ResetKind::HARD, ResetKind::RESTART}) {
    for (const bool admit : {false, true}) {
      Controller bus;
      bus.bounds = {50, 100, 10000};
      bus.frameTime = 0;
      Driver driver;
      CHECK(driver.bind(bus.config()).ok());
      const uint64_t reserve = reset == ResetKind::HARD ? 42300U : 41300U;
      auto allowance = budget(128, bus.now + reserve + (admit ? 1U : 0U));
      const auto token = start(driver, reset, allowance);
      toCommand(driver, bus, token);
      const auto frames = bus.frames;
      const auto status = driver.pollOperation(token);
      CHECK(status.inProgress() == admit);
      CHECK(bus.frames == frames + (admit ? 1U : 0U));
      if (!admit) {
        CHECK(status.detail == static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
        CHECK(!take(driver, token).mutationAttempted);
      } else {
        const auto due = driver.operationSnapshot().nextPollAtUs;
        CHECK(due == bus.now + 10000U + (reset == ResetKind::HARD ? 2000U : 1000U));
        bus.now = due - 1;
        CHECK(driver.pollOperation(token).inProgress() && bus.frames == frames + 1);
      }
    }
  }
  for (const auto reset : {ResetKind::HARD, ResetKind::RESTART}) {
    Controller bus;
    bus.stickyReset = true;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto token = start(driver, reset, budget(18));
    CHECK(drain(driver, bus, token).is(Err::OPERATION_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.commandAccepted && !result.protocolComplete && result.cleanupOwed);
    CHECK(bus.frames == 18);
  }
  // Frozen time stops at a finite stalled-poll bound while no frame can start.
  Controller frozen;
  Driver stopped;
  CHECK(stopped.bind(frozen.config()).ok());
  auto allowance = budget(); allowance.maxStalledPolls = 3;
  const auto token = start(stopped, ResetKind::HARD, allowance);
  toCommand(stopped, frozen, token);
  CHECK(stopped.pollOperation(token).inProgress());
  const auto frames = frozen.frames;
  CHECK(stopped.pollOperation(token).inProgress());
  CHECK(stopped.pollOperation(token).inProgress());
  CHECK(stopped.pollOperation(token).detail == static_cast<int32_t>(TimeoutDetail::POLL_BUDGET));
  CHECK(frozen.frames == frames && take(stopped, token).cleanupOwed);

  for (const bool late : {false, true}) {
    Controller bus;
    if (late) bus.lateBatch = 5; else bus.regressBatch = 5;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto failing = start(driver);
    CHECK(!drain(driver, bus, failing).ok());
    const auto result = take(driver, failing);
    CHECK(result.commandAccepted && result.cleanupOwed);
    if (late) CHECK(result.late && driver.operationSnapshot().busQuietValid);
    else {
      CHECK(result.clockInvalid && !driver.operationSnapshot().busQuietValid);
      uint16_t word = 0xCAFE;
      CHECK(driver.readClause22(2, word, {100000, 1}).detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
      CHECK(word == 0xCAFE);
    }
  }
  // Overflow of the operation's postwrite due is distinct from clock overflow.
  Controller overflow;
  overflow.frameTime = 0;
  Driver driver;
  CHECK(driver.bind(overflow.config()).ok());
  const auto nearMax = start(driver, ResetKind::HARD, budget(128, std::numeric_limits<uint64_t>::max()));
  toCommand(driver, overflow, nearMax);
  overflow.now = std::numeric_limits<uint64_t>::max() - 1000;
  const auto count = overflow.frames;
  CHECK(driver.pollOperation(nearMax).detail == static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
  CHECK(overflow.frames == count && !take(driver, nearMax).mutationAttempted);

  Controller empty;
  Driver local;
  CHECK(local.bind(empty.config()).ok());
  OperationToken sentinel{0xCAFE};
  CHECK(local.startInitialize({ResetKind::HARD}, budget(0), sentinel).detail == static_cast<int32_t>(TimeoutDetail::FRAME_BUDGET));
  CHECK(local.startInitialize({ResetKind::RESTART}, budget(128, 0), sentinel).detail == static_cast<int32_t>(TimeoutDetail::DEADLINE));
  uint16_t value = 0xBEEF;
  CHECK(local.readClause22(0x8000, value, {100000, 1}).is(Err::INVALID_PARAM));
  CHECK(local.writeClause22(0x8000, 0, {100000, 1}).is(Err::INVALID_PARAM));
  CHECK(value == 0xBEEF && sentinel.value == 0xCAFE && empty.calls == 0 && empty.clocks == 0);

  // An illegally late callback can put completion so close to UINT64_MAX that
  // no hard-floor timestamp is representable. Retain effects and forbid timed I/O.
  Controller endOfTime;
  endOfTime.lateBatch = 5;
  Driver expired;
  CHECK(expired.bind(endOfTime.config()).ok());
  const auto lateToken = start(expired, ResetKind::HARD,
      budget(128, std::numeric_limits<uint64_t>::max() - 1000));
  CHECK(drain(expired, endOfTime, lateToken).detail == static_cast<int32_t>(TimeoutDetail::DEADLINE));
  const auto lateResult = take(expired, lateToken);
  CHECK(lateResult.commandAccepted && lateResult.cleanupOwed && lateResult.late && lateResult.clockInvalid);
  CHECK(!expired.operationSnapshot().busQuietValid);
  CHECK(expired.readClause22(2, value, {std::numeric_limits<uint64_t>::max(), 1}).detail ==
        static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
}

void external() {
  // Cancel at each reachable batch boundary, preserving any attempted HARD floor.
  for (const auto reset : {ResetKind::HARD, ResetKind::RESTART}) {
    for (unsigned batches = 0; batches < 12; ++batches) {
      Controller bus;
      Driver driver;
      CHECK(driver.bind(bus.config()).ok());
      const auto token = start(driver, reset);
      for (unsigned done = 0; done < batches; ++done) {
        const auto snapshot = driver.operationSnapshot();
        if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
        if (snapshot.busQuietValid && bus.now < snapshot.busQuietUntilUs) bus.now = snapshot.busQuietUntilUs;
        CHECK(driver.pollOperation(token).inProgress());
      }
      const auto before = driver.operationSnapshot();
      const auto calls = bus.calls, clocks = bus.clocks;
      CHECK(driver.end().is(Err::BUSY));
      CHECK(driver.cancel(token).ok());
      CHECK(driver.pollOperation(token).is(Err::CANCELLED));
      CHECK(driver.bind(bus.config()).detail == static_cast<int32_t>(BusyDetail::RESULT_PENDING));
      const auto result = take(driver, token);
      CHECK(result.cleanupOwed == (batches >= 6));
      CHECK(bus.calls == calls && bus.clocks == clocks);
      CHECK(driver.operationSnapshot().busQuietValid == before.busQuietValid);
      CHECK(driver.operationSnapshot().busQuietUntilUs == before.busQuietUntilUs);
      CHECK(driver.abandonCleanup().ok());
      CHECK(driver.end().ok());
      CHECK(bus.calls == calls && bus.clocks == clocks);
    }
  }
  for (const auto reason : {InvalidationReason::EXTERNAL_RESET, InvalidationReason::EXTERNAL_POWER_ON,
                           InvalidationReason::EXTERNAL_MUTATION}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    const auto token = start(driver);
    toCommand(driver, bus, token);
    CHECK(driver.pollOperation(token).inProgress());
    const auto before = driver.operationSnapshot();
    const auto clocks = bus.clocks, calls = bus.calls;
    CHECK(driver.invalidateDeviceState(reason).ok());
    CHECK(bus.calls == calls && bus.clocks == clocks && !driver.isInitialized());
    CHECK(!driver.capabilities().valid && driver.identitySnapshot().stale);
    const auto result = take(driver, token);
    CHECK(result.status.detail == static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
    CHECK(result.identity.stale && result.straps.stale);
    CHECK(result.cleanupOwed == (reason == InvalidationReason::EXTERNAL_MUTATION));
    CHECK(driver.operationSnapshot().busQuietValid && driver.operationSnapshot().busQuietUntilUs == before.busQuietUntilUs);
    CHECK(driver.abandonCleanup().ok());
  }
  // RESTART retains initialized and mode evidence until mandatory VOD mutation.
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config()).ok());
  const auto initial = start(driver, ResetKind::NONE);
  CHECK(drain(driver, bus, initial).ok()); take(driver, initial);
  const auto restart = start(driver, ResetKind::RESTART);
  toCommand(driver, bus, restart);
  CHECK(driver.isInitialized() && driver.capabilities().valid);
  CHECK(driver.pollOperation(restart).inProgress());
  CHECK(driver.isInitialized() && driver.capabilities().valid && !driver.operationSnapshot().busQuietValid);
  CHECK(driver.cancel(restart).ok());
  const auto cancelled = take(driver, restart);
  CHECK(cancelled.cleanupOwed && driver.isInitialized());
  CHECK(driver.abandonCleanup().ok() && !driver.isInitialized());
}

void ownership() {
  Controller bus;
  Driver driver, other;
  CHECK(driver.bind(bus.config()).ok() && other.bind(bus.config(2)).ok());
  bus.active = &driver;
  bus.competitor = &other;
  bus.exerciseReentry = true;
  const auto token = start(driver);
  bus.activeToken = token;
  toCommand(driver, bus, token);
  CHECK(driver.pollOperation(token).inProgress());
  CHECK(!bus.locked && bus.acquisitions == bus.releases);
  // Owner exports the floor to all bus participants; the core has no registry.
  bus.ownerQuietUntil = driver.operationSnapshot().busQuietUntilUs;
  uint16_t word = 0xCAFE;
  CHECK(other.readClause22(2, word, {100000, 1}).is(Err::BUSY));
  CHECK(word == 0xCAFE);
  CHECK(drain(driver, bus, token).ok());
  CHECK(bus.excluded == 32 && bus.phys[2].vod == 0x8400);
  CHECK(take(driver, token).persistentRestored);
  bus.exerciseReentry = false;
  CHECK(other.readClause22(2, word, {100000, 1}).ok() && word == 0x2000);
  CHECK(bus.acquisitions == bus.releases && !bus.locked);
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"traces", traces}, {"floor", floor}, {"failures", failures},
    {"timing", timing}, {"external", external}, {"ownership", ownership}
  };
  return test::runSuites(argc, argv, suites);
}
