#include "support/Check.h"
#include <DP8382x/DP8382x.h>

#include <array>
#include <cstdint>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;

// Literal register behavior from SNLS647I tables 8-19, 8-23 through 8-27,
// and 8-31/32. Scripted partner events exercise software, not physical AN.
struct Wire {
  FrameKind kind;
  uint8_t phy;
  uint8_t reg;
  uint16_t value;
};
struct Controller {
  struct Phy {
    uint16_t bmcr{0x3100}, phyrcr{0}, anar{0x01E1};
    uint16_t selector{0}, address{0}, vod{0x8400}, page{0x0800};
    uint16_t aner{0x0004}, partnerBase{0}, partnerPage{0}, bmsr{0};
    uint16_t cr1{0}, phycr{0x8001}, cr3{0}, cr2{0x0100};
  };
  std::array<Phy, 32> phys{};
  std::array<Wire, 2048> trace{};
  std::array<uint8_t, 512> batchSizes{};
  uint64_t now{100}, frameTime{2};
  unsigned frames{0}, calls{0}, clocks{0}, locks{0}, releases{0};
  unsigned vodWrites{0}, restarts{0}, pageLoads{0}, anerReads{0};
  std::array<uint16_t, 16> partnerPages{};
  unsigned partnerCount{0}, partnerIndex{0};
  bool automaticPartner{false}, partnerRefuses{false}, completePartner{true};
  bool duplicateSecond{false}, pendingRealPage{false};
  uint16_t deferredPage{0};
  int failFrame{-1}, lateBatch{-1}, badAnar{-1}, badBmcr{-1};
  bool failedEffect{true}, locked{false};
  Driver* competitor{nullptr};
  unsigned exclusions{0};

