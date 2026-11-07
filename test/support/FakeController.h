#pragma once

#include "Check.h"
#include <DP8382x/DP8382x.h>
#include <array>

namespace test {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;

// One bounded controller model for direct and indirect protocol tests. This is
// software evidence only; neither its timing nor its register values model PHY
// qualification. Fault injection deliberately permits malformed terminal reports.
struct WireFrame {
  FrameKind kind{FrameKind::READ};
  uint8_t phy{0};
  uint8_t reg{0};
  uint16_t value{0};
  uint16_t selected{0};
};

enum class ReportFault {
  NONE, HOLE, CONTINUED_AFTER_FAILURE, MISSING_READ_VALUE, UNUSED_ATTEMPT,
  WRITE_READ_VALUE, NONTERMINAL_STATUS, UNKNOWN_STATUS, NULL_MESSAGE,
  REGRESSING_TIMESTAMP, EARLY_TIMESTAMP, TIMESTAMP_WITHOUT_VALIDITY, SHORT_OK
};

struct FakeController {
  explicit FakeController(bool indirect = false) noexcept : mmdModel(indirect) {}
  bool mmdModel;
  uint64_t duration{5};
  bool custom{false};
  bool regress{false};
  uint16_t readValue{0x2000};
  uint16_t written{0};
  BatchResult response{};
  void (*onFrame)(FakeController&, const IoBudget&) noexcept {nullptr};
  void (*onClock)(FakeController&) noexcept {nullptr};
  uint64_t now{100};
  uint64_t frameDuration{2};
  unsigned clocks{0};
  unsigned callbacks{0};
  unsigned acquisitions{0};
  unsigned releases{0};
  unsigned launches{0};
  bool locked{false};
  bool quarantined{false};
  bool releaseFail{false};
  int failAt{-1};
  int expireAfter{-1};
  ReportFault reportFault{ReportFault::NONE};
  bool acceptFailedWrite{false};
  Driver* active{nullptr};
  Driver* competing{nullptr};
  std::array<uint16_t, 32> selector{};
  std::array<uint16_t, 32> address{};
  std::array<uint16_t, 32> led{};
  std::array<WireFrame, 16> trace{};
  unsigned traceCount{0};
  MdioBatch lastRequest{};
  IoBudget lastBudget{};

