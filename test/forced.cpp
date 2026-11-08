#include "support/Check.h"
#include <DP8382x/DP8382x.h>

#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;

// Independent literal controller from SNLS647I tables 8-19, 8-28..30 and
// 8-43. This fixture establishes software sequencing, never physical routing.
struct Wire { FrameKind kind; uint8_t phy, reg; uint16_t value; };
struct Controller {
  struct Phy {
    std::array<uint16_t, 32> words{};
    uint16_t selector{0}, address{0}, vod{0x8400};
    Phy() { words[0] = 0x3100; words[4] = 0x01E1; }
  };
  std::array<Phy, 32> phys{};
  std::array<Wire, 2048> trace{};
  std::array<uint8_t, 1024> sizes{};
  uint64_t now{100}, frameTime{2};
  unsigned frames{0}, calls{0}, clocks{0}, locks{0}, releases{0}, vodWrites{0};
  int failFrame{-1}, lateBatch{-1}, mismatchReg{-1};
  uint16_t mismatchXor{0};
  bool failAfterEffect{true}, locked{false}, enforceForcedRobust{false};
  std::array<bool, 32> touched{};
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
    if (reg == 2) return 0x2000;
    if (reg == 3) return 0xA131;
    if (reg == 0) {
      if ((phy.words[0] & 0x8000U) != 0) phy.words[0] = 0x3100;
      phy.words[0] = static_cast<uint16_t>(phy.words[0] & 0xFDFFU);
    }
    if (reg == 0x1F) phy.words[reg] = 0;
    if (reg == 0x0E && phy.selector == 0x401F) {
      if (phy.address == 0x030E) return phy.vod;
      if (phy.address == 0x0467) return 0x1234;
      if (phy.address == 0x0468) return 0x5678;
    }
    const auto word = phy.words[reg];
    return static_cast<int>(reg) == mismatchReg && touched[reg] ?
        static_cast<uint16_t>(word ^ mismatchXor) : word;
  }
  void write(Phy& phy, uint8_t reg, uint16_t value) noexcept {
    if (reg == 0x0D) phy.selector = value;
    else if (reg == 0x0E && phy.selector == 0x001F) phy.address = value;
    else if (reg == 0x0E && phy.selector == 0x401F && phy.address == 0x030E) {
      phy.vod = value;
      ++vodWrites;
    } else {
      phy.words[reg] = value;
      touched[reg] = true;
      if (reg == 0 && (value & 0x8000U) != 0) {
        phy.words[4] = 0x01E1;
        phy.words[9] = 0;
        phy.words[10] = 0;
        phy.words[11] = 0;
        phy.words[25] = 0x8001;
      }
      if (reg == 0x1F && value == 0x8000) {
        phy.words.fill(0);
        phy.words[0] = 0x3100;
        phy.words[4] = 0x01E1;
        phy.vod = 0x8400;
      }
    }
  }
  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget& allowance) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    BatchResult result;
    result.releaseStatus = Status::Ok();
    result.controllerReusable = true;
    if (bus.locked) {
      result.status = {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), "Controller owned"};
      return result;
    }
    CHECK(bus.calls < bus.sizes.size());
    const auto call = bus.calls++;
    bus.sizes[call] = batch.frameCount;
    CHECK(batch.frameCount <= allowance.maxFrames);
    bus.locked = true;
    ++bus.locks;
    result.status = Status::Ok();
    auto& phy = bus.phys[batch.phyAddress];
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& request = batch.frames[i];
      auto written = request.value;
      if (request.readIndex != 0xFF) {
        CHECK(request.kind == FrameKind::WRITE && request.readIndex < i);
        CHECK(result.frames[request.readIndex].valueValid);
        written = static_cast<uint16_t>((result.frames[request.readIndex].value & request.preserveMask) | written);
      }
      CHECK(bus.frames < bus.trace.size());
      bus.trace[bus.frames] = {request.kind, batch.phyAddress, request.registerAddress, written};
      const bool fail = static_cast<int>(bus.frames++) == bus.failFrame;
      auto& receipt = result.frames[i];
      receipt.attempted = true;
      if (!fail || bus.failAfterEffect) {
        if (request.kind == FrameKind::WRITE) bus.write(phy, request.registerAddress, written);
        else receipt.value = bus.read(phy, request.registerAddress);
      }
      if (bus.enforceForcedRobust && request.kind == FrameKind::WRITE &&
          (phy.words[0] & 0x1000U) == 0U && (phy.words[0] & 0x8000U) == 0U) {
        CHECK((phy.words[9] & 0x0020U) != 0U);
      }
      bus.now += bus.frameTime;
      if (bus.competitor) {
        uint16_t sentinel = 0xDEAD;
        CHECK(bus.competitor->readClause22(2, sentinel, allowance).is(Err::BUSY));
        CHECK(sentinel == 0xDEAD);
        ++bus.exclusions;
      }
      if (fail) {
        receipt.effectPossible = bus.failAfterEffect;
        result.status = {Err::MDIO_TIMEOUT, -18018, "Injected forced profile timeout"};
        break;
      }
      receipt.completed = true;
      receipt.valueValid = request.kind == FrameKind::READ;
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
Status drain(Driver& driver, Controller& bus, OperationToken token) {
  Status result{Err::IN_PROGRESS, 0, "Pending"};
  for (unsigned i = 0; i < 256 && result.inProgress(); ++i) {
    const auto snapshot = driver.operationSnapshot();
    if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
    const auto calls = bus.calls;
    result = driver.pollOperation(token);
    CHECK(bus.calls - calls <= 1);
    CHECK(!bus.locked);
  }
  CHECK(!result.inProgress());
  return result;
}
OperationResult take(Driver& driver, OperationToken token) {
  OperationResult result;
  CHECK(driver.takeResult(token, result).ok());
  return result;
}
void initialize(Driver& driver, Controller& bus, ResetKind reset = ResetKind::NONE) {
  OperationToken token;
  CHECK(driver.startInitialize({reset}, budget(), token).inProgress());
  CHECK(drain(driver, bus, token).ok());
  CHECK(take(driver, token).persistentRestored);
  CHECK(driver.isInitialized());
}
LinkSettings forced(LinkSpeed speed = LinkSpeed::MBPS_100, Duplex duplex = Duplex::FULL,
                    PairSelection route = PairSelection::AUTOMATIC, bool mirror = false,
                    bool extension = false) {
  LinkSettings profile;
  profile.mode = LinkMode::FORCED;
  profile.forced = {speed, duplex};
  profile.crossover.pairSelection = route;
  profile.crossover.swapPair = mirror;
  profile.crossover.swapPolarity = mirror;
  profile.extendedFullDuplex = extension;
  return profile;
}
LinkSettings negotiated() {
  LinkSettings profile;
  profile.advertisement.technologyMask = 0x0F;
  return profile;
}
OperationToken start(Driver& driver, const LinkSettings& profile) {
  OperationToken token;
  CHECK(driver.startConfigure(profile, {}, budget(), token).inProgress());
  return token;
}
void seed(Controller& bus) {
  auto& words = bus.phys[1].words;
  words[0] = 0x317F;
  words[4] = 0xF21F;
  words[9] = 0xA59F;
  words[10] = 0x5ADF;
  words[11] = 0xC59F;
  words[25] = 0x7BBF;
  bus.touched.fill(false);
}
uint16_t forcedWord(const LinkSettings& profile) {
  return static_cast<uint16_t>((profile.forced.speed == LinkSpeed::MBPS_100 ? 0x2000U : 0U) |
                              (profile.forced.duplex == Duplex::FULL ? 0x0100U : 0U));
}
void checkProfile(const Driver& driver, const Controller& bus, const LinkSettings& profile) {
  const auto& words = bus.phys[1].words;
  CHECK(words[9] == static_cast<uint16_t>(0xA59FU | (profile.crossover.robust ? 0x0020U : 0U)));
  CHECK(words[10] == static_cast<uint16_t>(0x5ADFU | (profile.extendedFullDuplex ? 0x0020U : 0U)));
  CHECK(words[11] == static_cast<uint16_t>(0xC59FU | (profile.crossover.swapPair ? 0x0060U : 0U)));
  const auto route = profile.crossover.pairSelection;
  CHECK(words[25] == (route == PairSelection::AUTOMATIC ? 0xFBBF :
                       route == PairSelection::MDI ? 0x3BBF : 0x7BBF));
  const bool an = profile.mode == LinkMode::AUTONEGOTIATED;
  CHECK(words[0] == static_cast<uint16_t>(0x007FU | (an ? 0x1000U : forcedWord(profile))));
  CHECK(bus.phys[1].vod == 0x4A40 && bus.vodWrites == 1);
  const auto snapshot = driver.settingsSnapshot();
  CHECK(snapshot.trust == ConfigurationTrust::VERIFIED && snapshot.observedValid && !snapshot.observedStale);
  CHECK(snapshot.observed.mode == profile.mode);
  CHECK(snapshot.observed.forced.speed == profile.forced.speed && snapshot.observed.forced.duplex == profile.forced.duplex);
  CHECK(snapshot.observed.advertisement.technologyMask == profile.advertisement.technologyMask);
  CHECK(snapshot.observed.crossover.pairSelection == route);
  CHECK(snapshot.observed.crossover.robust == profile.crossover.robust);
  CHECK(snapshot.observed.crossover.swapPair == profile.crossover.swapPair);
  CHECK(snapshot.observed.crossover.swapPolarity == profile.crossover.swapPolarity);
  CHECK(snapshot.observed.extendedFullDuplex == profile.extendedFullDuplex);
  CHECK(snapshot.cr1Valid && snapshot.phycrValid && snapshot.cr3Valid && snapshot.cr2Valid && snapshot.bmcrValid);
  CHECK(snapshot.rawCr1 == words[9] && snapshot.rawPhycr == words[25]);
  CHECK(snapshot.rawCr3 == words[11] && snapshot.rawCr2 == words[10]);
}
void checkTrace(const Controller& bus, unsigned first, const LinkSettings& profile) {
  const bool an = profile.mode == LinkMode::AUTONEGOTIATED;
  CHECK(bus.frames - first == (an ? 24U : 20U));
  const std::array<uint8_t, 6> writeOrder = an && !profile.crossover.robust ?
      std::array<uint8_t, 6>{{25, 11, 10, 4, 0, 9}} :
      std::array<uint8_t, 6>{{9, 25, 11, 10, 4, 0}};
  unsigned writes = 0;
  for (unsigned i = first; i < bus.frames; ++i) {
    const auto& wire = bus.trace[i];
    CHECK(wire.reg != 0x0D && wire.reg != 0x0E); // No duplicate VOD setup.
    if (!an) CHECK(wire.reg != 4); // No ANAR read, write or verify in force mode.
    if (wire.kind == FrameKind::WRITE) {
      const auto expectedIndex = !an && writes == 4 ? 5U : writes;
      CHECK(wire.reg == writeOrder[expectedIndex]);
      if (wire.reg == 0) {
        CHECK(wire.value == static_cast<uint16_t>(0x007FU | (an ? 0x1200U : forcedWord(profile))));
        CHECK((wire.value & 0x8000U) == 0);
      }
      ++writes;
    }
  }
  CHECK(writes == (an ? 6U : 5U));
}