  Config config(uint32_t address = 1) noexcept {
    Config result;
    result.variant = Variant::DP83826I;
    result.interface = MacInterface::RMII;
    result.phyAddress = address;
    result.transport = {this, execute, clock, {8, 4, 1}};
    return result;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    ++bus.clocks;
    return bus.now;
  }
  uint16_t read(Phy& phy, uint8_t reg) noexcept {
    switch (reg) {
    case 0x00:
      if ((phy.bmcr & 0x8000U) != 0) phy.bmcr = 0x3100;
      phy.bmcr = static_cast<uint16_t>(phy.bmcr & 0xFDFFU);
      return badBmcr < 0 ? phy.bmcr : static_cast<uint16_t>(badBmcr);
    case 0x01: return phy.bmsr;
    case 0x02: return 0x2000;
    case 0x03: return 0xA131;
    case 0x04: return badAnar < 0 ? phy.anar : static_cast<uint16_t>(badAnar);
    case 0x05: return phy.partnerBase;
    case 0x06: {
      ++anerReads;
      const auto value = phy.aner;
      phy.aner = static_cast<uint16_t>(phy.aner & 0xFFFDU);
      return value;
    }
    case 0x07: return phy.page;
    case 0x08: {
      const auto value = phy.partnerPage;
      if (pendingRealPage) {
        phy.partnerPage = deferredPage;
        phy.aner = static_cast<uint16_t>(phy.aner | 0x0002U);
        pendingRealPage = false;
      }
      return value;
    }
    case 0x09: return phy.cr1;
    case 0x19: return phy.phycr;
    case 0x0B: return phy.cr3;
    case 0x0A: return phy.cr2;
    case 0x1F: phy.phyrcr = 0; return 0;
    case 0x0E:
      if (phy.selector == 0x401F) {
        if (phy.address == 0x030E) return phy.vod;
        if (phy.address == 0x0467) return 0x1234;
        if (phy.address == 0x0468) return 0x5678;
      }
      return 0;
    default: return 0;
    }
  }
  void write(Phy& phy, uint8_t reg, uint16_t value) noexcept {
    if (reg == 0x00) {
      phy.bmcr = value;
      if ((value & 0x0200U) != 0) {
        ++restarts;
        if (automaticPartner) {
          phy.aner = partnerRefuses ? 0x0007 : 0x000F;
          phy.partnerBase = partnerRefuses ? 0x0101 : 0x8101;
          if (partnerRefuses && completePartner) phy.bmsr = 0x0020;
        }
      }
      if ((value & 0x8000U) != 0) phy.anar = 0x01E1;
    } else if (reg == 0x1F) {
      phy.phyrcr = value;
      if ((value & 0x8000U) != 0) {
        phy.bmcr = 0x3100;
        phy.anar = 0x01E1;
        phy.vod = 0x8400;
      }
    } else if (reg == 0x04) phy.anar = value;
    else if (reg == 0x09) phy.cr1 = value;
    else if (reg == 0x19) phy.phycr = value;
    else if (reg == 0x0B) phy.cr3 = value;
    else if (reg == 0x0A) phy.cr2 = value;
    else if (reg == 0x07) {
      // Toggle is hardware-owned: transmitting code must preserve its sample.
      CHECK((value & 0x0800U) == (phy.page & 0x0800U));
      phy.page = value;
      ++pageLoads;
      if (automaticPartner && partnerIndex < partnerCount) {
        const auto incoming = partnerPages[partnerIndex++];
        if (duplicateSecond && partnerIndex == 2) {
          deferredPage = incoming;
          pendingRealPage = true;
        } else phy.partnerPage = incoming;
        phy.aner = static_cast<uint16_t>(phy.aner | 0x0002U);
        if ((value & 0x8000U) == 0 && (incoming & 0x8000U) == 0 && completePartner)
          phy.bmsr = 0x0020;
      }
    } else if (reg == 0x0D) phy.selector = value;
    else if (reg == 0x0E && phy.selector == 0x001F) phy.address = value;
    else if (reg == 0x0E && phy.selector == 0x401F && phy.address == 0x030E) {
      phy.vod = value;
      ++vodWrites;
    }
  }
  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget& allowance) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    BatchResult result;
    result.releaseStatus = Status::Ok();
    result.controllerReusable = true;
    if (bus.locked) {
      result.status = {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), "Shared controller owned"};
      return result;
    }
    const auto call = bus.calls++;
    CHECK(call < bus.batchSizes.size());
    bus.batchSizes[call] = batch.frameCount;
    CHECK(batch.frameCount <= allowance.maxFrames);
    bus.locked = true;
    ++bus.locks;
    result.status = Status::Ok();
    auto& phy = bus.phys[batch.phyAddress];
    for (uint8_t index = 0; index < batch.frameCount; ++index) {
      const auto& frame = batch.frames[index];
      auto value = frame.value;
      if (frame.readIndex != 0xFF) {
        CHECK(frame.kind == FrameKind::WRITE && frame.readIndex < index);
        CHECK(result.frames[frame.readIndex].valueValid);
        value = static_cast<uint16_t>((result.frames[frame.readIndex].value & frame.preserveMask) | value);
      }
      CHECK(bus.frames < bus.trace.size());
      bus.trace[bus.frames] = {frame.kind, batch.phyAddress, frame.registerAddress, value};
      const bool fail = static_cast<int>(bus.frames++) == bus.failFrame;
      auto& receipt = result.frames[index];
      receipt.attempted = true;
      if (!fail || bus.failedEffect) {
        if (frame.kind == FrameKind::WRITE) bus.write(phy, frame.registerAddress, value);
        else receipt.value = bus.read(phy, frame.registerAddress);
      }
      bus.now += bus.frameTime;
      if (bus.competitor) {
        uint16_t sentinel = 0xBEEF;
        CHECK(bus.competitor->readClause22(2, sentinel, allowance).is(Err::BUSY));
        CHECK(sentinel == 0xBEEF);
        ++bus.exclusions;
      }
      if (fail) {
        receipt.effectPossible = bus.failedEffect;
        result.status = {Err::MDIO_TIMEOUT, -17017, "Injected negotiation frame timeout"};
        break;
      }
      receipt.completed = true;
      receipt.valueValid = frame.kind == FrameKind::READ;
    }
    if (static_cast<int>(call) == bus.lateBatch) bus.now = allowance.deadlineUs;
    bus.locked = false;
    ++bus.releases;
    return result;
  }
};

OperationBudget budget(uint32_t frames = 512, uint64_t deadline = 1000000) {
  return {{deadline, frames}, 10, 8};
}
Status drain(Driver& driver, Controller& bus, OperationToken token, unsigned limit = 300) {
  Status status{Err::IN_PROGRESS, 0, "Pending"};
  for (unsigned index = 0; index < limit && status.is(Err::IN_PROGRESS); ++index) {
    const auto snapshot = driver.operationSnapshot();
    if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
    const auto calls = bus.calls;
    status = driver.pollOperation(token);
    CHECK(bus.calls - calls <= 1);
    CHECK(!bus.locked);
  }
  CHECK(!status.is(Err::IN_PROGRESS));
  return status;
}
OperationResult take(Driver& driver, OperationToken token) {
  OperationResult result;
  CHECK(driver.takeResult(token, result).ok());
  return result;
}
void initialize(Driver& driver, Controller& bus, ResetKind reset = ResetKind::NONE) {
  OperationToken token;
  CHECK(driver.startInitialize({reset}, budget(), token).is(Err::IN_PROGRESS));
  CHECK(drain(driver, bus, token).ok());
  CHECK(take(driver, token).persistentRestored);
  CHECK(driver.isInitialized());
}

