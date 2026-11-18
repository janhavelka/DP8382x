#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t phy, reg; uint16_t value; };

// Independent literal SNLS647I register model. The trace checks procedures,
// not silicon timing, strap behavior or electrical qualification.
struct Controller {
  std::array<uint16_t, 32> words{};
  std::array<Wire, 2048> trace{};
  std::array<uint8_t, 512> sizes{};
  uint16_t selector{0}, address{0}, vod{0x8400}, id2{0xA131};
  uint64_t now{100}, frameTime{2};
  unsigned calls{0}, frames{0}, clocks{0}, releases{0}, semanticWrites{0}, vodWrites{0}, resets{0};
  int failFrame{-1}, releaseCall{-1}, lateCall{-1}, regressCall{-1}, overflowCall{-1};
  int resetCall{-1};
  bool locked{false}, holdRestart{false}, holdReset{false}, holdPhyrcr{false}, failAfterEffect{true};
  Driver* competitor{nullptr};
  unsigned exclusions{0};
  Controller() { words[0] = 0x3100; words[4] = 0x01E1; words[25] = 0x8001; }
  Config config(uint32_t phy = 1) noexcept {
    Config c; c.variant = Variant::DP83826I; c.interface = MacInterface::RMII;
    c.phyAddress = phy; c.transport = {this, execute, clock, {8, 4, 1}}; return c;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<Controller*>(context); ++bus.clocks; return bus.now;
  }
  uint16_t read(uint8_t reg) noexcept {
    if (reg == 2) return 0x2000;
    if (reg == 3) return id2;
    if (reg == 0) {
      if (!holdReset && (words[0] & 0x8000U) != 0U) words[0] = 0x3100;
      if (!holdRestart) words[0] = static_cast<uint16_t>(words[0] & 0xFDFFU);
    }
    if (reg == 31 && !holdPhyrcr) words[31] = 0;
    if (reg == 14 && selector == 0x401F) {
      if (address == 0x030E) return vod;
      if (address == 0x0467) return 0x1234;
      if (address == 0x0468) return 0x5678;
      if (address < 32U) return words[address];
    }
    return words[reg];
  }
  void write(uint8_t reg, uint16_t value) noexcept {
    if (reg == 13) { selector = value; return; }
    if (reg == 14 && selector == 0x001F) { address = value; return; }
    ++semanticWrites;
    if (reg == 14 && selector == 0x401F) {
      if (address == 0x030E) { vod = value; ++vodWrites; }
      else if (address < 32U) words[address] = value;
    } else {
      words[reg] = value;
      if (reg == 0 && (value & 0x8000U) != 0U) {
        ++resets; words[4] = 0x01E1; words[9] = words[10] = words[11] = 0; words[25] = 0x8001;
      }
      if (reg == 31) {
        ++resets;
        if (value == 0x8000) {
          words.fill(0); words[0] = 0x3100; words[4] = 0x01E1; words[25] = 0x8001; vod = 0x8400;
        }
      }
    }
  }
  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget& budget) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    BatchResult result; result.status = Status::Ok(); result.releaseStatus = Status::Ok(); result.controllerReusable = true;
    if (bus.locked) {
      result.status = {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), "Other bus owner"}; return result;
    }
    const auto call = bus.calls++;
    if (static_cast<int>(call) == bus.resetCall) { bus.words[0] |= 0x8000; bus.holdReset = true; }
    CHECK(call < bus.sizes.size()); bus.sizes[call] = batch.frameCount;
    CHECK(batch.frameCount <= budget.maxFrames);
    bus.locked = true;
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& request = batch.frames[i]; auto& receipt = result.frames[i];
      uint16_t value = request.value;
      if (request.readIndex != 0xFF) {
        CHECK(request.kind == FrameKind::WRITE && request.readIndex < i);
        CHECK(result.frames[request.readIndex].valueValid);
        value = static_cast<uint16_t>((result.frames[request.readIndex].value & request.preserveMask) | value);
      }
      CHECK(bus.frames < bus.trace.size());
      bus.trace[bus.frames] = {request.kind, batch.phyAddress, request.registerAddress, value};
      const bool fail = static_cast<int>(bus.frames++) == bus.failFrame;
      receipt.attempted = true;
      if (!fail || bus.failAfterEffect) {
        if (request.kind == FrameKind::WRITE) bus.write(request.registerAddress, value);
        else receipt.value = bus.read(request.registerAddress);
      }
      bus.now += bus.frameTime;
      if (bus.competitor != nullptr) {
        uint16_t sentinel = 0xBEEF;
        CHECK(bus.competitor->readMmd(31, 0x0467, sentinel, budget).is(Err::BUSY));
        CHECK(sentinel == 0xBEEF); ++bus.exclusions;
      }
      if (fail) {
        receipt.effectPossible = bus.failAfterEffect;
        result.status = {Err::MDIO_TIMEOUT, -22022, "Injected verify failure"}; break;
      }
      receipt.completed = true; receipt.valueValid = request.kind == FrameKind::READ;
    }
    if (static_cast<int>(call) == bus.releaseCall) {
      result.releaseStatus = {Err::MDIO_ERROR, -37, "Release failure"}; result.controllerReusable = false;
    }
    if (static_cast<int>(call) == bus.lateCall) bus.now = budget.deadlineUs;
    if (static_cast<int>(call) == bus.regressCall) bus.now = 0;
    if (static_cast<int>(call) == bus.overflowCall) bus.now = std::numeric_limits<uint64_t>::max();
    bus.locked = false; ++bus.releases; return result;
  }
};
OperationBudget allowance(uint32_t frames = 512, uint64_t deadline = 1000000) { return {{deadline, frames}, 10, 8}; }
IoBudget io() { return {1000000, 32}; }
Status drain(Driver& driver, Controller& bus, OperationToken token) {
  Status status{Err::IN_PROGRESS, 0, "Pending"};
  for (unsigned i = 0; i < 128 && status.inProgress(); ++i) {
    const auto state = driver.operationSnapshot();
    if (state.nextPollValid && bus.now < state.nextPollAtUs) bus.now = state.nextPollAtUs;
    if (state.busQuietValid && bus.now < state.busQuietUntilUs) bus.now = state.busQuietUntilUs;
    const auto calls = bus.calls;
    status = driver.pollOperation(token);
    CHECK(bus.calls <= calls + 1 && !bus.locked);
  }
  CHECK(!status.inProgress()); return status;
}
OperationResult take(Driver& driver, OperationToken token) {
  OperationResult result; CHECK(driver.takeResult(token, result).ok()); return result;
}
LinkSettings profile(bool forced = false, bool fast = true, bool full = true) {
  LinkSettings s;
  if (forced) {
    s.mode = LinkMode::FORCED; s.forced.speed = fast ? LinkSpeed::MBPS_100 : LinkSpeed::MBPS_10;
    s.forced.duplex = full ? Duplex::FULL : Duplex::HALF;
  } else s.advertisement = {0x0F, true, true};
  s.crossover.pairSelection = PairSelection::MDIX;
  s.crossover.swapPair = true; s.crossover.swapPolarity = true;
  return s;
}
void initialize(Driver& driver, Controller& bus) {
  CHECK(driver.bind(bus.config()).ok());
  OperationToken token; CHECK(driver.startInitialize({ResetKind::NONE}, allowance(), token).inProgress());
  CHECK(drain(driver, bus, token).ok()); CHECK(take(driver, token).status.ok());
}
void configure(Driver& driver, Controller& bus, const LinkSettings& s) {
  OperationToken token; CHECK(driver.startConfigure(s, allowance(), token).inProgress());
  CHECK(drain(driver, bus, token).ok()); CHECK(take(driver, token).status.ok());
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
}
OperationToken verify(Driver& driver, Controller& bus, uint32_t frames = 512) {
  const auto calls = bus.calls, clocks = bus.clocks;
  OperationToken token{0xCAFE}; CHECK(driver.startVerify(allowance(frames), token).inProgress());
  CHECK(token.value != 0xCAFE && bus.calls == calls && bus.clocks == clocks);
  CHECK(driver.operationSnapshot().kind == OperationKind::VERIFY); return token;
}
void unchangedIntent(const SettingsSnapshot& before, const SettingsSnapshot& after) {
  CHECK(before.desiredValid == after.desiredValid);
  CHECK(before.desired.mode == after.desired.mode);
  CHECK(before.desired.advertisement.technologyMask == after.desired.advertisement.technologyMask);
  CHECK(before.desired.advertisement.symmetricPause == after.desired.advertisement.symmetricPause);
  CHECK(before.desired.advertisement.asymmetricPause == after.desired.advertisement.asymmetricPause);
  CHECK(before.desired.forced.speed == after.desired.forced.speed && before.desired.forced.duplex == after.desired.forced.duplex);
  CHECK(before.desired.crossover.pairSelection == after.desired.crossover.pairSelection);
  CHECK(before.desired.crossover.robust == after.desired.crossover.robust);
  CHECK(before.desired.crossover.swapPair == after.desired.crossover.swapPair);
  CHECK(before.desired.crossover.swapPolarity == after.desired.crossover.swapPolarity);
  CHECK(before.desired.extendedFullDuplex == after.desired.extendedFullDuplex);
}

