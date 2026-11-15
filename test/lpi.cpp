#include "support/Check.h"
#include "../examples/portable/Lpi.h"
#include <DP8382x/DP8382x.h>
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target, devad; uint64_t at; };

// Independent SNLS647I p27 section8.3.4 literals: preserve EFFF; set/clear1000.
// Shared setup independently models the p26 EEE recipe; no production masks.
// DEVAD3/7 storage is separate from vendor31 and direct C22; successful fake
// reads do not establish Q1 wire mapping or physical EEE interoperability.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<uint16_t,64> mmd3{},mmd7{};
  std::array<Wire,8192> trace{};
  uint16_t selector{0},address{0},id2{0xA131};
  uint64_t now{100};
  unsigned frames{0},calls{0},clocks{0},releases{0},mutations{0},hard{0};
  Err failStatus{Err::MDIO_TIMEOUT};
  int failFrame{-1},releaseCall{-1},corruptFrame{-1},lateCall{-1},regressCall{-1},invalidCall{-1},badReceiptCall{-1},brokenPrefixCall{-1};
  uint16_t corruptMask{1};
  bool quarantined{false};
  bool acceptedFailure{true},locked{false};
  uint64_t frameTime{2};
  Driver* competitor{nullptr};Driver* reentrant{nullptr}; unsigned excluded{0};
  Controller() { defaults(); }
  void defaults() {
    words.fill(0);ext.fill(0);mmd3.fill(0);mmd7.fill(0);words[0]=0x3100;words[4]=0x01E1;
    words[1]=0x0024;words[5]=0x0101;words[6]=1;words[16]=0x15;
    words[25]=0x8001;words[22]=0x0100;words[30]=0x0102;
    ext[0x030E]=0x8400;ext[0x04A1]=0x1000;ext[0x04D1]=0x0080;
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=Variant::DP83826I;c.interface=id2==0xA111?MacInterface::MII:MacInterface::RMII;c.phyAddress=phy;
    c.transport={this,execute,clock,{8,4,1}};return c;
  }
  static uint64_t clock(void* context) noexcept { auto& b=*static_cast<Controller*>(context);++b.clocks;return b.now; }
  uint16_t read(uint16_t target,uint16_t devad=31) {
    if(devad==3) {
      CHECK(target<64U);if(target>=64U)return 0xFFFF;
      if(target==0x14)return 2;
      const auto value=mmd3[target];if(target==1)mmd3[target]&=0xF3FF;if(target==0x16)mmd3[target]=0;
      return value;
    }
    if(devad==7) { CHECK(target<64U);return target<64U?mmd7[target]:uint16_t{0xFFFF}; }
    if(target==2)return 0x2000;
    if(target==3)return id2;
    if(target==0) { words[0]&=0x7DFFU;return words[0]; }
    if(target==31)words[31]=0;
    const auto value=target<32U?words[target]:ext[target];
    if(target==18 || target==19) { words[target]&=0x00FF;words[16]&=0xFF7F; }
    if(target==20) { words[20]&=0xFF00;words[16]&=0xF7FF; }
    if(target==21) { words[21]=0;words[16]&=0xDFFF; }
    if(target==0x04A1)ext[target]&=0xFF00;
    return value;
  }
  void write(uint8_t reg,uint16_t value) {
    if(reg==13) { CHECK((value&0xBFE0U)==0U && ((value&31U)==3U || (value&31U)==7U || (value&31U)==31U));selector=value;return; }
    if(reg==14 && (selector&0x4000U)==0U) { address=value;return; }
    const uint16_t target=reg==14?address:uint16_t{reg};++mutations;
    const uint16_t devad=reg==14?static_cast<uint16_t>(selector&31U):uint16_t{31};
    if(devad==3 || devad==7) {
      CHECK(target<64U);if(target>=64U)return;
      if(devad==3)mmd3[target]=value;else mmd7[target]=value;
      return;
    }
    if(target==31 && value==0x8000) { ++hard;defaults();return; }
    if(target==0 && value==0x8000) { words[0]=0x3100;return; }
    if(target==18 || target==19) { CHECK((value&0xFF00U)==0U);words[target]=static_cast<uint16_t>((words[target]&0xFF00U)|value);return; }
    if(target==0x04A1) {
      CHECK(value==0x1000 || value==0);
      ext[target]=static_cast<uint16_t>((ext[target]&0x00FFU)|value);return;
    }
    if(target<32U)words[target]=value;else ext[target]=value;
  }
  static BatchResult execute(void* context,const MdioBatch& batch,const IoBudget& budget) noexcept {
    auto& b=*static_cast<Controller*>(context);BatchResult r;
    r.status=Status::Ok();r.releaseStatus=Status::Ok();r.controllerReusable=true;
    if(b.quarantined) { r.status={Err::BUSY,static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED),"Quarantined"};r.controllerReusable=false;return r; }
    if(b.locked) { r.status={Err::BUSY,static_cast<int32_t>(BusyDetail::OWNER_ADMISSION),"Owned"};return r; }
    CHECK(batch.frameCount<=budget.maxFrames);b.locked=true;const auto call=b.calls++;
    for(uint8_t i=0;i<batch.frameCount;++i) {
      const auto& q=batch.frames[i];auto& f=r.frames[i];uint16_t value=q.value;
      if(q.readIndex!=NO_READ_INDEX) { CHECK(q.readIndex<i && r.frames[q.readIndex].valueValid);value=static_cast<uint16_t>((r.frames[q.readIndex].value&q.preserveMask)|q.value); }
      const uint16_t target=q.registerAddress==14 && (b.selector&0x4000U)!=0U?b.address:uint16_t{q.registerAddress};
      const uint16_t devad=q.registerAddress==14 && (b.selector&0x4000U)!=0U?static_cast<uint16_t>(b.selector&31U):uint16_t{31};
      CHECK(b.frames<b.trace.size());const auto frame=b.frames++;b.trace[frame]={q.kind,q.registerAddress,value,target,devad,b.now};
      f.attempted=true;const bool failed=static_cast<int>(frame)==b.failFrame;
      if(!failed || b.acceptedFailure) {
        if(q.kind==FrameKind::WRITE)b.write(q.registerAddress,value);
        else { f.value=b.read(target,devad);if(static_cast<int>(frame)==b.corruptFrame)f.value^=b.corruptMask;
}
      }
      b.now+=b.frameTime;
      if(b.reentrant!=nullptr) {
        LpiResult untouched;untouched.control=0xCAFE;
        CHECK(b.reentrant->stopLpi(untouched,budget).is(Err::BUSY));CHECK(untouched.control==0xCAFE);
        CHECK(b.reentrant->cancel(OperationToken{999}).is(Err::BUSY));
      }
      if(b.competitor!=nullptr) {
        uint16_t sentinel=0xCAFE;CHECK(b.competitor->readMmd(31,0x0467,sentinel,budget).is(Err::BUSY));CHECK(sentinel==0xCAFE);++b.excluded;
      }
      if(failed) { f.effectPossible=b.acceptedFailure;r.status={b.failStatus,-30030,"Injected frame"};break; }
      f.completed=true;f.valueValid=q.kind==FrameKind::READ;
    }
    if(static_cast<int>(call)==b.releaseCall) { r.releaseStatus={Err::MDIO_ERROR,-37,"Injected release"};r.controllerReusable=false;b.quarantined=true; }
    if(static_cast<int>(call)==b.lateCall)b.now=budget.deadlineUs;
    if(static_cast<int>(call)==b.regressCall)b.now=0;
    if(static_cast<int>(call)==b.invalidCall)r.status={Err::IN_PROGRESS,-29030,"Illegal terminal status"};
    if(static_cast<int>(call)==b.badReceiptCall)r.frames[0].attempted=false;
    if(static_cast<int>(call)==b.brokenPrefixCall)r.frames[0].completed=false;
    b.locked=false;++b.releases;return r;
  }
};
OperationBudget budget(uint32_t frames=2048,uint64_t deadline=1000000) { return {{deadline,frames},10,8}; }
Status poll(Driver& d,Controller& b,OperationToken t,uint32_t slice=MAX_BATCH_FRAMES) {
  const auto s=d.operationSnapshot();
  if(s.nextPollValid && b.now<s.nextPollAtUs)b.now=s.nextPollAtUs;
  if(s.busQuietValid && b.now<s.busQuietUntilUs)b.now=s.busQuietUntilUs;
  const auto calls=b.calls;const auto status=d.pollOperation(t,slice);CHECK(b.calls<=calls+1U && !b.locked);return status;
}
Status drain(Driver& d,Controller& b,OperationToken t) {
  Status s={Err::IN_PROGRESS,0,"Pending"};for(unsigned n=0;n<512U && s.inProgress();++n)s=poll(d,b,t);CHECK(!s.inProgress());return s;
}
OperationResult take(Driver& d,OperationToken t) { OperationResult r;CHECK(d.takeResult(t,r).ok());return r; }
void initialized(Driver& d,Controller& b) {
  CHECK(d.bind(b.config()).ok());OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}