LinkSettings settings(uint8_t technologies = 0x08, unsigned pause = 0) {
  LinkSettings result;
  result.advertisement.technologyMask = technologies;
  result.advertisement.symmetricPause = (pause & 1U) != 0;
  result.advertisement.asymmetricPause = (pause & 2U) != 0;
  return result;
}
uint16_t advertisement(uint8_t technologies, unsigned pause = 0) {
  uint16_t result = 0x0001;
  if ((technologies & 0x01U) != 0) result = static_cast<uint16_t>(result | 0x0020U);
  if ((technologies & 0x02U) != 0) result = static_cast<uint16_t>(result | 0x0040U);
  if ((technologies & 0x04U) != 0) result = static_cast<uint16_t>(result | 0x0080U);
  if ((technologies & 0x08U) != 0) result = static_cast<uint16_t>(result | 0x0100U);
  if ((pause & 1U) != 0) result = static_cast<uint16_t>(result | 0x0400U);
  if ((pause & 2U) != 0) result = static_cast<uint16_t>(result | 0x0800U);
  return result;
}
OperationToken configure(Driver& driver, const LinkSettings& desired = settings()) {
  OperationToken token;
  CHECK(driver.startConfigure(desired, {}, budget(), token).is(Err::IN_PROGRESS));
  return token;
}

void vectors() {
  // All fifteen technology subsets and four pause combinations. Expectations
  // are built from original field literals, never production masks/encoders.
  for (uint8_t technologies = 1; technologies <= 15; ++technologies) {
    for (unsigned pause = 0; pause != 4; ++pause) {
      Controller bus;
      Driver driver;
      CHECK(driver.bind(bus.config()).ok());
      initialize(driver, bus);
      auto& phy = bus.phys[1];
      phy.anar = 0xF21F; // Non-default selector, RF and NP; reserved/RO bits.
      phy.bmcr = 0x217F; // Existing speed/duplex and reserved low bits.
      const auto first = bus.frames, clocks = bus.clocks, calls = bus.calls;
      const auto token = configure(driver, settings(technologies, pause));
      CHECK(bus.clocks == clocks && bus.calls == calls);
      CHECK(driver.settingsSnapshot().desiredValid);
      CHECK(driver.settingsSnapshot().desired.advertisement.technologyMask == technologies);
      CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
      CHECK(drain(driver, bus, token).ok());
      const auto result = take(driver, token);
      CHECK(result.persistentRestored && !result.cleanupOwed);
      CHECK(!result.negotiationComplete); // Register application alone is sufficient here.
      CHECK(phy.anar == static_cast<uint16_t>(0x5200U | advertisement(technologies, pause)));
      CHECK((phy.bmcr & 0x1000U) != 0 && (phy.bmcr & 0x007FU) == 0x007FU);
      CHECK(bus.vodWrites == 1 && phy.vod == 0x4A40);
      CHECK(bus.restarts == 1 && bus.pageLoads == 0);
      CHECK(bus.frames - first == 24); // Six saves, six atomic RMWs, six readbacks.
      CHECK(bus.locks == bus.releases);
      const auto snapshot = driver.settingsSnapshot();
      CHECK(snapshot.trust == ConfigurationTrust::VERIFIED);
      CHECK(snapshot.observedValid && !snapshot.observedStale);
      CHECK(snapshot.observed.advertisement.technologyMask == technologies);
      CHECK(snapshot.observed.advertisement.symmetricPause == ((pause & 1U) != 0));
      CHECK(snapshot.observed.advertisement.asymmetricPause == ((pause & 2U) != 0));
      CHECK(snapshot.anarValid && snapshot.bmcrValid && snapshot.timeValid);
      unsigned anarWrites = 0, restartWrites = 0;
      for (auto index = first; index < bus.frames; ++index) {
        const auto& frame = bus.trace[index];
        if (frame.kind != FrameKind::WRITE) continue;
        CHECK(frame.reg == 0x04 || frame.reg == 0x00 || frame.reg == 0x09 ||
              frame.reg == 0x19 || frame.reg == 0x0B || frame.reg == 0x0A);
        if (frame.reg == 0x04) {
          ++anarWrites;
          CHECK((frame.value & 0xA000U) == 0); // Neither RF nor persistent NP.
          CHECK((frame.value & 0x001FU) == 1);
          CHECK(restartWrites == 0);
        } else if (frame.reg == 0x00) {
          CHECK(anarWrites == 1);
          CHECK(frame.value == 0x127F); // AN enable/restart; stable speed/duplex cleared.
          ++restartWrites;
        }
      }
      CHECK(anarWrites == 1 && restartWrites == 1);
    }
  }
}