void matrix() {
  unsigned forcedCases = 0, anCases = 0;
  for (const auto speed : {LinkSpeed::MBPS_10, LinkSpeed::MBPS_100}) {
    for (const auto duplex : {Duplex::HALF, Duplex::FULL}) {
      for (const auto route : {PairSelection::AUTOMATIC, PairSelection::MDI, PairSelection::MDIX}) {
        for (const bool mirror : {false, true}) {
          for (const bool extension : {false, true}) {
            if (extension && speed == LinkSpeed::MBPS_10) continue;
            Controller bus; Driver driver;
            CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus); seed(bus);
            const auto profile = forced(speed, duplex, route, mirror, extension);
            const auto first = bus.frames, calls = bus.calls, clocks = bus.clocks;
            const auto token = start(driver, profile);
            CHECK(bus.calls == calls && bus.clocks == clocks);
            CHECK(drain(driver, bus, token).ok());
            const auto result = take(driver, token);
            CHECK(result.persistentRestored && !result.cleanupOwed && !result.negotiationComplete);
            CHECK(!result.savedAnarValid);
            CHECK(result.savedCr1Valid && result.savedPhycrValid && result.savedCr3Valid && result.savedCr2Valid);
            CHECK(result.savedCr1 == 0xA59F && result.savedPhycr == 0x7BBF);
            checkProfile(driver, bus, profile); checkTrace(bus, first, profile);
            CHECK(bus.phys[1].words[4] == 0xF21F);
            ++forcedCases;
          }
        }
      }
    }
  }
  for (const auto route : {PairSelection::AUTOMATIC, PairSelection::MDI, PairSelection::MDIX}) {
    for (const bool robust : {false, true}) {
      for (const bool mirror : {false, true}) {
        for (const bool extension : {false, true}) {
          Controller bus; Driver driver;
          CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus); seed(bus);
          auto profile = negotiated();
          profile.crossover = {route, robust, mirror, mirror};
          profile.extendedFullDuplex = extension;
          const auto first = bus.frames;
          const auto token = start(driver, profile);
          CHECK(drain(driver, bus, token).ok());
          CHECK(take(driver, token).savedAnarValid);
          checkProfile(driver, bus, profile); checkTrace(bus, first, profile);
          CHECK(bus.phys[1].words[4] == 0x53E1);
          ++anCases;
        }
      }
    }
  }
  CHECK(forcedCases == 36 && anCases == 24);
  // Automatic routing preserves a preexisting manual bit of either polarity.
  for (const uint16_t prior : {uint16_t{0x3BBF}, uint16_t{0x7BBF}}) {
    Controller bus; Driver driver;
    CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus); seed(bus);
    bus.phys[1].words[25] = prior;
    const auto token = start(driver, forced());
    CHECK(drain(driver, bus, token).ok()); take(driver, token);
    CHECK(bus.phys[1].words[25] == static_cast<uint16_t>(prior | 0x8000U));
  }
}

