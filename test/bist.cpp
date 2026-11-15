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
  uint8_t liveErrors{0}, capturedErrors{0xCC};
  uint16_t statusBits{0x0A00}; unsigned captures{0};
  bool holdGeneratorActive{false}; unsigned heldOffReads{0};
  int changeStatusCall{-1}; uint16_t changedStatus{0};
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
    if (reg == 27) return static_cast<uint16_t>((uint32_t{capturedErrors} << 8U) | (words[27] & 0x00FFU));
    if (reg == 22) {
      const auto mode=words[22]&0x3000U;
      const uint16_t allowed=mode==0x2000U?0x0C00U:mode==0x1000U?0x0200U:mode==0x3000U?0x0E00U:0U;
      const bool held=mode==0U && (holdGeneratorActive || heldOffReads!=0U);
      if(mode==0U && heldOffReads!=0U) --heldOffReads;
      return static_cast<uint16_t>((words[22]&0xF1FFU)|(statusBits&allowed)|(held?0x0200U:0U));
    }
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
    CHECK(semanticWrites < writes.size());
    writes[semanticWrites++] = {reg == 14 ? address : uint16_t{reg}, value, reg == 14};
    if (reg == 14 && selector == 0x401F) {
      if (address == 0x030E) { vod = value; ++vodWrites; }
      else if (address == 0x0122) opaque[0] = value;
      else if (address == 0x0123) opaque[1] = value;
      else if (address == 0x0130) opaque[2] = value;
      else if (address < 32U) words[address] = value;
      else CHECK(false);
    } else if (reg == 27) {
      words[27] = static_cast<uint16_t>(value & 0x00FFU);
      if ((value & 0x8000U) != 0U) { capturedErrors=liveErrors; liveErrors=0; ++captures; }
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
          words.fill(0); words[0] = 0x3500; words[4] = 0x01E1; words[25] = 0x8001; words[1] = 0x0024; words[16] = 0x0005; words[22] = 0x0100; words[27]=0x007D; words[28]=0x05EE; capturedErrors=0; liveErrors=0; opaque.fill(0); vod = 0x8400;
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
    if (static_cast<int>(call)==bus.changeStatusCall) bus.statusBits=bus.changedStatus;
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
        result.status = {Err::MDIO_TIMEOUT, -22022, "Injected BIST failure"}; break;
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

void ready(Driver& driver, Controller& bus, MacInterface interface = MacInterface::RMII) {
  CHECK(driver.bind(bus.config(1, interface)).ok());
  OperationToken token; CHECK(driver.startInitialize({ResetKind::NONE}, allowance(), token).inProgress());
  CHECK(drain(driver, bus, token).ok()); CHECK(take(driver, token).status.ok());
  configure(driver, bus, profile());
  bus.words[17] = 0x0B55; // Saved FIFO=3, with unrelated interrupt/clock bits.
  bus.words[27] = 0x002D; bus.words[28] = 0xA123;
  bus.words[22] = 0x4100; // Counter-wrap policy set; PRBS disabled, normal power.
  bus.words[9] |= 0x0010; bus.words[10] |= 0x0002; bus.words[11] |= 0x0100;
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
  CHECK(!driver.bistSnapshot().temporaryMayBeActive && !driver.bistSnapshot().active);
  CHECK((bus.words[27]&0x00FFU)==0x002D && (bus.words[28]&0x07FFU)==0x0123);
  CHECK(driver.isInitialized() && driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
  CHECK((bus.words[0] & 0x7100U) == 0x1000U);
  CHECK((bus.words[22] & 0x705FU) == 0x4000U);
  CHECK((bus.words[17] & 0x0300U) == 0x0300U);
  CHECK((bus.words[25] & 0xC000U) == 0x4000U); // Manual MDIX intent restored.
  CHECK((bus.words[9] & 0x0020U) == 0U); // AN profile's robust opt-out restored.
  CHECK((bus.words[11] & 0x0060U) == 0x0060U); // Coupled mirror profile restored.
  unchangedIntent(desired, driver.settingsSnapshot());
}


BistOptions options(BistMode mode=BistMode::GENERATOR_CHECKER) {
  BistOptions o; o.mode=mode; o.loopback.maintenanceAdmitted=true;
  o.observationUs=100; return o;
}
BistOptions loopOptions(LoopbackMode path) {
  auto o=options(); o.useLoopback=true; o.loopback.mode=path;
  o.loopback.termination100Ohm=path==LoopbackMode::ANALOG;
  o.loopback.externalFixture=path==LoopbackMode::EXTERNAL;
  o.loopback.allowHardResetExit=path==LoopbackMode::DIGITAL;
  return o;
}
OperationToken begin(Driver& driver,Controller& bus,const BistOptions& o=options()) {
  const auto calls=bus.calls,clocks=bus.clocks;
  OperationToken token{999}; CHECK(driver.startBist(o,allowance(),token).inProgress());
  CHECK(token.value!=999 && bus.calls==calls && bus.clocks==clocks);
  CHECK(driver.operationSnapshot().kind==OperationKind::BIST_START);
  return token;
}
void activate(Driver& driver,Controller& bus,const BistOptions& o=options()) {
  const auto token=begin(driver,bus,o); CHECK(drain(driver,bus,token).ok());
  const auto result=take(driver,token);
  CHECK(result.status.ok() && result.temporaryMayBeActive && result.cleanupOwed);
  CHECK(driver.bistSnapshot().active && driver.bistSnapshot().token.value==token.value);
}
OperationToken acquire(Driver& driver,Controller& bus,BistObservation observation={}) {
  const auto calls=bus.calls,clocks=bus.clocks;
  OperationToken token{999}; CHECK(driver.startBistAcquire(observation,allowance(),token).inProgress());
  CHECK(token.value!=999 && bus.calls==calls && bus.clocks==clocks);
  CHECK(driver.operationSnapshot().kind==OperationKind::BIST_ACQUIRE);
  return token;
}
OperationToken stop(Driver& driver,Controller& bus) {
  const auto calls=bus.calls,clocks=bus.clocks;
  OperationToken token{999}; CHECK(driver.stopBist(allowance(),token).inProgress());
  CHECK(token.value!=999 && bus.calls==calls && bus.clocks==clocks);
  CHECK(driver.operationSnapshot().kind==OperationKind::BIST_STOP);
  return token;
}
Status pollOne(Driver& driver,Controller& bus,OperationToken token,uint32_t slice=MAX_BATCH_FRAMES) {
  const auto state=driver.operationSnapshot();
  if(state.nextPollValid && bus.now<state.nextPollAtUs) bus.now=state.nextPollAtUs;
  if(state.busQuietValid && bus.now<state.busQuietUntilUs) bus.now=state.busQuietUntilUs;
  const auto calls=bus.calls;
  const auto result=driver.pollOperation(token,slice);
  CHECK(bus.calls<=calls+1 && !bus.locked); return result;
}
void finishStop(Driver& driver,Controller& bus,const SettingsSnapshot& intent) {
  const auto token=stop(driver,bus); CHECK(drain(driver,bus,token).ok());
  const auto result=take(driver,token);
  CHECK(result.status.ok() && result.persistentRestored && !result.temporaryMayBeActive);
  restored(driver,bus,intent);
}
void matrix() {
  for(MacInterface interface:{MacInterface::MII,MacInterface::RMII}) {
    for(BistMode mode:{BistMode::OFF,BistMode::SINGLE_PACKET,BistMode::CHECKER,BistMode::GENERATOR_CHECKER}) {
      for(BistCounterMode policy:{BistCounterMode::SATURATE,BistCounterMode::WRAP}) {
        Controller bus; Driver driver; ready(driver,bus,interface);
        const auto intent=driver.settingsSnapshot(); const auto health=driver.healthSnapshot();
        auto o=options(mode); o.counterMode=policy;
        bus.liveErrors=0xA5; const auto first=bus.semanticWrites;
        const auto startFrames=bus.frames,startCalls=bus.calls;
        activate(driver,bus,o);
        // Normal-line loopback clear/verify12frames9calls + BIST setup13/10.
        CHECK(bus.frames-startFrames==25U && bus.calls-startCalls==19U);
        const auto target=static_cast<uint16_t>((static_cast<uint32_t>(mode)<<12U)|(policy==BistCounterMode::WRAP?0x4000U:0U));
        CHECK((bus.words[22]&0x7000U)==target);
        CHECK(bus.words[27]==0x007D && (bus.words[28]&0x07FFU)==0x05EE);
        CHECK(bus.liveErrors==0 && bus.captures==1);
        unsigned captures=0;
        for(unsigned i=first;i<bus.semanticWrites;++i) {
          const auto& w=bus.writes[i];
          if(w.address==0x1B && (w.value&0x8000U)!=0U) { CHECK(w.value==0x807D); ++captures; }
          CHECK(w.address!=0x030B);
        }
        CHECK(captures==1);
        unchangedIntent(intent,driver.settingsSnapshot()); sameHealth(health,driver.healthSnapshot());
        bus.liveErrors=7;
        const auto token=acquire(driver,bus,{true,true});
        const auto before=bus.frames;
        CHECK(drain(driver,bus,token).ok());
        const auto result=take(driver,token);
        CHECK(bus.frames-before==4U); // BISCR, atomic capture/read, BISCR.
        CHECK(result.bist.measurementValid && result.bist.counterValid && result.bist.rawCounter==0x077D);
        CHECK(result.bist.session.value==driver.bistSnapshot().token.value);
        CHECK(result.bist.intervalUncertain && result.bist.wrapPossible==(policy==BistCounterMode::WRAP));
        CHECK(result.bist.outcome==(mode==BistMode::OFF?BistOutcome::DISABLED:
            mode==BistMode::SINGLE_PACKET?BistOutcome::SINGLE_PACKET_OBSERVED:BistOutcome::ERRORS_OBSERVED));
        sameHealth(health,driver.healthSnapshot());
        const auto stopFrames=bus.frames,stopCalls=bus.calls;
        finishStop(driver,bus,intent);
        // Disable+verify3/2 + shared stop57/33 + saved packet/IPG5/4.
        CHECK(bus.frames-stopFrames==65U && bus.calls-stopCalls==39U);
      }
    }
  }
  for(auto path:{LoopbackMode::DIGITAL,LoopbackMode::ANALOG,LoopbackMode::EXTERNAL}) {
    Controller bus; Driver driver; ready(driver,bus);
    const auto intent=driver.settingsSnapshot(); const auto first=bus.frames;
    activate(driver,bus,loopOptions(path));
    bool restartSeen=false,linkAfterRestart=false,enabled=false;
    for(unsigned i=first;i<bus.frames;++i) {
      const auto& w=bus.trace[i];
      if(w.kind==FrameKind::WRITE && w.reg==0x1F && w.value==0x4000) { restartSeen=true; linkAfterRestart=false; }
      if(restartSeen && w.kind==FrameKind::READ && w.reg==0x10) linkAfterRestart=true;
      if(w.kind==FrameKind::WRITE && w.reg==0x16 && (w.value&0x3000U)==0x3000U) {
        CHECK(!restartSeen || linkAfterRestart); enabled=true;
      }
    }
    CHECK(enabled);
    bus.liveErrors=1;
    const auto token=acquire(driver,bus); CHECK(drain(driver,bus,token).ok());
    const auto result=take(driver,token);
    if(path==LoopbackMode::ANALOG) {
      CHECK(result.bist.rawBefore==0x3B08 && result.bist.rawAfter==0x3B08);
      CHECK(result.bist.outcome==BistOutcome::ERRORS_OBSERVED);
    }
    finishStop(driver,bus,intent);
  }
}
void fields() {
  for(uint16_t bytes:{uint16_t{0},uint16_t{1},uint16_t{64},uint16_t{1518},uint16_t{2047}}) {
    for(uint16_t gap:{uint16_t{0},uint16_t{4},uint16_t{500},uint16_t{1020}}) {
      Controller bus; Driver driver; ready(driver,bus);
      const auto intent=driver.settingsSnapshot(); auto o=options(); o.packetBytes=bytes; o.interPacketGapBytes=gap;
      activate(driver,bus,o);
      CHECK((bus.words[28]&0x07FFU)==bytes && (bus.words[28]&0xF800U)==0xA000U);
      CHECK(bus.words[27]==gap/4U);
      bus.liveErrors=0x80; const auto token=acquire(driver,bus);
      CHECK(drain(driver,bus,token).ok()); const auto result=take(driver,token);
      CHECK(result.bist.errorBytes==0x80 && result.bist.rawCounter==static_cast<uint16_t>(0x8000U|(gap/4U)));
      finishStop(driver,bus,intent);
    }
  }
  Controller bus; Driver driver; ready(driver,bus);
  const auto calls=bus.calls,clocks=bus.clocks; OperationToken sentinel{555};
  const auto reject=[&](const BistOptions& o,OperationBudget b=allowance()) {
    CHECK(!driver.startBist(o,b,sentinel).inProgress());
    CHECK(sentinel.value==555 && bus.calls==calls && bus.clocks==clocks);
  };
  auto o=options(); o.mode=static_cast<BistMode>(255); reject(o);
  o=options(); o.counterMode=static_cast<BistCounterMode>(255); reject(o);
  for(uint16_t value:{uint16_t{2048},uint16_t{65535}}) { o=options(); o.packetBytes=value; reject(o); }
  for(uint16_t value:{uint16_t{1},uint16_t{2},uint16_t{3},uint16_t{501},uint16_t{1024}}) { o=options(); o.interPacketGapBytes=value; reject(o); }
  o=options(); o.observationUs=0; reject(o);
  o=options(); o.loopback.maintenanceAdmitted=false; reject(o);
  for(auto mode:{LoopbackMode::MII,LoopbackMode::PCS_INPUT,LoopbackMode::PCS_OUTPUT,LoopbackMode::REVERSE}) {
    o=loopOptions(mode); reject(o);
  }
  reject(options(),allowance(0)); reject(options(),allowance(100,0));
  auto b=allowance(); b.pollIntervalUs=0; reject(options(),b);
  CHECK(!driver.startBistAcquire({},allowance(),sentinel).inProgress());
  CHECK(!driver.stopBist(allowance(),sentinel).inProgress());
  CHECK(bus.calls==calls && bus.clocks==clocks && sentinel.value==555);
  // Mode bits off are insufficient: table8-40 generator-active0200 must also
  // be quiescent before the first mutation (literal readback0300).
  Controller active; Driver subject; ready(subject,active);
  active.words[22]=0x0100; active.holdGeneratorActive=true;
  const auto mutations=active.semanticWrites;
  const auto token=begin(subject,active);
  CHECK(!drain(subject,active,token).ok()); const auto rejected=take(subject,token);
  CHECK(!rejected.mutationAttempted && !rejected.temporaryMayBeActive);
  CHECK(active.semanticWrites==mutations && !subject.operationSnapshot().cleanupOwed);
}
void measurements() {
  for(unsigned scenario=0;scenario<9;++scenario) {
    Controller bus; Driver driver; ready(driver,bus);
    auto o=options();
    if(scenario==1 || scenario==2) o.mode=BistMode::SINGLE_PACKET;
    if(scenario==3 || scenario==4) o.mode=BistMode::CHECKER;
    if(scenario==5) o.counterMode=BistCounterMode::WRAP;
    if(scenario==7) bus.statusBits=0x0200; // Generator activity without checker lock.
    if(scenario==8) bus.statusBits=0x0800; // Checker lock without generator activity.
    if(scenario==1 || scenario==2) bus.statusBits=0; // Packet finished before first observation.
    activate(driver,bus,o);
    bus.liveErrors=scenario==6?0xFF:scenario==7 || scenario==8?3:0;
    const auto token=acquire(driver,bus,{scenario==2,scenario==4});
    CHECK(drain(driver,bus,token).ok()); const auto result=take(driver,token);
    CHECK(result.bist.measurementValid && result.bist.counterValid && result.bist.intervalUncertain);
    CHECK(result.bist.saturated==(scenario==6));
    CHECK(result.bist.wrapPossible==(scenario==5));
    const auto expected=scenario==2?BistOutcome::SINGLE_PACKET_OBSERVED:
        scenario==6?BistOutcome::ERRORS_OBSERVED:BistOutcome::INCONCLUSIVE;
    CHECK(result.bist.outcome==expected);
  }
  // Lock loss and lock gained between bookends are retained, never called a stable pass.
  for(bool gained:{false,true}) {
    Controller bus; Driver driver; ready(driver,bus); bus.statusBits=gained?0x0200:0x0A00;
    activate(driver,bus); bus.liveErrors=0;
    const auto token=acquire(driver,bus); const auto calls=bus.calls;
    bus.changeStatusCall=static_cast<int>(calls+2U); bus.changedStatus=gained?0x0A00:0x0600;
    CHECK(drain(driver,bus,token).ok()); const auto result=take(driver,token);
    CHECK(result.bist.beforeValid && result.bist.afterValid && !result.bist.lockObserved);
    CHECK(result.bist.syncLossObserved==!gained && result.bist.outcome==BistOutcome::INCONCLUSIVE);
  }
}
void acquireFailures() {
  for(unsigned phase=0;phase<4;++phase) {
    Controller bus; Driver driver; ready(driver,bus); activate(driver,bus);
    const auto intent=driver.settingsSnapshot(); bus.liveErrors=3;
    const auto token=acquire(driver,bus); const auto frames=bus.frames;
    bus.failFrame=static_cast<int>(frames+phase);
    CHECK(drain(driver,bus,token).is(Err::MDIO_TIMEOUT)); const auto result=take(driver,token);
    CHECK(bus.frames==frames+phase+1 && result.status.detail==-22022);
    CHECK(!result.bist.measurementValid && result.bist.outcome==BistOutcome::INCONCLUSIVE);
    CHECK(result.cleanupOwed && result.temporaryMayBeActive);
    CHECK(result.bist.beforeValid==(phase>0));
    CHECK(result.bist.counterValid==(phase>2));
    if(phase==1 || phase==2) CHECK(result.bist.historyLost);
    if(phase==3) {
      CHECK(result.bist.rawCounter==0x037D);
      const auto sample=driver.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::BICSR1)];
      CHECK(sample.valid && sample.raw==0x037D);
    }
    CHECK(driver.bistSnapshot().stale);
    bus.failFrame=-1; finishStop(driver,bus,intent);
  }
  for(unsigned call=0;call<3;++call) {
    Controller bus; Driver driver; ready(driver,bus); activate(driver,bus); bus.liveErrors=7;
    bus.releaseCall=static_cast<int>(bus.calls+call);
    const auto token=acquire(driver,bus);
    CHECK(drain(driver,bus,token).is(Err::MDIO_ERROR)); const auto result=take(driver,token);
    CHECK(result.status.detail==-37 && result.failingBatch.release.detail==-37);
    CHECK(!result.bist.measurementValid && result.cleanupOwed);
    if(call==1 || call==2) CHECK(result.bist.counterValid && result.bist.errorBytes==7);
  }
}
constexpr LoopbackMode PATHS[]={LoopbackMode::EXTERNAL,LoopbackMode::ANALOG,LoopbackMode::DIGITAL};
void operationFailures() {
  for(auto path:PATHS) for(bool stopping:{false,true}) {
    auto o=path==LoopbackMode::EXTERNAL?options():loopOptions(path);
    Controller reference; Driver baseline; ready(baseline,reference);
    if(stopping) activate(baseline,reference,o);
    const auto firstFrame=reference.frames,firstCall=reference.calls;
    const auto referenceToken=stopping?stop(baseline,reference):begin(baseline,reference,o);
    CHECK(drain(baseline,reference,referenceToken).ok()); CHECK(take(baseline,referenceToken).status.ok());
    const auto frames=reference.frames-firstFrame,calls=reference.calls-firstCall;
    CHECK(frames<128 && calls<100);
    for(unsigned phase=0;phase<frames;++phase) {
      Controller bus; Driver driver; ready(driver,bus);
      const auto intent=driver.settingsSnapshot(); const auto health=driver.healthSnapshot();
      if(stopping) activate(driver,bus,o);
      const auto before=bus.frames;
      bus.failFrame=static_cast<int>(before+phase);
      const auto token=stopping?stop(driver,bus):begin(driver,bus,o);
      CHECK(drain(driver,bus,token).is(Err::MDIO_TIMEOUT)); const auto result=take(driver,token);
      CHECK(result.status.detail==-22022 && bus.frames==before+phase+1 && !result.persistentRestored);
      sameHealth(health,driver.healthSnapshot());
      if(result.temporaryMayBeActive) {
        CHECK(result.cleanupOwed && driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
        bus.failFrame=-1; finishStop(driver,bus,intent);
      }
    }
    for(unsigned phase=0;phase<calls;++phase) {
      Controller bus; Driver driver; ready(driver,bus);
      const auto intent=driver.settingsSnapshot();
      if(stopping) activate(driver,bus,o);
      bus.releaseCall=static_cast<int>(bus.calls+phase);
      const auto token=stopping?stop(driver,bus):begin(driver,bus,o);
      CHECK(drain(driver,bus,token).is(Err::MDIO_ERROR)); const auto result=take(driver,token);
      CHECK(result.status.detail==-37 && !result.persistentRestored);
      if(result.temporaryMayBeActive) { bus.releaseCall=-1; finishStop(driver,bus,intent); }
    }
  }
  // A valid mismatching read survives a later release or deadline failure.
  for(bool stopping:{false,true}) for(bool packet:{false,true}) for(bool late:{false,true}) {
    Controller bus; Driver driver; ready(driver,bus);
    if(stopping) activate(driver,bus);
    const auto base=bus.calls;
    const auto token=stopping?stop(driver,bus):begin(driver,bus);
    // Literal phase counts: saves2 + normal-path9 + apply2 => IPG read13,
    // packet read14; stop saved-field verifications are callbacks37/38.
    bus.corruptCall=static_cast<int>(base+(stopping?(packet?38U:37U):(packet?14U:13U)));
    bus.corruptRegister=packet?28:27; bus.corruptValue=packet?0xA122:0x007C;
    if(late) bus.lateCall=bus.corruptCall; else bus.releaseCall=bus.corruptCall;
    CHECK(drain(driver,bus,token).is(late?Err::OPERATION_TIMEOUT:Err::MDIO_ERROR));
    const auto result=take(driver,token);
    CHECK(result.mismatch.valid && result.mismatch.registerAddress==(packet?28:27));
    CHECK(result.mismatch.mask==(packet?0x07FF:0x00FF));
    CHECK(result.cleanupOwed && result.temporaryMayBeActive && !result.persistentRestored);
    CHECK(driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
}
void cancellation() {
  for(auto path:PATHS) for(unsigned action=0;action<3;++action) {
    const auto o=path==LoopbackMode::EXTERNAL?options():loopOptions(path);
    Controller reference; Driver baseline; ready(baseline,reference);
    if(action!=0) activate(baseline,reference,o);
    const auto first=reference.calls;
    const auto ref=action==0?begin(baseline,reference,o):action==1?acquire(baseline,reference):stop(baseline,reference);
    CHECK(drain(baseline,reference,ref).ok()); CHECK(take(baseline,ref).status.ok());
    const auto count=reference.calls-first;
    for(unsigned boundary=0;boundary<count;++boundary) {
      Controller bus; Driver driver; ready(driver,bus); const auto intent=driver.settingsSnapshot();
      if(action!=0) activate(driver,bus,o);
      const auto token=action==0?begin(driver,bus,o):action==1?acquire(driver,bus):stop(driver,bus);
      for(unsigned i=0;i<boundary;++i) CHECK(pollOne(driver,bus,token).inProgress());
      const auto calls=bus.calls,clocks=bus.clocks;
      CHECK(driver.cancel(token).ok() && bus.calls==calls && bus.clocks==clocks);
      const auto result=take(driver,token); CHECK(result.status.is(Err::CANCELLED));
      CHECK(!result.persistentRestored);
      if(result.temporaryMayBeActive) { CHECK(result.cleanupOwed); finishStop(driver,bus,intent); }
    }
  }
}
void lifecycle() {
  Controller bus; Driver driver; ready(driver,bus); const auto intent=driver.settingsSnapshot();
  OperationToken prior;
  for(unsigned epoch=0;epoch<2;++epoch) {
    const auto start=begin(driver,bus);
    if(epoch!=0) CHECK(driver.pollOperation(prior).is(Err::TOKEN_MISMATCH));
    CHECK(drain(driver,bus,start).ok()); CHECK(take(driver,start).status.ok());
    prior=start;
    bus.liveErrors=epoch==0?9:0;
    const auto token=acquire(driver,bus);
    CHECK(drain(driver,bus,token).ok()); const auto result=take(driver,token);
    CHECK(result.bist.errorBytes==(epoch==0?9:0) && result.bist.session.value==start.value);
    CHECK(result.bist.outcome==(epoch==0?BistOutcome::ERRORS_OBSERVED:BistOutcome::INCONCLUSIVE));
    const auto calls=bus.calls,clocks=bus.clocks;
    const auto one=driver.bistSnapshot(),two=driver.bistSnapshot();
    CHECK(one.measurement.rawCounter==two.measurement.rawCounter && bus.calls==calls && bus.clocks==clocks);
    OperationToken sentinel{999};
    CHECK(driver.stopLoopback(allowance(),sentinel).is(Err::BUSY));
    CHECK(driver.startVerify(allowance(),sentinel).is(Err::BUSY));
    CHECK(driver.startConfigure(profile(),allowance(),sentinel).is(Err::BUSY));
    CHECK(driver.end().is(Err::BUSY));
    CHECK(bus.calls==calls && bus.clocks==clocks && sentinel.value==999);
    if(epoch==0) {
      const auto failedStop=stop(driver,bus); bus.failFrame=static_cast<int>(bus.frames+1U);
      CHECK(drain(driver,bus,failedStop).is(Err::MDIO_TIMEOUT));
      const auto failure=take(driver,failedStop);
      CHECK(failure.cleanupOwed && failure.bist.measurementValid && failure.bist.rawCounter==result.bist.rawCounter);
      CHECK(driver.bistSnapshot().measurement.rawCounter==result.bist.rawCounter);
      CHECK(result.bist.measurementValid && result.status.ok());
      bus.failFrame=-1;
    }
    finishStop(driver,bus,intent);
    CHECK(driver.bistSnapshot().stale && driver.bistSnapshot().measurement.rawCounter==result.bist.rawCounter);
  }
  activate(driver,bus,loopOptions(LoopbackMode::DIGITAL));
  bus.liveErrors=0x12;
  const auto measuredToken=acquire(driver,bus); CHECK(drain(driver,bus,measuredToken).ok());
  const auto measured=take(driver,measuredToken);
  const auto savedCapture=driver.bistSnapshot().measurement;
  OperationToken hard; CHECK(driver.startInitialize({ResetKind::HARD},allowance(),hard).inProgress());
  CHECK(drain(driver,bus,hard).ok()); const auto hardResult=take(driver,hard);
  CHECK(hardResult.persistentRestored && hardResult.bist.rawCounter==measured.bist.rawCounter);
  CHECK(hardResult.bist.measurementValid && savedCapture.rawCounter==0x127D);
  CHECK(driver.bistSnapshot().measurement.rawCounter==0x127D && driver.bistSnapshot().stale);
  restored(driver,bus,intent);
  activate(driver,bus);
  const auto calls=bus.calls,clocks=bus.clocks;
  CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_POWER_ON).ok());
  CHECK(bus.calls==calls && bus.clocks==clocks && !driver.bistSnapshot().active);
  CHECK(!driver.bistSnapshot().temporaryMayBeActive && driver.bistSnapshot().stale);
  for(unsigned disposition=0;disposition<4;++disposition) {
    Controller b; Driver d; ready(d,b); activate(d,b); b.liveErrors=0x19;
    const auto t=acquire(d,b);
    b.failFrame=static_cast<int>(b.frames+2U); // Accepted capture/clear, failed data return.
    CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT)); const auto failed=take(d,t);
    CHECK(failed.bist.historyLost && !failed.bist.counterValid);
    CHECK(d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::BICSR1)].historyLost);
    b.failFrame=-1;
    const auto beforeCalls=b.calls,beforeClocks=b.clocks;
    if(disposition<2U) {
      CHECK(d.invalidateDeviceState(disposition==0U?InvalidationReason::EXTERNAL_RESET:InvalidationReason::EXTERNAL_POWER_ON).ok());
      // Reset retires the pending command, but historical loss remains until
      // the owner explicitly consumes/discards that host evidence.
      CHECK(d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::BICSR1)].historyLost);
      CHECK(d.clearEvidence().ok());
    } else {
      CHECK(d.abandonCleanup().ok());
      if(disposition==3U) CHECK(d.end().ok());
      CHECK(d.bind(b.config()).ok());
    }
    CHECK(b.calls==beforeCalls && b.clocks==beforeClocks);
    CHECK(!d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::BICSR1)].historyLost);
    uint16_t id=0; CHECK(d.readClause22(2,id,{1000000,1}).ok() && id==0x2000);
    // An unrelated next receipt cannot revive a retired capture-pending flag.
    CHECK(!d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::BICSR1)].historyLost);
    CHECK(failed.bist.historyLost); // Previously delivered failure is immutable.
  }
}
void timingOwnership() {
  // Minimum observation time starts after completion plus the declared clock
  // lag, never at the pre-write caller time. Exact due permits the next batch.
  {
    Controller b; Driver d; ready(d,b); activate(d,b);
    const auto completed=b.now;
    const auto token=acquire(d,b);
    const auto due=d.operationSnapshot().nextPollAtUs;
    CHECK(due==completed+101U); // options100us + binding clock lag1us.
    const auto calls=b.calls;
    b.now=due-1;
    CHECK(d.pollOperation(token).inProgress() && b.calls==calls);
    b.now=due;
    CHECK(d.pollOperation(token).inProgress() && b.calls==calls+1);
    CHECK(drain(d,b,token).ok()); CHECK(take(d,token).bist.measurementValid);
  }
  {
    Controller b; Driver d; ready(d,b);
    auto o=options(); o.observationUs=std::numeric_limits<uint64_t>::max();
    const auto token=begin(d,b,o);
    CHECK(drain(d,b,token).is(Err::OPERATION_TIMEOUT));
    CHECK(take(d,token).temporaryMayBeActive && !d.bistSnapshot().active);
    const auto cleanup=stop(d,b); CHECK(drain(d,b,cleanup).ok()); CHECK(take(d,cleanup).persistentRestored);
  }
  for(bool sticky:{false,true}) {
    Controller b; Driver d; ready(d,b); activate(d,b);
    const auto initialVod=b.vodWrites,initialFrames=b.frames,initialCalls=b.calls;
    if(sticky) b.holdGeneratorActive=true; else b.heldOffReads=2;
    OperationToken token;
    CHECK(d.stopBist(allowance(sticky?16:512),token).inProgress());
    const auto status=drain(d,b,token); const auto result=take(d,token);
    if(sticky) {
      CHECK(status.is(Err::OPERATION_TIMEOUT) && result.cleanupOwed && result.temporaryMayBeActive);
      CHECK(b.vodWrites==initialVod && b.frames==initialFrames+16U);
    } else {
      CHECK(status.ok() && result.persistentRestored);
      CHECK(b.frames==initialFrames+67U && b.calls==initialCalls+41U);
    }
  }
  Controller bus; Driver first,second; ready(first,bus); CHECK(second.bind(bus.config(2)).ok());
  bus.competitor=&second; const auto intent=first.settingsSnapshot();
  activate(first,bus,loopOptions(LoopbackMode::DIGITAL));
  const auto token=acquire(first,bus); const auto calls=bus.calls;
  CHECK(first.pollOperation(token,0).inProgress() && bus.calls==calls);
  CHECK(pollOne(first,bus,token,1).inProgress()); // First status fits.
  const auto before=bus.calls;
  CHECK(pollOne(first,bus,token,1).inProgress() && bus.calls==before); // Capture pair cannot split.
  CHECK(pollOne(first,bus,token,2).inProgress() && bus.calls==before+1);
  CHECK(drain(first,bus,token).ok()); CHECK(take(first,token).bist.measurementValid);
  finishStop(first,bus,intent); CHECK(bus.exclusions!=0 && !bus.locked);
  for(unsigned fault=0;fault<3;++fault) {
    Controller b; Driver d; ready(d,b); activate(d,b);
    const auto t=acquire(d,b);
    if(fault==0) b.lateCall=static_cast<int>(b.calls);
    if(fault==1) b.regressCall=static_cast<int>(b.calls);
    if(fault==2) b.overflowCall=static_cast<int>(b.calls);
    CHECK(!drain(d,b,t).ok()); const auto r=take(d,t);
    CHECK(!r.bist.measurementValid && r.cleanupOwed && r.temporaryMayBeActive);
  }
  Controller frozen; Driver d; ready(d,frozen); activate(d,frozen);
  const auto t=acquire(d,frozen); const auto count=frozen.calls;
  for(unsigned i=0;i<12 && d.operationSnapshot().state==OperationState::RUNNING;++i) d.pollOperation(t,0);
  CHECK(take(d,t).status.is(Err::OPERATION_TIMEOUT) && frozen.calls==count);
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"matrix",matrix},{"fields",fields},{"measurements",measurements},
    {"acquire_failures",acquireFailures},{"operation_failures",operationFailures},{"cancellation",cancellation},
    {"lifecycle",lifecycle},{"timing_ownership",timingOwnership}};
  return test::runSuites(argc,argv,suites);
}