void admission() {
  Controller bus;
  Driver driver;
  OperationToken token{0xDEAD};
  CHECK(driver.startConfigure(settings(), {}, budget(), token).is(Err::NOT_BOUND));
  CHECK(driver.bind(bus.config()).ok());
  CHECK(driver.startConfigure(settings(), {}, budget(), token).is(Err::NOT_INITIALIZED));
  CHECK(token.value == 0xDEAD && bus.calls == 0 && bus.clocks == 0);
  initialize(driver, bus);
  const auto calls = bus.calls, clocks = bus.clocks;
  for (unsigned index = 0; index != 9; ++index) {
    auto invalid = settings();
    if (index == 0) invalid.advertisement.technologyMask = 0;
    if (index == 1) invalid.advertisement.technologyMask = 0x10;
    if (index == 2) invalid.advertisement.technologyMask = 0xFF;
    if (index == 3) invalid.crossover.pairSelection = static_cast<PairSelection>(0xFF);
    if (index == 4) invalid.forced.speed = LinkSpeed::MBPS_100;
    if (index == 5) invalid.crossover.swapPair = true;
    if (index == 6) invalid.crossover.swapPolarity = true;
    if (index == 7) invalid.forced.duplex = Duplex::FULL;
    if (index == 8) {
      invalid.mode = LinkMode::FORCED;
      invalid.advertisement = {};
      invalid.forced = {LinkSpeed::MBPS_100, Duplex::FULL};
      invalid.crossover.robust = false; // Force Mode requires this even with manual routing.
    }
    const auto rejected = driver.startConfigure(invalid, {}, budget(), token);
    CHECK(!rejected.ok() && !rejected.inProgress());
    CHECK(token.value == 0xDEAD);
    CHECK(bus.calls == calls && bus.clocks == clocks);
    CHECK(!driver.settingsSnapshot().desiredValid);
  }
  CHECK(driver.isInitialized());
}

void replay() {
  for (const auto reset : {ResetKind::NONE, ResetKind::PCS, ResetKind::HARD, ResetKind::RESTART}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    const auto token = configure(driver, settings(0x03, 3)); // 10 Mbit/s only.
    CHECK(drain(driver, bus, token).ok());
    (void)take(driver, token);
    bus.phys[1].anar = 0x8101; // Untrusted hardware is repaired from desired state.
    bus.phys[1].bmcr = 0;
    initialize(driver, bus, reset);
    CHECK(bus.phys[1].anar == 0x0C61);
    CHECK((bus.phys[1].bmcr & 0x1000U) != 0);
    CHECK(bus.phys[1].vod == 0x4A40 && bus.vodWrites == 2);
    CHECK(bus.restarts == 2);
    for (const auto& frame : bus.trace) {
      if (frame.kind == FrameKind::WRITE && frame.reg == 0x04) CHECK((frame.value & 0x8000U) == 0);
    }
  }
}

void failures() {
  Controller baseline;
  Driver reference;
  CHECK(reference.bind(baseline.config()).ok());
  initialize(reference, baseline);
  const auto firstFrame = baseline.frames, firstCall = baseline.calls;
  auto token = configure(reference);
  CHECK(drain(reference, baseline, token).ok());
  const auto frameCount = baseline.frames - firstFrame;
  const auto batchCount = baseline.calls - firstCall;
  (void)take(reference, token);

  for (unsigned failed = 0; failed < frameCount; ++failed) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    const auto startFrame = bus.frames;
    bus.failFrame = static_cast<int>(startFrame + failed);
    token = configure(driver);
    CHECK(drain(driver, bus, token).is(Err::MDIO_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.status.detail == -17017);
    CHECK(bus.frames == startFrame + failed + 1);
    CHECK(bus.locks == bus.releases);
    bool mutation = false;
    for (unsigned index = startFrame; index < bus.frames; ++index)
      mutation = mutation || bus.trace[index].kind == FrameKind::WRITE;
    CHECK(result.cleanupOwed == mutation);
    CHECK(result.mutationPossible == mutation);
    const auto calls = bus.calls, clocks = bus.clocks;
    (void)driver.evidenceSnapshot();
    (void)driver.operationSnapshot();
    CHECK(bus.calls == calls && bus.clocks == clocks);
    bus.failFrame = -1;
    initialize(driver, bus);
    CHECK(bus.phys[1].anar == 0x0101);
  }

  for (unsigned boundary = 0; boundary < batchCount; ++boundary) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    token = configure(driver, settings(0x02));
    const auto startFrame = bus.frames;
    for (unsigned index = 0; index < boundary; ++index)
      CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
    const auto calls = bus.calls, clocks = bus.clocks;
    CHECK(driver.cancel(token).ok());
    CHECK(bus.calls == calls && bus.clocks == clocks);
    const auto result = take(driver, token);
    CHECK(result.status.is(Err::CANCELLED));
    CHECK(driver.settingsSnapshot().desiredValid);
    CHECK(driver.settingsSnapshot().desired.advertisement.technologyMask == 0x02);
    CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
    bool mutation = false;
    for (unsigned index = startFrame; index < bus.frames; ++index)
      mutation = mutation || bus.trace[index].kind == FrameKind::WRITE;
    CHECK(result.cleanupOwed == mutation);
    initialize(driver, bus);
    CHECK(bus.phys[1].anar == 0x0041); // Admitted intent survives cancellation.
  }

  // Every target readback comparison is meaningful; a later bus failure must
  // not discard an earlier mismatch or silently replace it with success.
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config()).ok());
  initialize(driver, bus);
  token = configure(driver);
  unsigned writeCount = 0;
  while (writeCount < 6) {
    const auto before = bus.frames;
    CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
    for (auto index = before; index < bus.frames; ++index)
      if (bus.trace[index].kind == FrameKind::WRITE) ++writeCount;
  }
  bus.badAnar = 0x0001;
  CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
  bus.failFrame = static_cast<int>(bus.frames);
  CHECK(drain(driver, bus, token).is(Err::CONFIG_MISMATCH));
  const auto mismatch = take(driver, token);
  CHECK(mismatch.mismatch.valid && mismatch.mismatch.registerAddress == 0x04);
  CHECK(mismatch.mismatch.expected == 0x0101 && mismatch.mismatch.observed == 0x0001);
  CHECK(mismatch.cleanupOwed);
  CHECK(mismatch.failingBatch.normalized.is(Err::MDIO_TIMEOUT));

  Controller controlBus;
  Driver controlDriver;
  CHECK(controlDriver.bind(controlBus.config()).ok());
  initialize(controlDriver, controlBus);
  token = configure(controlDriver);
  controlBus.badBmcr = 0x2000; // Complete readback proves AN remains disabled.
  CHECK(drain(controlDriver, controlBus, token).is(Err::CONFIG_MISMATCH));
  const auto controlMismatch = take(controlDriver, token);
  CHECK(controlMismatch.mismatch.valid && controlMismatch.mismatch.registerAddress == 0x00);
  CHECK(controlMismatch.mismatch.mask == 0x3100 && controlMismatch.mismatch.expected == 0x1000);
  CHECK(controlMismatch.mismatch.observed == 0x2000 && controlMismatch.cleanupOwed);
}