void admission() {
  Controller bus; Driver driver; OperationToken token{0xBAD};
  CHECK(driver.startConfigure(forced(), budget(), token).is(Err::NOT_BOUND));
  CHECK(driver.bind(bus.config()).ok());
  CHECK(driver.startConfigure(forced(), budget(), token).is(Err::NOT_INITIALIZED));
  initialize(driver, bus);
  const auto old = start(driver, negotiated()); CHECK(drain(driver, bus, old).ok()); take(driver, old);
  const auto calls = bus.calls, clocks = bus.clocks;
  for (unsigned i = 0; i < 10; ++i) {
    auto profile = forced(); ConfigureOptions options;
    if (i == 0) profile.crossover.robust = false;
    if (i == 1) { profile.forced.speed = LinkSpeed::MBPS_10; profile.extendedFullDuplex = true; }
    if (i == 2) profile.advertisement.technologyMask = 8;
    if (i == 3) profile.advertisement.symmetricPause = true;
    if (i == 4) profile.advertisement.asymmetricPause = true;
    if (i == 5) profile.crossover.swapPair = true;
    if (i == 6) profile.crossover.swapPolarity = true;
    if (i == 7) options.waitForCompletion = true;
    if (i == 8) { options.nextPages.pageCount = 1; options.nextPages.pages[0] = {1, true, false}; }
    if (i == 9) profile.forced.speed = static_cast<LinkSpeed>(255);
    CHECK(!driver.startConfigure(profile, options, budget(), token).inProgress());
    CHECK(token.value == 0xBAD);
    CHECK(driver.settingsSnapshot().desired.mode == LinkMode::AUTONEGOTIATED);
    CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
  }
  auto contradictory = negotiated(); contradictory.forced = {LinkSpeed::MBPS_100, Duplex::FULL};
  CHECK(!driver.startConfigure(contradictory, budget(), token).inProgress());
  CHECK(bus.calls == calls && bus.clocks == clocks);
  CHECK(driver.startConfigure(forced(), budget(0), token).is(Err::OPERATION_TIMEOUT));
  CHECK(bus.calls == calls && bus.clocks == clocks);
}

