#include "support/PartSelection.h"
#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target; };
// Literal SNLS647I control/result model. Hardware updates are deliberately not
// correlated with the software token or an invented silicon generation count.
struct Controller {
  std::array<uint16_t, 32> words{};
  std::array<uint16_t, 0x500> ext{};
  std::array<Wire, 4096> trace{};
  uint16_t selector{0}, address{0};
  uint64_t now{100};
  unsigned frames{0}, calls{0}, clocks{0}, releases{0}, mutations{0}, triggers{0}, hard{0}, progress{0};
  int failFrame{-1}, corruptFrame{-1}, releaseCall{-1}, lateCall{-1}, regressCall{-1}, overflowCall{-1};
  int updateFrame{-1}; uint16_t updateCdcr{0x0103}, corruptMask{0x0100};
  bool acceptedFailure{true}, locked{false};
  Driver* competitor{nullptr}; unsigned excluded{0};
  Controller() { defaults(); }
  void defaults() {
    words.fill(0); ext.fill(0); words[0]=0x3100; words[4]=0x01E1;
    words[25]=0x8001; words[22]=0x0100; words[30]=0x0102;
    ext[0x030E]=0x8400; ext[0x0170]=0x5C12; ext[0x0175]=0x1007;
    ext[0x0456]=8; ext[0x0180]=0x0078; ext[0x0185]=0x0011; ext[0x018A]=0;
  }
  Config config(uint32_t phy=1) {
    Config c; c.variant=test::declaredVariant; c.interface=MacInterface::RMII; c.phyAddress=phy;
    c.transport={this,execute,clock,{8,4,1}}; return c;
  }
  static uint64_t clock(void* context) noexcept { auto& b=*static_cast<Controller*>(context); ++b.clocks; return b.now; }
  uint16_t read(uint16_t target) {
    if(target==2) return 0x2000;
    if(target==3) return test::enhancedId2();
    if(target==0) words[0]&=0x7DFF;
    if(target==31) words[31]=0;
    if(target==30 && (words[30]&0x8000U)!=0U && triggers!=0U) {
      if(++progress==1U) return 0x8100;
      words[30]=0x0102;
    }
    CHECK(target!=0x0411 && target!=0x0416);
    CHECK(!(target>=0x0181 && target<=0x0184) && !(target>=0x0186 && target<=0x0189));
    return target<32U?words[target]:ext[target];
  }
  void write(uint8_t reg, uint16_t value) {
    if(reg==13) { CHECK(value==0x001F || value==0x401F); selector=value; return; }
    if(reg==14 && selector==0x001F) { address=value; return; }
    const uint16_t target=reg==14?address:uint16_t{reg}; ++mutations;
    if(target==31 && value==0x8000) { ++hard; defaults(); return; }
    if(target==30 && (value&0x8000U)!=0U) { ++triggers; progress=0; }
    if(target<32U) words[target]=value; else ext[target]=value;
  }
  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget& budget) noexcept {
    auto& b=*static_cast<Controller*>(context); BatchResult r;
    r.status=Status::Ok(); r.releaseStatus=Status::Ok(); r.controllerReusable=true;
    if(b.locked) { r.status=Status::Error(Err::BUSY,"Owned",static_cast<int32_t>(BusyDetail::OWNER_ADMISSION)); return r; }
    CHECK(batch.frameCount<=budget.maxFrames); b.locked=true; const auto call=b.calls++;
    for(uint8_t i=0;i<batch.frameCount;++i) {
      const auto& q=batch.frames[i]; auto& f=r.frames[i]; uint16_t value=q.value;
      if(q.readIndex!=NO_READ_INDEX) {
        CHECK(q.readIndex<i && r.frames[q.readIndex].valueValid);
        value=static_cast<uint16_t>((r.frames[q.readIndex].value&q.preserveMask)|q.value);
      }
      const uint16_t target=q.registerAddress==14 && b.selector==0x401F?b.address:uint16_t{q.registerAddress};
      CHECK(b.frames<b.trace.size()); const auto frame=b.frames++;
      b.trace[frame]={q.kind,q.registerAddress,value,target};
      if(static_cast<int>(frame)==b.updateFrame) {
        b.words[30]=b.updateCdcr; b.ext[0x0180]=0x0042; b.ext[0x0185]=0x007F; b.ext[0x018A]=0x0820;
      }
      const bool fail=static_cast<int>(frame)==b.failFrame; f.attempted=true;
      if(!fail || b.acceptedFailure) {
        if(q.kind==FrameKind::WRITE) b.write(q.registerAddress,value);
        else { f.value=b.read(target); if(static_cast<int>(frame)==b.corruptFrame) f.value^=b.corruptMask; }
      }
      b.now+=2;
      if(b.competitor!=nullptr) {
        uint16_t sentinel=0xBEEF;
        CHECK(b.competitor->readMmd(31,0x0180,sentinel,budget).is(Err::BUSY));
        CHECK(sentinel==0xBEEF); ++b.excluded;
      }
      if(fail) { f.effectPossible=b.acceptedFailure; r.status={Err::MDIO_TIMEOUT,-26026,"Injected frame"}; break; }
      f.completed=true; f.valueValid=q.kind==FrameKind::READ;
    }
    if(static_cast<int>(call)==b.releaseCall) { r.releaseStatus={Err::MDIO_ERROR,-37,"Injected release"}; r.controllerReusable=false; }
    if(static_cast<int>(call)==b.lateCall) b.now=budget.deadlineUs;
    if(static_cast<int>(call)==b.regressCall) b.now=0;
    if(static_cast<int>(call)==b.overflowCall) b.now=std::numeric_limits<uint64_t>::max();
    b.locked=false; ++b.releases; return r;
  }
};
OperationBudget budget(uint32_t frames=2048, uint64_t deadline=1000000) { return {{deadline,frames},10,8}; }
Status poll(Driver& d, Controller& b, OperationToken t, uint32_t slice=MAX_BATCH_FRAMES) {
  const auto snapshot=d.operationSnapshot();
  if(snapshot.nextPollValid && b.now<snapshot.nextPollAtUs) b.now=snapshot.nextPollAtUs;
  if(snapshot.busQuietValid && b.now<snapshot.busQuietUntilUs) b.now=snapshot.busQuietUntilUs;
  const auto calls=b.calls; const auto s=d.pollOperation(t,slice);
  CHECK(b.calls<=calls+1U && !b.locked); return s;
}
Status drain(Driver& d, Controller& b, OperationToken t) {
  Status s=Status::Error(Err::IN_PROGRESS,"Pending");
  for(unsigned n=0;n<512U && s.inProgress();++n) s=poll(d,b,t);
  CHECK(!s.inProgress()); return s;
}
OperationResult take(Driver& d, OperationToken t) { OperationResult r; CHECK(d.takeResult(t,r).ok()); return r; }
void ready(Driver& d, Controller& b) {
  CHECK(d.bind(b.config()).ok()); OperationToken t;
  CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress()); CHECK(drain(d,b,t).ok()); take(d,t);
  LinkSettings settings; settings.advertisement={0x0F,true,true};
  CHECK(d.startConfigure(settings,budget(),t).inProgress()); CHECK(drain(d,b,t).ok()); take(d,t);
}
OperationToken configure(Driver& d, Controller& b, bool enabled) {
  OperationToken t{999}; const auto calls=b.calls,clocks=b.clocks;
  CHECK(d.startConfigureCableAutorun({enabled,true},budget(),t).inProgress());
  CHECK(t.value!=999 && b.calls==calls && b.clocks==clocks); return t;
}
void configured(Driver& d, Controller& b, bool enabled) {
  const auto t=configure(d,b,enabled); CHECK(drain(d,b,t).ok()); const auto r=take(d,t);
  CHECK(r.cableAutorun.desiredValid && r.cableAutorun.desired==enabled);
  CHECK(r.cableAutorun.observedValid && r.cableAutorun.observed==enabled);
  CHECK(r.cableAutorun.trust==ConfigurationTrust::VERIFIED && !r.cleanupOwed);
}
OperationToken acquire(Driver& d, Controller& b) {
  OperationToken t{999};const auto calls=b.calls,clocks=b.clocks;
  CHECK(d.startCableAutorunAcquire(budget(),t).inProgress());
  CHECK(t.value!=999 && b.calls==calls && b.clocks==clocks); return t;
}
void noHistoryLoss(const EvidenceSnapshot& before,const EvidenceSnapshot& after) {
  for(unsigned i=0;i<static_cast<unsigned>(EvidenceSource::COUNT);++i) CHECK(before.samples[i].historyLost==after.samples[i].historyLost);
}