void ownership() {
  Controller bus;
  Driver driver, other;
  CHECK(driver.bind(bus.config()).ok());
  CHECK(other.bind(bus.config(2)).ok());
  initialize(driver, bus);
  bus.competitor = &other;
  const auto token = configure(driver);
  const auto before = bus.frames;
  CHECK(driver.pollOperation(token, 0).is(Err::IN_PROGRESS));
  CHECK(bus.frames == before);
  CHECK(drain(driver, bus, token).ok());
  CHECK(take(driver, token).persistentRestored);
  CHECK(bus.exclusions == bus.frames - before);
  CHECK(bus.locks == bus.releases);
  CHECK(bus.phys[2].anar == 0x01E1 && bus.phys[2].vod == 0x8400);
}

ConfigureOptions exchangeOptions() {
  ConfigureOptions options;
  options.nextPages.pageCount = 2;
  options.nextPages.pages[0] = {4, true, true}; // Remote fault defining one UP.
  options.nextPages.pages[1] = {3, false, true};
  return options;
}
OperationToken exchange(Driver& driver, const ConfigureOptions& options = exchangeOptions(),
                        OperationBudget allowance = budget()) {
  OperationToken token;
  CHECK(driver.startConfigure(settings(), options, allowance, token).is(Err::IN_PROGRESS));
  return token;
}
void preparePartner(Controller& bus) {
  bus.automaticPartner = true;
  bus.partnerPages[0] = 0xA801; // NP, MP, inverted base toggle, null message.
  bus.partnerPages[1] = 0x2001; // Final null, alternating toggle.
  bus.partnerCount = 2;
}
void advanceUntilLoaded(Driver& driver, Controller& bus, OperationToken token) {
  for (unsigned count = 0; count < 30 && bus.pageLoads == 0; ++count) {
    const auto snapshot = driver.operationSnapshot();
    if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
    CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
  }
  CHECK(bus.pageLoads == 1);
}