void ready(Driver& d,Controller& b) {
  initialized(d,b);LinkSettings profile;profile.advertisement.technologyMask=8;
  OperationToken t;CHECK(d.startConfigure(profile,{},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}
EeeSettings settings(bool enabled=true) { EeeSettings s;s.enabled=enabled;s.maintenanceAdmitted=true;return s; }
OperationToken configure(Driver& d,Controller& b,const EeeSettings& s=settings()) {
  OperationToken t{999};const auto calls=b.calls,clocks=b.clocks;
  CHECK(d.startConfigureEee(s,budget(),t).inProgress());
  CHECK(t.value!=999 && b.calls==calls && b.clocks==clocks);return t;
}
void configured(Driver& d,Controller& b,const EeeSettings& s=settings()) {
  const auto t=configure(d,b,s);CHECK(drain(d,b,t).ok());const auto r=take(d,t);
  CHECK(r.status.ok() && !r.cleanupOwed && r.persistentRestored && d.isInitialized());
  CHECK(d.eeeSnapshot().desiredValid && d.eeeSnapshot().trust==ConfigurationTrust::VERIFIED);
}

LpiOptions admitted() { LpiOptions options;options.macTxQuiesced=options.maintenanceAdmitted=true;return options; }
IoBudget io(uint32_t frames=9) { return {1000000,frames}; }
void observations(Driver& d,Controller& b) {
  b.mmd7[0x3D]=2;LinkSnapshot link;PowerSnapshot power;EeeSnapshot eee;
  CHECK(d.readLinkStatus(link,io(10)).ok());CHECK(link.resolvedModeValid && link.linkUp);
  CHECK(d.readPowerStatus(power,io(3)).ok());CHECK(power.state==PowerState::NORMAL_INDICATED);
  CHECK(d.readEeeStatus(eee,io(31)).ok());CHECK(eee.negotiatedValid && eee.negotiated);
}
void available(Driver& d,Controller& b) { ready(d,b);configured(d,b);observations(d,b); }
void trace(const Controller& b,unsigned first,uint16_t value) {
  const uint8_t regs[]={13,14,13,14,14,13,14,13,14};
  const uint16_t words[]={31,0x04D1,0x401F,0,value,31,0x04D1,0x401F,0};
  for(unsigned i=0;i<9U;++i) {
    const auto& f=b.trace[first+i];CHECK(f.reg==regs[i]);
    CHECK(f.kind==((i==3U || i==8U)?FrameKind::READ:FrameKind::WRITE));
    if(i!=3U && i!=8U)CHECK(f.value==words[i]);
  }
}
void traces() {
  // Every surrounding bit is preserved, including opaque recipe bit1 and
  // DP83825's unrelated bits14:13. Only exact DP83826 bit12 is owned.
  for(uint16_t before:{uint16_t{0x008B},uint16_t{0xEFFF},uint16_t{0x0009},uint16_t{0xA00B}}) {
    Controller b;Driver d;available(d,b);b.ext[0x04D1]=before;
    const auto first=b.frames,calls=b.calls;const auto health=d.healthSnapshot();LpiResult enter;
    CHECK(d.startLpi(admitted(),enter,io()).ok());trace(b,first,static_cast<uint16_t>(before|0x1000));
    CHECK(b.frames==first+9U && b.calls==calls+2U && b.releases==b.calls && !b.locked);
    CHECK(enter.status.ok() && enter.primary.ok() && enter.release.ok() && enter.forcedLpi==ForcedLpi::REQUESTED);
    CHECK(enter.mutationAttempted && enter.mutationCompleted && enter.readbackValid);
    CHECK(enter.attemptedFrames==9 && enter.completedFrames==9 && enter.transactions==2);
    CHECK(enter.control==static_cast<uint16_t>(before|0x1000));
    CHECK(d.operationSnapshot().state==OperationState::IDLE && d.operationSnapshot().temporaryMayBeActive && d.operationSnapshot().cleanupOwed);
    const auto at=b.frames;LpiResult exit;
    CHECK(d.stopLpi(exit,io()).ok());trace(b,at,before);CHECK(b.ext[0x04D1]==before);
    CHECK(exit.status.ok() && exit.forcedLpi==ForcedLpi::OFF && exit.readbackValid);
    CHECK(!d.operationSnapshot().temporaryMayBeActive && !d.operationSnapshot().cleanupOwed);
    const auto after=d.healthSnapshot();CHECK(after.totalSuccess==health.totalSuccess && after.totalFailures==health.totalFailures);
    CHECK(after.lastOkUs==health.lastOkUs && after.consecutiveFailures==health.consecutiveFailures);
    CHECK(d.lpiSnapshot().forcedLpi==ForcedLpi::OFF && d.lpiSnapshot().lastResult.status.ok());
    const auto cachedCalls=b.calls,clocks=b.clocks;const auto one=d.lpiSnapshot(),two=d.lpiSnapshot();
    CHECK(one.forcedLpi==two.forcedLpi && b.calls==cachedCalls && b.clocks==clocks);
    LpiResult sentinel;sentinel.control=0xCAFE;CHECK(!d.stopLpi(sentinel,io()).ok());
    CHECK(sentinel.control==0xCAFE && b.calls==cachedCalls);
  }
  // Stop validates its owned bit only: an unrelated readback difference is
  // retained raw, never turned into a fabricated LPI readiness condition.
  Controller b;Driver d;available(d,b);LpiResult r;CHECK(d.startLpi(admitted(),r,io()).ok());
  b.corruptFrame=static_cast<int>(b.frames+8U);b.corruptMask=0x8000;
  CHECK(d.stopLpi(r,io()).ok());CHECK(r.control==0x808B && r.forcedLpi==ForcedLpi::OFF);
}
void admission() {
  for(unsigned fault=0;fault<14U;++fault) {
    Controller b;Driver d;if(fault!=0U)available(d,b);auto options=admitted();
    if(fault==1U)options.macTxQuiesced=false;
    if(fault==2U)options.maintenanceAdmitted=false;
    if(fault==3U) { b.words[1]=0;b.words[16]=0;LinkSnapshot link;CHECK(d.readLinkStatus(link,io(10)).ok()); }
    if(fault==4U) { b.words[16]=0x17;LinkSnapshot link;CHECK(d.readLinkStatus(link,io(10)).ok()); }
    if(fault==5U) { b.mmd7[0x3D]=0;EeeSnapshot e;CHECK(d.readEeeStatus(e,io(31)).ok()); }
    if(fault==6U) { b.words[0]|=0x0800;PowerSnapshot p;CHECK(d.readPowerStatus(p,io(3)).ok()); }
    if(fault==7U)CHECK(d.invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION).ok());
    if(fault==8U) { b.failFrame=static_cast<int>(b.frames);PowerSnapshot p;CHECK(!d.readPowerStatus(p,io(3)).ok());b.failFrame=-1; }
    if(fault==9U) { auto s=settings();s.bypassBit0=true;configured(d,b,s); }
    if(fault==10U) { auto s=settings();s.bypassBit3=true;configured(d,b,s); }
    if(fault==11U) { LpiResult r;CHECK(d.startLpi(options,r,io()).ok()); }
    OperationToken token;
    if(fault>=12U)CHECK(d.startVerify(budget(),token).inProgress());
    if(fault==13U)CHECK(drain(d,b,token).ok());
    const auto calls=b.calls;LpiResult sentinel;sentinel.control=0xCAFE;const auto before=d.lpiSnapshot();
    CHECK(!d.startLpi(options,sentinel,io()).ok());CHECK(sentinel.control==0xCAFE && b.calls==calls);
    CHECK(d.lpiSnapshot().forcedLpi==before.forcedLpi);
  }
  Controller b;Driver d;available(d,b);
  for(uint32_t frames=0;frames<9U;++frames) {
    const auto calls=b.calls;LpiResult r;r.control=0xCAFE;
    CHECK(d.startLpi(admitted(),r,io(frames)).is(Err::OPERATION_TIMEOUT));CHECK(b.calls==calls && r.control==0xCAFE);
  }
}
void failures() {
  {
    Controller b;Driver d;available(d,b);LpiResult r;
    b.brokenPrefixCall=static_cast<int>(b.calls);
    CHECK(d.startLpi(admitted(),r,io()).is(Err::MDIO_ERROR));
    CHECK(r.attemptedFrames==1 && r.completedFrames==0 && r.transactions==1);
    CHECK(r.forcedLpi==ForcedLpi::UNCERTAIN && d.operationSnapshot().cleanupOwed);
  }

  for(unsigned failure=0;failure<3U;++failure) {
    Controller b;Driver d;available(d,b);LpiResult r;CHECK(d.startLpi(admitted(),r,io()).ok());
    b.ext[0x04D1]=0x1080;
    if(failure!=0U)b.failFrame=static_cast<int>(b.frames+(failure==1U?4U:8U));
    const auto status=d.stopLpi(r,io());
    CHECK(status.ok()==(failure==0U));CHECK(d.eeeSnapshot().trust==ConfigurationTrust::DIRTY);
    CHECK(d.operationSnapshot().cleanupOwed);
    CHECK(r.forcedLpi==(failure==0U?ForcedLpi::OFF:ForcedLpi::UNCERTAIN));
  }
  for(unsigned defect=0;defect<3U;++defect) {
    Controller b;Driver d;available(d,b);const auto calls=b.calls;LpiResult r;
    if(defect==0U)b.badReceiptCall=static_cast<int>(calls);
    if(defect==1U)b.invalidCall=static_cast<int>(calls);
    if(defect==2U) { b.failFrame=static_cast<int>(b.frames+4U);b.releaseCall=static_cast<int>(calls); }
    CHECK(!d.startLpi(admitted(),r,io()).ok());
    CHECK(r.forcedLpi==ForcedLpi::UNCERTAIN && r.mutationPossible && r.transactions==1U);
    if(defect==2U)CHECK(r.status.is(Err::MDIO_TIMEOUT) && r.status.detail==-30030 && r.primary.detail==-30030 && r.release.detail==-37);
    else CHECK(r.status.is(Err::MDIO_ERROR));
  }
  for(bool stop:{false,true})for(bool accepted:{false,true})for(unsigned phase=0;phase<9U;++phase) {
    Controller b;Driver d;available(d,b);LpiResult first;if(stop)CHECK(d.startLpi(admitted(),first,io()).ok());
    const auto start=b.frames,calls=b.calls; b.failFrame=static_cast<int>(start+phase);b.acceptedFailure=accepted;
    LpiResult r;const auto status=stop?d.stopLpi(r,io()):d.startLpi(admitted(),r,io());
    CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail==-30030 && r.primary.detail==-30030);
    CHECK(b.frames==start+phase+1U && b.calls==calls+(phase<5U?1U:2U));
    CHECK(r.attemptedFrames==phase+1U && r.completedFrames==phase);
    CHECK(b.releases==b.calls && !b.locked && d.operationSnapshot().state==OperationState::IDLE);
    if (phase>=4U) { CHECK(d.eeeSnapshot().stale && !d.eeeSnapshot().negotiatedValid); }
    const bool temporary=stop || phase>=4U;
    CHECK(d.operationSnapshot().temporaryMayBeActive==temporary);
    CHECK(d.operationSnapshot().cleanupOwed==temporary);
    CHECK(d.lpiSnapshot().forcedLpi==(temporary?ForcedLpi::UNCERTAIN:ForcedLpi::OFF));
    if(temporary)CHECK(r.warning!=nullptr && std::strcmp(r.warning,"forced LPI may remain active; frames sent now can be lost")==0);
    b.failFrame=-1;
    if(temporary) { const auto failed=r;CHECK(d.stopLpi(r,io()).ok());CHECK(failed.status.is(Err::MDIO_TIMEOUT)); }
  }
  for(bool stop:{false,true}) {
    Controller b;Driver d;available(d,b);LpiResult r;if(stop)CHECK(d.startLpi(admitted(),r,io()).ok());
    b.corruptFrame=static_cast<int>(b.frames+8U);b.corruptMask=0x1000;
    CHECK((stop?d.stopLpi(r,io()):d.startLpi(admitted(),r,io())).is(Err::CONFIG_MISMATCH));
    CHECK(r.readbackValid && r.forcedLpi==ForcedLpi::UNCERTAIN && d.operationSnapshot().cleanupOwed);
  }
  for(unsigned call=0;call<2U;++call) {
    Controller b;Driver d;available(d,b);LpiResult r;const auto start=b.calls;b.releaseCall=static_cast<int>(start+call);
    CHECK(!d.startLpi(admitted(),r,io()).ok());CHECK(r.primary.ok() && r.release.detail==-37);
    CHECK(r.forcedLpi==ForcedLpi::UNCERTAIN && r.warning!=nullptr);
    const auto calls=b.calls;LpiResult untouched;untouched.control=0xCAFE;CHECK(!d.stopLpi(untouched,io()).ok());
    CHECK(b.calls==calls && untouched.transactions==1 && untouched.forcedLpi==ForcedLpi::UNCERTAIN);
    b.quarantined=false;b.releaseCall=-1;CHECK(d.stopLpi(untouched,io()).ok());
  }
  // A backend cannot asynchronously cancel a synchronous owner call. Illegal
  // statuses normalize to MDIO_ERROR, with signed detail and effects retained.
  for(Err status:{Err::CANCELLED,Err::IN_PROGRESS})for(unsigned phase=0;phase<9U;++phase) {
    Controller b;Driver d;available(d,b);b.failFrame=static_cast<int>(b.frames+phase);b.failStatus=status;LpiResult r;
    CHECK(d.startLpi(admitted(),r,io()).is(Err::MDIO_ERROR));CHECK(r.status.detail==-30030);
    if(phase>=4U)CHECK(r.forcedLpi==ForcedLpi::UNCERTAIN);
  }
}
void timing() {
  for(unsigned fault=0;fault<4U;++fault) {
    Controller b;Driver d;available(d,b);LpiResult r;r.control=0xCAFE;const auto calls=b.calls;IoBudget allowed=io();
    if(fault==0U)allowed.deadlineUs=b.now;
    if(fault==1U)allowed.deadlineUs=b.now+53U; // 9*4 + 2*8 + quantization1; strict exclusive bound.
    if(fault==2U)b.now=std::numeric_limits<uint64_t>::max();
    if(fault==3U)b.now=0;
    CHECK(!d.startLpi(admitted(),r,allowed).ok());CHECK(b.calls==calls && r.control==0xCAFE);
  }
  {
    Controller b;Driver d;available(d,b);LpiResult r;
    CHECK(d.startLpi(admitted(),r,{b.now+54U,9}).ok()); // first admissible strict bound
  }
  for(unsigned call=0;call<2U;++call) {
    Controller b;Driver d;available(d,b);LpiResult r;b.lateCall=static_cast<int>(b.calls+call);
    CHECK(!d.startLpi(admitted(),r,io()).ok());CHECK(r.forcedLpi==ForcedLpi::UNCERTAIN && d.operationSnapshot().cleanupOwed);
    CHECK(r.transactions==call+1U && b.releases==b.calls);
  }
  for(unsigned call=0;call<2U;++call) {
    Controller b;Driver d;available(d,b);LpiResult r;b.regressCall=static_cast<int>(b.calls+call);
    CHECK(!d.startLpi(admitted(),r,io()).ok());
    CHECK(d.transportSnapshot().clockInvalid && r.forcedLpi==ForcedLpi::UNCERTAIN);
    const auto calls=b.calls;LpiResult untouched;untouched.control=0xCAFE;
    CHECK(!d.stopLpi(untouched,io()).ok());CHECK(b.calls==calls && untouched.control==0xCAFE);
  }
  Controller b;Driver d;available(d,b);b.frameTime=0;const auto calls=b.calls;LpiResult r;
  CHECK(d.startLpi(admitted(),r,io()).ok());CHECK(b.calls==calls+2U);
  CHECK(d.stopLpi(r,io()).ok());CHECK(b.calls==calls+4U); // no waiting, drain or readiness polling
}
void ownership() {
  for(bool failExit:{false,true}) {
    Controller bus;Driver driver;available(driver,bus);LpiOwnerAttempt owner;
    CHECK(enterOwnerLpi(driver,admitted(),io(),owner).ok());CHECK(owner.keepMacStopped);
    const auto calls=bus.calls,clocks=bus.clocks;
    CHECK(!permitMacAfterLpi(driver,false,owner));CHECK(!permitMacAfterLpi(driver,true,owner));
    CHECK(bus.calls==calls && bus.clocks==clocks); // Dwell is application-owned and passive.
    if(failExit)bus.failFrame=static_cast<int>(bus.frames+4U);
    CHECK(exitOwnerLpi(driver,io(),owner).ok()!=failExit);
    CHECK(!permitMacAfterLpi(driver,false,owner));
    CHECK(permitMacAfterLpi(driver,true,owner)!=failExit);
    CHECK(owner.keepMacStopped==failExit);
    if(failExit) {
      CHECK(owner.exit.warning!=nullptr && owner.entry.status.ok());const auto failed=owner.exit;
      bus.failFrame=-1;CHECK(exitOwnerLpi(driver,io(),owner).ok());
      CHECK(failed.status.is(Err::MDIO_TIMEOUT) && permitMacAfterLpi(driver,true,owner));
    }
  }
  Controller b;Driver d,other;available(d,b);CHECK(other.bind(b.config(2)).ok());b.competitor=&other;b.reentrant=&d;
  const auto first=b.excluded;LpiResult r;CHECK(d.startLpi(admitted(),r,io()).ok());CHECK(b.excluded==first+9U);b.competitor=nullptr;b.reentrant=nullptr;
  const auto frames=b.frames,clocks=b.clocks;OperationToken token{999};
  CHECK(d.end().is(Err::BUSY));CHECK(d.bind(b.config()).is(Err::BUSY));CHECK(!d.cancel(token).ok());
  CHECK(!d.startInitialize({ResetKind::PCS},budget(),token).inProgress());
  CHECK(!d.writeClause22(0x19,0,io()).ok());CHECK(!d.writeMmd(31,0x0467,0,io()).ok());
  CHECK(b.frames==frames && b.clocks==clocks && d.lpiSnapshot().forcedLpi==ForcedLpi::REQUESTED);
  uint16_t raw=0;CHECK(d.readMmd(31,0x0467,raw,io(4)).ok());CHECK(raw==b.ext[0x0467]);
  LinkSnapshot link;CHECK(d.readLinkStatus(link,io(10)).ok());
  CHECK(link.linkUp && link.temporaryMayBeActive && !link.resolvedModeValid && !link.pauseValid);
  EeeSnapshot eee;CHECK(d.readEeeStatus(eee,io(31)).ok());CHECK(eee.trust==ConfigurationTrust::VERIFIED && !eee.negotiatedValid);
  CHECK(d.lpiSnapshot().forcedLpi==ForcedLpi::REQUESTED);
  // A later raw zero is observation, never an owning stop or permission repair.
  b.ext[0x04D1]=0x008B;CHECK(d.readEeeStatus(eee,io(31)).ok());
  CHECK(d.lpiSnapshot().forcedLpi!=ForcedLpi::OFF && d.operationSnapshot().cleanupOwed);
  CHECK(d.stopLpi(r,io()).ok());CHECK(d.isInitialized());
  // Explicit stopping is possible even if link/power observations became stale.
  observations(d,b);CHECK(d.startLpi(admitted(),r,io()).ok());b.words[1]=0;b.words[16]=0;
  CHECK(d.readLinkStatus(link,io(10)).ok());CHECK(d.stopLpi(r,io()).ok());
}
void recovery() {
  // Every HARD/recipe/profile restoration phase can fail independently. The
  // earlier failed-exit result is a value, never overwritten by recovery.
  Controller baseline;Driver reference;available(reference,baseline);LpiResult entered;
  CHECK(reference.startLpi(admitted(),entered,io()).ok());const auto first=baseline.frames;OperationToken baselineToken;
  CHECK(reference.startInitialize({ResetKind::HARD},budget(),baselineToken).inProgress());
  CHECK(drain(reference,baseline,baselineToken).ok());take(reference,baselineToken);const auto count=baseline.frames-first;
  for(unsigned phase=0;phase<count;++phase) {
    Controller bus;Driver driver;available(driver,bus);LpiResult r;CHECK(driver.startLpi(admitted(),r,io()).ok());
    const auto start=bus.frames;bus.failFrame=static_cast<int>(start+phase);OperationToken token;
    CHECK(driver.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    CHECK(drain(driver,bus,token).is(Err::MDIO_TIMEOUT));const auto failed=take(driver,token);
    CHECK(failed.status.detail==-30030 && failed.cleanupOwed && !failed.persistentRestored);
    CHECK(bus.frames==start+phase+1U && bus.releases==bus.calls && !bus.locked);
  }
  // Cancellation at every restoration callback boundary retains temporary
  // evidence until the complete owning restoration has succeeded.
  bool completionReached=false;
  for(unsigned phase=0;phase<baseline.calls;++phase) {
    Controller bus;Driver driver;available(driver,bus);LpiResult r;CHECK(driver.startLpi(admitted(),r,io()).ok());
    OperationToken token;CHECK(driver.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    unsigned n=0;while(n<phase && driver.operationSnapshot().state==OperationState::RUNNING) { poll(driver,bus,token);++n; }
    if(driver.operationSnapshot().state!=OperationState::RUNNING) { completionReached=true;break; }
    const auto frames=bus.frames,clocks=bus.clocks;CHECK(driver.cancel(token).ok());const auto cancelled=take(driver,token);
    CHECK(cancelled.status.is(Err::CANCELLED) && cancelled.cleanupOwed && bus.frames==frames && bus.clocks==clocks);
    CHECK(driver.lpiSnapshot().forcedLpi!=ForcedLpi::OFF);
  }
  CHECK(completionReached);
  for(InvalidationReason why:{InvalidationReason::EXTERNAL_MUTATION,InvalidationReason::EXTERNAL_RESET,InvalidationReason::EXTERNAL_POWER_ON}) {
    Controller b;Driver d;available(d,b);LpiResult r;CHECK(d.startLpi(admitted(),r,io()).ok());
    const auto calls=b.calls,clocks=b.clocks;CHECK(d.invalidateDeviceState(why).ok());CHECK(b.calls==calls && b.clocks==clocks);
    CHECK(d.lpiSnapshot().forcedLpi==(why==InvalidationReason::EXTERNAL_MUTATION?ForcedLpi::UNCERTAIN:ForcedLpi::OFF));
  }
  Controller b;Driver d;available(d,b);LpiResult r;CHECK(d.startLpi(admitted(),r,io()).ok());
  b.failFrame=static_cast<int>(b.frames+4U);CHECK(!d.stopLpi(r,io()).ok());const auto failed=r;b.failFrame=-1;
  OperationToken token;CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(d,b,token).ok());
  CHECK(take(d,token).persistentRestored && d.lpiSnapshot().forcedLpi==ForcedLpi::OFF && failed.status.is(Err::MDIO_TIMEOUT));
  CHECK((b.ext[0x04D1]&0x1000U)==0U && !d.operationSnapshot().cleanupOwed);
  observations(d,b);CHECK(d.startLpi(admitted(),r,io()).ok());
  for(uint32_t frames=0;frames<9U;++frames) {
    const auto calls=b.calls;LpiResult sentinel;sentinel.control=0xCAFE;
    CHECK(d.stopLpi(sentinel,io(frames)).is(Err::OPERATION_TIMEOUT));
    CHECK(b.calls==calls && sentinel.control==0xCAFE && d.lpiSnapshot().forcedLpi==ForcedLpi::REQUESTED);
  }
  const auto calls=b.calls,clocks=b.clocks;
  CHECK(d.abandonCleanup().ok());CHECK(b.calls==calls && b.clocks==clocks);
  CHECK(d.lpiSnapshot().forcedLpi!=ForcedLpi::OFF && d.operationSnapshot().temporaryMayBeActive);
  const auto retained=d.lpiSnapshot();CHECK(d.end().ok());CHECK(retained.forcedLpi!=ForcedLpi::OFF);
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"traces",traces},{"admission",admission},{"failures",failures},
    {"timing",timing},{"ownership",ownership},{"recovery",recovery}};
  return test::runSuites(argc,argv,suites);
}