void policy() {
  for(const bool enabled:{false,true}) {
    Controller b; Driver d; ready(d,b); b.words[9]=enabled?0x00A2:0x03F3;
    b.words[16]=1; // Setting policy while linked does not require an artificial trigger.
    const auto begin=b.frames,calls=b.calls,writes=b.mutations; const auto health=d.healthSnapshot();
    const auto t=configure(d,b,enabled);CHECK(d.cableAutorunSnapshot().desired==enabled);
    CHECK(d.cableAutorunSnapshot().trust!=ConfigurationTrust::VERIFIED);
    CHECK(drain(d,b,t).ok()); const auto r=take(d,t);
    CHECK(b.frames-begin==10U && b.calls-calls==8U && b.mutations-writes==1U);
    CHECK(b.words[9]==(enabled?0x01A2:0x02F3));
    CHECK(b.trace[begin+7U].kind==FrameKind::READ && b.trace[begin+7U].reg==9);
    CHECK(b.trace[begin+8U].kind==FrameKind::WRITE && b.trace[begin+8U].reg==9);
    CHECK(b.trace[begin+9U].kind==FrameKind::READ && b.trace[begin+9U].reg==9);
    CHECK(r.status.ok() && !r.cleanupOwed && !r.temporaryMayBeActive && b.triggers==0U);
    CHECK(d.isInitialized() && d.settingsSnapshot().trust==ConfigurationTrust::VERIFIED);
    CHECK(d.healthSnapshot().totalSuccess-health.totalSuccess==8U);
    const auto view=d.cableAutorunSnapshot(); const auto cb=b.calls,ck=b.clocks;
    CHECK(view.desiredValid && view.observedValid && !view.observedStale && view.timeValid);
    CHECK(view.observed==enabled && view.trust==ConfigurationTrust::VERIFIED);
    CHECK(d.cableAutorunSnapshot().observed==view.observed && b.calls==cb && b.clocks==ck);
  }
  Controller b;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+9U);b.corruptMask=0x0020;
  configured(d,b,true); // Unowned readback changes are not policy mismatch.
}