void transitions() {
  Controller bus; Driver driver;
  CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus);
  auto an = negotiated(); an.advertisement.symmetricPause = true; an.advertisement.asymmetricPause = true;
  auto token = start(driver, an); CHECK(drain(driver, bus, token).ok()); take(driver, token);
  const auto advertisement = bus.phys[1].words[4];
  for (const auto speed : {LinkSpeed::MBPS_10, LinkSpeed::MBPS_100}) {
    const auto first = bus.frames;
    token = start(driver, forced(speed, Duplex::HALF, PairSelection::MDI, true));
    CHECK(drain(driver, bus, token).ok());
    CHECK(!take(driver, token).negotiationComplete);
    CHECK(bus.phys[1].words[4] == advertisement);
    for (unsigned i = first; i < bus.frames; ++i) CHECK(bus.trace[i].reg != 4);
  }
  auto invalid = forced(); invalid.mode = LinkMode::AUTONEGOTIATED;
  const auto calls = bus.calls;
  CHECK(!driver.startConfigure(invalid, budget(), token).inProgress() && bus.calls == calls);
  bus.phys[1].words[10] = static_cast<uint16_t>(bus.phys[1].words[10] | 0x0020U);
  an.advertisement = {0x02, false, false};
  an.crossover.robust = false;
  bus.enforceForcedRobust = true;
  token = start(driver, an); CHECK(drain(driver, bus, token).ok()); take(driver, token);
  CHECK(bus.phys[1].words[4] == 0x0041 && bus.phys[1].words[0] == 0x1000);
  CHECK(bus.vodWrites == 1);
  CHECK((bus.phys[1].words[9] & 0x0020U) == 0); // Robust actually cleared after AN enable.
  CHECK((bus.phys[1].words[10] & 0x0020U) == 0); // Default extension cannot survive old opt-in.
  CHECK((bus.phys[1].words[11] & 0x0060U) == 0); // Default mirror restores both bits.
}