void pages() {
  for (const bool duplicate : {false, true}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    bus.duplicateSecond = duplicate;
    const auto startFrame = bus.frames;
    const auto token = exchange(driver);
    CHECK(drain(driver, bus, token).ok());
    const auto result = take(driver, token);
    CHECK(result.nextPage.requested && result.nextPage.complete);
    CHECK(result.negotiationComplete);
    CHECK(result.nextPage.baseReceived && result.nextPage.basePageValid);
    CHECK(result.nextPage.basePage == 0x8101);
    CHECK(!result.nextPage.partnerRefused && !result.nextPage.npMayRemainSet);
    CHECK(result.nextPage.transmittedPages == 2);
    CHECK(result.nextPage.receivedPages == (duplicate ? 3 : 2));
    CHECK(result.nextPage.duplicatePages == (duplicate ? 1 : 0));
    CHECK(result.nextPage.pages[0].valid && result.nextPage.pages[0].raw == 0xA801);
    CHECK(result.nextPage.pages[duplicate ? 2 : 1].valid);
    CHECK(result.nextPage.pages[duplicate ? 2 : 1].raw == 0x2001);
    CHECK(bus.phys[1].anar == 0x0101 && bus.restarts == 1);
    CHECK(bus.vodWrites == 1 && bus.pageLoads == 2);
    CHECK(bus.frames - startFrame == (duplicate ? 41U : 39U));
    bool sawRestart = false, sawBase = false;
    unsigned pageWrites = 0, advertisementWrites = 0;
    for (auto index = startFrame; index < bus.frames; ++index) {
      const auto& frame = bus.trace[index];
      if (frame.reg == 0x05 && frame.kind == FrameKind::READ) sawBase = true;
      if (frame.kind != FrameKind::WRITE) continue;
      if (frame.reg == 0x00) sawRestart = true;
      if (frame.reg == 0x04) {
        ++advertisementWrites;
        CHECK(frame.value == (advertisementWrites == 1 ? 0x8101 : 0x0101));
      }
      if (frame.reg == 0x07) {
        CHECK(sawRestart && sawBase);
        CHECK(frame.value == (pageWrites == 0 ? 0xB804 : 0x1803));
        ++pageWrites;
      }
    }
    CHECK(advertisementWrites == 2 && pageWrites == 2);
  }

  // Local one-page completion cannot abandon a partner with more data.
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config()).ok());
  initialize(driver, bus);
  preparePartner(bus);
  auto options = exchangeOptions();
  options.nextPages.pageCount = 1;
  options.nextPages.pages[0] = {1, true, false};
  const auto token = exchange(driver, options);
  CHECK(drain(driver, bus, token).ok());
  const auto result = take(driver, token);
  CHECK(result.nextPage.transmittedPages == 2 && result.nextPage.receivedPages == 2);
  CHECK(bus.phys[1].page == 0x2801); // Generated null MP1, NP0, preserved toggle.
}

void pageFailures() {
  // The epoch's initial drain is destructive even when its terminal read fails.
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    const auto token = exchange(driver);
    bus.failFrame = static_cast<int>(bus.frames);
    CHECK(drain(driver, bus, token).is(Err::MDIO_TIMEOUT));
    const auto result = take(driver, token);
    const auto sample = driver.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::ANER)];
    CHECK(sample.historyLost && !sample.valid);
    CHECK(!result.cleanupOwed && !result.nextPage.npMayRemainSet);
  }
  // Partner refusal still completes ordinary AN and clears the owned NP epoch.
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    bus.partnerRefuses = true;
    const auto token = exchange(driver);
    CHECK(drain(driver, bus, token).ok());
    const auto result = take(driver, token);
    CHECK(result.nextPage.partnerRefused && !result.nextPage.npMayRemainSet);
    CHECK(bus.pageLoads == 0 && bus.phys[1].anar == 0x0101);
    CHECK(bus.restarts == 1);
  }
  // A consumed NEXT event remains represented when the following payload read fails.
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    const auto token = exchange(driver);
    advanceUntilLoaded(driver, bus, token);
    const auto sampleCount = driver.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::ANER)].samples;
    const auto snapshot = driver.operationSnapshot();
    if (snapshot.nextPollValid) bus.now = snapshot.nextPollAtUs;
    CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
    CHECK(driver.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::ANER)].samples > sampleCount);
    bus.failFrame = static_cast<int>(bus.frames);
    CHECK(drain(driver, bus, token).is(Err::MDIO_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.nextPage.pagePayloadMissing && result.nextPage.receivedPages == 1);
    CHECK(!result.nextPage.pages[0].valid);
    CHECK(result.cleanupOwed && result.nextPage.npMayRemainSet);
    CHECK((bus.phys[1].anar & 0x8000U) != 0 && bus.restarts == 1);
  }
  // A fresh epoch with no page service is pending until a finite budget ends it.
  for (const bool cancel : {false, true}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    bus.partnerCount = 0;
    const auto token = exchange(driver, exchangeOptions(), budget(48));
    advanceUntilLoaded(driver, bus, token);
    CHECK((bus.phys[1].anar & 0x8000U) != 0 && bus.phys[1].bmsr == 0);
    if (cancel) {
      const auto calls = bus.calls, clocks = bus.clocks;
      CHECK(driver.cancel(token).ok());
      CHECK(bus.calls == calls && bus.clocks == clocks);
    } else CHECK(drain(driver, bus, token).is(Err::OPERATION_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.cleanupOwed && result.nextPage.npMayRemainSet && !result.nextPage.complete);
    CHECK(bus.restarts == 1 && (bus.phys[1].anar & 0x8000U) != 0);
    bus.automaticPartner = false;
    initialize(driver, bus);
    CHECK(bus.phys[1].anar == 0x0101 && bus.restarts == 2 && bus.pageLoads == 1);
  }
  // The accepted page budget includes duplicates and generated null service.
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    auto options = exchangeOptions();
    options.nextPages.maxPartnerPages = 1;
    const auto token = exchange(driver, options);
    CHECK(drain(driver, bus, token).is(Err::OPERATION_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.nextPage.receivedPages == 1 && result.nextPage.pages[0].valid);
    CHECK(result.cleanupOwed && result.nextPage.npMayRemainSet && bus.pageLoads == 1);
    bus.automaticPartner = false;
    const auto retry = configure(driver, settings(0x02));
    CHECK(drain(driver, bus, retry).ok());
    CHECK(!take(driver, retry).cleanupOwed);
    CHECK(bus.phys[1].anar == 0x0041 && bus.restarts == 2);
  }
}

