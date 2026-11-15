#include "../../examples/portable/CableCapture.h"
#include <array>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;

// Small host-only controller for executing the public owner example. It proves
// linking and callback sequencing, not hardware readiness, TDR time or distance.
struct Controller {
  std::array<uint16_t, 0x500> words{};
  uint64_t now{100}, quietUntil{0};
  uint16_t selector{0}, address{0};
  unsigned callbacks{0}, clockCalls{0}, triggers{0}, hardResets{0}, completionReads{0};
  bool locked{false}, violated{false};

  Controller() { resetWords(); }
  void resetWords() noexcept {
    words.fill(0);
    words[0] = 0x3100; words[4] = 0x01E1; words[25] = 0x8000;
    words[22] = 0x0100; words[30] = 0x0102; words[0x030E] = 0x8400;
    words[0x0170] = 0x5C12; words[0x0175] = 0x1007;
    words[0x0456] = 8;
  }
  static uint64_t clock(void* context) noexcept {
    auto& self = *static_cast<Controller*>(context); ++self.clockCalls; return self.now;
  }
  uint16_t read(uint16_t target) noexcept {
    if (target == 2) return 0x2000;
    if (target == 3) return 0xA131;
    if (target == 0x0467 || target == 0x0468) return 0;
    if (target == 30 && (words[30] & 0x8000U) != 0U) {
      if (++completionReads == 1U) return 0x8100; // Observed current-run progress.
      words[30] = 0x0102;
    }
    if (target == 0x0180) return 21;
    if (target == 0x0185) return 34;
    if (target == 0x018A) return 0x0800;
    return words[target];
  }
  void write(uint16_t target, uint16_t value) noexcept {
    if (target == 31 && value == 0x8000) {
      ++hardResets; resetWords();
      quietUntil = now + 2U + 1U + 2000U; // Frame completion, declared lag, HARD floor.
    } else if (target == 0) {
      words[0] = static_cast<uint16_t>(value & 0xFDFFU);
    } else {
      words[target] = value;
      if (target == 30 && (value & 0x8000U) != 0U) { ++triggers; completionReads = 0; }
    }
  }
  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget& budget) noexcept {
    auto& self = *static_cast<Controller*>(context);
    BatchResult result; result.status = Status::Ok(); result.releaseStatus = Status::Ok();
    result.controllerReusable = true;
    if (self.locked || self.now < self.quietUntil || batch.frameCount > budget.maxFrames) {
      self.violated = true; result.status = Status::Error(Err::MDIO_ERROR, "Host owner contract failed"); return result;
    }
    self.locked = true; ++self.callbacks;
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& frame = batch.frames[i]; auto& receipt = result.frames[i];
      uint16_t value = frame.value;
      if (frame.readIndex != NO_READ_INDEX) {
        if (frame.readIndex >= i || !result.frames[frame.readIndex].valueValid) {
          self.violated = true; result.status = Status::Error(Err::MDIO_ERROR, "Host RMW dependency failed"); break;
        }
        value = static_cast<uint16_t>((result.frames[frame.readIndex].value & frame.preserveMask) | value);
      }
      receipt.attempted = receipt.completed = true;
      if (frame.kind == FrameKind::WRITE && frame.registerAddress == 13) {
        if (value != 0x001F && value != 0x401F) self.violated = true;
        self.selector = value;
      } else if (frame.kind == FrameKind::WRITE && frame.registerAddress == 14 && self.selector == 0x001F) {
        self.address = value;
      } else {
        const uint16_t target = frame.registerAddress == 14 && self.selector == 0x401F ? self.address : frame.registerAddress;
        if (target >= self.words.size()) {
          self.violated = true; receipt.completed = false;
          result.status = Status::Error(Err::MDIO_ERROR, "Host address rejected"); break;
        }
        if (frame.kind == FrameKind::READ) { receipt.value = self.read(target); receipt.valueValid = true; }
        else self.write(target, value);
      }
      self.now += 2;
    }
    self.locked = false;
    return result;
  }
};