void failures() {
  for (unsigned profileIndex = 0; profileIndex < 3; ++profileIndex) {
    const bool an = profileIndex != 0;
    auto testedProfile = an ? negotiated() : forced(LinkSpeed::MBPS_10, Duplex::FULL, PairSelection::MDIX, true);
    if (profileIndex == 2) testedProfile.crossover.robust = false;
    const unsigned count = an ? 24U : 20U;
    for (unsigned offset = 0; offset < count; ++offset) {
      Controller bus; Driver driver;
      CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus); seed(bus);
      const auto first = bus.frames;
      const auto profile = testedProfile;
      const auto token = start(driver, profile);
      bus.failFrame = static_cast<int>(first + offset);
      CHECK(drain(driver, bus, token).is(Err::MDIO_TIMEOUT));
      const auto result = take(driver, token);
      CHECK(result.status.detail == -18018 && bus.frames == first + offset + 1U);
      CHECK(result.cleanupOwed == (offset >= (an ? 7U : 6U)));
      CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
      CHECK(driver.settingsSnapshot().desired.mode == profile.mode);
      CHECK(bus.locks == bus.releases);
      const auto calls = bus.calls;
      CHECK(driver.settingsSnapshot().observedStale && bus.calls == calls);
      bus.failFrame = -1;
      initialize(driver, bus);
      CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
    }
    // Every batch boundary, including before the first effect, is cancellable.
    const unsigned batches = an ? 18U : 15U;
    for (unsigned boundary = 0; boundary < batches; ++boundary) {
      Controller bus; Driver driver;
      CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus);
      const auto token = start(driver, testedProfile);
      for (unsigned i = 0; i < boundary; ++i) CHECK(driver.pollOperation(token).inProgress());
      const auto calls = bus.calls, clocks = bus.clocks;
      CHECK(driver.cancel(token).ok());
      CHECK(bus.calls == calls && bus.clocks == clocks);
      const auto result = take(driver, token);
      CHECK(result.status.is(Err::CANCELLED));
      CHECK(result.cleanupOwed == (boundary > (an ? 6U : 5U)));
      CHECK(driver.settingsSnapshot().desiredValid);
    }
  }
  // Each owned stable field has an independent verification failure vector.
  for (const uint8_t reg : {uint8_t{9}, uint8_t{25}, uint8_t{11}, uint8_t{10}, uint8_t{0}}) {
    Controller bus; Driver driver;
    CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus); seed(bus);
    bus.mismatchReg = reg;
    bus.mismatchXor = reg == 25 ? 0x8000 : reg == 0 ? 0x0100 : 0x0020;
    const auto token = start(driver, forced());
    CHECK(drain(driver, bus, token).is(Err::CONFIG_MISMATCH));
    const auto result = take(driver, token);
    CHECK(result.mismatch.valid && result.mismatch.registerAddress == reg && result.cleanupOwed);
    CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
  }
  // Complete readback is fresh observation even when it disagrees with intent.
  for (const uint8_t reg : {uint8_t{9}, uint8_t{10}, uint8_t{0}}) {
    Controller observedBus; Driver observedDriver;
    CHECK(observedDriver.bind(observedBus.config()).ok()); initialize(observedDriver, observedBus); seed(observedBus);
    observedBus.mismatchReg = reg;
    observedBus.mismatchXor = reg == 0 ? 0x2000 : 0x0020;
    const auto observedToken = start(observedDriver, forced());
    CHECK(drain(observedDriver, observedBus, observedToken).is(Err::CONFIG_MISMATCH));
    take(observedDriver, observedToken);
    const auto snapshot = observedDriver.settingsSnapshot();
    CHECK(snapshot.observedValid && !snapshot.observedStale && snapshot.timeValid);
    CHECK(snapshot.observedAtUs >= 100 && snapshot.trust == ConfigurationTrust::DIRTY);
    CHECK(snapshot.desired.crossover.robust && !snapshot.desired.extendedFullDuplex);
    CHECK(snapshot.desired.forced.speed == LinkSpeed::MBPS_100);
    if (reg == 9) CHECK(!snapshot.observed.crossover.robust);
    if (reg == 10) CHECK(snapshot.observed.extendedFullDuplex);
    if (reg == 0) CHECK(snapshot.observed.forced.speed == LinkSpeed::MBPS_10);
  }
  // An unexpected observed mode must be decoded from current words. Old ANAR
  // cannot complete an unexpected AN tuple during a forced-only acquisition.
  for (const bool requestedAn : {false, true}) {
    Controller modeBus; Driver modeDriver;
    CHECK(modeDriver.bind(modeBus.config()).ok()); initialize(modeDriver, modeBus);
    auto baseline = start(modeDriver, negotiated());
    CHECK(drain(modeDriver, modeBus, baseline).ok()); take(modeDriver, baseline);
    const auto prior = modeDriver.settingsSnapshot();
    modeBus.mismatchReg = 0; modeBus.mismatchXor = 0x1000;
    const auto changed = start(modeDriver, requestedAn ? negotiated() : forced());
    CHECK(drain(modeDriver, modeBus, changed).is(Err::CONFIG_MISMATCH)); take(modeDriver, changed);
    const auto snapshot = modeDriver.settingsSnapshot();
    CHECK(snapshot.trust == ConfigurationTrust::DIRTY);
    if (requestedAn) {
      CHECK(snapshot.observedValid && !snapshot.observedStale && snapshot.timeValid);
      CHECK(snapshot.observed.mode == LinkMode::FORCED);
      CHECK(snapshot.observed.forced.speed == LinkSpeed::MBPS_10 && snapshot.observed.forced.duplex == Duplex::HALF);
      CHECK(snapshot.observed.advertisement.technologyMask == 0);
    } else {
      CHECK(snapshot.observedValid == prior.observedValid && snapshot.observedStale);
      CHECK(snapshot.observed.mode == prior.observed.mode);
      CHECK(snapshot.observed.advertisement.technologyMask == prior.observed.advertisement.technologyMask);
      CHECK(snapshot.observedAtUs == prior.observedAtUs && snapshot.timeValid == prior.timeValid);
      CHECK((snapshot.rawBmcr & 0x1000U) != 0);
    }
  }
  // A non-IEEE selector cannot turn its technology bits into a new canonical
  // IEEE advertisement. Preserve the last known tuple, with explicit staleness.
  {
    Controller selectorBus; Driver selectorDriver;
    CHECK(selectorDriver.bind(selectorBus.config()).ok()); initialize(selectorDriver, selectorBus);
    auto baseline = start(selectorDriver, negotiated());
    CHECK(drain(selectorDriver, selectorBus, baseline).ok()); take(selectorDriver, baseline);
    const auto prior = selectorDriver.settingsSnapshot();
    auto changedProfile = negotiated(); changedProfile.advertisement.technologyMask = 0x08;
    selectorBus.mismatchReg = 4; selectorBus.mismatchXor = 0x0003;
    const auto changed = start(selectorDriver, changedProfile);
    CHECK(drain(selectorDriver, selectorBus, changed).is(Err::CONFIG_MISMATCH));
    const auto result = take(selectorDriver, changed);
    const auto snapshot = selectorDriver.settingsSnapshot();
    CHECK(result.mismatch.valid && result.mismatch.registerAddress == 4);
    CHECK(snapshot.rawAnar == 0x0102 && snapshot.anarValid);
    CHECK(snapshot.desired.advertisement.technologyMask == 0x08);
    CHECK(snapshot.observedValid && snapshot.observedStale && snapshot.trust == ConfigurationTrust::DIRTY);
    CHECK(snapshot.observed.advertisement.technologyMask == 0x0F);
    CHECK(snapshot.observedAtUs == prior.observedAtUs && snapshot.timeValid == prior.timeValid);
  }
  // Partial verification retains the old observed tuple and its historical
  // timestamp. The admitted new intent is separate and remains authoritative.
  {
    Controller partialBus; Driver partialDriver;
    CHECK(partialDriver.bind(partialBus.config()).ok()); initialize(partialDriver, partialBus);
    auto baseline = start(partialDriver, negotiated());
    CHECK(drain(partialDriver, partialBus, baseline).ok()); take(partialDriver, baseline);
    const auto prior = partialDriver.settingsSnapshot();
    const auto firstFrame = partialBus.frames;
    const auto changed = start(partialDriver, forced());
    partialBus.failFrame = static_cast<int>(firstFrame + 16U);
    CHECK(drain(partialDriver, partialBus, changed).is(Err::MDIO_TIMEOUT)); take(partialDriver, changed);
    const auto snapshot = partialDriver.settingsSnapshot();
    CHECK(snapshot.desired.mode == LinkMode::FORCED && snapshot.observed.mode == LinkMode::AUTONEGOTIATED);
    CHECK(snapshot.observed.advertisement.technologyMask == prior.observed.advertisement.technologyMask);
    CHECK(snapshot.observedStale && snapshot.observedAtUs == prior.observedAtUs && snapshot.timeValid == prior.timeValid);
    CHECK(snapshot.trust == ConfigurationTrust::DIRTY);
  }
  // An earlier CR1 mismatch survives a later read timeout with backend detail.
  Controller bus; Driver driver;
  CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus); seed(bus);
  const auto first = bus.frames;
  bus.mismatchReg = 9; bus.mismatchXor = 0x0020;
  const auto token = start(driver, forced());
  bus.failFrame = static_cast<int>(first + 16U); // PHYCR verify follows CR1 verify.
  CHECK(drain(driver, bus, token).is(Err::MDIO_TIMEOUT));
  const auto result = take(driver, token);
  CHECK(result.status.detail == -18018);
  CHECK(result.mismatch.registerAddress == 9 && result.failingBatch.normalized.is(Err::MDIO_TIMEOUT));
  CHECK(result.failingBatch.normalized.detail == -18018 && result.cleanupOwed);
}