void pageBoundaries() {
  Controller referenceBus;
  Driver reference;
  CHECK(reference.bind(referenceBus.config()).ok());
  initialize(reference, referenceBus);
  preparePartner(referenceBus);
  const auto firstFrame = referenceBus.frames, firstCall = referenceBus.calls;
  auto token = exchange(reference);
  CHECK(drain(reference, referenceBus, token).ok());
  (void)take(reference, token);
  const auto frames = referenceBus.frames - firstFrame;
  const auto batches = referenceBus.calls - firstCall;
  for (unsigned failed = 0; failed < frames; ++failed) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    const auto before = bus.frames;
    bus.failFrame = static_cast<int>(before + failed);
    token = exchange(driver);
    CHECK(drain(driver, bus, token).is(Err::MDIO_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.status.detail == -17017);
    CHECK(bus.frames == before + failed + 1 && bus.locks == bus.releases);
    bool mutation = false;
    for (auto index = before; index < bus.frames; ++index)
      mutation = mutation || bus.trace[index].kind == FrameKind::WRITE;
    CHECK(result.cleanupOwed == mutation);
    for (unsigned index = 0; index < result.nextPage.receivedPages; ++index) {
      if (result.nextPage.pages[index].valid)
        CHECK(result.nextPage.pages[index].raw == (index == 0 ? 0xA801 : 0x2001));
    }
  }
  for (unsigned boundary = 0; boundary < batches; ++boundary) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    token = exchange(driver);
    for (unsigned index = 0; index < boundary; ++index) {
      const auto snapshot = driver.operationSnapshot();
      if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
      CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
    }
    const auto calls = bus.calls, clocks = bus.clocks;
    EvidenceSnapshot sentinel;
    sentinel.lastRequested = 0xDEADBEEF;
    CHECK(driver.acquireEvidence(evidenceBit(EvidenceSource::ANER), sentinel, {1000000, 1}).is(Err::BUSY));
    CHECK(sentinel.lastRequested == 0xDEADBEEF);
    CHECK(driver.cancel(token).ok());
    CHECK(bus.calls == calls && bus.clocks == clocks);
    CHECK(take(driver, token).status.is(Err::CANCELLED));
  }
}

void pageAdmission() {
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config()).ok());
  initialize(driver, bus);
  const auto calls = bus.calls, clocks = bus.clocks;
  for (unsigned index = 0; index < 10; ++index) {
    auto options = exchangeOptions();
    if (index == 0) options.nextPages.pages[0].data = 0x0800;
    if (index == 1) options.nextPages.pages[0] = {0, true, false}; // Reserved message.
    if (index == 2) options.nextPages.pages[0] = {2, true, false}; // Unsupported message.
    if (index == 3) options.nextPages.pages[0].message = false; // UP has no defining message.
    if (index == 4) options.nextPages.pageCount = 1; // Missing referenced UP.
    if (index == 5) options.nextPages.pages[1].data = 4; // Remote-fault UP legal 0..3.
    if (index == 6) options.nextPages.maxPartnerPages = 0;
    if (index == 7) options.nextPages.pageCount = 17;
    if (index == 8) options.nextPages.maxPartnerPages = 17;
    if (index == 9) {
      options.nextPages.pages[0] = {1, true, false};
      options.nextPages.pages[1] = {1, true, false}; // Null terminates local information.
    }
    OperationToken token{0xDEAD};
    const auto status = driver.startConfigure(settings(), options, budget(), token);
    CHECK(!status.ok() && !status.is(Err::IN_PROGRESS));
    CHECK(token.value == 0xDEAD && bus.calls == calls && bus.clocks == clocks);
  }
}

void completion() {
  // Restart-clear and complete register readback do not manufacture AN-complete.
  for (const bool partnerComplete : {false, true}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    bus.phys[1].bmsr = partnerComplete ? 0x0020 : 0;
    ConfigureOptions options;
    options.waitForCompletion = true;
    OperationToken token;
    CHECK(driver.startConfigure(settings(), options, budget(40), token).is(Err::IN_PROGRESS));
    const auto status = drain(driver, bus, token);
    CHECK(status.is(partnerComplete ? Err::OK : Err::OPERATION_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(!result.nextPage.requested && bus.pageLoads == 0);
    CHECK(result.negotiationComplete == partnerComplete);
    CHECK(bus.restarts == 1 && bus.phys[1].anar == 0x0101);
    CHECK(result.cleanupOwed != partnerComplete);
  }
  // Bilateral NP termination still does not prove that AN has completed.
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    bus.completePartner = false;
    const auto token = exchange(driver);
    bool observedWait = false;
    for (unsigned index = 0; index < 30 && !observedWait; ++index) {
      const auto snapshot = driver.operationSnapshot();
      if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
      const auto previous = bus.frames;
      CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS));
      observedWait = bus.frames > previous && bus.trace[bus.frames - 1].reg == 0x01;
    }
    CHECK(observedWait && bus.pageLoads == 2);
    CHECK(driver.cancel(token).ok());
    const auto result = take(driver, token);
    CHECK(!result.negotiationComplete && !result.nextPage.complete);
    CHECK(result.nextPage.receivedPages == 2 && result.cleanupOwed);
  }
}