void traces() {
  for (unsigned mode = 0; mode < 5; ++mode) {
    Controller bus; Driver driver; initialize(driver, bus);
    configure(driver, bus, profile(mode != 0, mode < 3, mode == 0 || mode == 1 || mode == 3));
    const auto intent = driver.settingsSnapshot();
    if (mode != 0) bus.words[4] = 0xFFFF; // Inactive ANAR cannot fail forced verification.
    const auto begin = bus.frames, calls = bus.calls, writes = bus.semanticWrites, vod = bus.vodWrites;
    const auto token = verify(driver, bus);
    CHECK(drain(driver, bus, token).ok()); const auto result = take(driver, token);
    const unsigned count = mode == 0 ? 12U : 11U;
    CHECK(bus.frames == begin + count && bus.calls == calls + (mode == 0 ? 8U : 7U));
    CHECK(bus.semanticWrites == writes && bus.vodWrites == vod);
    const uint8_t expectedAn[] = {2,3,13,14,13,14,4,9,25,11,10,0};
    const uint8_t expectedForced[] = {2,3,13,14,13,14,9,25,11,10,0};
    for (unsigned i = 0; i < count; ++i) {
      const auto& frame = bus.trace[begin + i];
      CHECK(frame.reg == (mode == 0 ? expectedAn[i] : expectedForced[i]));
      CHECK(frame.kind == (i >= 2 && i <= 4 ? FrameKind::WRITE : FrameKind::READ));
      if (i == 2) CHECK(frame.value == 0x001F);
      if (i == 3) CHECK(frame.value == 0x030E);
      if (i == 4) CHECK(frame.value == 0x401F);
    }
    CHECK(result.kind == OperationKind::VERIFY && result.status.ok() && result.persistentRestored);
    CHECK(!result.mutationAttempted && !result.mutationCompleted && !result.mutationPossible);
    CHECK(!result.commandRequired && !result.commandAccepted && !result.protocolComplete);
    CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
    unchangedIntent(intent, driver.settingsSnapshot());
  }
  Controller bus; Driver driver; initialize(driver, bus);
  const auto frames = bus.frames, calls = bus.calls, writes = bus.semanticWrites;
  const auto token = verify(driver, bus); CHECK(drain(driver, bus, token).ok());
  CHECK(take(driver, token).persistentRestored);
  CHECK(bus.frames == frames + 6 && bus.calls == calls + 2 && bus.semanticWrites == writes);
  CHECK(!driver.settingsSnapshot().desiredValid && driver.settingsSnapshot().trust != ConfigurationTrust::VERIFIED);
}