void verificationOutcomes() {
  // A mismatch is retained data until the complete pass can freeze its result.
  // Check both direct configuration and the identical initialization replay.
  for (const bool restoring : {false, true}) {
    for (unsigned interruption = 0; interruption < 6; ++interruption) {
      Controller bus; Driver driver;
      CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus);
      if (restoring) {
        const auto initial = start(driver, forced());
        CHECK(drain(driver, bus, initial).ok()); take(driver, initial);
      }
      const auto first = bus.frames;
      // NONE initialization has twenty frames before its owned profile replay.
      const unsigned untilMismatch = (restoring ? 20U : 0U) + 16U;
      auto allowance = budget(interruption == 5U ? untilMismatch : 512U);
      OperationToken token;
      CHECK((restoring ? driver.startInitialize({ResetKind::NONE}, allowance, token) :
          driver.startConfigure(forced(), allowance, token)).inProgress());
      bus.mismatchReg = 9; bus.mismatchXor = 0x0020;
      for (unsigned poll = 0; poll < 32U && bus.frames < first + untilMismatch; ++poll) {
        CHECK(driver.pollOperation(token).inProgress());
      }
      CHECK(bus.frames == first + untilMismatch);
      const auto calls = bus.calls;
      Err expected = Err::MDIO_TIMEOUT;
      int32_t detail = -18018;
      if (interruption == 0U) bus.failFrame = static_cast<int>(bus.frames);
      if (interruption == 1U) {
        CHECK(driver.cancel(token).ok()); expected = Err::CANCELLED; detail = 0;
      }
      if (interruption == 2U) {
        --bus.now; expected = Err::INVALID_CONFIG; detail = static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT);
      }
      if (interruption == 3U) {
        bus.now = allowance.io.deadlineUs; expected = Err::OPERATION_TIMEOUT;
        detail = static_cast<int32_t>(TimeoutDetail::DEADLINE);
      }
      if (interruption == 4U) {
        CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION).ok());
        expected = Err::INVALID_CONFIG; detail = static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED);
      }
      if (interruption == 5U) {
        expected = Err::OPERATION_TIMEOUT; detail = static_cast<int32_t>(TimeoutDetail::FRAME_BUDGET);
      }
      CHECK(driver.pollOperation(token).is(expected));
      CHECK(bus.calls == calls + (interruption == 0U ? 1U : 0U));
      const auto result = take(driver, token);
      CHECK(result.status.is(expected) && result.status.detail == detail);
      CHECK(result.mismatch.valid && result.mismatch.registerAddress == 9);
      CHECK(result.mismatch.mask == 0x0020 && result.mismatch.expected == 0x0020);
      CHECK(result.cleanupOwed && !result.persistentRestored);
      CHECK(driver.settingsSnapshot().lastVerificationStatus.is(expected));
      CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
      CHECK(driver.settingsSnapshot().observedStale);
    }
  }
}