void timing() {
  // A short slice yields before an indivisible CR1 RMW; the next full slice
  // still reads the hardware value immediately before its owned update.
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    const auto token = exchange(driver);
    for (unsigned index = 0; index < 7; ++index) CHECK(driver.pollOperation(token, 1).is(Err::IN_PROGRESS));
    const auto frames = bus.frames;
    CHECK(driver.pollOperation(token, 0).is(Err::IN_PROGRESS));
    CHECK(driver.pollOperation(token, 1).is(Err::IN_PROGRESS));
    CHECK(bus.frames == frames);
    CHECK(driver.pollOperation(token, 2).is(Err::IN_PROGRESS));
    CHECK(bus.frames == frames + 2);
    CHECK(driver.cancel(token).ok());
    CHECK(take(driver, token).cleanupOwed);
  }
  // Frozen clocks cannot make an unserviced NP epoch run without bound.
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    bus.frameTime = 0;
    const auto token = exchange(driver);
    Status status{Err::IN_PROGRESS, 0, "Pending"};
    for (unsigned index = 0; index < 30 && status.is(Err::IN_PROGRESS); ++index)
      status = driver.pollOperation(token);
    CHECK(status.is(Err::OPERATION_TIMEOUT));
    CHECK(status.detail == static_cast<int32_t>(TimeoutDetail::POLL_BUDGET));
    const auto result = take(driver, token);
    CHECK(result.nextPage.npMayRemainSet && result.cleanupOwed);
    CHECK(bus.pageLoads == 0 && bus.restarts == 1);
  }
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    const auto token = exchange(driver);
    advanceUntilLoaded(driver, bus, token);
    bus.now = 1;
    const auto frames = bus.frames;
    CHECK(driver.pollOperation(token).is(Err::INVALID_CONFIG));
    const auto result = take(driver, token);
    CHECK(result.status.detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    CHECK(result.clockInvalid && result.cleanupOwed && bus.frames == frames);
    CHECK(driver.transportSnapshot().clockInvalid);
  }
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    bus.lateBatch = static_cast<int>(bus.calls + 10); // Accepted ANAR write then deadline.
    const auto token = configure(driver);
    CHECK(drain(driver, bus, token).is(Err::OPERATION_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.late && result.cleanupOwed && result.mutationCompleted);
    CHECK(bus.phys[1].anar == 0x0101 && bus.restarts == 0);
  }
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    OperationToken token;
    const auto deadline = bus.now + 20;
    CHECK(driver.startConfigure(settings(), budget(40, deadline), token).is(Err::IN_PROGRESS));
    bus.now = deadline;
    const auto frames = bus.frames;
    CHECK(driver.pollOperation(token).is(Err::OPERATION_TIMEOUT));
    CHECK(!take(driver, token).cleanupOwed && bus.frames == frames);
  }
  // A valid page received in a late callback is retained; it cannot make the
  // expired exchange successful or authorize an implicit NP cleanup write.
  {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    initialize(driver, bus);
    preparePartner(bus);
    const auto token = exchange(driver);
    advanceUntilLoaded(driver, bus, token);
    auto snapshot = driver.operationSnapshot();
    if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
    CHECK(driver.pollOperation(token).is(Err::IN_PROGRESS)); // Consume page event.
    bus.lateBatch = static_cast<int>(bus.calls);
    CHECK(driver.pollOperation(token).is(Err::OPERATION_TIMEOUT));
    const auto result = take(driver, token);
    CHECK(result.late && result.cleanupOwed && result.nextPage.npMayRemainSet);
    CHECK(result.nextPage.receivedPages == 1 && result.nextPage.pages[0].valid);
    CHECK(result.nextPage.pages[0].raw == 0xA801 && !result.negotiationComplete);
    CHECK(bus.pageLoads == 1 && bus.restarts == 1);
  }
}

} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"vectors", vectors}, {"admission", admission}, {"replay", replay},
    {"failures", failures}, {"ownership", ownership}, {"pages", pages},
    {"page_failures", pageFailures}, {"page_admission", pageAdmission}, {"completion", completion},
    {"page_boundaries", pageBoundaries}, {"timing", timing}
  };
  return test::runSuites(argc, argv, suites);
}