void comparisons() {
  const uint8_t regs[] = {4,9,25,11,10,0};
  const uint16_t flips[] = {0x0020,0x0020,0x4000,0x0060,0x0020,0x1000};
  const uint16_t masks[] = {0xADFF,0x0020,0xC000,0x0060,0x0020,0x3100};
  for (unsigned item = 0; item < 6; ++item) {
    Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
    const auto old = driver.settingsSnapshot();
    bus.words[regs[item]] ^= flips[item];
    const auto frames = bus.frames, writes = bus.semanticWrites;
    const auto token = verify(driver, bus); CHECK(drain(driver, bus, token).is(Err::CONFIG_MISMATCH));
    const auto result = take(driver, token);
    CHECK(bus.frames == frames + 12 && bus.semanticWrites == writes);
    CHECK(result.mismatch.valid && result.mismatch.registerAddress == regs[item] && result.mismatch.mask == masks[item]);
    CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
    CHECK(driver.settingsSnapshot().lastVerificationStatus.is(Err::CONFIG_MISMATCH));
    unchangedIntent(old, driver.settingsSnapshot());
  }
  Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
  bus.vod = 0x4A7F; // Six unowned low bits cannot fail FFC0 verification.
  bus.words[4] |= 0x5200; bus.words[9] |= 0x0010; bus.words[10] |= 0x0002; bus.words[11] |= 0x0100;
  bus.words[25] |= 0x0001; bus.words[0] |= 0x0200; bus.holdRestart = true;
  const auto token = verify(driver, bus); CHECK(drain(driver, bus, token).ok());
  CHECK(take(driver, token).status.ok()); CHECK(driver.settingsSnapshot().rawVod == 0x4A7F);
  CHECK(driver.settingsSnapshot().vodValid && driver.settingsSnapshot().rawBmcr == 0x1200);
  // Automatic routing owns bit 15 only: retained bit 14 is not a mismatch.
  bus.holdRestart = false;
  auto automatic = profile(); automatic.crossover.pairSelection = PairSelection::AUTOMATIC;
  configure(driver, bus, automatic); bus.words[25] ^= 0x4000;
  const auto autoToken = verify(driver, bus); CHECK(drain(driver, bus, autoToken).ok()); CHECK(take(driver, autoToken).status.ok());
  // VOD mismatch survives further complete profile acquisition.
  bus.vod = 0x8400; const auto frames = bus.frames;
  const auto vodToken = verify(driver, bus); CHECK(drain(driver, bus, vodToken).is(Err::CONFIG_MISMATCH));
  const auto result = take(driver, vodToken);
  CHECK(bus.frames == frames + 12 && result.mismatch.mmd && result.mismatch.devad == 31);
  CHECK(result.mismatch.registerAddress == 0x030E && result.mismatch.mask == 0xFFC0 && result.mismatch.expected == 0x4A40);
  CHECK(!driver.isInitialized() && driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
}

void failures() {
  for (int phase = 0; phase < 12; ++phase) {
    Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
    const auto old = driver.settingsSnapshot(); const auto frames = bus.frames, writes = bus.semanticWrites;
    bus.failFrame = static_cast<int>(frames) + phase;
    const auto token = verify(driver, bus); CHECK(drain(driver, bus, token).is(Err::MDIO_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.status.detail == -22022 && bus.frames == frames + static_cast<unsigned>(phase) + 1);
    CHECK(bus.semanticWrites == writes && !result.mutationPossible && !result.cleanupOwed);
    CHECK(driver.settingsSnapshot().trust != ConfigurationTrust::VERIFIED && driver.settingsSnapshot().observedStale);
    CHECK(driver.settingsSnapshot().observedAtUs == old.observedAtUs);
    unchangedIntent(old, driver.settingsSnapshot());
    if (phase >= 2 && phase <= 5) {
      const auto retryFrame = bus.frames;
      const auto retry = verify(driver, bus); CHECK(drain(driver, bus, retry).ok());
      CHECK(take(driver, retry).status.ok());
      CHECK(bus.trace[retryFrame + 2].reg == 13 && bus.trace[retryFrame + 2].value == 0x001F);
      CHECK(bus.trace[retryFrame + 3].reg == 14 && bus.trace[retryFrame + 3].value == 0x030E);
      CHECK(bus.trace[retryFrame + 4].reg == 13 && bus.trace[retryFrame + 4].value == 0x401F);
    }
  }
  for (bool vodMismatch : {false, true}) {
    Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
    const auto old = driver.settingsSnapshot();
    if (vodMismatch) bus.vod = 0x8400; else bus.words[4] ^= 0x0020;
    bus.failFrame = static_cast<int>(bus.frames) + 8; // Later PHYCR read.
    const auto token = verify(driver, bus); CHECK(drain(driver, bus, token).is(Err::MDIO_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.mismatch.valid && result.mismatch.registerAddress == (vodMismatch ? 0x030E : 4));
    CHECK(result.status.detail == -22022 && driver.settingsSnapshot().lastVerificationStatus.is(Err::MDIO_TIMEOUT));
    CHECK(driver.settingsSnapshot().observedAtUs == old.observedAtUs && driver.settingsSnapshot().observedStale);
  }
  for (unsigned phase = 0; phase < 8; ++phase) {
    Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
    bus.releaseCall = static_cast<int>(bus.calls + phase);
    const auto token = verify(driver, bus); CHECK(drain(driver, bus, token).is(Err::MDIO_ERROR));
    const auto result = take(driver, token); CHECK(result.status.detail == -37);
    CHECK(result.failingBatch.release.detail == -37 && !result.persistentRestored && !result.mutationPossible);
    if (phase == 1) CHECK(driver.settingsSnapshot().vodValid && driver.settingsSnapshot().rawVod == 0x4A40);
  }
}

void admission() {
  Controller bus; Driver driver; OperationToken sentinel{777};
  CHECK(driver.startVerify(allowance(), sentinel).is(Err::NOT_BOUND) && sentinel.value == 777);
  auto bad = allowance(); bad.pollIntervalUs = 0;
  CHECK(driver.startVerify(bad, sentinel).is(Err::INVALID_PARAM));
  CHECK(driver.bind(bus.config()).ok()); CHECK(driver.startVerify(allowance(), sentinel).is(Err::NOT_INITIALIZED));
  CHECK(bus.calls == 0 && bus.clocks == 0);
  initialize(driver, bus); configure(driver, bus, profile());
  const auto calls = bus.calls, clocks = bus.clocks;
  CHECK(driver.startVerify(allowance(0), sentinel).is(Err::OPERATION_TIMEOUT));
  CHECK(driver.startVerify(allowance(12, 0), sentinel).is(Err::OPERATION_TIMEOUT));
  CHECK(sentinel.value == 777 && bus.calls == calls && bus.clocks == clocks);
  const auto token = verify(driver, bus, 12);
  CHECK(driver.startVerify(allowance(), sentinel).is(Err::BUSY));
  CHECK(driver.end().is(Err::BUSY) && driver.bind(bus.config()).is(Err::BUSY));
  CHECK(driver.pollOperation(token, 1).inProgress() && bus.calls == calls);
  CHECK(driver.pollOperation(token, 2).inProgress() && bus.calls == calls + 1);
  CHECK(driver.pollOperation(token, 3).inProgress() && bus.calls == calls + 1); // Whole MMD cannot split.
  CHECK(driver.pollOperation(token, 4).inProgress() && bus.calls == calls + 2);
  CHECK(drain(driver, bus, token).ok());
  CHECK(driver.startVerify(allowance(), sentinel).is(Err::BUSY));
  CHECK(driver.end().is(Err::BUSY)); CHECK(take(driver, token).status.ok());
  OperationResult untouched; untouched.status = {Err::MDIO_ERROR, -99, "Sentinel"};
  CHECK(driver.takeResult(token, untouched).is(Err::RESULT_NOT_AVAILABLE) && untouched.status.detail == -99);
  const auto next = verify(driver, bus);
  CHECK(driver.pollOperation(token).is(Err::TOKEN_MISMATCH));
  CHECK(driver.cancel(next).ok()); CHECK(take(driver, next).status.is(Err::CANCELLED));
}

void lifecycle() {
  for (unsigned boundary = 0; boundary < 8; ++boundary) {
    Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
    const auto token = verify(driver, bus);
    for (unsigned n = 0; n < boundary; ++n) CHECK(driver.pollOperation(token).inProgress());
    const auto calls = bus.calls, clocks = bus.clocks, writes = bus.semanticWrites;
    CHECK(driver.cancel(token).ok()); const auto result = take(driver, token);
    CHECK(result.status.is(Err::CANCELLED) && !result.mutationAttempted && !result.cleanupOwed);
    CHECK(bus.calls == calls && bus.clocks == clocks && bus.semanticWrites == writes);
    CHECK(driver.settingsSnapshot().trust != ConfigurationTrust::VERIFIED);
  }
  Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
  bus.id2 = 0xA111; const auto frames = bus.frames;
  const auto token = verify(driver, bus); CHECK(drain(driver, bus, token).is(Err::INVALID_CONFIG));
  const auto changed = take(driver, token);
  CHECK(changed.status.detail == static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
  CHECK(bus.frames == frames + 2 && !driver.isInitialized() && driver.identitySnapshot().mode == OperatingMode::BASIC);
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
  OperationToken recovery; CHECK(driver.startInitialize({ResetKind::NONE}, allowance(), recovery).inProgress());
  CHECK(drain(driver, bus, recovery).ok()); CHECK(take(driver, recovery).status.ok());
  CHECK(driver.isInitialized() && driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
  bus.id2 = 0xA132;
  const auto wrong = verify(driver, bus); CHECK(drain(driver, bus, wrong).is(Err::DEVICE_MISMATCH));
  CHECK(take(driver, wrong).status.detail == static_cast<int32_t>(IdentityDetail::REVISION_NOT_ACCEPTED));
  Controller active; Driver resetDriver; initialize(resetDriver, active); configure(resetDriver, active, profile());
  active.words[0] |= 0x8000; active.holdReset = true;
  const auto writes = active.semanticWrites;
  const auto activeToken = verify(resetDriver, active);
  CHECK(drain(resetDriver, active, activeToken).is(Err::BUSY));
  const auto resetResult = take(resetDriver, activeToken);
  CHECK(resetResult.status.detail == static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
  CHECK(!resetResult.mismatch.valid && !resetDriver.isInitialized() && active.semanticWrites == writes);
  for (unsigned failure = 0; failure < 3; ++failure) {
    Controller resetBus; Driver subject; initialize(subject, resetBus); configure(subject, resetBus, profile());
    resetBus.words[0] = 0x8000; resetBus.holdReset = true;
    const auto finalCall = static_cast<int>(resetBus.calls + 7);
    if (failure == 0) resetBus.releaseCall = finalCall;
    if (failure == 1) resetBus.lateCall = finalCall;
    if (failure == 2) resetBus.regressCall = finalCall;
    const auto resetWrites = resetBus.semanticWrites;
    const auto resetToken = verify(subject, resetBus);
    const auto resetStatus = drain(subject, resetBus, resetToken);
    CHECK(resetStatus.is(failure == 0 ? Err::MDIO_ERROR : failure == 1 ? Err::OPERATION_TIMEOUT : Err::INVALID_CONFIG));
    const auto result = take(subject, resetToken);
    CHECK(!result.mismatch.valid && !subject.isInitialized());
    CHECK(subject.settingsSnapshot().trust == ConfigurationTrust::DIRTY && resetBus.semanticWrites == resetWrites);
    CHECK(!result.mutationPossible && !result.persistentRestored);
  }
  Controller declared; Driver declaredDriver;
  auto config = declared.config(); config.requiredMode = OperatingMode::ENHANCED;
  CHECK(declaredDriver.bind(config).ok());
  CHECK(declaredDriver.startInitialize({ResetKind::NONE}, allowance(), recovery).inProgress());
  CHECK(drain(declaredDriver, declared, recovery).ok()); CHECK(take(declaredDriver, recovery).status.ok());
  declared.id2 = 0xA111;
  const auto declaredToken = verify(declaredDriver, declared);
  CHECK(drain(declaredDriver, declared, declaredToken).is(Err::DEVICE_MISMATCH));
  CHECK(take(declaredDriver, declaredToken).status.detail == static_cast<int32_t>(IdentityDetail::MODE_MISMATCH));
}

void recovery() {
  // Last target write applied but timed out; a verify can prove stable fields
  // while retaining the pre-existing opaque-state restoration obligation.
  Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
  OperationToken apply; CHECK(driver.startConfigure(profile(), allowance(), apply).inProgress());
  for (unsigned n = 0; n < 11; ++n) CHECK(driver.pollOperation(apply).inProgress());
  bus.failFrame = static_cast<int>(bus.frames) + 1; // Final BMCR RMW write.
  CHECK(driver.pollOperation(apply).is(Err::MDIO_TIMEOUT));
  const auto failed = take(driver, apply); CHECK(failed.mutationPossible && failed.cleanupOwed);
  CHECK(driver.isInitialized()); const auto token = verify(driver, bus);
  CHECK(drain(driver, bus, token).ok()); const auto verified = take(driver, token);
  CHECK(verified.persistentRestored && verified.cleanupOwed && driver.operationSnapshot().cleanupOwed);
  CHECK(driver.end().is(Err::BUSY) && driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
  OperationToken restore; CHECK(driver.startInitialize({ResetKind::NONE}, allowance(), restore).inProgress());
  bus.failFrame = static_cast<int>(bus.frames); CHECK(drain(driver, bus, restore).is(Err::MDIO_TIMEOUT));
  const auto failedRestore = take(driver, restore); CHECK(failedRestore.cleanupOwed);
  CHECK(failed.status.detail == -22022 && failedRestore.token.value != failed.token.value);
  const auto intent = driver.settingsSnapshot();
  CHECK(driver.startInitialize({ResetKind::NONE}, allowance(), restore).inProgress());
  const auto vodWrites = bus.vodWrites, resets = bus.resets;
  CHECK(drain(driver, bus, restore).ok()); CHECK(take(driver, restore).persistentRestored);
  CHECK(!driver.operationSnapshot().cleanupOwed && bus.vodWrites == vodWrites + 1 && bus.resets == resets);
  unchangedIntent(intent, driver.settingsSnapshot());
  // Raw write and external invalidation do not establish managed transport failure.
  const auto health = driver.healthSnapshot();
  CHECK(driver.writeClause22(0x18, 0x0012, io()).ok());
  CHECK(!driver.isInitialized() && driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
  CHECK(driver.healthSnapshot().totalFailures == health.totalFailures);
  uint16_t raw = 0; CHECK(driver.readClause22(2, raw, io()).ok());
  DeviceIdentity identity; CHECK(driver.readIdentity(identity, io()).ok());
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
  CHECK(driver.startVerify(allowance(), restore).is(Err::NOT_INITIALIZED));
  for (ResetKind kind : {ResetKind::NONE, ResetKind::PCS, ResetKind::HARD, ResetKind::RESTART}) {
    const auto calls = bus.calls, clocks = bus.clocks;
    CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
    CHECK(bus.calls == calls && bus.clocks == clocks);
    const auto beforeVod = bus.vodWrites;
    CHECK(driver.startInitialize({kind}, allowance(), restore).inProgress());
    CHECK(drain(driver, bus, restore).ok()); CHECK(take(driver, restore).persistentRestored);
    CHECK(bus.vodWrites == beforeVod + 1 && driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
    unchangedIntent(intent, driver.settingsSnapshot());
  }
}

void interruptedReplay() {
  for (ResetKind kind : {ResetKind::NONE, ResetKind::PCS, ResetKind::HARD, ResetKind::RESTART}) {
    Controller reference; Driver referenceDriver; initialize(referenceDriver, reference);
    configure(referenceDriver, reference, profile());
    OperationToken baseline;
    const auto initialFrames = reference.frames;
    CHECK(referenceDriver.startInitialize({kind}, allowance(), baseline).inProgress());
    CHECK(drain(referenceDriver, reference, baseline).ok()); CHECK(take(referenceDriver, baseline).persistentRestored);
    const unsigned replayFrames = reference.frames - initialFrames;
    CHECK(replayFrames < 128);
    // Inject at every actual reset/reapply phase, including accepted target
    // writes. Restoration is a fresh explicit operation, never a hidden retry.
    for (unsigned phase = 0; phase < replayFrames; ++phase) {
      Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
      const auto desired = driver.settingsSnapshot(); const auto frames = bus.frames;
      bus.failFrame = static_cast<int>(frames + phase);
      OperationToken token; CHECK(driver.startInitialize({kind}, allowance(), token).inProgress());
      CHECK(drain(driver, bus, token).is(Err::MDIO_TIMEOUT));
      const auto failed = take(driver, token);
      CHECK(failed.status.detail == -22022 && bus.frames == frames + phase + 1);
      CHECK(!failed.persistentRestored);
      if (failed.mutationCompleted || failed.mutationPossible) CHECK(driver.operationSnapshot().cleanupOwed);
      unchangedIntent(desired, driver.settingsSnapshot());
      bus.failFrame = -1;
      OperationToken retry; CHECK(driver.startInitialize({kind}, allowance(), retry).inProgress());
      const auto retryStatus = drain(driver, bus, retry);
      CHECK(retry.value != token.value && retryStatus.ok());
      const auto restored = take(driver, retry);
      CHECK(restored.persistentRestored && !driver.operationSnapshot().cleanupOwed);
      CHECK(failed.status.detail == -22022 && driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
      unchangedIntent(desired, driver.settingsSnapshot());
    }
  }
}

void resetObservationErrors() {
  // Valid reset evidence is independent of a later release/clock/deadline
  // failure. Literal BMCR PCS and PHYRCR HARD invalidate settings; RESTART
  // only interrupts link/history and must not invent a hardware mutation.
  for (unsigned scenario = 0; scenario < 5; ++scenario) {
    for (unsigned fault = 0; fault < 4; ++fault) {
      Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
      const auto intent = driver.settingsSnapshot();
      OperationToken token;
      CHECK(driver.startInitialize({scenario == 3 ? ResetKind::RESTART : ResetKind::NONE}, allowance(), token).inProgress());
      const unsigned prefix = scenario == 0 ? 3U : scenario == 2 ? 5U : scenario == 3 ? 6U : 4U;
      if (scenario == 2) { bus.words[31] = 0x4000; bus.holdPhyrcr = true; }
      for (unsigned i = 0; i < prefix; ++i) CHECK(driver.pollOperation(token).inProgress());
      CHECK(driver.isInitialized() && driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
      const auto pending = driver.operationSnapshot();
      if (pending.nextPollValid && bus.now < pending.nextPollAtUs) bus.now = pending.nextPollAtUs;
      if (scenario == 0) { bus.words[0] = 0x8000; bus.holdReset = true; }
      else { bus.words[31] = scenario == 4 ? 0x4000 : 0x8000; bus.holdPhyrcr = true; }
      const auto calls = bus.calls, frames = bus.frames, writes = bus.semanticWrites;
      const auto health = driver.healthSnapshot();
      if (fault == 0) bus.releaseCall = static_cast<int>(calls);
      if (fault == 1) bus.lateCall = static_cast<int>(calls);
      if (fault == 2) bus.regressCall = static_cast<int>(calls);
      if (fault == 3) bus.overflowCall = static_cast<int>(calls);
      const auto status = driver.pollOperation(token);
      CHECK(status.is(fault == 0 ? Err::MDIO_ERROR : fault == 1 ? Err::OPERATION_TIMEOUT : Err::INVALID_CONFIG));
      CHECK(bus.calls == calls + 1 && bus.frames == frames + 1 && bus.semanticWrites == writes);
      CHECK(bus.trace[frames].reg == (scenario == 0 ? 0 : 31));
      const auto result = take(driver, token);
      CHECK(result.status.code == status.code && result.status.detail == status.detail);
      CHECK(result.failingBatch.completedFrames == 1 && !result.persistentRestored);
      CHECK(result.mutationPossible == (scenario == 3) && result.cleanupOwed == (scenario == 3));
      CHECK(driver.isInitialized() == (scenario == 4));
      CHECK(driver.identitySnapshot().stale == (scenario != 4));
      CHECK(driver.settingsSnapshot().trust == (scenario == 4 ? ConfigurationTrust::VERIFIED : ConfigurationTrust::DIRTY));
      if (scenario != 4) CHECK(result.identity.stale && result.straps.stale && !result.straps.timeValid);
      unchangedIntent(intent, driver.settingsSnapshot());
      const auto after = driver.healthSnapshot();
      CHECK(after.totalFailures == health.totalFailures + (fault == 0 ? 1U : 0U));
      CHECK(after.totalSuccess == health.totalSuccess + (fault == 0 ? 0U : 1U));
    }
  }
}

void rawResetObservation() {
  // Tables 8-19/48: PCS/HARD invalidate configuration; RESTART retains it.
  // Both access namespaces must invalidate current knowledge, not frozen results.
  for (bool alias : {false, true}) for (bool pending : {false, true})
  for (unsigned kind = 0; kind < 3; ++kind) for (unsigned fault = 0; fault < 4; ++fault) {
    Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
    OperationToken token;
    if (pending) {
      CHECK(driver.startInitialize({ResetKind::NONE},allowance(),token).inProgress());
      CHECK(drain(driver,bus,token).ok());
    }
    const auto intent = driver.settingsSnapshot();
    const auto frames = bus.frames, calls = bus.calls, writes = bus.semanticWrites;
    const uint16_t address = kind == 0 ? 0 : 0x1F;
    const uint16_t expected = kind == 0 ? 0x9000 : kind == 1 ? 0x8000 : 0x4000;
    bus.words[address] = expected; bus.holdReset = bus.holdPhyrcr = true;
    if (fault == 1) bus.releaseCall = static_cast<int>(calls);
    if (fault == 2) bus.lateCall = static_cast<int>(calls);
    if (fault == 3) bus.regressCall = static_cast<int>(calls);
    uint16_t raw = 0xCAFE;
    const auto status = alias ? driver.readMmd(31, address, raw, io()) : driver.readClause22(address, raw, io());
    CHECK(status.is(fault == 0 ? Err::OK : fault == 1 ? Err::MDIO_ERROR :
                    fault == 2 ? Err::OPERATION_TIMEOUT : Err::INVALID_CONFIG));
    CHECK(raw == (fault == 0 ? expected : 0xCAFE));
    CHECK(driver.isInitialized() == (kind == 2));
    CHECK(driver.capabilities().valid == (kind == 2));
    CHECK(driver.settingsSnapshot().trust == (kind == 2 ? ConfigurationTrust::VERIFIED : ConfigurationTrust::DIRTY));
    unchangedIntent(intent, driver.settingsSnapshot());
    CHECK(bus.frames == frames + (alias ? 4U : 1U) && bus.calls == calls + 1U && bus.semanticWrites == writes);
    CHECK(!driver.operationSnapshot().busQuietValid && !driver.operationSnapshot().cleanupOwed);
    if (pending) {
      const auto result = take(driver, token);
      CHECK(result.status.ok() && result.persistentRestored && !result.identity.stale);
      CHECK(result.identity.acceptance == IdentityAcceptance::ACCEPTED);
      CHECK(!result.straps.stale && result.straps.timeValid && result.straps.sor1==0x1234U && result.straps.sor2==0x5678U);
    }
  }
}

void configurationResetObservation() {
  // Table 8-19: reset-in-progress cannot validate a profile. Inject on each
  // BMCR read (preflight, atomic RMW, readback), not just between owner polls.
  for (bool forced : {false, true}) {
    Controller reference; Driver baseline; initialize(baseline, reference);
    const auto firstCall = reference.calls, firstFrame = reference.frames;
    configure(baseline, reference, profile(forced));
    unsigned frame = firstFrame, readCount = 0;
    for (unsigned call = firstCall; call < reference.calls; ++call) {
      const auto size = reference.sizes[call];
      const bool bmcr = reference.trace[frame].reg == 0 && reference.trace[frame].kind == FrameKind::READ;
      frame += size;
      if (!bmcr) continue;
      ++readCount;
      for (unsigned fault = 0; fault < (size == 2 ? 5U : 4U); ++fault) {
        Controller bus; Driver driver; initialize(driver, bus);
        const auto startCalls = bus.calls;
        bus.resetCall = static_cast<int>(startCalls + call - firstCall);
        if (fault == 1) bus.releaseCall = bus.resetCall;
        if (fault == 2) bus.lateCall = bus.resetCall;
        if (fault == 3) bus.regressCall = bus.resetCall;
        if (fault == 4) bus.failFrame = static_cast<int>(frame - size + 1);
        OperationToken token;
        CHECK(driver.startConfigure(profile(forced), allowance(), token).inProgress());
        const auto status = drain(driver, bus, token);
        CHECK(status.is(fault == 1 ? Err::MDIO_ERROR : fault == 2 ? Err::OPERATION_TIMEOUT :
                        fault == 4 ? Err::MDIO_TIMEOUT : Err::INVALID_CONFIG));
        if (fault == 0) CHECK(status.detail == static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
        if (fault == 1) CHECK(status.detail == -37);
        if (fault == 4) CHECK(status.detail == -22022);
        CHECK(bus.calls == static_cast<unsigned>(bus.resetCall) + 1U);
        CHECK(bus.frames == frame && bus.releases == bus.calls && !bus.locked);
        CHECK(!driver.isInitialized() && !driver.capabilities().valid && driver.identitySnapshot().stale);
        CHECK(driver.settingsSnapshot().trust != ConfigurationTrust::VERIFIED);
        const auto result = take(driver, token);
        CHECK(!result.persistentRestored && result.status.code == status.code && result.status.detail == status.detail);
        CHECK(result.mutationPossible == (readCount != 1U));
        CHECK(result.cleanupOwed == result.mutationPossible);
        CHECK(result.failingBatch.completedFrames == (fault == 4 ? 1U : size));
        const auto callsAfter = bus.calls;
        CHECK(!driver.pollOperation(token).inProgress() && bus.calls == callsAfter);
      }
    }
    CHECK(readCount == 3U);
  }
}

void retainedBarrier() {
  Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
  OperationToken apply; CHECK(driver.startConfigure(profile(), allowance(), apply).inProgress());
  for (unsigned n = 0; n < 11; ++n) CHECK(driver.pollOperation(apply).inProgress());
  bus.failFrame = static_cast<int>(bus.frames) + 1;
  CHECK(driver.pollOperation(apply).is(Err::MDIO_TIMEOUT));
  const auto original = take(driver, apply);
  const auto cancelToken = verify(driver, bus); CHECK(driver.pollOperation(cancelToken).inProgress());
  const auto calls = bus.calls, clocks = bus.clocks;
  CHECK(driver.cancel(cancelToken).ok()); CHECK(take(driver, cancelToken).cleanupOwed);
  CHECK(bus.calls == calls && bus.clocks == clocks && driver.operationSnapshot().cleanupOwed);
  const auto failedVerify = verify(driver, bus); CHECK(driver.pollOperation(failedVerify).inProgress());
  bus.failFrame = static_cast<int>(bus.frames); CHECK(driver.pollOperation(failedVerify).is(Err::MDIO_TIMEOUT));
  CHECK(take(driver, failedVerify).cleanupOwed);
  // VERIFY must not replace CONFIGURE as the original cleanup owner.
  CHECK(driver.startConfigure(profile(), allowance(), apply).inProgress());
  CHECK(drain(driver, bus, apply).ok()); CHECK(!take(driver, apply).cleanupOwed);
  CHECK(original.status.detail == -22022);
  // A later accepted failure and explicit abandon retain historical result/intent.
  CHECK(driver.startConfigure(profile(), allowance(), apply).inProgress());
  for (unsigned n = 0; n < 11; ++n) CHECK(driver.pollOperation(apply).inProgress());
  bus.failFrame = static_cast<int>(bus.frames) + 1;
  CHECK(driver.pollOperation(apply).is(Err::MDIO_TIMEOUT));
  const auto before = driver.settingsSnapshot();
  const auto abandonCalls = bus.calls, abandonClocks = bus.clocks;
  CHECK(driver.abandonCleanup().ok());
  CHECK(bus.calls == abandonCalls && bus.clocks == abandonClocks);
  CHECK(!driver.isInitialized() && driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
  unchangedIntent(before, driver.settingsSnapshot());
  CHECK(take(driver, apply).cleanupOwed); // Frozen historical obligation is unchanged.
  CHECK(!driver.operationSnapshot().cleanupOwed);
}

void ownership() {
  Controller bus; Driver driver, competitor; initialize(driver, bus); configure(driver, bus, profile());
  CHECK(competitor.bind(bus.config(2)).ok()); bus.competitor = &competitor;
  const auto token = verify(driver, bus); const auto writes = bus.semanticWrites;
  CHECK(drain(driver, bus, token).ok()); CHECK(take(driver, token).status.ok());
  CHECK(bus.exclusions == 12 && bus.semanticWrites == writes && bus.calls == bus.releases);
  bus.competitor = nullptr;
  const auto settings = driver.settingsSnapshot(); const auto calls = bus.calls, clocks = bus.clocks;
  CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION).ok());
  (void)driver.settingsSnapshot(); (void)driver.identitySnapshot(); (void)driver.operationSnapshot(); (void)driver.healthSnapshot();
  CHECK(bus.calls == calls && bus.clocks == clocks); unchangedIntent(settings, driver.settingsSnapshot());
}

void timing() {
  for (unsigned fault = 0; fault < 3; ++fault) {
    Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
    const auto token = verify(driver, bus);
    CHECK(driver.pollOperation(token).inProgress());
    if (fault == 0) bus.lateCall = static_cast<int>(bus.calls);
    if (fault == 1) bus.regressCall = static_cast<int>(bus.calls);
    if (fault == 2) bus.overflowCall = static_cast<int>(bus.calls);
    const auto status = driver.pollOperation(token);
    CHECK(status.is(fault == 0 ? Err::OPERATION_TIMEOUT : Err::INVALID_CONFIG));
    const auto result = take(driver, token); CHECK(!result.persistentRestored && !result.mutationPossible);
    CHECK(driver.settingsSnapshot().trust != ConfigurationTrust::VERIFIED);
    CHECK(driver.settingsSnapshot().vodValid && driver.settingsSnapshot().rawVod == 0x4A40);
    if (fault != 0) CHECK(result.clockInvalid);
  }
  Controller bus; Driver driver; initialize(driver, bus); configure(driver, bus, profile());
  const auto token = verify(driver, bus, 5); CHECK(driver.pollOperation(token).inProgress());
  const auto calls = bus.calls; CHECK(driver.pollOperation(token).is(Err::OPERATION_TIMEOUT));
  CHECK(bus.calls == calls && take(driver, token).status.detail == static_cast<int32_t>(TimeoutDetail::FRAME_BUDGET));
}
} // namespace
int main(int argc, char** argv) {
  const test::Suite suites[] = {{"traces", traces}, {"comparisons", comparisons}, {"failures", failures},
    {"admission", admission}, {"lifecycle", lifecycle}, {"recovery", recovery}, {"ownership", ownership}, {"timing", timing},
    {"interrupted_replay", interruptedReplay}, {"reset_observation_errors", resetObservationErrors},
    {"raw_reset_observation", rawResetObservation}, {"configuration_reset_observation", configurationResetObservation},
    {"barrier", retainedBarrier}};
  return test::runSuites(argc, argv, suites);
}
