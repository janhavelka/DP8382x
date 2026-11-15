#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include "../examples/portable/Electrical.h"
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target, devad; uint64_t at; };

// Independent SNLS647I tables8-19/40/46/50/76 and SNLA266B5-1/2/4/5 literals.
// The fake models consumption, not physical latency, thresholds or SNR.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<Wire,8192> trace{};
  uint16_t selector{0},address{0},id2{0xA131};
  MacInterface boardInterface{MacInterface::MII};
  uint64_t now{100};
  unsigned frames{0},calls{0},clocks{0},releases{0},mutations{0},hard{0};
  std::array<unsigned,2048> firstFrame{};
  int failFrame{-1},releaseCall{-1},corruptFrame{-1},lateCall{-1},regressCall{-1},invalidCall{-1},badReceiptCall{-1};
  uint16_t corruptMask{1};
  bool acceptedFailure{true},locked{false},stickyHard{false},exposePcs{false};
  uint64_t frameTime{2};
  Driver* competitor{nullptr}; unsigned excluded{0};
  Controller() { defaults(); }
  void defaults() {
    words.fill(0);ext.fill(0);words[0]=0x3100;words[4]=0x01E1;
    words[1]=0x0024;words[5]=0x0101;words[6]=1;words[16]=0x15;
    words[25]=0x8001;words[22]=0x0100;words[30]=0x0102;words[23]=boardInterface==MacInterface::MII?0x0041:0x0061;
    ext[0x0303]=ext[0x0304]=ext[0x0306]=8;ext[0x0305]=0x000B;
    ext[0x117]=0x8147;ext[0x131]=0x2284;ext[0x2A]=0x7998;ext[0x030E]=0x8400;ext[0x04A1]=0x1000;ext[0x04D1]=0x0080;
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=Variant::DP83826I;c.interface=boardInterface;c.phyAddress=phy;
    c.transport={this,execute,clock,{8,4,1}};return c;
  }
  static uint64_t clock(void* context) noexcept { auto& b=*static_cast<Controller*>(context);++b.clocks;return b.now; }
  uint16_t read(uint16_t target,uint16_t devad=31) {
    CHECK(devad==31);
    if(target==2)return 0x2000;
    if(target==3)return id2;
    if(target==0 && exposePcs) { exposePcs=false;return static_cast<uint16_t>(words[0]|0x8000U); }
    if(target==0) { words[0]&=0x7DFFU;return words[0]; }
    if(target==31)words[31]=stickyHard?0x8000:0;
    if(target==0x0308)return static_cast<uint16_t>((ext[0x0302]&0x0040U)!=0U?1U:0U);
    const auto value=target<32U?words[target]:ext[target];
    if(target==18 || target==19) { words[target]&=0x00FF;words[16]&=0xFF7F; }
    if(target==20) { words[20]&=0xFF00;words[16]&=0xF7FF; }
    if(target==21) { words[21]=0;words[16]&=0xDFFF; }
    if(target==23)words[23]&=0xFFF3;
    if(target==15)words[15]&=0xFE0F;
    if(target==26) { words[26]&=0xFFEF;words[16]&=0xEFFF; }
    if(target==6) { words[6]&=0xFFFD;words[16]&=0xFEFF; }
    if(target==0x04A1)ext[target]&=0xFF00;
    return value;
  }
  void write(uint8_t reg,uint16_t value) {
    if(reg==13) { CHECK((value&0xBFE0U)==0U && (value&31U)==31U);selector=value;return; }
    if(reg==14 && (selector&0x4000U)==0U) { address=value;return; }
    const uint16_t target=reg==14?address:uint16_t{reg};++mutations;
    const uint16_t devad=reg==14?static_cast<uint16_t>(selector&31U):uint16_t{31};
    CHECK(devad==31);
    if(target==31 && value==0x8000) { ++hard;defaults();return; }
    if(target==0 && value==0x8000) { words[0]=0x3100;words[9]=words[10]=words[17]=0;words[23]=boardInterface==MacInterface::MII?0x0041:0x0061;return; }
    if(target==26)CHECK((value&0x0010U)==0U);
    if(target==23)CHECK((value&0x000CU)==0U);
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
    if(b.locked) { r.status={Err::BUSY,static_cast<int32_t>(BusyDetail::OWNER_ADMISSION),"Owned"};return r; }
    CHECK(batch.frameCount<=budget.maxFrames);b.locked=true;const auto call=b.calls++;CHECK(call<b.firstFrame.size());b.firstFrame[call]=b.frames;
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
      if(b.competitor!=nullptr) {
        uint16_t sentinel=0xCAFE;CHECK(b.competitor->readMmd(31,0x0467,sentinel,budget).is(Err::BUSY));CHECK(sentinel==0xCAFE);++b.excluded;
      }
      if(failed) { f.effectPossible=b.acceptedFailure;r.status={Err::MDIO_TIMEOUT,-33033,"Injected frame"};break; }
      f.completed=true;f.valueValid=q.kind==FrameKind::READ;
    }
    if(static_cast<int>(call)==b.releaseCall) { r.releaseStatus={Err::MDIO_ERROR,-37,"Injected release"};r.controllerReusable=false; }
    if(static_cast<int>(call)==b.lateCall)b.now=budget.deadlineUs;
    if(static_cast<int>(call)==b.regressCall)b.now=0;
    if(static_cast<int>(call)==b.invalidCall)r.status={Err::IN_PROGRESS,-33034,"Illegal terminal status"};
    if(static_cast<int>(call)==b.badReceiptCall)r.frames[0].attempted=false;
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
  b.words[23]=b.boardInterface==MacInterface::MII?0x0041:0x0061;
  CHECK(d.bind(b.config()).ok());OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}
