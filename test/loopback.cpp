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
  std::array<Wire, 4096> trace{};
  std::array<uint8_t, 1024> sizes{};
  uint16_t selector{0}, address{0}, vod{0x8400}, id2{0xA131};
  struct Target { uint16_t address, value; bool mmd; };
  std::array<Target, 2048> writes{};
  std::array<uint16_t, 3> opaque{};
  bool holdLinkDown{false}; unsigned phystsReads{0};
  uint64_t now{100}, frameTime{2};
  unsigned calls{0}, frames{0}, clocks{0}, releases{0}, semanticWrites{0}, vodWrites{0}, resets{0};
  int failFrame{-1}, releaseCall{-1}, lateCall{-1}, regressCall{-1}, overflowCall{-1};
  int corruptCall{-1}; uint8_t corruptRegister{0}; uint16_t corruptValue{0};
  bool locked{false}, holdRestart{false}, holdReset{false}, holdPhyrcr{false}, failAfterEffect{true};
  Driver* competitor{nullptr};
  unsigned exclusions{0};
  Controller() { words[0] = 0x3100; words[4] = 0x01E1; words[25] = 0x8001; words[1] = 0x0024; words[16] = 0x0005; words[22] = 0x0100; }
  Config config(uint32_t phy = 1, MacInterface interface = MacInterface::RMII) noexcept {
    Config c; c.variant = Variant::DP83826I; c.interface = interface;
    c.phyAddress = phy; c.transport = {this, execute, clock, {8, 4, 1}}; return c;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<Controller*>(context); ++bus.clocks; return bus.now;
  }
  uint16_t read(uint8_t reg) noexcept {
    if (reg == 16) { ++phystsReads; return holdLinkDown ? uint16_t{0} : words[16]; }
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
      CHECK(address != 0x0122 && address != 0x0123 && address != 0x0130); // Opaque recipe is not saveable.
    }
    return words[reg];
  }
  void write(uint8_t reg, uint16_t value) noexcept {
    if (reg == 13) { selector = value; return; }
    if (reg == 14 && selector == 0x001F) { address = value; return; }
    // Table 8-40: line transmission is legal only while BMCR14 is set.
    if (reg == 22 && (value & 0x0040U) != 0U) CHECK((words[0] & 0x4000U) != 0U);
    if (reg == 0 && (value & 0xC000U) == 0U) CHECK((words[22] & 0x0040U) == 0U);
    CHECK(semanticWrites < writes.size());
    writes[semanticWrites++] = {reg == 14 ? address : uint16_t{reg}, value, reg == 14};
    if (reg == 14 && selector == 0x401F) {
      if (address == 0x030E) { vod = value; ++vodWrites; }
      else if (address == 0x0122) opaque[0] = value;
      else if (address == 0x0123) opaque[1] = value;
      else if (address == 0x0130) opaque[2] = value;
      else if (address < 32U) words[address] = value;
      else CHECK(false);
    } else {
      words[reg] = reg == 22 ? static_cast<uint16_t>((words[reg] & 0x8FA0U) | (value & 0x705FU)) : value;
      if (reg == 0 && (value & 0x8000U) != 0U) {
        ++resets; words[4] = 0x01E1; words[9] = words[10] = words[11] = 0; words[25] = 0x8001;
      }
      if (reg == 31) {
        ++resets;
        if (value == 0x8000) {
          // Model an eligible strap reload selecting isolation. Owning exit
          // must restore its saved ordinary BMCR fields after HARD replay.
          words.fill(0); words[0] = 0x3500; words[4] = 0x01E1; words[25] = 0x8001; words[1] = 0x0024; words[16] = 0x0005; words[22] = 0x0100; opaque.fill(0); vod = 0x8400;
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
        else {
          receipt.value = bus.read(request.registerAddress);
          if (static_cast<int>(call)==bus.corruptCall && request.registerAddress==bus.corruptRegister) receipt.value=bus.corruptValue;
        }
      }
      bus.now += bus.frameTime;
      if (bus.competitor != nullptr) {
        uint16_t sentinel = 0xBEEF;
        CHECK(bus.competitor->readMmd(31, 0x0467, sentinel, budget).is(Err::BUSY));
        CHECK(sentinel == 0xBEEF); ++bus.exclusions;
      }
      if (fail) {
        receipt.effectPossible = bus.failAfterEffect;
        result.status = {Err::MDIO_TIMEOUT, -22022, "Injected loopback failure"}; break;
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
  for (unsigned i = 0; i < 256 && status.inProgress(); ++i) {
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
  s.crossover.robust = forced;
  s.crossover.pairSelection = PairSelection::MDIX;
  s.crossover.swapPair = true; s.crossover.swapPolarity = true;
  return s;
}
void configure(Driver& driver, Controller& bus, const LinkSettings& s) {
  OperationToken token; CHECK(driver.startConfigure(s, allowance(), token).inProgress());
  CHECK(drain(driver, bus, token).ok()); CHECK(take(driver, token).status.ok());
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
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

LoopbackOptions options(LoopbackMode mode, LinkSpeed speed = LinkSpeed::MBPS_100) {
  LoopbackOptions o; o.mode = mode; o.speed = speed; o.maintenanceAdmitted = true;
  o.termination100Ohm = mode == LoopbackMode::ANALOG;
  o.partnerReady = mode == LoopbackMode::REVERSE;
  o.externalFixture = mode == LoopbackMode::EXTERNAL;
  o.allowHardResetExit = mode == LoopbackMode::DIGITAL;
  o.reverseFifoDepth = mode == LoopbackMode::REVERSE ? 1 : 0;
  return o;
}
void ready(Driver& driver, Controller& bus, MacInterface interface = MacInterface::RMII) {
  CHECK(driver.bind(bus.config(1, interface)).ok());
  OperationToken token; CHECK(driver.startInitialize({ResetKind::NONE}, allowance(), token).inProgress());
  CHECK(drain(driver, bus, token).ok()); CHECK(take(driver, token).status.ok());
  configure(driver, bus, profile());
  bus.words[17] = 0x0B55; // Saved FIFO=3, with unrelated interrupt/clock bits.
  bus.words[22] = 0x4100; // Counter-wrap policy set; PRBS disabled, normal power.
  bus.words[9] |= 0x0010; bus.words[10] |= 0x0002; bus.words[11] |= 0x0100;
}
OperationToken begin(Driver& driver, Controller& bus, const LoopbackOptions& o) {
  const auto calls = bus.calls, clocks = bus.clocks;
  OperationToken token{999}; CHECK(driver.startLoopback(o, allowance(), token).inProgress());
  CHECK(token.value != 999 && bus.calls == calls && bus.clocks == clocks);
  CHECK(driver.operationSnapshot().kind == OperationKind::LOOPBACK_START);
  return token;
}
OperationToken stop(Driver& driver, Controller& bus) {
  const auto calls = bus.calls, clocks = bus.clocks;
  OperationToken token{999}; CHECK(driver.stopLoopback(allowance(), token).inProgress());
  CHECK(token.value != 999 && bus.calls == calls && bus.clocks == clocks);
  CHECK(driver.operationSnapshot().kind == OperationKind::LOOPBACK_STOP);
  return token;
}
void sameHealth(const HealthSnapshot& a, const HealthSnapshot& b) {
  CHECK(a.totalSuccess == b.totalSuccess && a.totalFailures == b.totalFailures);
  CHECK(a.consecutiveFailures == b.consecutiveFailures);
  CHECK(a.lastError.code == b.lastError.code && a.lastError.detail == b.lastError.detail);
  CHECK(a.lastOkUs == b.lastOkUs && a.lastErrorUs == b.lastErrorUs);
  CHECK(a.lastOkTimeValid == b.lastOkTimeValid && a.lastErrorTimeValid == b.lastErrorTimeValid);
}
void restored(Driver& driver, Controller& bus, const SettingsSnapshot& desired) {
  CHECK(!driver.operationSnapshot().cleanupOwed && !driver.operationSnapshot().temporaryMayBeActive);
  CHECK(!driver.loopbackSnapshot().temporaryMayBeActive);
  CHECK(driver.isInitialized() && driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
  CHECK((bus.words[0] & 0x7100U) == 0x1000U);
  CHECK((bus.words[22] & 0x705FU) == 0x4000U);
  CHECK((bus.words[17] & 0x0300U) == 0x0300U);
  CHECK((bus.words[25] & 0xC000U) == 0x4000U); // Manual MDIX intent restored.
  CHECK((bus.words[9] & 0x0020U) == 0U); // AN profile's robust opt-out restored.
  CHECK((bus.words[11] & 0x0060U) == 0x0060U); // Coupled mirror profile restored.
  unchangedIntent(desired, driver.settingsSnapshot());
}

void matrix() {
  struct Mode { LoopbackMode mode; uint16_t selector; bool only10, only100, restart; unsigned frames, calls; };
  // Four preflight reads + two clear RMWs = 8 frames/6 callbacks.
  // Internal paths add 5 preflight reads, 5 RMWs and 5 verifies =20/15.
  // Add literal activation commands/companions and one completion observation.
  const Mode modes[] = {
    {LoopbackMode::MII, 0, false, false, false,34,25},
    {LoopbackMode::PCS_INPUT, 0x01, true, false, false,32,24},
    {LoopbackMode::PCS_OUTPUT, 0x02, false, false, false,32,24},
    {LoopbackMode::DIGITAL, 0x04, false, true, true,46,30},
    {LoopbackMode::ANALOG, 0x08, false, false, true,34,26},
    {LoopbackMode::REVERSE, 0x10, false, false, true,19,15},
    {LoopbackMode::EXTERNAL, 0, false, false, false,12,9}
  };
  for (MacInterface interface : {MacInterface::MII, MacInterface::RMII}) for (const auto& mode : modes) {
    for (LinkSpeed speed : {LinkSpeed::MBPS_10, LinkSpeed::MBPS_100}) {
      if ((mode.only10 && speed != LinkSpeed::MBPS_10) || (mode.only100 && speed != LinkSpeed::MBPS_100)) continue;
      Controller bus; Driver driver; ready(driver, bus, interface);
      bus.words[16] = speed == LinkSpeed::MBPS_10 ? 0x0007 : 0x0005;
      const auto intent = driver.settingsSnapshot(); const auto health = driver.healthSnapshot();
      const auto writes = bus.semanticWrites, vod = bus.vodWrites, reset = bus.resets;
      const auto startFrames=bus.frames,startCalls=bus.calls;
      auto o = options(mode.mode, speed);
      o.transmitOnLine = mode.mode == LoopbackMode::MII && speed == LinkSpeed::MBPS_100;
      const auto token = begin(driver, bus, o);
      CHECK(drain(driver, bus, token).ok());
      const auto result = take(driver, token);
      CHECK(bus.frames-startFrames==mode.frames && bus.calls-startCalls==mode.calls);
      // Literal owned-write plan from BMCR/BISCR/PHYCR tables, with our explicit
      // unowned starting bits. Selector wire frames are checked separately by
      // complete batches; none are treated as target writes in this plan.
      std::array<Controller::Target,16> expected{};
      unsigned used=0;
      const auto target=[&](uint16_t address,uint16_t value) { expected[used++]={address,value,address>31U}; };
      target(0,0x1000); target(0x16,0x4100);
      const bool internal=mode.mode!=LoopbackMode::REVERSE && mode.mode!=LoopbackMode::EXTERNAL;
      const uint16_t forced=speed==LinkSpeed::MBPS_100?0x2100:0x0100;
      if(internal) {
        target(9,0x0030); target(25,0x0001); target(11,0x0100); target(10,0x0002); target(0,forced);
      }
      if(mode.mode==LoopbackMode::DIGITAL) {
        target(0,0x2100); target(0x16,0x0104); target(0x0122,0x2000);
        target(0x0123,0x2000); target(0x0130,0x47FF); target(0x1F,0x4000);
      } else {
        if(mode.mode==LoopbackMode::REVERSE) target(0x11,0x0955);
        const uint16_t bits=mode.mode==LoopbackMode::MII?(speed==LinkSpeed::MBPS_100?0x0044:0x0001):mode.selector;
        if(mode.mode==LoopbackMode::MII) target(0,static_cast<uint16_t>(forced|0x4000U));
        target(0x16,static_cast<uint16_t>(0x4100U|bits));
        if(mode.restart) target(0x1F,0x4000);
      }
      CHECK(bus.semanticWrites-writes==used);
      for(unsigned i=0;i<used;++i) {
        CHECK(bus.writes[writes+i].address==expected[i].address);
        CHECK(bus.writes[writes+i].value==expected[i].value && bus.writes[writes+i].mmd==expected[i].mmd);
      }
      CHECK(result.kind == OperationKind::LOOPBACK_START && result.loopback.controlsVerified);
      CHECK(result.cleanupOwed && result.temporaryMayBeActive && !result.persistentRestored);
      CHECK(driver.operationSnapshot().state == OperationState::IDLE);
      CHECK(driver.operationSnapshot().temporaryMayBeActive && driver.loopbackSnapshot().controlsVerified);
      CHECK(driver.loopbackSnapshot().token.value == token.value);
      const uint16_t selection = mode.mode == LoopbackMode::MII ? (speed == LinkSpeed::MBPS_100 ? 4 : 1) : mode.selector;
      CHECK((bus.words[22] & 0x005FU) == (selection | (o.transmitOnLine ? 0x0040U : 0U)));
      CHECK((bus.words[0] & 0x4000U) == (mode.mode == LoopbackMode::MII ? 0x4000U : 0U));
      if (mode.mode != LoopbackMode::REVERSE && mode.mode != LoopbackMode::EXTERNAL) {
        CHECK((bus.words[0] & 0x3100U) == (speed == LinkSpeed::MBPS_100 ? 0x2100U : 0x0100U));
        CHECK((bus.words[25] & 0xC000U) == 0U && (bus.words[9] & 0x0020U) == 0x0020U);
      }
      CHECK(bus.vodWrites == vod && bus.resets == reset + (mode.restart ? 1U : 0U));
      for (unsigned i = writes; i < bus.semanticWrites; ++i) CHECK(bus.writes[i].address != 0x030B);
      if (mode.mode == LoopbackMode::DIGITAL) {
        const uint16_t address[] = {0x0000,0x0016,0x0122,0x0123,0x0130,0x001F};
        const uint16_t value[] = {0x2100,0x0104,0x2000,0x2000,0x47FF,0x4000};
        bool found = false;
        for (unsigned first = writes; first + 6U <= bus.semanticWrites; ++first) {
          bool matches = true;
          for (unsigned j = 0; j < 6; ++j) matches = matches && bus.writes[first+j].address == address[j] && bus.writes[first+j].value == value[j];
          found = found || matches;
        }
        CHECK(found && result.loopback.hardResetExit);
      }
      if (mode.mode == LoopbackMode::REVERSE) {
        CHECK((bus.words[17] & 0x0300U) == 0x0100U);
        CHECK(result.loopback.phystsValid && (result.loopback.rawPhysts & 1U) != 0U);
      }
      unchangedIntent(intent, driver.settingsSnapshot()); sameHealth(health, driver.healthSnapshot());
      const auto calls = bus.calls, clocks = bus.clocks;
      const auto copy1 = driver.loopbackSnapshot(), copy2 = driver.loopbackSnapshot();
      CHECK(copy1.controlsVerified && copy2.controlsVerified && bus.calls == calls && bus.clocks == clocks);
      const auto stopFrames=bus.frames,stopCalls=bus.calls;
      const auto stopWrites=bus.semanticWrites;
      const auto stopToken = stop(driver, bus); CHECK(drain(driver, bus, stopToken).ok());
      const auto stopped = take(driver, stopToken);
      if (o.transmitOnLine) {
        CHECK(bus.writes[stopWrites].address == 0x16 && bus.writes[stopWrites].value == 0x4100);
        CHECK(bus.writes[stopWrites+1].address == 0 && bus.writes[stopWrites+1].value == 0x2100);
      }
      // Stop identity+two RMWs=6/3, initialization NONE20/7 or HARD32/12,
      // AN replay24/18, two saved-extra RMWs + three checks=7/5. HARD also
      // restores BMCR ordinary extras in one2-frame RMW after strap reload.
      CHECK(bus.frames-stopFrames==(mode.mode==LoopbackMode::DIGITAL?71U:57U));
      CHECK(bus.calls-stopCalls==(mode.mode==LoopbackMode::DIGITAL?39U:33U));
      CHECK((bus.words[0]&0x0480U)==0U);
      CHECK(stopped.loopback.stopped && stopped.persistentRestored && !stopped.temporaryMayBeActive);
      CHECK(token.value != stopToken.value && result.cleanupOwed && result.temporaryMayBeActive);
      restored(driver, bus, intent); sameHealth(health, driver.healthSnapshot());
      if (mode.mode != LoopbackMode::DIGITAL) {
        CHECK((bus.words[17] & 0xFCFFU) == 0x0855U);
        CHECK((bus.words[9] & 0x0010U) != 0U && (bus.words[10] & 0x0002U) != 0U && (bus.words[11] & 0x0100U) != 0U);
      }
      CHECK(driver.end().ok());
    }
  }
  for (uint8_t fifo=0;fifo<4;++fifo) {
    Controller bus; Driver driver; ready(driver,bus);
    auto o=options(LoopbackMode::REVERSE); o.reverseFifoDepth=fifo;
    const auto t=begin(driver,bus,o); CHECK(drain(driver,bus,t).ok()); CHECK(take(driver,t).status.ok());
    CHECK((bus.words[17]&0x0300U)==static_cast<uint16_t>(uint16_t{fifo}<<8U));
    const auto cleanup=stop(driver,bus); CHECK(drain(driver,bus,cleanup).ok()); CHECK(take(driver,cleanup).status.ok());
    CHECK((bus.words[17]&0x0300U)==0x0300U);
  }
}

void admission() {
  Controller bus; Driver driver; OperationToken token{444};
  CHECK(!driver.startLoopback(options(LoopbackMode::MII), allowance(), token).inProgress());
  CHECK(token.value == 444 && bus.calls == 0 && bus.clocks == 0);
  ready(driver, bus);
  const auto calls = bus.calls, clocks = bus.clocks;
  const auto old = driver.settingsSnapshot();
  auto base = options(LoopbackMode::MII);
  const auto reject = [&](const LoopbackOptions& o, OperationBudget budget = allowance()) {
    CHECK(!driver.startLoopback(o, budget, token).inProgress());
    CHECK(token.value == 444 && bus.calls == calls && bus.clocks == clocks);
    unchangedIntent(old, driver.settingsSnapshot());
  };
  auto o = base; o.maintenanceAdmitted = false; reject(o);
  o = base; o.mode = static_cast<LoopbackMode>(255); reject(o);
  o = base; o.speed = LinkSpeed::UNKNOWN; reject(o);
  o = options(LoopbackMode::PCS_INPUT, LinkSpeed::MBPS_100); reject(o);
  o = options(LoopbackMode::DIGITAL, LinkSpeed::MBPS_10); reject(o);
  o = options(LoopbackMode::DIGITAL); o.allowHardResetExit = false; reject(o);
  o = options(LoopbackMode::ANALOG); o.termination100Ohm = false; reject(o);
  o = options(LoopbackMode::REVERSE); o.partnerReady = false; reject(o);
  o = options(LoopbackMode::REVERSE); o.reverseFifoDepth = 4; reject(o);
  o = options(LoopbackMode::EXTERNAL); o.externalFixture = false; reject(o);
  for (auto mode : {LoopbackMode::MII, LoopbackMode::PCS_INPUT, LoopbackMode::PCS_OUTPUT, LoopbackMode::ANALOG, LoopbackMode::REVERSE, LoopbackMode::EXTERNAL}) {
    o = options(mode, LinkSpeed::MBPS_10); o.transmitOnLine = true; reject(o);
  }
  reject(base, allowance(0)); reject(base, allowance(100,0));
  auto budget = allowance(); budget.pollIntervalUs = 0; reject(base,budget);
  budget = allowance(); budget.maxStalledPolls = 0; reject(base,budget);
  CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION).ok());
  reject(base);
  for (unsigned prior = 0; prior < 10; ++prior) {
    Controller b; Driver d; ready(d,b);
    if (prior < 5) b.words[0] |= static_cast<uint16_t>(prior == 0 ? 0x8000 : prior == 1 ? 0x4000 : prior == 2 ? 0x0800 : prior == 3 ? 0x0400 : 0x0080);
    else if (prior < 8) b.words[22] |= static_cast<uint16_t>(prior == 5 ? 1 : prior == 6 ? 0x1000 : 0x0040);
    else { b.words[31] = prior == 8 ? 0x8000 : 0x4000; b.holdPhyrcr = true; }
    b.holdReset = true; const auto mutations = b.semanticWrites;
    const auto t = begin(d,b,base); CHECK(!drain(d,b,t).ok()); CHECK(!take(d,t).mutationAttempted);
    CHECK(b.semanticWrites == mutations && !d.operationSnapshot().temporaryMayBeActive);
  }
  for (uint16_t link : {uint16_t{0x0000},uint16_t{0x0007}}) {
    Controller b; Driver d; ready(d,b); b.words[16]=link;
    const auto mutations=b.semanticWrites;
    const auto t=begin(d,b,options(LoopbackMode::REVERSE)); CHECK(!drain(d,b,t).ok());
    CHECK(!take(d,t).mutationAttempted && b.semanticWrites==mutations);
  }
}

constexpr LoopbackMode TEST_MODES[] = {LoopbackMode::MII, LoopbackMode::PCS_INPUT,
    LoopbackMode::PCS_OUTPUT, LoopbackMode::DIGITAL, LoopbackMode::ANALOG,
    LoopbackMode::REVERSE, LoopbackMode::EXTERNAL};
LoopbackOptions representative(LoopbackMode mode) {
  auto result = options(mode, mode == LoopbackMode::PCS_INPUT ? LinkSpeed::MBPS_10 : LinkSpeed::MBPS_100);
  result.transmitOnLine = mode == LoopbackMode::MII;
  return result;
}
void activate(Driver& driver, Controller& bus, LoopbackMode mode) {
  const auto token = begin(driver,bus,representative(mode));
  CHECK(drain(driver,bus,token).ok()); CHECK(take(driver,token).loopback.controlsVerified);
}
Status pollOne(Driver& driver, Controller& bus, OperationToken token, uint32_t slice = MAX_BATCH_FRAMES) {
  const auto state = driver.operationSnapshot();
  if (state.nextPollValid && bus.now < state.nextPollAtUs) bus.now = state.nextPollAtUs;
  if (state.busQuietValid && bus.now < state.busQuietUntilUs) bus.now = state.busQuietUntilUs;
  const auto calls = bus.calls;
  const auto result = driver.pollOperation(token,slice);
  CHECK(bus.calls <= calls+1 && !bus.locked); return result;
}

void startFailures() {
  for (auto mode : TEST_MODES) {
    Controller reference; Driver baseline; ready(baseline,reference);
    const auto startFrame = reference.frames;
    const auto startCall = reference.calls;
    activate(baseline,reference,mode);
    const auto count = reference.frames-startFrame;
    CHECK(count > 0 && count < 128);
    for (unsigned phase = 0; phase < count; ++phase) {
      Controller bus; Driver driver; ready(driver,bus);
      const auto intent=driver.settingsSnapshot();
      const auto health=driver.healthSnapshot(); const auto frames=bus.frames;
      bus.failFrame=static_cast<int>(frames+phase);
      const auto token=begin(driver,bus,representative(mode));
      CHECK(drain(driver,bus,token).is(Err::MDIO_TIMEOUT));
      const auto result=take(driver,token);
      CHECK(result.status.detail==-22022 && bus.frames==frames+phase+1);
      CHECK(!result.persistentRestored && !result.loopback.controlsVerified);
      sameHealth(health,driver.healthSnapshot()); unchangedIntent(intent,driver.settingsSnapshot());
      const auto active=driver.operationSnapshot().temporaryMayBeActive;
      CHECK(result.temporaryMayBeActive==active && result.cleanupOwed==active);
      if (active) {
        CHECK(driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
        bus.failFrame=-1;
        const auto cleanup=stop(driver,bus); CHECK(drain(driver,bus,cleanup).ok());
        CHECK(take(driver,cleanup).loopback.stopped); restored(driver,bus,intent);
        CHECK(result.status.detail==-22022 && result.cleanupOwed);
      }
    }
    for (unsigned phase=0;phase<reference.calls-startCall;++phase) {
      Controller bus; Driver driver; ready(driver,bus);
      const auto intent=driver.settingsSnapshot(); const auto health=driver.healthSnapshot();
      bus.releaseCall=static_cast<int>(bus.calls+phase);
      const auto token=begin(driver,bus,representative(mode)); CHECK(drain(driver,bus,token).is(Err::MDIO_ERROR));
      const auto failed=take(driver,token); CHECK(failed.status.detail==-37 && failed.failingBatch.release.detail==-37);
      sameHealth(health,driver.healthSnapshot());
      if (failed.temporaryMayBeActive) {
        bus.releaseCall=-1; // The application repaired ownership before retry.
        const auto cleanup=stop(driver,bus); CHECK(drain(driver,bus,cleanup).ok());
        CHECK(take(driver,cleanup).loopback.stopped); restored(driver,bus,intent);
      }
    }
  }
}

void stopFailures() {
  for (auto mode : TEST_MODES) {
    Controller reference; Driver baseline; ready(baseline,reference); activate(baseline,reference,mode);
    const auto first=reference.frames;
    const auto firstCall=reference.calls;
    const auto stopToken=stop(baseline,reference); CHECK(drain(baseline,reference,stopToken).ok());
    CHECK(take(baseline,stopToken).loopback.stopped);
    const auto count=reference.frames-first; CHECK(count>0 && count<160);
    for (unsigned phase=0;phase<count;++phase) {
      Controller bus; Driver driver; ready(driver,bus); activate(driver,bus,mode);
      const auto intent=driver.settingsSnapshot(); const auto health=driver.healthSnapshot();
      const auto frames=bus.frames;
      bus.failFrame=static_cast<int>(frames+phase);
      const auto token=stop(driver,bus); CHECK(drain(driver,bus,token).is(Err::MDIO_TIMEOUT));
      const auto failed=take(driver,token);
      CHECK(failed.status.detail==-22022 && bus.frames==frames+phase+1);
      CHECK(failed.cleanupOwed && failed.temporaryMayBeActive && !failed.persistentRestored);
      CHECK(driver.operationSnapshot().temporaryMayBeActive && driver.operationSnapshot().cleanupOwed);
      CHECK(driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
      sameHealth(health,driver.healthSnapshot()); unchangedIntent(intent,driver.settingsSnapshot());
      bus.failFrame=-1;
      const auto retry=stop(driver,bus); CHECK(retry.value!=token.value && drain(driver,bus,retry).ok());
      CHECK(take(driver,retry).loopback.stopped); restored(driver,bus,intent);
      CHECK(failed.status.detail==-22022 && failed.cleanupOwed);
    }
    for (unsigned phase=0;phase<reference.calls-firstCall;++phase) {
      Controller bus; Driver driver; ready(driver,bus); activate(driver,bus,mode);
      const auto intent=driver.settingsSnapshot(); const auto health=driver.healthSnapshot();
      bus.releaseCall=static_cast<int>(bus.calls+phase);
      const auto token=stop(driver,bus); CHECK(drain(driver,bus,token).is(Err::MDIO_ERROR));
      const auto failed=take(driver,token);
      CHECK(failed.status.detail==-37 && failed.cleanupOwed && failed.temporaryMayBeActive && !failed.persistentRestored);
      CHECK(driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
      sameHealth(health,driver.healthSnapshot());
      bus.releaseCall=-1;
      const auto retry=stop(driver,bus); CHECK(drain(driver,bus,retry).ok());
      CHECK(take(driver,retry).loopback.stopped); restored(driver,bus,intent);
    }
  }
  // Stable mismatch survives a failing release/late return from that same read.
  for (bool fifo : {false,true}) for (bool late : {false,true}) {
    Controller bus; Driver driver; ready(driver,bus); activate(driver,bus,LoopbackMode::DIGITAL);
    const auto token=stop(driver,bus);
    bus.corruptCall=static_cast<int>(bus.calls+(fifo?38U:37U));
    bus.corruptRegister=fifo?17:22; bus.corruptValue=fifo?0x0155:0x0100;
    if (late) bus.lateCall=bus.corruptCall; else bus.releaseCall=bus.corruptCall;
    CHECK(drain(driver,bus,token).is(late?Err::OPERATION_TIMEOUT:Err::MDIO_ERROR));
    const auto failed=take(driver,token);
    CHECK(failed.mismatch.valid && failed.mismatch.registerAddress==(fifo?17:22));
    CHECK(failed.mismatch.mask==(fifo?0x0300:0x705F) && failed.mismatch.expected==(fifo?0x0300:0x4000));
    CHECK(failed.temporaryMayBeActive && failed.cleanupOwed && !failed.persistentRestored && !driver.isInitialized());
    CHECK(driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
}

void cancellation() {
  for (auto mode : TEST_MODES) for (bool stopping : {false,true}) {
    Controller reference; Driver baseline; ready(baseline,reference);
    if (stopping) activate(baseline,reference,mode);
    const auto first=reference.calls;
    const auto referenceToken=stopping?stop(baseline,reference):begin(baseline,reference,representative(mode));
    CHECK(drain(baseline,reference,referenceToken).ok()); CHECK(take(baseline,referenceToken).status.ok());
    const auto count=reference.calls-first;
    for (unsigned boundary=0;boundary<count;++boundary) {
      Controller bus; Driver driver; ready(driver,bus);
      if (stopping) activate(driver,bus,mode);
      const auto intent=driver.settingsSnapshot();
      const auto token=stopping?stop(driver,bus):begin(driver,bus,representative(mode));
      for (unsigned i=0;i<boundary;++i) CHECK(pollOne(driver,bus,token).inProgress());
      const auto calls=bus.calls,clocks=bus.clocks;
      const bool active=driver.operationSnapshot().temporaryMayBeActive;
      CHECK(driver.cancel(token).ok());
      CHECK(bus.calls==calls && bus.clocks==clocks);
      const auto cancelled=take(driver,token);
      CHECK(cancelled.status.is(Err::CANCELLED));
      CHECK(cancelled.temporaryMayBeActive==active && cancelled.cleanupOwed==active);
      if (active) {
        const auto cleanup=stop(driver,bus); CHECK(drain(driver,bus,cleanup).ok());
        CHECK(take(driver,cleanup).loopback.stopped); restored(driver,bus,intent);
      }
    }
  }
}

void barriers() {
  Controller bus; Driver driver; ready(driver,bus);
  OperationToken first{0};
  for (unsigned epoch=0;epoch<2;++epoch) {
    const auto token=begin(driver,bus,options(LoopbackMode::MII));
    if (epoch==0) first=token;
    else CHECK(driver.pollOperation(first).is(Err::TOKEN_MISMATCH));
    OperationToken sentinel{777};
    CHECK(driver.stopLoopback(allowance(),sentinel).is(Err::BUSY));
    CHECK(drain(driver,bus,token).ok());
    CHECK(driver.stopLoopback(allowance(),sentinel).is(Err::BUSY) && sentinel.value==777);
    const auto started=take(driver,token); CHECK(started.temporaryMayBeActive);
    const auto calls=bus.calls,clocks=bus.clocks;
    CHECK(driver.startLoopback(options(LoopbackMode::MII),allowance(),sentinel).is(Err::BUSY));
    CHECK(driver.startConfigure(profile(),allowance(),sentinel).is(Err::BUSY));
    CHECK(driver.startVerify(allowance(),sentinel).is(Err::BUSY));
    for (ResetKind kind:{ResetKind::NONE,ResetKind::PCS,ResetKind::RESTART}) CHECK(driver.startInitialize({kind},allowance(),sentinel).is(Err::BUSY));
    CHECK(driver.end().is(Err::BUSY) && driver.bind(bus.config()).is(Err::BUSY));
    CHECK(driver.writeClause22(0x18,0x12,io()).is(Err::BUSY));
    CHECK(bus.calls==calls && bus.clocks==clocks);
    uint16_t raw=0; CHECK(driver.readClause22(2,raw,io()).ok() && raw==0x2000);
    EvidenceSnapshot evidence; CHECK(driver.acquireEvidence(evidenceBit(EvidenceSource::PHYSTS),evidence,io()).ok());
    bus.words[1]=0x0004; // Coherent link-up, AN-not-complete bookends with PHYSTS0005.
    LinkSnapshot link; CHECK(driver.readLinkStatus(link,io()).ok());
    CHECK(link.temporaryMayBeActive && !link.resolvedModeValid && !link.pauseValid);
    const auto stopToken=stop(driver,bus); CHECK(drain(driver,bus,stopToken).ok());
    CHECK(take(driver,stopToken).loopback.stopped);
    OperationResult sentinelResult; sentinelResult.status={Err::MDIO_ERROR,-77,"Sentinel"};
    CHECK(driver.takeResult(stopToken,sentinelResult).is(Err::RESULT_NOT_AVAILABLE));
    CHECK(sentinelResult.status.detail==-77 && started.temporaryMayBeActive);
  }
  activate(driver,bus,LoopbackMode::DIGITAL);
  CHECK(driver.abandonCleanup().ok());
  CHECK(driver.operationSnapshot().temporaryMayBeActive && !driver.operationSnapshot().cleanupOwed);
  OperationToken token;
  CHECK(driver.startInitialize({ResetKind::PCS},allowance(),token).is(Err::BUSY));
  CHECK(driver.startInitialize({ResetKind::HARD},allowance(),token).inProgress());
  CHECK(drain(driver,bus,token).ok()); CHECK(take(driver,token).persistentRestored);
  CHECK(!driver.operationSnapshot().temporaryMayBeActive && !driver.operationSnapshot().cleanupOwed);
  activate(driver,bus,LoopbackMode::ANALOG);
  const auto calls=bus.calls,clocks=bus.clocks;
  CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
  CHECK(bus.calls==calls && bus.clocks==clocks && !driver.operationSnapshot().temporaryMayBeActive);
  CHECK(!driver.isInitialized() && !driver.operationSnapshot().cleanupOwed);
  // Even an attempted first write reported effect-free may leave a session
  // active under OP-9. A fresh bounded stop remains the only ordinary exit.
  Controller ref; Driver baseline; ready(baseline,ref);
  const auto firstFrame=ref.frames; activate(baseline,ref,LoopbackMode::MII);
  unsigned target=firstFrame;
  while (target<ref.frames && ref.trace[target].kind!=FrameKind::WRITE) ++target;
  CHECK(target<ref.frames && ref.trace[target].reg==0);
  Controller attempted; Driver subject; ready(subject,attempted);
  const auto old=subject.settingsSnapshot();
  attempted.failFrame=static_cast<int>(attempted.frames+target-firstFrame); attempted.failAfterEffect=false;
  const auto failedToken=begin(subject,attempted,options(LoopbackMode::MII));
  CHECK(drain(subject,attempted,failedToken).is(Err::MDIO_TIMEOUT));
  const auto failed=take(subject,failedToken);
  CHECK(failed.mutationAttempted && !failed.mutationPossible && failed.cleanupOwed && failed.temporaryMayBeActive);
  attempted.failFrame=-1;
  const auto cleanup=stop(subject,attempted); CHECK(drain(subject,attempted,cleanup).ok());
  CHECK(take(subject,cleanup).loopback.stopped); restored(subject,attempted,old);
}

void timing() {
  Controller bus; Driver driver; ready(driver,bus);
  const auto token=begin(driver,bus,options(LoopbackMode::DIGITAL));
  const auto calls=bus.calls;
  CHECK(driver.pollOperation(token,0).inProgress() && bus.calls==calls);
  bool wholeMmd=false;
  for (unsigned n=0;n<100 && driver.operationSnapshot().state==OperationState::RUNNING;++n) {
    const auto before=bus.calls;
    const auto status=pollOne(driver,bus,token,3);
    if (status.inProgress() && bus.calls==before) {
      CHECK(pollOne(driver,bus,token,4).inProgress()); wholeMmd=true;
    }
  }
  CHECK(wholeMmd && take(driver,token).status.ok());
  const auto cleanup=stop(driver,bus); CHECK(drain(driver,bus,cleanup).ok()); CHECK(take(driver,cleanup).status.ok());
  for (unsigned fault=0;fault<4;++fault) {
    Controller b; Driver d; ready(d,b);
    const auto t=begin(d,b,options(LoopbackMode::ANALOG));
    while (!d.operationSnapshot().temporaryMayBeActive) CHECK(pollOne(d,b,t).inProgress());
    if (fault==0) b.lateCall=static_cast<int>(b.calls);
    if (fault==1) b.regressCall=static_cast<int>(b.calls);
    if (fault==2) b.overflowCall=static_cast<int>(b.calls);
    if (fault==3) b.releaseCall=static_cast<int>(b.calls);
    const auto before=b.calls;
    const auto status=pollOne(d,b,t);
    CHECK(!status.ok() && !status.inProgress() && b.calls==before+1);
    const auto result=take(d,t); CHECK(result.cleanupOwed && result.temporaryMayBeActive);
  }
  Controller stalled; Driver wait; ready(wait,stalled);
  const auto t=begin(wait,stalled,options(LoopbackMode::MII));
  const auto before=stalled.calls;
  for(unsigned n=0;n<10 && wait.operationSnapshot().state==OperationState::RUNNING;++n) wait.pollOperation(t,0);
  CHECK(take(wait,t).status.is(Err::OPERATION_TIMEOUT) && stalled.calls==before);
  Controller reverse; Driver far; ready(far,reverse);
  const auto r=begin(far,reverse,options(LoopbackMode::REVERSE));
  while (!far.operationSnapshot().temporaryMayBeActive) CHECK(pollOne(far,reverse,r).inProgress());
  reverse.holdLinkDown=true; reverse.now=999950;
  CHECK(drain(far,reverse,r).is(Err::OPERATION_TIMEOUT));
  CHECK(take(far,r).cleanupOwed);
  Controller changed; Driver partner; ready(partner,changed);
  const auto restartCount=changed.resets;
  const auto change=begin(partner,changed,options(LoopbackMode::REVERSE));
  while(changed.resets==restartCount) CHECK(pollOne(partner,changed,change).inProgress());
  changed.words[16]=0x0007; // New negotiation resolves10/full after100 admission.
  CHECK(!drain(partner,changed,change).ok());
  const auto changedResult=take(partner,change);
  CHECK(changedResult.cleanupOwed && !changedResult.loopback.controlsVerified && changedResult.loopback.rawPhysts==0x0007);
  Controller sticky; Driver stuck; ready(stuck,sticky);
  OperationToken finite;
  const auto startFrames=sticky.frames,resetCount=sticky.resets;
  CHECK(stuck.startLoopback(options(LoopbackMode::ANALOG),allowance(64),finite).inProgress());
  while(sticky.resets==resetCount) CHECK(pollOne(stuck,sticky,finite).inProgress());
  sticky.holdPhyrcr=true;
  CHECK(drain(stuck,sticky,finite).is(Err::OPERATION_TIMEOUT));
  CHECK(sticky.frames==startFrames+64 && take(stuck,finite).cleanupOwed);
}

void ownership() {
  Controller bus; Driver first,second; ready(first,bus);
  CHECK(second.bind(bus.config(2)).ok()); bus.competitor=&second;
  const auto before=bus.exclusions;
  activate(first,bus,LoopbackMode::DIGITAL);
  const auto cleanup=stop(first,bus); CHECK(drain(first,bus,cleanup).ok()); CHECK(take(first,cleanup).status.ok());
  CHECK(bus.exclusions>before && !bus.locked);
  CHECK(second.healthSnapshot().totalSuccess==0 && second.healthSnapshot().totalFailures==0);
  CHECK(second.isBound() && !second.isInitialized());
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"matrix",matrix},{"admission",admission},{"start_failures",startFailures},
    {"stop_failures",stopFailures},{"cancellation",cancellation},{"barriers",barriers},{"timing",timing},{"ownership",ownership}};
  return test::runSuites(argc,argv,suites);
}