void replay() {
  for (const auto reset : {ResetKind::NONE, ResetKind::PCS, ResetKind::HARD, ResetKind::RESTART}) {
    for (const auto speed : {LinkSpeed::MBPS_10, LinkSpeed::MBPS_100}) {
      Controller bus; Driver driver;
      CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus);
      const auto profile = forced(speed, Duplex::FULL, PairSelection::MDIX, true, speed == LinkSpeed::MBPS_100);
      auto token = start(driver, profile); CHECK(drain(driver, bus, token).ok()); take(driver, token);
      auto& words = bus.phys[1].words;
      words[0] = 0x3100; words[9] = 0; words[25] = 0x8001; words[11] = 0; words[10] = 0;
      const auto first = bus.frames;
      initialize(driver, bus, reset);
      CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED && driver.isInitialized());
      CHECK((words[0] & 0x3100U) == forcedWord(profile));
      CHECK((words[9] & 0x0020U) != 0 && (words[25] & 0xC000U) == 0x4000);
      CHECK((words[11] & 0x0060U) == 0x0060);
      CHECK((words[10] & 0x0020U) == (profile.extendedFullDuplex ? 0x0020U : 0U));
      CHECK(bus.vodWrites == 2);
      bool vod = false;
      for (unsigned i = first; i < bus.frames; ++i) {
        const auto& wire = bus.trace[i];
        CHECK(wire.reg != 4); // Forced replay never uses advertisement.
        if (wire.kind == FrameKind::WRITE && wire.reg == 0x0E && wire.value == 0x4A40) vod = true;
        if (wire.kind == FrameKind::WRITE && wire.reg == 9) CHECK(vod);
      }
    }
  }
}

