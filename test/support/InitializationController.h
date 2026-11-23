#pragma once

#include "FakeController.h"

namespace test {

enum class Fault { NONE, NONTERMINAL, SHORT, MISSING_VALUE };

/// Transport for initialization and reset tests. Each suite supplies its own
/// register model. The trace spans callbacks, so failFrame and the resulting
/// receipt refer to the same wire step throughout the complete operation.
template<typename WireModel>
struct InitializationController : ControllerState, WireModel {
  std::array<WireFrame, 512> trace{};
  std::array<uint8_t, 128> batchSizes{};
  std::array<IoBudget, 128> budgets{};
  uint64_t frameTime{2}, clockQuantumUs{1}, ownerQuietUntil{0};
  TransportBounds bounds{8, 4, 1};
  unsigned calls{0}, frames{0}, jumpClockCall{0};
  uint64_t jumpClockTo{0};
  bool acceptedFailure{true}, failedEffectPossible{true};
  int failFrame{-1}, releaseBatch{-1}, reusableFalseBatch{-1}, faultyBatch{-1}, lateBatch{-1}, regressBatch{-1};
  Fault fault{Fault::NONE};
  Driver* active{nullptr};
  Driver* competitor{nullptr};
  OperationToken activeToken{};
  bool exerciseReentry{false};
  unsigned excluded{0};

  Config config(uint32_t address = 1) noexcept {
    Config value;
    value.variant = declaredVariant;
    value.interface = MacInterface::RMII;
    value.phyAddress = address;
    value.transport = {this, execute, clock, bounds};
    return value;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<InitializationController*>(context);
    ++bus.clocks;
    if (bus.clocks == bus.jumpClockCall) bus.now = bus.jumpClockTo;
    return bus.now - bus.now % bus.clockQuantumUs;
  }
  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget& budget) noexcept {
    auto& bus = *static_cast<InitializationController*>(context);
    const unsigned call = bus.calls++;
    BatchResult result;
    result.releaseStatus = Status::Ok();
    result.controllerReusable = !bus.quarantined;
    if (bus.locked || bus.quarantined || bus.now < bus.ownerQuietUntil) {
      result.status = {Err::BUSY, static_cast<int32_t>(bus.quarantined ? BusyDetail::CONTROLLER_QUARANTINED : BusyDetail::OWNER_ADMISSION), "Controller busy"};
      return result;
    }
    CHECK(call < bus.batchSizes.size());
    if (call >= bus.batchSizes.size()) {
      result.status = {Err::MDIO_ERROR, 0, "Test batch trace is full"};
      return result;
    }
    bus.batchSizes[call] = batch.frameCount;
    bus.budgets[call] = budget;
    CHECK(batch.frameCount > 0 && batch.frameCount <= 8);
    CHECK(budget.maxFrames >= batch.frameCount);
    if (batch.frameCount == 0 || batch.frameCount > batch.frames.size() ||
        batch.phyAddress >= bus.phys.size() || budget.maxFrames < batch.frameCount) {
      result.status = {Err::MDIO_ERROR, 0, "Invalid test batch"};
      return result;
    }
    bus.acquire();
    result.status = Status::Ok();
    auto& phy = bus.phys[batch.phyAddress];
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& frame = batch.frames[i];
      auto& receipt = result.frames[i];
      CHECK(frame.registerAddress < 32);
      CHECK(frame.readIndex == 0xFF); // Initialization has no RMW command.
      CHECK(frame.preserveMask == 0);
      CHECK(bus.frames < bus.trace.size());
      if (bus.frames >= bus.trace.size()) {
        result.status = {Err::MDIO_ERROR, 0, "Test frame trace is full"};
        break;
      }
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
    }
    if (static_cast<int>(call) == bus.reusableFalseBatch) {
      result.controllerReusable = false;
    }
    bus.release(result);
    return result;
  }
};

template<typename Model>
void wire(const InitializationController<Model>& bus, unsigned index, FrameKind kind, uint8_t reg, uint16_t value = 0, uint8_t phy = 1) {
  CHECK(index < bus.frames);
  if (index >= bus.frames) return;
  CHECK(bus.trace[index].kind == kind);
  CHECK(bus.trace[index].phy == phy);
  CHECK(bus.trace[index].reg == reg);
  CHECK(bus.trace[index].value == value);
}
template<typename Model>
void indirect(const InitializationController<Model>& bus, unsigned first, uint16_t address, bool write, uint16_t word = 0) {
  wire(bus, first, FrameKind::WRITE, 0x0D, 0x001F);
  wire(bus, first + 1, FrameKind::WRITE, 0x0E, address);
  wire(bus, first + 2, FrameKind::WRITE, 0x0D, 0x401F);
  wire(bus, first + 3, write ? FrameKind::WRITE : FrameKind::READ, 0x0E, word);
}

} // namespace test