void ready(Driver& d,Controller& b) {
  initialized(d,b);LinkSettings profile;profile.advertisement.technologyMask=8;
  OperationToken t;CHECK(d.startConfigure(profile,{},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}

ComplianceOptions options(ComplianceMode mode=ComplianceMode::TX100,uint8_t pattern=0) {
  auto value=admittedTx100Fixture();value.mode=mode;value.pattern=pattern;return value;
}
OperationToken start(Driver& d,Controller& b,const ComplianceOptions& value) {
  OperationToken t;const auto frames=b.frames,clocks=b.clocks;
  CHECK(d.startCompliance(value,budget(),t).inProgress());CHECK(b.frames==frames && b.clocks==clocks);return t;
}
OperationResult complete(Driver& d,Controller& b,OperationToken t) {
  const auto status=drain(d,b,t);
  if(!status.ok())std::fprintf(stderr,"electrical finish %d/%d: %s\n",static_cast<int>(status.code),status.detail,status.msg);
  CHECK(status.ok());return take(d,t);
}
bool semantic(const Wire& w) { return w.kind==FrameKind::WRITE && w.reg!=13 && !(w.reg==14 && w.target==14); }
unsigned writes(const Controller& b,unsigned first,uint16_t target) {
  unsigned count=0;for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==target)++count;return count;
}
bool sessionWriteAttempted(const Controller& b,unsigned first) {
  for (unsigned i=first;i<b.frames;++i) {
    if (semantic(b.trace[i]) && b.trace[i].target!=0x1FU) { return true; }
  }
  return false;
}
void vectors() {
  for(const auto mode:{ComplianceMode::TX100,ComplianceMode::NLP10,ComplianceMode::PRBS10,ComplianceMode::PATTERN10}) {
    const unsigned patterns=mode==ComplianceMode::PATTERN10?14U:1U;
    for(unsigned code=0;code<patterns;++code)for(const auto pair:{PairSelection::MDI,PairSelection::MDIX}) {
      Controller b;Driver d;ready(d,b);auto value=options(mode,static_cast<uint8_t>(code));value.pair=pair;
      const auto first=b.frames;const auto t=start(d,b,value);const auto result=complete(d,b,t);
      CHECK(result.electrical.controlsVerified && result.cleanupOwed && result.temporaryMayBeActive);
      CHECK(d.electricalSnapshot().complianceSession && !d.isInitialized());
      CHECK(b.hard==1U && (b.words[9]&0x20U)!=0U);
      CHECK((b.words[25]&0xC000U)==(pair==PairSelection::MDI?0U:0x4000U));
      CHECK((b.words[25]&0x3FFFU)==1U); // No script0021/4021 imports.
      CHECK((b.words[0]&0x3F00U)==(mode==ComplianceMode::TX100?0x2100U:0x0100U));
      CHECK(writes(b,first,0x030E)==1U && b.ext[0x030E]==0x4A40);
      if(mode==ComplianceMode::PRBS10)CHECK((b.words[22]&0x711FU)==0x7001U);
      if(mode==ComplianceMode::PATTERN10)CHECK((b.ext[0x27]&0x1FU)==(0x10U|code));
      unsigned hardWrites=0,restartWrites=0;
      for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i])) {
        const auto& w=b.trace[i];if(w.target==0)CHECK((w.value&0x0C00U)==0U);
        if(w.target==0x1F) { CHECK(w.value==0x8000 || w.value==0x4000);hardWrites+=w.value==0x8000?1U:0U;restartWrites+=w.value==0x4000?1U:0U; }
        if(w.target==0x19)CHECK((w.value&0x003FU)==1U);
        if(w.target==0x16)CHECK(w.value==0x7001);
      }
      CHECK(hardWrites==1U && restartWrites==(mode==ComplianceMode::PATTERN10?0U:1U));
      const auto cached=b.frames;d.electricalSnapshot();d.electricalSnapshot();CHECK(b.frames==cached);
      OperationToken stop;CHECK(stopElectricalFixture(d,budget(),stop).inProgress());const auto stopped=complete(d,b,stop);
      CHECK(stopped.electrical.stopped && stopped.electrical.resetComplete && stopped.cleanupOwed && !stopped.temporaryMayBeActive);
      CHECK(!d.isInitialized() && b.hard==2U);
      OperationToken restore;CHECK(d.startInitialize({ResetKind::NONE},budget(),restore).inProgress());const auto restored=complete(d,b,restore);
      CHECK(restored.persistentRestored && !restored.cleanupOwed && d.isInitialized());
    }
  }
}
void invalid() {
  Controller b;Driver d;ready(d,b);const auto frames=b.frames,clocks=b.clocks;OperationToken t;
  CollisionOptions missing{LinkSpeed::MBPS_100,true,true,true,false};
  CHECK(d.startCollisionTest(missing,budget(),t).is(Err::INVALID_CONFIG));
  CHECK(d.startCalibrationRecovery({true,true,false},budget(),t).is(Err::INVALID_CONFIG));
  for(uint8_t count=0;count<32U;++count) {
    const auto status=d.startCompliance(options(ComplianceMode::COUNT100,count),budget(),t);
    CHECK(status.is(Err::INVALID_CONFIG) && status.detail==static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
  }
  CHECK(d.startCompliance(options(ComplianceMode::COUNT100,32),budget(),t).is(Err::INVALID_PARAM));
  for(uint8_t code=14;code<17U;++code)CHECK(d.startCompliance(options(ComplianceMode::PATTERN10,code),budget(),t).is(Err::INVALID_PARAM));
  for(unsigned bit=0;bit<4U;++bit) {
    auto value=options();if(bit==0)value.macStopped=false;if(bit==1)value.maintenanceAdmitted=false;
    if(bit==2) { value.fixtureAdmitted=false; } if(bit==3) { value.hardResetAdmitted=false; }
    CHECK(d.startCompliance(value,budget(),t).is(Err::INVALID_CONFIG));
  }
  auto value=options();value.pair=PairSelection::AUTOMATIC;CHECK(d.startCompliance(value,budget(),t).is(Err::INVALID_PARAM));
  value=options();value.mode=static_cast<ComplianceMode>(99);CHECK(d.startCompliance(value,budget(),t).is(Err::INVALID_PARAM));
  CHECK(d.startCompliance(options(),budget(0),t).is(Err::OPERATION_TIMEOUT));
  CHECK(d.stopCompliance(budget(),t).is(Err::INVALID_CONFIG));CHECK(d.stopCollisionTest(budget(),t).is(Err::INVALID_CONFIG));
  CHECK(b.frames==frames && b.clocks==clocks);
  b.words[0]|=0x0800;const auto startToken=start(d,b,options());CHECK(drain(d,b,startToken).is(Err::INVALID_CONFIG));
  const auto r=take(d,startToken);CHECK(!r.mutationAttempted && !r.cleanupOwed);
}
void failures() {
  for(const auto mode:{ComplianceMode::TX100,ComplianceMode::PRBS10,ComplianceMode::PATTERN10}) {
    Controller baseline;Driver reference;ready(reference,baseline);const auto first=baseline.frames;
    complete(reference,baseline,start(reference,baseline,options(mode,mode==ComplianceMode::PATTERN10?3:0)));
    const auto count=baseline.frames-first;
    for(unsigned offset=0;offset<count;++offset) {
      Controller b;Driver d;ready(d,b);const auto base=b.frames;b.failFrame=static_cast<int>(base+offset);
      const auto t=start(d,b,options(mode,mode==ComplianceMode::PATTERN10?3:0));CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));
      const auto r=take(d,t);CHECK(r.status.detail==-33033 && b.frames==base+offset+1U);
      CHECK(b.calls==b.releases && !b.locked);
      CHECK(r.temporaryMayBeActive==sessionWriteAttempted(b,base));
      if(r.mutationPossible) { CHECK(r.cleanupOwed);CHECK(d.end().is(Err::BUSY)); }
      CHECK(!r.electrical.controlsVerified);
      if(d.operationSnapshot().busQuietValid && b.now<d.operationSnapshot().busQuietUntilUs) { const auto at=b.frames;uint16_t sentinel=0xCAFE;CHECK(!d.readClause22(2,sentinel,{b.now+1000,1}).ok());CHECK(b.frames==at && sentinel==0xCAFE); }
    }
  }
  // Readback mismatch and malformed/late callbacks never publish verified stimulus.
  Controller correct;Driver good;ready(good,correct);const auto offsetStart=correct.frames,callStart=correct.calls;
  complete(good,correct,start(good,correct,options(ComplianceMode::PRBS10)));
  const auto lastRead=correct.frames-offsetStart-1U,callCount=correct.calls-callStart;
  for(unsigned fault=0;fault<5U;++fault) {
    Controller b;Driver d;ready(d,b);
    if(fault==0U) { b.corruptFrame=static_cast<int>(b.frames+lastRead);b.corruptMask=0x0001; }
    if(fault==1U)b.invalidCall=static_cast<int>(b.calls+callCount-1U);
    if(fault==2U)b.badReceiptCall=static_cast<int>(b.calls+callCount-1U);
    if(fault==3U)b.lateCall=static_cast<int>(b.calls+callCount-1U);
    if(fault==4U)b.releaseCall=static_cast<int>(b.calls+callCount-1U);
    const auto t=start(d,b,options(ComplianceMode::PRBS10));CHECK(!drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(r.cleanupOwed && r.temporaryMayBeActive && !r.electrical.controlsVerified);
    if(fault==0U)CHECK(r.status.is(Err::CONFIG_MISMATCH));
  }
  // Independent stop failures retain the earlier successful start result.
  Controller baseline;Driver reference;ready(reference,baseline);complete(reference,baseline,start(reference,baseline,options()));
  const auto first=baseline.frames;OperationToken stop;CHECK(reference.stopCompliance(budget(),stop).inProgress());complete(reference,baseline,stop);
  for(unsigned offset=0;offset<baseline.frames-first;++offset) {
    Controller b;Driver d;ready(d,b);const auto earlier=complete(d,b,start(d,b,options()));const auto base=b.frames;
    b.failFrame=static_cast<int>(base+offset);CHECK(d.stopCompliance(budget(),stop).inProgress());CHECK(drain(d,b,stop).is(Err::MDIO_TIMEOUT));
    const auto failed=take(d,stop);CHECK(earlier.status.ok() && failed.cleanupOwed && failed.temporaryMayBeActive && b.frames==base+offset+1U);
  }
  Controller replay;Driver restored;ready(restored,replay);complete(restored,replay,start(restored,replay,options()));
  OperationToken restore;CHECK(restored.stopCompliance(budget(),restore).inProgress());complete(restored,replay,restore);
  const auto restoreFirst=replay.frames;CHECK(restored.startInitialize({ResetKind::NONE},budget(),restore).inProgress());complete(restored,replay,restore);
  for(unsigned phase=0;phase<replay.frames-restoreFirst;++phase) {
    Controller b;Driver d;ready(d,b);const auto session=complete(d,b,start(d,b,options()));
    CHECK(d.stopCompliance(budget(),restore).inProgress());const auto stopped=complete(d,b,restore);
    b.failFrame=static_cast<int>(b.frames+phase);CHECK(d.startInitialize({ResetKind::NONE},budget(),restore).inProgress());
    CHECK(drain(d,b,restore).is(Err::MDIO_TIMEOUT));const auto failed=take(d,restore);
    CHECK(session.status.ok() && stopped.status.ok() && failed.cleanupOwed && !failed.persistentRestored);
  }
}
void stopAfterExistingReset();
void externalResetReconciliation();
void resetBeforeStimulus();
void cancellation() {
  stopAfterExistingReset();
  externalResetReconciliation();
  resetBeforeStimulus();
  Controller baseline;Driver reference;ready(reference,baseline);const auto calls=baseline.calls;complete(reference,baseline,start(reference,baseline,options(ComplianceMode::PRBS10)));
  for(unsigned boundary=0;boundary<baseline.calls-calls;++boundary) {
    Controller b;Driver d;ready(d,b);const auto first=b.frames;const auto t=start(d,b,options(ComplianceMode::PRBS10));
    for(unsigned i=0;i<boundary;++i)CHECK(poll(d,b,t).inProgress());
    const auto frames=b.frames,clocks=b.clocks;CHECK(d.cancel(t).ok());CHECK(b.frames==frames && b.clocks==clocks);
    const auto r=take(d,t);CHECK(r.status.is(Err::CANCELLED));
    CHECK(r.temporaryMayBeActive==sessionWriteAttempted(b,first));
    if(r.mutationPossible)CHECK(r.cleanupOwed);
    OperationResult again;CHECK(d.takeResult(t,again).is(Err::RESULT_NOT_AVAILABLE));
  }
}
void stopAfterExistingReset() {
  Controller b;Driver d;ready(d,b);complete(d,b,start(d,b,options(ComplianceMode::PATTERN10,3)));
  b.exposePcs=true;const auto first=b.frames;OperationToken stop;CHECK(d.stopCompliance(budget(),stop).inProgress());
  const auto r=complete(d,b,stop);CHECK(r.commandAccepted && r.protocolComplete && writes(b,first,0x1F)==1U && b.ext[0x27]==0);
  Controller idle;Driver driver;ready(driver,idle);const auto t=start(driver,idle,options());
  CHECK(driver.cancel(t).ok());take(driver,t);CHECK(!driver.electricalSnapshot().complianceSession);
  LinkSettings profile;profile.advertisement.technologyMask=8;OperationToken apply;
  CHECK(driver.startConfigure(profile,{},budget(),apply).inProgress());const auto applied=complete(driver,idle,apply);
  CHECK(!applied.temporaryMayBeActive && !applied.cleanupOwed);
}
void resetBeforeStimulus() {
  for (bool fail : {false,true}) {
    Controller b; Driver d; ready(d,b); const auto first=b.frames;
    const auto token=start(d,b,options());
    for (unsigned n=0;n<32U && d.operationSnapshot().phase!=OperationPhase::COMMAND;++n) {
      CHECK(poll(d,b,token).inProgress());
    }
    CHECK(d.operationSnapshot().phase==OperationPhase::COMMAND);
    if (fail) { b.failFrame=static_cast<int>(b.frames); }
    const auto status=poll(d,b,token);
    CHECK(fail?status.is(Err::MDIO_TIMEOUT):status.inProgress());
    CHECK(writes(b,first,0x1F)==1U && !sessionWriteAttempted(b,first));
    CHECK(d.operationSnapshot().busQuietValid && !d.operationSnapshot().temporaryMayBeActive);
    if (!fail) { CHECK(d.cancel(token).ok()); }
    const auto result=take(d,token);
    CHECK(result.cleanupOwed && result.mutationPossible && !result.temporaryMayBeActive);
    CHECK(!result.electrical.complianceSession && !d.electricalSnapshot().complianceSession);
    // The pending reset still owns its floor and cleanup, but no stimulus has
    // started. Explicit NONE reconciliation can observe completion and replay.
    b.failFrame=-1; OperationToken restore;
    CHECK(d.startInitialize({ResetKind::NONE},budget(),restore).inProgress());
    const auto before=b.frames; CHECK(d.pollOperation(restore).inProgress());
    CHECK(b.frames==before);
    CHECK(complete(d,b,restore).persistentRestored && b.hard==1U);
  }
}
void externalResetReconciliation() {
  for (const auto reason : {InvalidationReason::EXTERNAL_RESET, InvalidationReason::EXTERNAL_POWER_ON}) {
    // A reported physical reset clears current HARD debt, never the frozen failure.
    Controller b; Driver d; ready(d,b); b.words[30]=0x4102; OperationToken token;
    b.failFrame=static_cast<int>(b.frames+1U);
    CHECK(d.startCalibrationRecovery({true,true,true},budget(),token).inProgress());
    CHECK(drain(d,b,token).is(Err::MDIO_TIMEOUT));
    CHECK(d.electricalSnapshot().hardResetOwed && d.operationSnapshot().cleanupOwed);
    b.defaults(); b.failFrame=-1;
    const auto frames=b.frames,clocks=b.clocks;
    CHECK(d.invalidateDeviceState(reason).ok());
    CHECK(b.frames==frames && b.clocks==clocks);
    CHECK(!d.electricalSnapshot().hardResetOwed && !d.operationSnapshot().cleanupOwed);
    const auto failed=take(d,token);
    CHECK(failed.status.is(Err::MDIO_TIMEOUT) && failed.electrical.hardResetOwed && failed.cleanupOwed);
    CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());
    CHECK(complete(d,b,token).persistentRestored && b.hard==0U);

    for (bool collisionSession : {false,true}) {
      Controller active; Driver session; ready(session,active); OperationToken started;
      if (collisionSession) {
        CHECK(session.startCollisionTest({LinkSpeed::MBPS_100,true,true,true,true},budget(),started).inProgress());
      } else { started=start(session,active,options(ComplianceMode::PATTERN10,3)); }
      CHECK(drain(session,active,started).ok());
      CHECK(session.electricalSnapshot().temporaryMayBeActive);
      active.defaults();
      const auto before=active.frames,beforeClock=active.clocks;
      CHECK(session.invalidateDeviceState(reason).ok());
      CHECK(active.frames==before && active.clocks==beforeClock);
      const auto current=session.electricalSnapshot();
      CHECK(!current.complianceSession && !current.collisionSession && !current.temporaryMayBeActive);
      CHECK(!current.controlsVerified && !session.operationSnapshot().cleanupOwed);
      const auto historical=take(session,started);
      CHECK(historical.status.ok() && historical.temporaryMayBeActive && historical.cleanupOwed);
      CHECK(collisionSession?historical.electrical.collisionSession:historical.electrical.complianceSession);
      CHECK(session.startInitialize({ResetKind::NONE},budget(),started).inProgress());
      CHECK(complete(session,active,started).persistentRestored);
    }
  }
}
void collision() {
  for(const auto speed:{LinkSpeed::MBPS_10,LinkSpeed::MBPS_100})for(bool an:{false,true})for(const uint16_t mux:std::array<uint16_t,3>{{3,5,6}}) {
    Controller b;Driver d;ready(d,b);b.ext[0x305]=static_cast<uint16_t>(8U|mux);
    LinkSettings profile;
    if(an)profile.advertisement.technologyMask=speed==LinkSpeed::MBPS_100?8:2;
    else { profile.mode=LinkMode::FORCED;profile.advertisement.technologyMask=0;profile.forced={speed,Duplex::FULL}; }
    OperationToken configured;CHECK(d.startConfigure(profile,{},budget(),configured).inProgress());complete(d,b,configured);
    b.words[16]=speed==LinkSpeed::MBPS_10?0x0007:0x0005;
    CollisionOptions value{speed,true,true,true,true};OperationToken t;const auto first=b.frames;
    CHECK(d.startCollisionTest(value,budget(),t).inProgress());const auto started=complete(d,b,t);
    CHECK(started.electrical.controlsVerified && started.temporaryMayBeActive && (b.words[0]&0x80U)!=0U);
    CHECK(writes(b,first,0)==1U && writes(b,first,0x1F)==0U);
    CHECK(d.stopCollisionTest(budget(),t).inProgress());const auto stopped=complete(d,b,t);
    CHECK(stopped.electrical.stopped && !stopped.temporaryMayBeActive && !stopped.cleanupOwed && (b.words[0]&0x80U)==0U);
  }
  for(unsigned offset=0;offset<11U;++offset) {
    Controller b;Driver d;ready(d,b);b.failFrame=static_cast<int>(b.frames+offset);OperationToken t;
    CHECK(d.startCollisionTest({LinkSpeed::MBPS_100,true,true,true,true},budget(),t).inProgress());
    CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));const auto r=take(d,t);if(offset>=9U)CHECK(r.cleanupOwed && r.temporaryMayBeActive);
  }
  for(unsigned phase=0;phase<3U;++phase) {
    Controller b;Driver d;ready(d,b);OperationToken t;
    CHECK(d.startCollisionTest({LinkSpeed::MBPS_100,true,true,true,true},budget(),t).inProgress());complete(d,b,t);
    b.failFrame=static_cast<int>(b.frames+phase);CHECK(d.stopCollisionTest(budget(),t).inProgress());
    CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));const auto r=take(d,t);CHECK(r.cleanupOwed && r.temporaryMayBeActive);
  }
  for(bool stopDrift:{false,true}) {
    Controller b;Driver d;ready(d,b);OperationToken t;
    if(stopDrift) { CHECK(d.startCollisionTest({LinkSpeed::MBPS_100,true,true,true,true},budget(),t).inProgress());complete(d,b,t); }
    b.words[0]=static_cast<uint16_t>(0x2100U|(stopDrift?0x80U:0U));const auto first=b.frames;
    CHECK((stopDrift?d.stopCollisionTest(budget(),t):d.startCollisionTest({LinkSpeed::MBPS_100,true,true,true,true},budget(),t)).inProgress());
    CHECK(drain(d,b,t).is(Err::CONFIG_MISMATCH));const auto r=take(d,t);CHECK(r.mismatch.valid && !r.persistentRestored);
    CHECK(d.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
    if(!stopDrift)CHECK(writes(b,first,0)==0U);
  }
  for(const uint16_t mux:std::array<uint16_t,5>{{0,1,2,4,7}}) {
    Controller b;Driver d;ready(d,b);b.ext[0x305]=mux;OperationToken t;const auto first=b.frames;
    CHECK(d.startCollisionTest({LinkSpeed::MBPS_100,true,true,true,true},budget(),t).inProgress());
    CHECK(drain(d,b,t).is(Err::INVALID_CONFIG));CHECK(!take(d,t).mutationAttempted && writes(b,first,0)==0U);
  }
}
void calibration() {
  for(const uint16_t initial:std::array<uint16_t,2>{{0x0102,0xC142}}) {
    Controller b;Driver d;ready(d,b);b.words[30]=initial;const auto first=b.frames;OperationToken t;
    CHECK(d.startCalibrationRecovery({true,true,true},budget(),t).inProgress());const auto result=complete(d,b,t);
    CHECK(result.electrical.cdcrValid && result.electrical.rawCdcr==initial);
    CHECK(result.electrical.recoveryNeeded==((initial&0x4000U)!=0U));
    CHECK(b.trace[first].kind==FrameKind::READ && b.trace[first].reg==0x1E);
    if((initial&0x4000U)!=0U) {
      CHECK(b.trace[first+1U].reg==0x1E && b.trace[first+1U].value==0x0142);
      CHECK(b.trace[first+2U].reg==0x1F && b.trace[first+2U].value==0x8000);
      CHECK(result.electrical.resetComplete && result.cleanupOwed && !d.isInitialized());
      CHECK(b.trace[first+3U].at>=b.trace[first+2U].at+2002U);
    } else CHECK(b.frames==first+1U && !result.mutationAttempted && !result.cleanupOwed && d.isInitialized());
  }
  for(unsigned offset=0;offset<6U;++offset) {
    Controller b;Driver d;ready(d,b);b.words[30]=0xC102;const auto first=b.frames;b.failFrame=static_cast<int>(first+offset);OperationToken t;
    CHECK(d.startCalibrationRecovery({true,true,true},budget(),t).inProgress());CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));
    const auto r=take(d,t);CHECK(b.frames==first+offset+1U);CHECK(r.electrical.cdcrValid==(offset!=0U));
    if(offset!=0U)CHECK(r.cleanupOwed);
    if(offset==1U)CHECK(writes(b,first,0x1F)==0U);
    if(offset==2U)CHECK(d.operationSnapshot().busQuietValid);
  }
  // A clear accepted before an error owes HARD; no lesser reset can erase it.
  Controller uncertain;Driver recovery;ready(recovery,uncertain);uncertain.words[30]=0x4102;OperationToken recover;
  uncertain.failFrame=static_cast<int>(uncertain.frames+1U);
  CHECK(recovery.startCalibrationRecovery({true,true,true},budget(),recover).inProgress());
  CHECK(drain(recovery,uncertain,recover).is(Err::MDIO_TIMEOUT));const auto failedClear=take(recovery,recover);
  CHECK(failedClear.electrical.hardResetOwed && failedClear.cleanupOwed);
  const auto untouched=uncertain.frames;
  for(const auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::RESTART})CHECK(recovery.startInitialize({reset},budget(),recover).is(Err::BUSY));
  CHECK(uncertain.frames==untouched);uncertain.failFrame=-1;
  CHECK(recovery.startInitialize({ResetKind::HARD},budget(),recover).inProgress());complete(recovery,uncertain,recover);
  CHECK(!recovery.electricalSnapshot().hardResetOwed && recovery.isInitialized());
  // Existing cable evidence can establish the need for this explicit contingency.
  Controller known;Driver cable;ready(cable,known);known.words[30]=0x4102;
  CHECK(cable.startCableAutorunAcquire(budget(),recover).inProgress());CHECK(!drain(cable,known,recover).inProgress());take(cable,recover);
  CHECK(cable.startConfigureCableAutorun({false,true},budget(),recover).is(Err::INVALID_CONFIG));
  CHECK(cable.startCalibrationRecovery({true,true,true},budget(),recover).inProgress());complete(cable,known,recover);
  CHECK(cable.startInitialize({ResetKind::NONE},budget(),recover).inProgress());complete(cable,known,recover);
  CHECK(cable.startConfigureCableAutorun({false,true},budget(),recover).inProgress());complete(cable,known,recover);
  Controller released;Driver owner;ready(owner,released);released.words[30]=0x4102;OperationToken pair;
  CHECK(owner.startCalibrationRecovery({true,true,true},budget(),pair).inProgress());CHECK(poll(owner,released,pair).inProgress());
  released.releaseCall=static_cast<int>(released.calls);CHECK(poll(owner,released,pair).is(Err::MDIO_ERROR));
  const auto releasedResult=take(owner,pair);CHECK(releasedResult.commandAccepted && releasedResult.cleanupOwed);
  CHECK(owner.operationSnapshot().busQuietValid);
  // Never split the clear+HARD pair, even with a short cooperative slice.
  Controller b;Driver d;ready(d,b);b.words[30]=0x4102;OperationToken t;
  CHECK(d.startCalibrationRecovery({true,true,true},budget(),t).inProgress());CHECK(poll(d,b,t).inProgress());
  const auto frames=b.frames;CHECK(poll(d,b,t,1).inProgress());CHECK(b.frames==frames);
  complete(d,b,t);
}
void bounds() {
  Controller b;Driver d;ready(d,b);const auto t=start(d,b,options());const auto frames=b.frames;
  CHECK(d.pollOperation(t,0).inProgress() && b.frames==frames);
  Driver competitor;CHECK(competitor.bind(b.config(2)).ok());b.competitor=&competitor;
  complete(d,b,t);CHECK(b.excluded!=0U && !b.locked && b.calls==b.releases);
  Controller frozen;Driver stalled;ready(stalled,frozen);frozen.frameTime=0;OperationToken token;
  CHECK(stalled.startCompliance(options(),budget(),token).inProgress());
  CHECK(drain(stalled,frozen,token).ok());take(stalled,token); // Finite caller advances only exposed waits.
  Controller regression;Driver clocked;ready(clocked,regression);regression.regressCall=static_cast<int>(regression.calls);
  token=start(clocked,regression,options());CHECK(drain(clocked,regression,token).is(Err::INVALID_CONFIG));take(clocked,token);
  Controller overflow;Driver large;ready(large,overflow);overflow.frameTime=0;
  CHECK(large.startCompliance(options(),budget(2048,std::numeric_limits<uint64_t>::max()),token).inProgress());
  for(unsigned n=0;n<32U && large.operationSnapshot().phase!=OperationPhase::COMMAND;++n)CHECK(poll(large,overflow,token).inProgress());
  CHECK(large.operationSnapshot().phase==OperationPhase::COMMAND);overflow.now=std::numeric_limits<uint64_t>::max()-1000U;
  const auto before=overflow.frames;CHECK(large.pollOperation(token).detail==static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
  CHECK(overflow.frames==before && !take(large,token).mutationAttempted);
  Controller stuck;Driver never;ready(never,stuck);CHECK(never.startCompliance(options(),budget(40),token).inProgress());
  for(unsigned n=0;n<32U && stuck.hard==0U;++n)CHECK(poll(never,stuck,token).inProgress());
  CHECK(stuck.hard==1U);stuck.stickyHard=true;const auto held=drain(never,stuck,token);CHECK(held.is(Err::OPERATION_TIMEOUT));
  CHECK(take(never,token).cleanupOwed && !stuck.locked);
  Controller reset;Driver observed;ready(observed,reset);reset.exposePcs=true;reset.releaseCall=static_cast<int>(reset.calls);
  token=start(observed,reset,options());CHECK(drain(observed,reset,token).is(Err::MDIO_ERROR));
  CHECK(!observed.isInitialized() && !observed.capabilities().valid);take(observed,token);
  Controller expiry;Driver expired;ready(expired,expiry);OperationBudget empty=budget(2048,expiry.now);const auto count=expiry.frames;
  CHECK(expired.startCompliance(options(),empty,token).inProgress());CHECK(expired.pollOperation(token).is(Err::OPERATION_TIMEOUT));
  CHECK(expiry.frames==count);take(expired,token);
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={ {"traces",vectors},{"admission",invalid},{"failures",failures},{"lifecycle",cancellation},
    {"collision",collision},{"calibration",calibration},{"timing",bounds} };
  return test::runSuites(argc,argv,suites);
}