void ownership() {
  Controller bus; Driver driver, other;
  CHECK(driver.bind(bus.config()).ok() && other.bind(bus.config(2)).ok());
  initialize(driver, bus); initialize(other, bus);
  bus.competitor = &other;
  const auto first = bus.frames;
  const auto token = start(driver, forced());
  CHECK(drain(driver, bus, token).ok()); take(driver, token);
  CHECK(bus.exclusions == bus.frames - first);
  CHECK(bus.locks == bus.releases && !bus.locked);
  CHECK(other.settingsSnapshot().trust != ConfigurationTrust::VERIFIED);
  bus.competitor = nullptr;
  const auto otherToken = start(other, forced(LinkSpeed::MBPS_10, Duplex::HALF));
  CHECK(drain(other, bus, otherToken).ok()); take(other, otherToken);
  CHECK((bus.phys[1].words[0] & 0x3100U) == 0x2100 && (bus.phys[2].words[0] & 0x3100U) == 0);
}

void timing() {
  // A sampled AN restart bit is irrelevant to a forced stable comparison;
  // forcing does not claim a negotiated-command completion or wait for AN.
  {
    Controller bus; Driver driver;
    CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus); seed(bus);
    bus.mismatchReg = 0; bus.mismatchXor = 0x0200;
    const auto first = bus.frames;
    const auto token = start(driver, forced());
    CHECK(drain(driver, bus, token).ok());
    const auto result = take(driver, token);
    CHECK(!result.protocolComplete && !result.negotiationComplete);
    CHECK(bus.frames == first + 20U && !result.mismatch.valid);
  }
  // Save reads fit a one-frame slice; every RMW is indivisible at two frames.
  Controller bus; Driver driver;
  CHECK(driver.bind(bus.config()).ok()); initialize(driver, bus);
  auto token = start(driver, forced());
  for (unsigned i = 0; i < 5; ++i) CHECK(driver.pollOperation(token, 1).inProgress());
  const auto frames = bus.frames;
  CHECK(driver.pollOperation(token, 0).inProgress());
  CHECK(driver.pollOperation(token, 1).inProgress());
  CHECK(bus.frames == frames);
  CHECK(driver.pollOperation(token, 2).inProgress() && bus.frames == frames + 2);
  CHECK(drain(driver, bus, token).ok()); take(driver, token);
  // Callback accepted the first owned write but returns at the raw deadline.
  token = start(driver, forced(LinkSpeed::MBPS_10, Duplex::HALF));
  bus.lateBatch = static_cast<int>(bus.calls + 5U);
  CHECK(drain(driver, bus, token).is(Err::OPERATION_TIMEOUT));
  const auto result = take(driver, token);
  CHECK(result.late && result.mutationCompleted && result.cleanupOwed);
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
  // Exact deadline expiry before submission cannot emit another frame.
  Controller expiry; Driver expired;
  CHECK(expired.bind(expiry.config()).ok()); initialize(expired, expiry);
  OperationToken expToken;
  CHECK(expired.startConfigure(forced(), budget(512, expiry.now), expToken).inProgress());
  const auto calls = expiry.calls;
  CHECK(expired.pollOperation(expToken).is(Err::OPERATION_TIMEOUT));
  CHECK(expiry.calls == calls && !take(expired, expToken).cleanupOwed);
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"matrix", matrix}, {"admission", admission}, {"transitions", transitions},
    {"failures", failures}, {"verification_outcomes", verificationOutcomes},
    {"replay", replay}, {"ownership", ownership}, {"timing", timing}
  };
  return test::runSuites(argc, argv, suites);
}
