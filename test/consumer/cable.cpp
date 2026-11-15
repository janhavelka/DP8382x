#include "../../examples/portable/Eee.h"
#include "../../examples/portable/CableCapture.h"
#include "../../examples/portable/CableAutorun.h"
#include "../../examples/portable/Interrupts.h"
#include "../../examples/portable/Power.h"
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
    words[0x0456] = 8; words[0x04A1] = 0x1000; words[0x11] = 0x0108;
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
    const auto value = words[target];
    if (target == 18 || target == 19) words[target] &= 0x00FF;
    if (target == 0x04A1) words[target] &= 0xFF1E;
    return value;
  }
  void write(uint16_t target, uint16_t value) noexcept {
    if (target == 31 && value == 0x8000) {
      ++hardResets; resetWords();
      quietUntil = now + 2U + 1U + 2000U; // Frame completion, declared lag, HARD floor.
    } else if (target == 0) {
      words[0] = static_cast<uint16_t>(value & 0xFDFFU);
    } else if (target == 18 || target == 19) {
      words[target] = static_cast<uint16_t>((words[target] & 0xFF00U) | (value & 0x00FFU));
    } else if (target == 0x04A1) {
      words[target] = static_cast<uint16_t>((words[target] & 0x00E1U) | value);
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

  // Explicit silicon policy, later bounded raw observation, then explicit
  // disable for manual handoff. This host model has no physical link-down event.
  CableAutorunOptions autorun; autorun.enabled = true; autorun.maintenanceAdmitted = true;
  CableAttempt policy;
  if (!configureCableAutorun(phy, autorun, budget(), policy).inProgress() ||
      !finish(phy, bus, policy, quiet)) return 13;
  if ((bus.words[9] & 0x0100U) == 0U || !policy.result.cableAutorun.observed ||
      policy.result.cableAutorun.trust != ConfigurationTrust::VERIFIED || bus.triggers != 0U) return 14;
  const auto policyCalls = bus.callbacks, policyClocks = bus.clockCalls;
  const auto policyOne = phy.cableAutorunSnapshot(), policyTwo = phy.cableAutorunSnapshot();
  if (!policyOne.desired || !policyTwo.observed || bus.callbacks != policyCalls || bus.clockCalls != policyClocks) return 15;
  CableAttempt observation;
  if (!acquireCableAutorun(phy, budget(), observation).inProgress() ||
      !finish(phy, bus, observation, quiet)) return 16;
  if (bus.callbacks != policyCalls + 5U || bus.triggers != 0U || bus.hardResets != 0U ||
      !observation.result.cable.complete || !observation.result.cable.stale ||
      !observation.result.cable.autorun || observation.result.cable.statusChanged ||
      !observation.result.cable.samples[0].index.valid ||
      observation.result.cable.samples[0].freshnessObserved) return 17;
  CableDiagnosis autorunSentinel; autorunSentinel.metres = 777;
  if (interpretCable(observation.result.cable, 4.6, autorunSentinel).ok() || autorunSentinel.metres != 777) return 18;
  autorun.enabled = false;
  CableAttempt handoff;
  if (!configureCableAutorun(phy, autorun, budget(), handoff).inProgress() ||
      !finish(phy, bus, handoff, quiet)) return 19;
  if ((bus.words[9] & 0x0100U) != 0U || handoff.result.cableAutorun.observed ||
      handoff.result.cableAutorun.trust != ConfigurationTrust::VERIFIED) return 20;

  // This host-only permission models an admitted board; no real IRQ wire is assumed.
  InterruptSettings irq; irq.maintenanceAdmitted = irq.pinAvailable = true;
  irq.globalEnable = true; irq.misr1Mask = 0x20; irq.misr2Mask = 0x02;
  InterruptAttempt irqPolicy;
  if (!beginInterruptPolicy(phy, irq, budget(), irqPolicy).inProgress() ||
      !finish(phy, bus, irqPolicy, quiet)) return 22;
  bus.words[18] |= 0x2000; bus.words[19] |= 0x0200;
  InterruptServiceSnapshot irqView; bool reschedule = false;
  if (!serviceDeferredInterrupt(phy, {{1000000,5},2,5}, irqView, reschedule).ok() || !reschedule ||
      !irqView.misr2.wakeSeen) return 23;
  const auto irqCalls = bus.callbacks, irqClocks = bus.clockCalls;
  const auto irqOne = cachedInterruptView(phy), irqTwo = cachedInterruptView(phy);
  if (!irqOne.valid || !irqTwo.valid || bus.callbacks != irqCalls || bus.clockCalls != irqClocks) return 24;
  if (!serviceDeferredInterrupt(phy, {{1000000,5},2,5}, irqView, reschedule).ok() || reschedule) return 25;
  InterruptAttempt stimulus, clearStimulus;
  if (!phy.startInterruptTest(budget(), stimulus.token).inProgress() || !finish(phy,bus,stimulus,quiet)) return 26;
  if ((bus.words[17] & 4U) == 0U || !stimulus.result.cleanupOwed) return 27;
  if (!phy.stopInterruptTest(budget(), clearStimulus.token).inProgress() || !finish(phy,bus,clearStimulus,quiet) ||
      (bus.words[17] & 4U) != 0U) return 28;

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
  const auto replayed = phy.cableAutorunSnapshot();
  if (!replayed.desiredValid || replayed.desired || replayed.observed ||
      replayed.trust != ConfigurationTrust::VERIFIED || (bus.words[9] & 0x0100U) != 0U) return 21;
  if (phy.interruptSnapshot().trust != ConfigurationTrust::VERIFIED || (bus.words[17] & 0x000FU) != 0x000BU) return 29;
  // Board admissions are explicit host assumptions, not fixture qualification.
  PowerSettings power; power.activeSleep = true;
  power.maintenanceAdmitted = power.managementClockAvailable = power.clockLossApproved = true;
  PowerAttempt sleepPolicy;
  if (!beginPowerPolicy(phy,power,budget(),sleepPolicy).inProgress()) return 30;
  if (!finish(phy,bus,sleepPolicy,quiet)) return 35;
  if (sleepPolicy.result.power.state != PowerState::NORMAL_INDICATED) return 36;
  power.request = PowerRequest::IEEE_POWER_DOWN; power.disablePll = true;
  PowerAttempt down;
  if (!beginPowerPolicy(phy,power,budget(),down).inProgress() || !finish(phy,bus,down,quiet) ||
      !down.result.power.requestedPowerDown || (bus.words[17]&0x8000U)==0U) return 31;
  power.request = PowerRequest::NORMAL; power.disablePll = false;
  PowerAttempt wake;
  if (!beginPowerPolicy(phy,power,budget(),wake).inProgress() || !finish(phy,bus,wake,quiet)) return 32;
  PowerSnapshot powerView;
  if (!acquirePowerView(phy,powerView,{1000000,3}).ok() || powerView.observedBmcrPowerDown) return 33;
  const auto powerCalls=bus.callbacks,powerClocks=bus.clockCalls;
  const auto powerOne=phy.powerSnapshot(),powerTwo=phy.powerSnapshot();
  if (powerOne.observedStale || powerTwo.requestedPowerDown || bus.callbacks!=powerCalls || bus.clockCalls!=powerClocks ||
      (bus.words[17]&0x000FU)!=0x000BU) return 34;
  // Explicit disable compiles and executes the same HARD/owned replay contract.
  EeeSettings eee; eee.maintenanceAdmitted=true;
  EeeAttempt eeeOff;
  if (!beginEeePolicy(phy,eee,budget(),eeeOff).inProgress() || !finish(phy,bus,eeeOff,quiet)) return 37;
  if (phy.eeeSnapshot().trust!=ConfigurationTrust::VERIFIED || negotiatedEee(phy.eeeSnapshot())) return 38;
  EeeSnapshot eeeView;
  const auto eeeStatus=acquireEeeView(phy,eeeView,{1000000,31});
  if (!eeeStatus.ok() || negotiatedEee(eeeView)) return 39;
  return phy.end().ok() ? 0 : 12;
}