void acquisition() {
  for(unsigned change=0;change<15U;++change) {
    Controller b;Driver d;ready(d,b);configured(d,b,true);
    const auto first=b.frames,calls=b.calls,writes=b.mutations;const auto health=d.healthSnapshot();
    if(change<14U) b.updateFrame=static_cast<int>(first+change);
    const auto t=acquire(d,b);CHECK(drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(b.frames-first==14U && b.calls-calls==5U && b.mutations==writes && b.triggers==0U);
    CHECK(r.cable.autorun && r.cable.recipe==CableRecipe::RAW && r.cable.requestedMask==1U);
    CHECK(r.cable.complete && r.cable.stale);const auto& s=r.cable.samples[0];
    CHECK(s.cdcr.valid && s.index.valid && s.amplitude.valid && s.peak.valid);
    CHECK(!s.triggerAccepted && !s.freshnessObserved);
    CHECK(s.index.timeValid && !s.index.stale);
    CHECK(r.cable.statusChanged==(change>0U && change<14U));
    for(unsigned row=1;row<6U;++row) CHECK(r.cable.samples[row].state==CableRunState::NOT_RUN);
    CableDiagnosis sentinel;sentinel.metres=999;CHECK(!interpretCable(r.cable,4.6,sentinel).ok() && sentinel.metres==999);
    CHECK(d.healthSnapshot().totalSuccess==health.totalSuccess);
    const auto cb=b.calls,ck=b.clocks;const auto one=d.cableSnapshot(),two=d.cableSnapshot();
    CHECK(one.result.complete && two.result.complete && !one.settingsValid && !two.settingsValid && b.calls==cb && b.clocks==ck);
    const auto again=acquire(d,b);CHECK(drain(d,b,again).ok());CHECK(take(d,again).cable.stale);
  }
  for(unsigned changed=1;changed<13U;++changed) {
    Controller b;Driver d;ready(d,b);b.updateFrame=static_cast<int>(b.frames+changed);b.updateCdcr=0x0102;
    const auto t=acquire(d,b);CHECK(drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(r.cable.complete && r.cable.stale && !r.cable.statusChanged);
    CHECK(!r.cable.samples[0].freshnessObserved && !r.cable.samples[0].triggerAccepted);
    CHECK(!d.cableAutorunSnapshot().desiredValid); // Raw capture never creates policy ownership.
  }
}

void failures() {
  for(unsigned phase=0;phase<10U;++phase) for(const bool accepted:{false,true}) {
    Controller b;Driver d;ready(d,b);const auto first=b.frames; b.failFrame=static_cast<int>(first+phase);b.acceptedFailure=accepted;
    const auto t=configure(d,b,true);CHECK(!drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(r.status.is(Err::MDIO_TIMEOUT) && r.status.detail==-26026 && b.frames-first==phase+1U);
    CHECK(d.cableAutorunSnapshot().desiredValid && d.cableAutorunSnapshot().desired);
    CHECK(d.cableAutorunSnapshot().trust!=ConfigurationTrust::VERIFIED);
    CHECK(r.cleanupOwed==(phase==9U || (phase==8U && accepted)));
  }
  for(unsigned phase=0;phase<14U;++phase) {
    Controller b;Driver d;ready(d,b);configured(d,b,true);const auto before=d.evidenceSnapshot();
    const auto first=b.frames;b.failFrame=static_cast<int>(first+phase);const auto t=acquire(d,b);
    CHECK(!drain(d,b,t).ok());const auto r=take(d,t);const auto& s=r.cable.samples[0];
    CHECK(r.status.is(Err::MDIO_TIMEOUT) && r.status.detail==-26026 && b.frames-first==phase+1U);
    CHECK(!r.cable.complete && r.cable.stale && !r.cleanupOwed);
    CHECK(r.cable.prerequisiteCdcr.valid==(phase>0U));
    CHECK(s.index.valid==(phase>4U));CHECK(s.amplitude.valid==(phase>8U));CHECK(s.peak.valid==(phase>12U));
    noHistoryLoss(before,d.evidenceSnapshot());b.failFrame=-1;
    const auto again=acquire(d,b);CHECK(drain(d,b,again).ok());CHECK(take(d,again).cable.complete && b.triggers==0U);
  }
  for(unsigned call=0;call<8U;++call) {
    Controller b;Driver d;ready(d,b);b.releaseCall=static_cast<int>(b.calls+call);
    const auto t=configure(d,b,true);CHECK(!drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(r.status.is(Err::MDIO_ERROR) && r.status.detail==-37 && !b.locked);
    CHECK(d.cableAutorunSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
  for(unsigned call=0;call<5U;++call) {
    Controller b;Driver d;ready(d,b);const auto before=d.evidenceSnapshot();b.releaseCall=static_cast<int>(b.calls+call);
    const auto t=acquire(d,b);CHECK(!drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(r.status.is(Err::MDIO_ERROR) && r.status.detail==-37);noHistoryLoss(before,d.evidenceSnapshot());
  }
  Controller b;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+9U);
  const auto t=configure(d,b,true);CHECK(drain(d,b,t).is(Err::CONFIG_MISMATCH));const auto r=take(d,t);
  CHECK(r.mismatch.valid && r.mismatch.mask==0x0100 && r.cleanupOwed);
}

void replay() {
  // An unowned flag adds no policy mutation/read tail to initialization/verify.
  {
    Controller b;Driver d;ready(d,b);OperationToken t;
    CHECK(!d.cableAutorunSnapshot().desiredValid);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    CHECK(b.trace[b.frames-1U].target==0 && !d.cableAutorunSnapshot().desiredValid);
    CHECK(d.startVerify(budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    CHECK(b.trace[b.frames-1U].target==0 && !d.cableAutorunSnapshot().desiredValid);
  }
  for(const auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
    Controller b;Driver d;ready(d,b);configured(d,b,true);const auto first=b.frames;OperationToken t;
    CHECK(d.startInitialize({reset},budget(),t).inProgress());CHECK(drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(r.persistentRestored && !r.cleanupOwed && d.cableAutorunSnapshot().trust==ConfigurationTrust::VERIFIED);
    CHECK((b.words[9]&0x0100U)!=0U && b.frames>first+3U);
    CHECK(b.trace[b.frames-3U].kind==FrameKind::READ && b.trace[b.frames-3U].reg==9);
    CHECK(b.trace[b.frames-2U].kind==FrameKind::WRITE && b.trace[b.frames-2U].reg==9);
    CHECK(b.trace[b.frames-1U].kind==FrameKind::READ && b.trace[b.frames-1U].reg==9);
  }
  for(unsigned fault=0;fault<3U;++fault) {
    Controller baseline;Driver probe;ready(probe,baseline);configured(probe,baseline,true);OperationToken t;
    const auto first=baseline.frames;CHECK(probe.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(drain(probe,baseline,t).ok());take(probe,t);
    const auto count=baseline.frames-first;
    Controller b;Driver d;ready(d,b);configured(d,b,true);b.failFrame=static_cast<int>(b.frames+count-3U+fault);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(!drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(!r.persistentRestored && r.cleanupOwed && !d.isInitialized() && d.cableAutorunSnapshot().trust!=ConfigurationTrust::VERIFIED);
    b.failFrame=-1;CHECK(d.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    CHECK(d.cableAutorunSnapshot().trust==ConfigurationTrust::VERIFIED);
  }
  // The two policy-tail callbacks are independently fallible/cancellable. A
  // later successful restoration cannot replace the retained first result.
  for(unsigned point=0;point<2U;++point) for(unsigned fault=0;fault<3U;++fault) {
    Controller baseline;Driver probe;ready(probe,baseline);configured(probe,baseline,true);OperationToken t;
    const auto first=baseline.calls;CHECK(probe.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(drain(probe,baseline,t).ok());take(probe,t);
    const auto count=baseline.calls-first;
    Controller b;Driver d;ready(d,b);configured(d,b,true);const auto target=b.calls+count-2U+point;
    if(fault==1U) b.releaseCall=static_cast<int>(target);
    if(fault==2U) b.lateCall=static_cast<int>(target);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),t).inProgress());
    if(fault==0U) {
      while(b.calls<target)CHECK(poll(d,b,t).inProgress());
      const auto before=b.calls;CHECK(d.cancel(t).ok() && b.calls==before);
    } else CHECK(!drain(d,b,t).ok());
    const auto retained=take(d,t);CHECK(retained.cleanupOwed && !retained.persistentRestored);
    CHECK(!d.isInitialized() && d.cableAutorunSnapshot().trust!=ConfigurationTrust::VERIFIED);
    if(point==1U && fault!=0U) {
      const auto evidence=d.cableAutorunSnapshot();
      CHECK(evidence.observedValid && evidence.observed && evidence.observedStale);
    }
    if(fault==0U)CHECK(retained.status.is(Err::CANCELLED));
    if(fault==1U)CHECK(retained.status.is(Err::MDIO_ERROR) && retained.status.detail==-37);
    if(fault==2U)CHECK(retained.status.is(Err::OPERATION_TIMEOUT));
    // A failed controller release needs application repair/rebinding, not an
    // automatic core retry. Other failures can explicitly replay on this bind.
    if(fault!=1U) {
      b.lateCall=-1;OperationToken recovery;CHECK(d.startInitialize({ResetKind::HARD},budget(2048,b.now+1000000U),recovery).inProgress());
      CHECK(drain(d,b,recovery).ok());CHECK(take(d,recovery).persistentRestored);
      CHECK(retained.status.is(fault==0U?Err::CANCELLED:Err::OPERATION_TIMEOUT));
    }
  }
  Controller b;Driver d;ready(d,b);configured(d,b,true);OperationToken t;
  const auto mutations=b.mutations;CHECK(d.startVerify(budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
  CHECK(b.mutations==mutations && b.trace[b.frames-1U].target==9);
  b.words[9]&=0xFEFF;CHECK(d.startVerify(budget(),t).inProgress());CHECK(drain(d,b,t).is(Err::CONFIG_MISMATCH));
  CHECK(take(d,t).mismatch.mask==0x0100 && d.cableAutorunSnapshot().trust!=ConfigurationTrust::VERIFIED);
  // Link-profile application preserves and verifies policy without rewriting it.
  Controller linked;Driver profile;ready(profile,linked);configured(profile,linked,true);
  LinkSettings changed;changed.advertisement.technologyMask=LINK_10_FULL;
  const auto first=linked.frames;CHECK(profile.startConfigure(changed,budget(),t).inProgress());CHECK(drain(profile,linked,t).ok());take(profile,t);
  CHECK(linked.trace[linked.frames-1U].target==9 && linked.trace[linked.frames-1U].kind==FrameKind::READ);
  CHECK((linked.words[9]&0x0100U)!=0U && profile.cableAutorunSnapshot().trust==ConfigurationTrust::VERIFIED);
  unsigned cr1Writes=0;
  for(unsigned i=first;i<linked.frames;++i) if(linked.trace[i].target==9 && linked.trace[i].kind==FrameKind::WRITE) ++cr1Writes;
  CHECK(cr1Writes==1U); // Only the complete profile's robust-routing write.
}

void handoff() {
  for(unsigned phase=0;phase<10U;++phase) {
    Controller b;Driver d;ready(d,b);configured(d,b,true);
    b.updateFrame=static_cast<int>(b.frames+phase);configured(d,b,false);
    CHECK((b.words[9]&0x0100U)==0U && b.words[30]==0x0103 && b.triggers==0U);
    // The update can occur anywhere during disable without becoming a claimed
    // idle/freshness proof. Manual still performs its own live preflight.
    CableSettings options;options.runMask=1;options.maintenanceAdmitted=options.peerQuiet=options.allowHardResetExit=true;
    OperationToken t;CHECK(d.startCableTest(options,budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    CHECK(b.triggers==1U && d.cableSnapshot().settingsValid);CHECK(d.stopCableTest(budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    CHECK(d.cableAutorunSnapshot().desiredValid && !d.cableAutorunSnapshot().observed && !d.operationSnapshot().cleanupOwed);
  }
  for(unsigned phase=0;phase<10U;++phase) {
    Controller bus;Driver driver;ready(driver,bus);configured(driver,bus,true);
    const auto writes=bus.mutations;bus.updateFrame=static_cast<int>(bus.frames+phase);bus.updateCdcr=0x8102;
    const auto token=configure(driver,bus,false);const auto status=drain(driver,bus,token);take(driver,token);
    CHECK(status.ok()==(phase>=7U));
    CHECK(bus.mutations-writes==(phase>=7U?1U:0U));
    CHECK(bus.triggers==0U); // Disable never claims or performs engine abort.
    if(phase>=7U) {
      CableSettings s;s.runMask=1;s.maintenanceAdmitted=s.peerQuiet=s.allowHardResetExit=true;OperationToken manual;
      CHECK(driver.startCableTest(s,budget(),manual).inProgress());CHECK(!drain(driver,bus,manual).ok());take(driver,manual);
      CHECK(bus.triggers==0U);
    }
  }
  Controller b;Driver d;ready(d,b);configured(d,b,true);CableSettings options;
  options.runMask=1;options.maintenanceAdmitted=options.peerQuiet=options.allowHardResetExit=true;OperationToken t;
  const auto writes=b.mutations;CHECK(d.startCableTest(options,budget(),t).inProgress());CHECK(!drain(d,b,t).ok());take(d,t);
  CHECK(b.mutations==writes && b.triggers==0U);
  configured(d,b,false);b.words[30]=0x8102;
  CHECK(d.startCableTest(options,budget(),t).inProgress());CHECK(!drain(d,b,t).ok());take(d,t);CHECK(b.triggers==0U);
}

void lifecycle() {
  for(unsigned boundary=0;boundary<8U;++boundary) {
    Controller b;Driver d;ready(d,b);const auto t=configure(d,b,true);
    for(unsigned n=0;n<boundary;++n) CHECK(poll(d,b,t).inProgress());
    const auto calls=b.calls,clocks=b.clocks;CHECK(d.cancel(t).ok());CHECK(b.calls==calls && b.clocks==clocks);
    CHECK(d.bind(b.config()).is(Err::BUSY));const auto r=take(d,t);CHECK(r.status.is(Err::CANCELLED));
    OperationResult sentinel;sentinel.status={Err::UNSUPPORTED,77,"Sentinel"};CHECK(d.takeResult(t,sentinel).is(Err::RESULT_NOT_AVAILABLE));CHECK(sentinel.status.detail==77);
    CHECK(r.cleanupOwed==(boundary==7U));
    if(r.cleanupOwed) { OperationToken next;CHECK(d.startConfigureCableAutorun({false,true},budget(),next).is(Err::BUSY)); }
  }
  for(unsigned boundary=0;boundary<5U;++boundary) {
    Controller b;Driver d;ready(d,b);const auto t=acquire(d,b);
    for(unsigned n=0;n<boundary;++n) CHECK(poll(d,b,t).inProgress());
    const auto calls=b.calls;CHECK(d.cancel(t).ok() && b.calls==calls);CHECK(take(d,t).status.is(Err::CANCELLED));
  }
  Controller b;Driver d;OperationToken sentinel{999};
  CHECK(d.startConfigureCableAutorun({true,false},budget(),sentinel).is(Err::INVALID_CONFIG));
  CHECK(d.startConfigureCableAutorun({true,true},budget(),sentinel).is(Err::NOT_BOUND));
  CHECK(d.bind(b.config()).ok());CHECK(d.startConfigureCableAutorun({true,true},budget(),sentinel).is(Err::NOT_INITIALIZED));
  CHECK(sentinel.value==999 && b.calls==0U && b.clocks==0U);
  ready(d,b);configured(d,b,true);CHECK(d.invalidateDeviceState(InvalidationReason::EXTERNAL_POWER_ON).ok());
  CHECK(d.cableAutorunSnapshot().desiredValid && d.cableAutorunSnapshot().desired && d.cableAutorunSnapshot().observedStale);
  CHECK(d.cableAutorunSnapshot().trust!=ConfigurationTrust::VERIFIED);CHECK(d.end().ok());CHECK(d.bind(b.config()).ok());
  CHECK(!d.cableAutorunSnapshot().desiredValid);
  // A later zero is only a new raw observation, not proof that the required
  // calibration clear-then-HARD recovery occurred. Keep the source gate sticky.
  for(const auto external:{InvalidationReason::EXTERNAL_RESET,InvalidationReason::EXTERNAL_POWER_ON}) {
    Controller bus;Driver driver;ready(driver,bus);configured(driver,bus,false);
    bus.words[30]=0x4002;bus.updateFrame=static_cast<int>(bus.frames+1U);bus.updateCdcr=0x0102;
    const auto capture=acquire(driver,bus);CHECK(drain(driver,bus,capture).ok());const auto retained=take(driver,capture);
    CHECK(retained.cable.prerequisiteCdcr.raw==0x4002 && retained.cable.samples[0].cdcr.raw==0x0102);
    const auto again=acquire(driver,bus);CHECK(drain(driver,bus,again).ok());take(driver,again);
    const auto callbacks=bus.calls,clockReads=bus.clocks;OperationToken unchanged{999};
    for(const auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
      const auto status=driver.startInitialize({reset},budget(),unchanged);
      CHECK(status.is(Err::INVALID_CONFIG) && status.detail==static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
    }
    CableSettings manual;manual.runMask=1;manual.maintenanceAdmitted=manual.peerQuiet=manual.allowHardResetExit=true;
    const auto manualStatus=driver.startCableTest(manual,budget(),unchanged);
    const auto policyStatus=driver.startConfigureCableAutorun({true,true},budget(),unchanged);
    CHECK(manualStatus.is(Err::INVALID_CONFIG) && manualStatus.detail==static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
    CHECK(policyStatus.is(Err::INVALID_CONFIG) && policyStatus.detail==static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
    CHECK(unchanged.value==999 && bus.calls==callbacks && bus.clocks==clockReads);
    CHECK(!driver.cableAutorunSnapshot().desired); // Rejected admission cannot change intent.
    CHECK(driver.invalidateDeviceState(external).ok() && bus.calls==callbacks && bus.clocks==clockReads);
    OperationToken recovery;CHECK(driver.startInitialize({ResetKind::NONE},budget(),recovery).inProgress());
    CHECK(drain(driver,bus,recovery).ok());CHECK(take(driver,recovery).persistentRestored);
    configured(driver,bus,true);
  }
}

void timing() {
  Controller b;Driver d;ready(d,b);OperationToken sentinel{999};const auto calls=b.calls,clocks=b.clocks;
  CHECK(d.startConfigureCableAutorun({true,true},budget(0),sentinel).is(Err::OPERATION_TIMEOUT));
  CHECK(d.startCableAutorunAcquire(budget(0),sentinel).is(Err::OPERATION_TIMEOUT));
  CHECK(sentinel.value==999 && b.calls==calls && b.clocks==clocks);
  const auto t=configure(d,b,true);CHECK(d.pollOperation(t,1).inProgress() && b.calls==calls);
  for(unsigned n=0;n<6U;++n) CHECK(poll(d,b,t).inProgress());
  const auto before=b.frames;CHECK(d.pollOperation(t,1).inProgress() && b.frames==before);CHECK(drain(d,b,t).ok());take(d,t);
  const auto a=acquire(d,b);CHECK(poll(d,b,a).inProgress());const auto frame=b.frames;
  CHECK(d.pollOperation(a,3).inProgress() && b.frames==frame);CHECK(drain(d,b,a).ok());take(d,a);
  for(unsigned fault=0;fault<3U;++fault) {
    Controller bus;Driver driver;ready(driver,bus);bus.lateCall=fault==0?static_cast<int>(bus.calls+6U):-1;
    bus.regressCall=fault==1?static_cast<int>(bus.calls+6U):-1;bus.overflowCall=fault==2?static_cast<int>(bus.calls+6U):-1;
    const auto token=configure(driver,bus,true);CHECK(!drain(driver,bus,token).ok());const auto r=take(driver,token);
    CHECK(r.cleanupOwed && driver.cableAutorunSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
  // Valid reset/restart observations remain evidence when release or timing
  // fails after the read; preserve the primary error instead of semantic error.
  for(const uint16_t reset:{uint16_t{0x4000},uint16_t{0x8000}}) for(unsigned fault=0;fault<3U;++fault) {
    Controller bus;Driver driver;ready(driver,bus);const auto first=bus.frames,callsBefore=bus.calls;
    bus.corruptFrame=static_cast<int>(first+3U);bus.corruptMask=reset;
    if(fault==1U)bus.releaseCall=static_cast<int>(callsBefore+2U);
    if(fault==2U)bus.lateCall=static_cast<int>(callsBefore+2U);
    const auto token=configure(driver,bus,true);CHECK(!drain(driver,bus,token).ok());const auto r=take(driver,token);
    CHECK(bus.frames-first==4U && !r.mutationCompleted && !r.mutationPossible);
    CHECK(driver.isInitialized()==(reset==0x4000));
    if(fault==0U)CHECK(r.status.is(Err::INVALID_CONFIG));
    if(fault==1U)CHECK(r.status.is(Err::MDIO_ERROR) && r.status.detail==-37);
    if(fault==2U)CHECK(r.status.is(Err::OPERATION_TIMEOUT));
  }
  Controller frozen;Driver stalled;ready(stalled,frozen);const auto token=configure(stalled,frozen,true);const auto cb=frozen.calls;
  for(unsigned n=0;n<10U && stalled.operationSnapshot().state==OperationState::RUNNING;++n) stalled.pollOperation(token,0);
  CHECK(take(stalled,token).status.is(Err::OPERATION_TIMEOUT) && frozen.calls==cb);
  Controller owned;Driver one,two;ready(one,owned);CHECK(two.bind(owned.config(2)).ok());owned.competitor=&two;
  configured(one,owned,true);const auto raw=acquire(one,owned);CHECK(drain(one,owned,raw).ok());take(one,raw);
  CHECK(owned.excluded>0U && !owned.locked && !two.cableAutorunSnapshot().desiredValid);
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"policy",policy},{"acquisition",acquisition},{"failures",failures},
    {"replay",replay},{"handoff",handoff},{"lifecycle",lifecycle},{"timing",timing}};
  return test::runVariantSuites(argc,argv,suites);
}