  Config config(uint32_t phy = 1) noexcept {
    Config result;
    result.variant = Variant::DP83826I;
    result.interface = MacInterface::RMII;
    result.phyAddress = phy;
    const TransportBounds bounds = mmdModel ? TransportBounds{8, 4, 1}
                                           : TransportBounds{9, 5, 2};
    result.transport = {this, execute, clock, bounds};
    return result;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<FakeController*>(context);
    ++bus.clocks;
    if (bus.onClock != nullptr) bus.onClock(bus);
    return bus.now;
  }
  void write(uint8_t phy, uint8_t reg, uint16_t value) noexcept {
    written = value;
    if (reg == 0x0DU) { selector[phy] = value; }
    else if (reg == 0x0EU && selector[phy] == 0x001FU) { address[phy] = value; }
    else if (reg == 0x0EU && selector[phy] == 0x401FU && address[phy] == 0x0018U) { led[phy] = value; }
    else if (reg == 0x18U) { led[phy] = value; }
  }
  uint16_t read(uint8_t phy, uint8_t reg) const noexcept {
    if (!mmdModel) return readValue;
    if (reg == 0x0EU && selector[phy] == 0x001FU) return address[phy];
    if ((reg == 0x0EU && selector[phy] == 0x401FU && address[phy] == 0x0018U) || reg == 0x18U)
      return led[phy];
    return 0x2000;
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
    result.status = {Err::BUSY, static_cast<int32_t>(reason), "Owner admission"};
    result.releaseStatus = Status::Ok();
    result.controllerReusable = reason != BusyDetail::CONTROLLER_QUARANTINED;
    return result;
  }
  static BatchResult execute(void* context, const MdioBatch& batch,
                             const IoBudget& budget) noexcept {
    auto& bus = *static_cast<FakeController*>(context);
    ++bus.callbacks;
    if (bus.quarantined) return admission(BusyDetail::CONTROLLER_QUARANTINED);
    if (bus.locked) return admission(BusyDetail::OWNER_ADMISSION);
    CHECK(batch.frameCount >= 1U && batch.frameCount <= 8U);
    CHECK(batch.phyAddress < 32U);
    // The wrapper validates descriptors before acquisition, then executes frames
    // inside the existing scope. No mediator reentry or nested lock is used.
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& frame = batch.frames[i];
      CHECK(frame.registerAddress <= 31U);
      if (frame.kind == FrameKind::READ) {
        CHECK(frame.value == 0U && frame.preserveMask == 0U && frame.readIndex == 0xFFU);
      } else if (frame.readIndex == 0xFFU) {
        CHECK(frame.preserveMask == 0U);
      } else {
        CHECK(frame.readIndex < i);
        CHECK(batch.frames[frame.readIndex].kind == FrameKind::READ);
        CHECK((frame.preserveMask & frame.value) == 0U);
      }
    }
    for (uint8_t i = batch.frameCount; i < 8U; ++i) {
      const auto& frame = batch.frames[i];
      CHECK(frame.kind == FrameKind::READ && frame.registerAddress == 0U &&
            frame.value == 0U && frame.preserveMask == 0U && frame.readIndex == 0xFFU);
    }
    bus.locked = true;
    ++bus.acquisitions;
    bus.lastRequest = batch;
    bus.lastBudget = budget;
    bus.traceCount = 0;
    BatchResult result = bus.custom ? bus.response : BatchResult{};
    if (bus.custom) {
      CHECK(batch.frameCount == 1U);
      if (result.frames[0].attempted) {
        ++bus.launches;
        if (batch.frames[0].kind == FrameKind::WRITE)
          bus.write(batch.phyAddress, batch.frames[0].registerAddress, batch.frames[0].value);
      }
      if (bus.onFrame != nullptr) bus.onFrame(bus, budget);
      bus.now = bus.regress ? 99U : bus.now + bus.duration;
    } else {
      result.status = Status::Ok();
      result.releaseStatus = Status::Ok();
      result.controllerReusable = true;
      for (uint8_t i = 0; i < batch.frameCount; ++i) {
        if (bus.now >= budget.deadlineUs) {
          result.status = {Err::OPERATION_TIMEOUT, static_cast<int32_t>(TimeoutDetail::DEADLINE), "Deadline"};
          break;
        }
        auto& completed = result.frames[i];
        const auto& frame = batch.frames[i];
        uint16_t word = frame.value;
        if (frame.kind == FrameKind::WRITE && frame.readIndex != 0xFFU) {
          CHECK(result.frames[frame.readIndex].completed && result.frames[frame.readIndex].valueValid);
          word = static_cast<uint16_t>((result.frames[frame.readIndex].value & frame.preserveMask) | frame.value);
        }
        completed.attempted = true;
        ++bus.launches;
        bus.trace[bus.traceCount++] = {frame.kind, batch.phyAddress, frame.registerAddress, word, bus.address[batch.phyAddress]};
        const bool fail = bus.failAt == static_cast<int>(i);
        if (frame.kind == FrameKind::WRITE && (!fail || bus.acceptFailedWrite))
          bus.write(batch.phyAddress, frame.registerAddress, word);
        if (!fail) {
          completed.completed = true;
          if (frame.kind == FrameKind::READ) {
            completed.valueValid = true;
            completed.value = bus.read(batch.phyAddress, frame.registerAddress);
          }
        } else {
          completed.effectPossible = true;
          result.status = {Err::MDIO_TIMEOUT, -700 - static_cast<int32_t>(i), "Injected terminal failure"};
        }
        bus.now = bus.regress ? 99U : bus.now + (bus.mmdModel ? bus.frameDuration : bus.duration);
        if (bus.onFrame != nullptr) bus.onFrame(bus, budget);
        if (bus.expireAfter == static_cast<int>(i)) bus.now = budget.deadlineUs;
        if (fail) break;
      }
    }
    // Deliberately corrupt terminal metadata only after the modeled wire steps.
    if (bus.reportFault != ReportFault::NONE) {
      switch (bus.reportFault) {
        case ReportFault::HOLE: result.frames[1] = {}; break; // Hole followed by attempts.
        case ReportFault::CONTINUED_AFTER_FAILURE: result.frames[1].completed = false; break; // Continued after failure.
        case ReportFault::MISSING_READ_VALUE: result.frames[3].valueValid = false; break;
        case ReportFault::UNUSED_ATTEMPT:
          CHECK(batch.frameCount < result.frames.size());
          result.frames[batch.frameCount].attempted = true;
          break;
        case ReportFault::WRITE_READ_VALUE: result.frames[0].valueValid = true; break; // WRITE cannot supply READ.
        case ReportFault::NONTERMINAL_STATUS: result.status = {Err::IN_PROGRESS, -37, "Illegal terminal"}; break;
        case ReportFault::UNKNOWN_STATUS: result.status = {static_cast<Err>(255), -38, "Unknown terminal"}; break;
        case ReportFault::NULL_MESSAGE: result.status.msg = nullptr; break;
        case ReportFault::REGRESSING_TIMESTAMP: result.frames[0].timeValid = true; result.frames[0].completedAtUs = bus.now;
                result.frames[1].timeValid = true; result.frames[1].completedAtUs = bus.now - 1U; break;
        case ReportFault::EARLY_TIMESTAMP: result.frames[0].timeValid = true; result.frames[0].completedAtUs = 99; break;
        case ReportFault::TIMESTAMP_WITHOUT_VALIDITY: result.frames[2].completedAtUs = bus.now; break;
        case ReportFault::SHORT_OK: result.frames[3] = {}; break;
        case ReportFault::NONE: break;
      }
    }
    if (bus.releaseFail) {
      result.releaseStatus = {Err::MDIO_ERROR, -901, "Release failed"};
      result.controllerReusable = false;
      bus.quarantined = true;
    }
    if (result.releaseStatus.ok()) {
      bus.locked = false;
      ++bus.releases;
    }
    if (!result.releaseStatus.ok() || !result.controllerReusable) bus.quarantined = true;
    return result;
  }
};

} // namespace test