OperationBudget budget() noexcept { return {{1000000, 2048}, 10, 8}; }

// Finite simulated owner turns. A real application schedules each turn instead
// of advancing this fake clock. The same loop drives initialization and capture.
bool finish(Driver& phy, Controller& bus, CableAttempt& attempt, SharedMdioQuiet& quiet) noexcept {
  for (unsigned turn = 0; turn < 256 && !attempt.resultAvailable; ++turn) {
    const auto state = phy.operationSnapshot();
    if (state.nextPollValid && bus.now < state.nextPollAtUs) bus.now = state.nextPollAtUs;
    if (bus.now < quiet.untilUs) bus.now = quiet.untilUs;
    if (!quiet.canIssue(bus.now)) return false;
    const auto before = bus.callbacks;
    const auto status = pollCableCapture(phy, attempt, quiet);
    if (bus.callbacks > before + 1U || bus.locked) return false;
    if (!status.inProgress()) return status.ok() && attempt.resultAvailable;
    ++bus.now;
  }
  return false;
}
} // namespace

int main() {
  Controller bus; Driver phy; SharedMdioQuiet quiet;
  Config config; config.variant = Variant::DP83826I; config.interface = MacInterface::RMII;
  config.phyAddress = 1; config.transport = {&bus, Controller::execute, Controller::clock, {8,4,1}};
  if (!phy.bind(config).ok()) return 1;
  CableAttempt initialization;
  if (!beginInitialization(phy, ResetKind::NONE, budget(), initialization).inProgress() ||
      !finish(phy, bus, initialization, quiet)) return 2;
  LinkSettings profile; profile.advertisement.technologyMask = LINK_100_FULL;
  CableAttempt configuration;
  if (!phy.startConfigure(profile, budget(), configuration.token).inProgress() ||
      !finish(phy, bus, configuration, quiet)) return 3;

  CableSettings settings; settings.recipe = CableRecipe::RAW; settings.runMask = 1;
  settings.controls.channel = CableChannel::B_TO_A;
  settings.maintenanceAdmitted = settings.peerQuiet = settings.allowHardResetExit = true;
  CableAttempt capture;
  if (!beginCableCapture(phy, settings, budget(), capture).inProgress() ||
      !finish(phy, bus, capture, quiet)) return 4;
  const auto& row = capture.result.cable.samples[0];
  if (!capture.result.cable.complete || !row.index.valid || row.index.raw != 21 ||
      !row.amplitude.valid || row.amplitude.raw != 34 || !row.peak.valid || row.peak.raw != 0x0800) return 5;
  const auto reads = bus.callbacks, clocks = bus.clockCalls;
  const auto viewOne = phy.cableSnapshot(), viewTwo = phy.cableSnapshot();
  if (!viewOne.result.complete || !viewTwo.result.complete || bus.callbacks != reads || bus.clockCalls != clocks) return 6;
  CableDiagnosis sentinel; sentinel.metres = 999;
  if (interpretCable(capture.result.cable, 4.6, sentinel).ok() || sentinel.metres != 999) return 7;

  CableAttempt reread;
  if (!reacquireCableCapture(phy, budget(), reread).inProgress() || !finish(phy, bus, reread, quiet) || bus.triggers != 1U) return 8;
  CableAttempt restoration;
  if (!stopCableCapture(phy, budget(), restoration).inProgress() || !finish(phy, bus, restoration, quiet)) return 9;
  if (!restoration.result.persistentRestored || !restoration.result.cable.stopped ||
      phy.operationSnapshot().cleanupOwed || bus.hardResets != 1U || bus.violated || !phy.isInitialized()) return 10;
  if (!capture.result.cable.complete || capture.result.cable.stale || !capture.result.cleanupOwed) return 11;
  return phy.end().ok() ? 0 : 12;
}
