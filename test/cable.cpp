#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target; };
struct Target { uint16_t address, value; };
// Independent SNLS647I/SNLA428 literal model. No silicon timing/accuracy claim.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<Wire,8192> trace{};
  std::array<Target,2048> writes{};
  std::array<unsigned,2048> callFirst{};
  std::array<std::array<uint16_t,3>,6> peaks{{{{6,68,0}},{{12,6,0x0800}},{{0,0,0}},{{0,0,0}},{{0,0,0}},{{120,17,0}}}};
  uint16_t selector{0}, address{0}, id2{0xA131};
  uint64_t now{100}, frameTime{2};
  unsigned frames{0}, calls{0}, clocks{0}, writeCount{0}, releases{0}, triggers{0}, cdcrReads{0}, hardResets{0};
  int failFrame{-1}, releaseCall{-1}, lateCall{-1}, regressCall{-1}, overflowCall{-1};
  int corruptFrame{-1}; uint16_t corruptMask{0x0010};
  bool failAfterEffect{true}, locked{false}, staleDone{false}, neverDone{false}, cableFail{false}, noDone{false};
  bool changeAfterPeak{false};
  Driver* competitor{nullptr}; unsigned exclusions{0};
  Controller() { defaults(); }
  void defaults() {
    words.fill(0); ext.fill(0);
    words[0]=0x3100; words[1]=0x0000; words[4]=0x01E1; words[25]=0x8001;
    words[22]=0x0100; words[30]=0x0102;
    ext[0x030E]=0x8400; ext[0x0456]=0x0008;
    ext[0x0467]=0x1234; ext[0x0468]=0x5678;
  }
  Config config(uint32_t phy=1, MacInterface interface=MacInterface::RMII) {
    Config c; c.variant=Variant::DP83826I; c.interface=interface; c.phyAddress=phy;
    c.transport={this,execute,clock,{8,4,1}}; return c;
  }
  static uint64_t clock(void* context) noexcept { auto& b=*static_cast<Controller*>(context); ++b.clocks; return b.now; }
  uint16_t readTarget(uint16_t target) {
    if(target==2) return 0x2000;
    if(target==3) return id2;
    if(target==0) words[0]&=0x7DFF;
    if(target==31) words[31]=0;
    if(target==30 && (words[30]&0x8000U)!=0U) {
      ++cdcrReads;
      if(staleDone) return 0x0102;
      if(neverDone || cdcrReads==1U) return 0x8100;
      if(noDone) return 0x0100;
      words[30]=static_cast<uint16_t>(0x0102U|(cableFail?1U:0U));
    }
    if(target==0x0180 || target==0x0185 || target==0x018A) {
      const unsigned row=triggers==0U?0U:(triggers-1U)%6U;
      if(target==0x018A && changeAfterPeak) words[30]=0x4102;
      return peaks[row][target==0x0180?0U:target==0x0185?1U:2U];
    }
    CHECK(target!=0x0411 && target!=0x0416); // Opaque recipe is not saveable.
    CHECK(!(target>=0x0181 && target<=0x0184) && !(target>=0x0186 && target<=0x0189));
    return target<32U?words[target]:ext[target];
  }
  uint16_t read(uint8_t reg) { return readTarget(reg==14 && selector==0x401F?address:reg); }
  void write(uint8_t reg,uint16_t value) {
    if(reg==13) { CHECK(value==0x001F || value==0x401F); selector=value; return; }
    if(reg==14 && selector==0x001F) { address=value; return; }
    const uint16_t target=reg==14?address:uint16_t{reg};
    CHECK(writeCount<writes.size()); writes[writeCount++]={target,value};
    if(target==31 && value==0x8000) { ++hardResets; defaults(); words[0]=0x3500; return; }
    if(target==30) {
      CHECK((value&0x4000U)==0U);
      words[30]=value;
      if((value&0x8000U)!=0U) { ++triggers; cdcrReads=0; }
      return;
    }
    if(target<32U) words[target]=value; else ext[target]=value;
  }
  static BatchResult execute(void* context,const MdioBatch& batch,const IoBudget& budget) noexcept {
    auto& b=*static_cast<Controller*>(context);
    BatchResult r; r.status=Status::Ok(); r.releaseStatus=Status::Ok(); r.controllerReusable=true;
    if(b.locked) { r.status=Status::Error(Err::BUSY,"Controller owned",static_cast<int32_t>(BusyDetail::OWNER_ADMISSION)); return r; }
    const unsigned call=b.calls++;
    CHECK(call<b.callFirst.size()); b.callFirst[call]=b.frames;
    CHECK(batch.frameCount<=budget.maxFrames); b.locked=true;
    for(uint8_t i=0;i<batch.frameCount;++i) {
      const auto& q=batch.frames[i]; auto& f=r.frames[i]; uint16_t value=q.value;
      if(q.readIndex!=NO_READ_INDEX) {
        CHECK(q.kind==FrameKind::WRITE && q.readIndex<i && r.frames[q.readIndex].valueValid);
        value=static_cast<uint16_t>((r.frames[q.readIndex].value&q.preserveMask)|q.value);
      }
      const uint16_t target=q.registerAddress==14 && b.selector==0x401F?b.address:uint16_t{q.registerAddress};
      CHECK(b.frames<b.trace.size()); b.trace[b.frames]={q.kind,q.registerAddress,value,target};
      const auto frame=b.frames++;
      const bool fail=static_cast<int>(frame)==b.failFrame; f.attempted=true;
      if(!fail || b.failAfterEffect) {
        if(q.kind==FrameKind::WRITE) b.write(q.registerAddress,value); else {
          f.value=b.read(q.registerAddress);
          if(static_cast<int>(frame)==b.corruptFrame) f.value^=b.corruptMask;
        }
      }
      b.now+=b.frameTime;
      if(b.competitor!=nullptr) {
        EvidenceSnapshot sentinel; sentinel.configurationDirty=true;
        CHECK(b.competitor->acquireEvidence(evidenceBit(EvidenceSource::MISR1),sentinel,budget).is(Err::BUSY));
        CHECK(sentinel.configurationDirty); ++b.exclusions;
      }
      if(fail) { f.effectPossible=b.failAfterEffect; r.status={Err::MDIO_TIMEOUT,-25025,"Cable fault"}; break; }
      f.completed=true; f.valueValid=q.kind==FrameKind::READ;
    }
    if(static_cast<int>(call)==b.releaseCall) { r.releaseStatus={Err::MDIO_ERROR,-37,"Release fault"}; r.controllerReusable=false; }
    if(static_cast<int>(call)==b.lateCall) b.now=budget.deadlineUs;
    if(static_cast<int>(call)==b.regressCall) b.now=0;
    if(static_cast<int>(call)==b.overflowCall) b.now=std::numeric_limits<uint64_t>::max();
    b.locked=false; ++b.releases; return r;
  }
};
OperationBudget allowance(uint32_t frames=2048,uint64_t deadline=1000000) { return {{deadline,frames},10,8}; }
Status pollOne(Driver& d,Controller& b,OperationToken token,uint32_t slice=MAX_BATCH_FRAMES) {
  const auto s=d.operationSnapshot();
  if(s.nextPollValid && b.now<s.nextPollAtUs) b.now=s.nextPollAtUs;
  if(s.busQuietValid && b.now<s.busQuietUntilUs) b.now=s.busQuietUntilUs;
  const auto calls=b.calls;
  const auto status=d.pollOperation(token,slice); CHECK(b.calls<=calls+1U && !b.locked); return status;
}
Status drain(Driver& d,Controller& b,OperationToken token) {
  Status s=Status::Error(Err::IN_PROGRESS,"Pending");
  for(unsigned n=0;n<1024U && s.inProgress();++n) s=pollOne(d,b,token);
  CHECK(!s.inProgress()); return s;
}
OperationResult take(Driver& d,OperationToken token) { OperationResult r; CHECK(d.takeResult(token,r).ok()); return r; }
CableSettings settings(uint8_t mask=0x3F) {
  CableSettings s; s.runMask=mask; s.maintenanceAdmitted=s.peerQuiet=s.allowHardResetExit=true; return s;
}
void ready(Driver& d,Controller& b,MacInterface interface=MacInterface::RMII) {
  CHECK(d.bind(b.config(1,interface)).ok()); OperationToken t;
  CHECK(d.startInitialize({ResetKind::NONE},allowance(),t).inProgress()); CHECK(drain(d,b,t).ok()); take(d,t);
  LinkSettings profile; profile.advertisement={0x0F,true,true}; profile.crossover.pairSelection=PairSelection::MDIX;
  CHECK(d.startConfigure(profile,allowance(),t).inProgress()); CHECK(drain(d,b,t).ok()); take(d,t);
  b.ext[0x0170]=0xD912; b.ext[0x0173]=0xABCD; b.ext[0x0175]=0xA805;
  b.ext[0x0176]=0xA516; b.ext[0x0177]=0xB234; b.ext[0x0178]=0xAAAD; b.ext[0x0456]=0x0408;
}
OperationToken start(Driver& d,Controller& b,const CableSettings& s=settings()) {
  const auto calls=b.calls,clocks=b.clocks; OperationToken t{999};
  CHECK(d.startCableTest(s,allowance(),t).inProgress());
  CHECK(t.value!=999 && b.calls==calls && b.clocks==clocks); return t;
}
OperationToken stop(Driver& d,Controller& b) {
  const auto calls=b.calls,clocks=b.clocks; OperationToken t{999};
  CHECK(d.stopCableTest(allowance(),t).inProgress()); CHECK(t.value!=999 && b.calls==calls && b.clocks==clocks); return t;
}
void restored(Driver& d,Controller& b) {
  CHECK(d.isInitialized() && d.settingsSnapshot().trust==ConfigurationTrust::VERIFIED);
  CHECK(!d.operationSnapshot().cleanupOwed && !d.cableSnapshot().temporaryMayBeActive);
  CHECK((b.words[0]&0x7D80U)==0x1000U && (b.words[25]&0xC000U)==0x4000U);
  const uint16_t addr[]={0x0170,0x0173,0x0175,0x0176,0x0177,0x0178,0x0456};
  const uint16_t mask[]={0x7770,0xFFFF,0x380F,0x001F,0x1F00,0x0007,0x0600};
  const uint16_t saved[]={0xD912,0xABCD,0xA805,0xA516,0xB234,0xAAAD,0x0408};
  const unsigned fields=d.cableSnapshot().settings.recipe==CableRecipe::SNLA428?7U:6U;
  for(unsigned i=0;i<fields;++i) CHECK((b.ext[addr[i]]&mask[i])==(saved[i]&mask[i]));
}
void finishStop(Driver& d,Controller& b) { const auto t=stop(d,b); CHECK(drain(d,b,t).ok()); CHECK(take(d,t).persistentRestored); restored(d,b); }

void matrix() {
  const uint16_t addresses[]={0x0170,0x0173,0x0175,0x0178,0x0416,0x0411,0x0456,0x001E};
  const uint16_t values[6][8]={
    {0x5C12,0x0D00,0x1007,2,0x1FA0,0x0813,0x0608,0x8102},
    {0x5C12,0x0D0C,0x1007,2,0x1FA0,0x0813,0x0608,0x8102},
    {0x5C22,0x0D13,0x1007,2,0x1FA0,0x0813,0x0608,0x8102},
    {0x5E32,0x1A20,0x1007,2,0x1FA0,0x0815,0x0608,0x8102},
    {0x5E42,0x343A,0x100A,2,0x1FA0,0x0816,0x0608,0x8102},
    {0x5E52,0x8F6E,0x100D,6,0x1F90,0x0816,0x0608,0x8102}};
  for(auto interface:{MacInterface::MII,MacInterface::RMII}) for(uint8_t mask:{uint8_t{0x3F},uint8_t{0x25},uint8_t{1}}) {
    Controller b; Driver d; ready(d,b,interface); const auto first=b.writeCount,frames=b.frames,calls=b.calls;
    const auto health=d.healthSnapshot(); const auto t=start(d,b,settings(mask));
    CHECK(drain(d,b,t).ok()); const auto result=take(d,t);
    unsigned rowCount=0;
    for(unsigned row=0;row<6;++row) {
      const auto& sample=result.cable.samples[row];
      if((mask&(1U<<row))==0U) { CHECK(sample.state==CableRunState::NOT_RUN && !sample.index.valid); continue; }
      CHECK(sample.state==CableRunState::COMPLETE && sample.freshnessObserved && sample.triggerAccepted);
      CHECK(sample.cdcr.valid && sample.index.valid && sample.amplitude.valid && sample.peak.valid);
      CHECK(!sample.failed && !sample.index.stale && sample.index.timeValid);
      for(unsigned field=0;field<8;++field) {
        CHECK(b.writes[first+rowCount*8U+field].address==addresses[field]);
        CHECK(b.writes[first+rowCount*8U+field].value==values[row][field]);
      }
      ++rowCount;
    }
    CHECK(b.writeCount-first==rowCount*8U && b.triggers==rowCount);
    CHECK(result.cable.complete && !result.cable.stale && result.cable.session.value==t.value);
    CHECK(result.cleanupOwed && result.temporaryMayBeActive && !result.persistentRestored);
    // Preflight8 + saved7*4, then each row29apply+20verify+1trigger+
    // 2status+12results+1bookend. Call counts are independently enumerated.
    CHECK(b.frames-frames==36U+65U*rowCount && b.calls-calls==14U+19U*rowCount);
    CHECK(b.calls==b.releases);
    CHECK(d.healthSnapshot().totalSuccess==health.totalSuccess && d.healthSnapshot().totalFailures==health.totalFailures);
    const auto cachedCalls=b.calls,cachedClocks=b.clocks;
    CHECK(d.cableSnapshot().result.complete && d.cableSnapshot().temporaryMayBeActive);
    CHECK(b.calls==cachedCalls && b.clocks==cachedClocks);
    CableDiagnosis diagnosis; CHECK(interpretCable(result.cable,4.6,diagnosis).ok());
    if(mask==0x3F) CHECK(diagnosis.run==CableRun::RUN_5 && diagnosis.conclusion==CableConclusion::OPEN && std::abs(diagnosis.metres-100.0)<1e-9);
    const auto stopFrames=b.frames,stopCalls=b.calls;
    finishStop(d,b); CHECK(b.hardResets==1U);
    // Stop5/4 + HARD initialization/profile56/30 + saved tail66/16.
    CHECK(b.frames-stopFrames==127U && b.calls-stopCalls==50U);
    CHECK(result.cable.complete && !result.cable.stale); // Historical primary value is immutable.
  }
  Controller b; Driver d; ready(d,b); b.ext[0x0456]=0x0400;
  const auto first=b.writeCount; const auto t=start(d,b,settings(1)); CHECK(drain(d,b,t).ok()); take(d,t);
  CHECK(b.writes[first+6U].address==0x0456 && b.writes[first+6U].value==0x0600);
}

void raw() {
  for(unsigned channel=0;channel<4;++channel) for(bool maximum:{false,true}) {
    Controller b; Driver d; ready(d,b); auto s=settings(1); s.recipe=CableRecipe::RAW;
    s.controls.channel=static_cast<CableChannel>(channel); s.controls.dcNoInit=maximum;
    s.controls.averaging=maximum?6:0; s.controls.segment=maximum?5:1;
    s.controls.duration=maximum?255:0; s.controls.initialSkip=maximum?255:0;
    s.controls.shadowAverage=maximum?7:0; s.controls.forwardShadow=maximum?15:0;
    s.controls.locationThreshold=maximum?31:0; s.controls.shortThreshold=maximum?31:0; s.controls.pulseWidth=maximum?7:0;
    const auto first=b.writeCount,frames=b.frames,calls=b.calls;
    const auto t=start(d,b,s); CHECK(drain(d,b,t).ok()); const auto r=take(d,t);
    CHECK(b.frames-frames==102U && b.calls-calls==32U);
    const uint16_t expected=static_cast<uint16_t>((0xD912U&0x888FU)|(channel<<13U)|(maximum?0x1650U:0x0010U));
    CHECK(b.ext[0x0170]==expected && b.ext[0x0173]==(maximum?0xFFFFU:0U));
    CHECK(b.ext[0x0175]==((0xA805U&0xC7F0U)|(maximum?0x380FU:0U)));
    CHECK(b.ext[0x0176]==((0xA516U&0xFFE0U)|(maximum?0x001FU:0U)));
    CHECK(b.ext[0x0177]==((0xB234U&0xE0FFU)|(maximum?0x1F00U:0U)));
    CHECK(b.ext[0x0178]==((0xAAADU&0xFFF8U)|(maximum?0x0007U:0U)));
    for(unsigned i=first;i<b.writeCount;++i) CHECK(b.writes[i].address!=0x0411 && b.writes[i].address!=0x0416);
    CHECK(r.cable.complete); CableDiagnosis sentinel; sentinel.metres=999;
    CHECK(!interpretCable(r.cable,5.35,sentinel).ok() && sentinel.metres==999);
    const auto stopFrames=b.frames,stopCalls=b.calls;finishStop(d,b);
    CHECK(b.frames-stopFrames==118U && b.calls-stopCalls==48U);
  }
  // Separate SNLS647I tables8-56/57 words, not silently replaced by SNLA428.
  const uint16_t thresholds[]={0x0D07,0x0D14,0x1A21,0x343B,0x8F6F};
  const uint16_t shadows[]={0x1004,0x1004,0x1005,0x1008,0x100B};
  for(unsigned segment=0;segment<5;++segment) {
    Controller b; Driver d; ready(d,b); auto s=settings(1); s.recipe=CableRecipe::RAW;
    s.controls.segment=static_cast<uint8_t>(segment+1U);
    s.controls.duration=static_cast<uint8_t>(thresholds[segment]>>8U); s.controls.initialSkip=static_cast<uint8_t>(thresholds[segment]&0xFFU);
    s.controls.shadowAverage=2; s.controls.forwardShadow=static_cast<uint8_t>(shadows[segment]&0xFU);
    const auto t=start(d,b,s); CHECK(drain(d,b,t).ok()); take(d,t);
    CHECK(b.ext[0x0173]==thresholds[segment] && (b.ext[0x0175]&0x380FU)==shadows[segment]); finishStop(d,b);
  }
}

void admission() {
  Controller b; Driver d; ready(d,b);
  const auto reject=[&](const CableSettings& s) {
    const auto calls=b.calls,clocks=b.clocks,writes=b.writeCount; OperationToken t{999};
    CHECK(!d.startCableTest(s,allowance(),t).inProgress());
    CHECK(t.value==999 && b.calls==calls && b.clocks==clocks && b.writeCount==writes);
  };
  auto s=settings(); s.maintenanceAdmitted=false; reject(s); s=settings();s.peerQuiet=false;reject(s);
  s=settings();s.allowHardResetExit=false;reject(s);s=settings(0);reject(s);s=settings(0x40);reject(s);
  s=settings();s.recipe=static_cast<CableRecipe>(99);reject(s);
  for(unsigned field=0;field<8;++field) {
    s=settings(1);s.recipe=CableRecipe::RAW;
    switch(field) {
      case 0:s.controls.channel=static_cast<CableChannel>(4);break;
      case 1:s.controls.averaging=7;break;case 2:s.controls.segment=0;break;
      case 3:s.controls.segment=6;break;case 4:s.controls.shadowAverage=8;break;
      case 5:s.controls.forwardShadow=16;break;case 6:s.controls.locationThreshold=32;break;
      case 7:s.controls.shortThreshold=32;break;
    }
    reject(s);
  }
  s=settings(1);s.recipe=CableRecipe::RAW;s.controls.pulseWidth=8;reject(s);
  s=settings(3);s.recipe=CableRecipe::RAW;reject(s);
  // Invalid saved encodings cannot be replayed safely after the required HARD.
  for(uint16_t invalidSaved:{uint16_t{0x5F12},uint16_t{0x5C02},uint16_t{0x5C62},uint16_t{0x5C72}}) {
    Controller bus;Driver driver;ready(driver,bus);bus.ext[0x0170]=invalidSaved;
    const auto writes=bus.writeCount;const auto t=start(driver,bus,settings(1));
    CHECK(!drain(driver,bus,t).ok());const auto r=take(driver,t);
    CHECK(bus.writeCount==writes && !r.mutationAttempted && !r.temporaryMayBeActive);
  }
  for(unsigned state=0;state<7;++state) {
    Controller bus; Driver driver; ready(driver,bus);
    if(state==0) bus.words[9]|=0x0100;
    if(state==1) bus.words[30]|=0x4000;
    if(state==2) bus.words[16]=1;
    if(state==3) bus.words[0]|=0x0800;
    if(state==4) bus.words[22]|=0x3000;
    if(state==5) bus.words[22]|=0x0010;
    if(state==6) bus.id2=0xA141;
    const auto writes=bus.writeCount; const auto t=start(driver,bus,settings(1));
    CHECK(!drain(driver,bus,t).ok()); const auto r=take(driver,t);
    CHECK(bus.writeCount==writes && !r.mutationAttempted && !r.temporaryMayBeActive);
    if(state==1) CHECK(r.cable.prerequisiteCdcr.valid && (r.cable.prerequisiteCdcr.raw&0x4000U)!=0U);
  }
  // PHYRCR restart invalidates observations, while HARD also invalidates the
  // retained identity/configuration. Valid read evidence survives a late/error exit.
  for(uint16_t command:{uint16_t{0x4000},uint16_t{0x8000}}) for(unsigned fault=0;fault<3;++fault) {
    Controller bus;Driver driver;ready(driver,bus);
    LinkSnapshot link;CHECK(driver.readLinkStatus(link,{1000000,10}).ok());
    CHECK(link.valid && !link.stale);
    const auto before=driver.evidenceSnapshot();
    const auto bmsr=static_cast<unsigned>(EvidenceSource::BMSR);
    CHECK(before.samples[bmsr].valid && !before.samples[bmsr].stale);
    const auto first=bus.frames,calls=bus.calls,writes=bus.writeCount;
    bus.corruptFrame=static_cast<int>(first+3U);bus.corruptMask=command;
    if(fault==1)bus.releaseCall=static_cast<int>(calls+2U);
    if(fault==2)bus.lateCall=static_cast<int>(calls+2U);
    const auto token=start(driver,bus,settings(1));CHECK(!drain(driver,bus,token).ok());
    const auto r=take(driver,token);
    CHECK(bus.frames-first==4U && bus.calls-calls==3U && bus.writeCount==writes);
    CHECK(bus.trace[first+3U].kind==FrameKind::READ && bus.trace[first+3U].reg==0x1F);
    CHECK(!r.mutationAttempted && !r.temporaryMayBeActive);
    if(fault==0)CHECK(r.status.is(Err::INVALID_CONFIG));
    if(fault==1)CHECK(r.status.is(Err::MDIO_ERROR) && r.status.detail==-37);
    if(fault==2)CHECK(r.status.is(Err::OPERATION_TIMEOUT));
    CHECK(driver.linkSnapshot().stale);
    const auto after=driver.evidenceSnapshot();
    CHECK(after.samples[bmsr].stale && after.samples[bmsr].discontinuous);
    CHECK(after.samples[bmsr].historyLost==before.samples[bmsr].historyLost);
    if(command==0x4000U) {
      CHECK(driver.isInitialized() && driver.capabilities().valid);
      CHECK(driver.settingsSnapshot().trust==ConfigurationTrust::VERIFIED);
    } else {
      CHECK(!driver.isInitialized() && !driver.capabilities().valid);
      CHECK(driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
    }
  }
}

void capture() {
  for(unsigned mode=0;mode<4;++mode) {
    Controller b; Driver d; ready(d,b); b.staleDone=mode==0; b.neverDone=mode==1; b.noDone=mode==2; b.cableFail=mode==3;
    OperationToken t; CHECK(d.startCableTest(settings(1),allowance(128),t).inProgress());
    const auto status=drain(d,b,t); const auto r=take(d,t);
    if(mode<3) { CHECK(status.is(Err::OPERATION_TIMEOUT) && !r.cable.complete && r.cleanupOwed); }
    else { CHECK(status.ok() && r.cable.complete && r.cable.samples[0].failed); }
    CableDiagnosis sentinel;sentinel.metres=999; CHECK(!interpretCable(r.cable,4.6,sentinel).ok() && sentinel.metres==999);
  }
  Controller changed;Driver subject;ready(subject,changed);changed.changeAfterPeak=true;
  const auto altered=start(subject,changed,settings(1));CHECK(!drain(subject,changed,altered).ok());
  const auto raced=take(subject,altered);
  CHECK(!raced.cable.complete && raced.cable.samples[0].peak.valid && raced.cable.samples[0].peak.stale);
  OperationToken denied;CHECK(!subject.startCableAcquire(allowance(),denied).inProgress());
  // Failure after each nondestructive result read retains only independently
  // completed data; a new explicit acquisition never issues another trigger.
  Controller reference; Driver baseline; ready(baseline,reference); const auto first=reference.frames;
  const auto complete=start(baseline,reference,settings(1));CHECK(drain(baseline,reference,complete).ok());take(baseline,complete);
  for(uint16_t target:{uint16_t{0x0180},uint16_t{0x0185},uint16_t{0x018A}}) {
    unsigned offset=first;
    while(offset<reference.frames && !(reference.trace[offset].kind==FrameKind::READ && reference.trace[offset].target==target)) ++offset;
    CHECK(offset<reference.frames);
    Controller b; Driver d; ready(d,b); const auto evidence=d.evidenceSnapshot();
    b.failFrame=static_cast<int>(b.frames+offset-first); const auto t=start(d,b,settings(1));
    CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT)); const auto failed=take(d,t);
    CHECK(failed.status.detail==-25025 && !failed.cable.complete);
    const auto& row=failed.cable.samples[0];
    CHECK(row.cdcr.valid && row.freshnessObserved);
    CHECK(row.index.valid==(target!=0x0180)); CHECK(row.amplitude.valid==(target==0x018A)); CHECK(!row.peak.valid);
    const auto after=d.evidenceSnapshot();
    for(unsigned i=0;i<static_cast<unsigned>(EvidenceSource::COUNT);++i) CHECK(after.samples[i].historyLost==evidence.samples[i].historyLost);
    b.failFrame=-1;const auto triggers=b.triggers; OperationToken again;
    CHECK(d.startCableAcquire(allowance(),again).inProgress());CHECK(drain(d,b,again).ok());const auto read=take(d,again);
    CHECK(read.cable.complete && b.triggers==triggers && failed.status.detail==-25025);finishStop(d,b);
  }
  // Interrupt the second selected row after its index read. Reacquisition owns
  // only that accepted trigger: it must retain row1A, finish row1B without a new
  // trigger, and leave the requested but unstarted row2 explicitly NOT_RUN.
  for(bool cancelled:{false,true}) {
    Controller b; Driver d; ready(d,b); const auto t=start(d,b,settings(0x07));
    for(unsigned turn=0;turn<128U && !d.cableSnapshot().result.samples[1].index.valid;++turn) {
      CHECK(pollOne(d,b,t).inProgress());
    }
    const auto before=d.cableSnapshot().result;
    CHECK(before.samples[0].state==CableRunState::COMPLETE);
    CHECK(before.samples[1].index.valid && !before.samples[1].amplitude.valid);
    CHECK(before.samples[2].state==CableRunState::NOT_RUN && b.triggers==2U);
    const auto frames=b.frames,calls=b.calls,clocks=b.clocks;
    if(cancelled) {
      CHECK(d.cancel(t).ok());
      CHECK(b.frames==frames && b.calls==calls && b.clocks==clocks);
    } else {
      b.failFrame=static_cast<int>(b.frames+3U); // Second row's0185 data read.
      CHECK(pollOne(d,b,t).is(Err::MDIO_TIMEOUT));
      CHECK(b.frames==frames+4U && b.calls==calls+1U);
    }
    const auto interrupted=take(d,t);
    CHECK(interrupted.status.is(cancelled?Err::CANCELLED:Err::MDIO_TIMEOUT));
    CHECK(!interrupted.cable.complete && interrupted.cable.stale && interrupted.cleanupOwed);
    CHECK(interrupted.cable.samples[0].index.raw==before.samples[0].index.raw);
    CHECK(interrupted.cable.samples[0].index.observedAtUs==before.samples[0].index.observedAtUs);
    CHECK(interrupted.cable.samples[0].state==CableRunState::COMPLETE);
    CHECK(interrupted.cable.samples[1].index.valid && !interrupted.cable.samples[1].amplitude.valid);
    CHECK(interrupted.cable.samples[2].state==CableRunState::NOT_RUN);
    b.failFrame=-1; const auto rereadFrames=b.frames,rereadCalls=b.calls,writes=b.writeCount;
    OperationToken again; CHECK(d.startCableAcquire(allowance(),again).inProgress());
    CHECK(drain(d,b,again).ok()); const auto reread=take(d,again);
    CHECK(b.frames==rereadFrames+14U && b.calls==rereadCalls+5U && b.writeCount==writes && b.triggers==2U);
    CHECK(reread.cable.samples[0].state==CableRunState::COMPLETE && reread.cable.samples[1].state==CableRunState::COMPLETE);
    CHECK(reread.cable.samples[0].index.observedAtUs==before.samples[0].index.observedAtUs);
    CHECK(reread.cable.samples[2].state==CableRunState::NOT_RUN && !reread.cable.samples[2].index.valid);
    CHECK(!reread.cable.complete && reread.cable.stale && reread.cleanupOwed);
    CHECK(!interrupted.cable.samples[1].amplitude.valid); // Prior retained result stays immutable.
    CableDiagnosis sentinel; sentinel.metres=999;
    CHECK(interpretCable(reread.cable,4.6,sentinel).is(Err::RESULT_NOT_AVAILABLE) && sentinel.metres==999);
    finishStop(d,b); CHECK(b.triggers==2U);
  }
}

void failures() {
  for(bool stopping:{false,true}) {
    Controller reference;Driver baseline;ready(baseline,reference);
    if(stopping) { const auto t=start(baseline,reference,settings(1));CHECK(drain(baseline,reference,t).ok());take(baseline,t); }
    const auto first=reference.frames,firstCall=reference.calls;
    const auto successful=stopping?stop(baseline,reference):start(baseline,reference,settings(1));
    CHECK(drain(baseline,reference,successful).ok());take(baseline,successful);
    const auto frameCount=reference.frames-first,callCount=reference.calls-firstCall;
    CHECK(frameCount>0 && frameCount<512 && callCount<128);
    // A valid mismatch remains retained when release from that same read fails.
    unsigned mismatchFrame=first,seen=0;
    if(stopping) mismatchFrame=reference.frames-1U; // Last GENCFG readback.
    else for(unsigned i=first;i<reference.frames;++i) {
      if(reference.trace[i].kind==FrameKind::READ && reference.trace[i].target==0x0170 && ++seen==2U) { mismatchFrame=i;break; }
    }
    CHECK(mismatchFrame>first);
    unsigned mismatchCall=firstCall;
    while(mismatchCall+1U<reference.calls && reference.callFirst[mismatchCall+1U]<=mismatchFrame) ++mismatchCall;
    for(bool release:{false,true}) {
      Controller b;Driver d;ready(d,b);
      if(stopping) { const auto t=start(d,b,settings(1));CHECK(drain(d,b,t).ok());take(d,t); }
      b.corruptFrame=static_cast<int>(b.frames+mismatchFrame-first);b.corruptMask=stopping?0x0200:0x0010;
      if(release)b.releaseCall=static_cast<int>(b.calls+mismatchCall-firstCall);
      const auto t=stopping?stop(d,b):start(d,b,settings(1));
      CHECK(drain(d,b,t).is(release?Err::MDIO_ERROR:Err::CONFIG_MISMATCH));const auto r=take(d,t);
      CHECK(r.mismatch.valid && r.cleanupOwed && !r.persistentRestored);
      if(release)CHECK(r.status.detail==-37);
      b.corruptFrame=b.releaseCall=-1;finishStop(d,b);
    }
    for(unsigned phase=0;phase<frameCount;++phase) {
      Controller b;Driver d;ready(d,b);
      if(stopping) { const auto t=start(d,b,settings(1));CHECK(drain(d,b,t).ok());take(d,t); }
      const auto frames=b.frames;b.failFrame=static_cast<int>(frames+phase);
      const auto t=stopping?stop(d,b):start(d,b,settings(1));
      CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT)); const auto r=take(d,t);
      CHECK(r.status.detail==-25025 && b.frames==frames+phase+1U && !r.persistentRestored);
      if(r.temporaryMayBeActive) { CHECK(r.cleanupOwed);b.failFrame=-1;finishStop(d,b); }
    }
    for(unsigned phase=0;phase<callCount;++phase) for(bool cancel:{false,true}) {
      Controller b;Driver d;ready(d,b);
      if(stopping) { const auto t=start(d,b,settings(1));CHECK(drain(d,b,t).ok());take(d,t); }
      const auto t=stopping?stop(d,b):start(d,b,settings(1));
      if(cancel) {
        for(unsigned i=0;i<phase;++i) CHECK(pollOne(d,b,t).inProgress());
        const auto calls=b.calls,clocks=b.clocks;CHECK(d.cancel(t).ok());CHECK(b.calls==calls && b.clocks==clocks);
      } else { b.releaseCall=static_cast<int>(b.calls+phase);CHECK(drain(d,b,t).is(Err::MDIO_ERROR)); }
      const auto r=take(d,t);CHECK(r.status.is(cancel?Err::CANCELLED:Err::MDIO_ERROR));
      if(!cancel) CHECK(r.failingBatch.release.detail==-37);
      if(r.temporaryMayBeActive) { CHECK(r.cleanupOwed);b.releaseCall=-1;finishStop(d,b); }
    }
  }
}

void lifecycle() {
  // Admission precedence is syntax, slot/barrier, initialized, then zero budget.
  Controller unready;Driver notInitialized;CHECK(notInitialized.bind(unready.config()).ok());
  OperationToken unchanged{999};
  CHECK(notInitialized.startCableTest(settings(1),allowance(0),unchanged).is(Err::NOT_INITIALIZED));
  CHECK(unchanged.value==999 && unready.calls==0U && unready.clocks==0U);
  // A partial stop cannot leave a completed-tail flag usable after another
  // HARD. Exercise every final restore-frame failure and each tail cancel.
  for(bool cancelStop:{false,true}) for(unsigned phase=0;phase<(cancelStop?4U:9U);++phase) {
    Controller bus;Driver driver;ready(driver,bus);const auto startToken=start(driver,bus,settings(1));
    CHECK(drain(driver,bus,startToken).ok());take(driver,startToken);
    const auto stopToken=stop(driver,bus);
    if(cancelStop) {
      for(unsigned i=0;i<46U+phase;++i) CHECK(pollOne(driver,bus,stopToken).inProgress());
      CHECK(driver.cancel(stopToken).ok());
    } else {
      bus.failFrame=static_cast<int>(bus.frames+118U+phase);
      CHECK(drain(driver,bus,stopToken).is(Err::MDIO_TIMEOUT));
    }
    const auto failed=take(driver,stopToken);
    CHECK(failed.cleanupOwed && failed.temporaryMayBeActive && !failed.persistentRestored);
    bus.failFrame=-1;OperationToken hard;
    CHECK(driver.startInitialize({ResetKind::HARD},allowance(),hard).inProgress());
    CHECK(drain(driver,bus,hard).ok());CHECK(take(driver,hard).persistentRestored);restored(driver,bus);
    CHECK(failed.status.is(cancelStop?Err::CANCELLED:Err::MDIO_TIMEOUT));
  }
  Controller b;Driver d;ready(d,b);const auto t=start(d,b,settings(1));
  OperationResult sentinel;sentinel.cable.requestedMask=0xEE;
  CHECK(d.takeResult(t,sentinel).is(Err::RESULT_NOT_AVAILABLE) && sentinel.cable.requestedMask==0xEE);
  CHECK(d.pollOperation({t.value+1}).is(Err::TOKEN_MISMATCH));CHECK(drain(d,b,t).ok());
  const auto calls=b.calls,clocks=b.clocks;CHECK(d.bind(b.config()).is(Err::BUSY));CHECK(d.end().is(Err::BUSY));
  CHECK(d.pollOperation(t).ok() && b.calls==calls && b.clocks==clocks);
  const auto first=take(d,t);CHECK(d.takeResult(t,sentinel).is(Err::RESULT_NOT_AVAILABLE));
  CHECK(d.end().is(Err::BUSY));OperationToken other;
  CHECK(d.startInitialize({ResetKind::NONE},allowance(),other).is(Err::BUSY));
  CHECK(d.startCableTest(settings(1),allowance(),other).is(Err::BUSY));
  CHECK(d.writeClause22(0x18,0,{1000000,1}).is(Err::BUSY));
  b.words[30]|=0x4000;const auto writes=b.writeCount;const auto rejected=stop(d,b);
  CHECK(!drain(d,b,rejected).ok());const auto failed=take(d,rejected);
  CHECK(b.writeCount==writes && failed.cleanupOwed && failed.cable.prerequisiteCdcr.valid);
  const auto blockedCalls=b.calls,blockedClocks=b.clocks;
  CHECK(d.startInitialize({ResetKind::HARD},allowance(),other).is(Err::INVALID_CONFIG));
  CHECK(b.calls==blockedCalls && b.clocks==blockedClocks);
  b.words[30]=0x0102;finishStop(d,b);
  const auto second=start(d,b,settings(1));CHECK(second.value!=t.value);CHECK(drain(d,b,second).ok());take(d,second);
  CHECK(first.cable.session.value==t.value && first.cable.complete);
  CHECK(d.startInitialize({ResetKind::HARD},allowance(),other).inProgress());CHECK(drain(d,b,other).ok());CHECK(take(d,other).persistentRestored);restored(d,b);
  const auto third=start(d,b,settings(1));CHECK(drain(d,b,third).ok());take(d,third);
  const auto before=b.calls;CHECK(d.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());CHECK(b.calls==before);
  CHECK(d.cableSnapshot().result.stale && !d.cableSnapshot().temporaryMayBeActive);
  CHECK(!d.startCableAcquire(allowance(),other).inProgress());
  Controller abandoned;Driver effects;ready(effects,abandoned);
  const auto owned=start(effects,abandoned,settings(1));CHECK(drain(effects,abandoned,owned).ok());take(effects,owned);
  CHECK(effects.abandonCleanup().ok());CHECK(!effects.isInitialized());
  const auto silent=abandoned.calls;OperationToken guarded{999};
  CHECK(effects.startCableTest(settings(1),allowance(0),guarded).is(Err::BUSY));
  CHECK(guarded.value==999 && abandoned.calls==silent);
}

void timing() {
  Controller zeroClock;zeroClock.now=zeroClock.frameTime=0;
  Driver timestamp;ready(timestamp,zeroClock);const auto stamp=start(timestamp,zeroClock,settings(1));
  CHECK(drain(timestamp,zeroClock,stamp).ok());const auto stamped=take(timestamp,stamp);
  CHECK(stamped.cable.prerequisiteCdcr.timeValid && stamped.cable.prerequisiteCdcr.observedAtUs==1U);
  Controller b;Driver d;ready(d,b);OperationToken zero{999};const auto calls=b.calls,clocks=b.clocks;
  CHECK(d.startCableTest(settings(1),allowance(0),zero).is(Err::OPERATION_TIMEOUT));
  CHECK(zero.value==999 && b.calls==calls && b.clocks==clocks);
  const auto t=start(d,b,settings(1));const auto before=b.calls;
  CHECK(d.pollOperation(t,0).inProgress() && b.calls==before);
  // Each indirect selector/read or RMW is indivisible under a short poll slice.
  while(d.operationSnapshot().state==OperationState::RUNNING && b.calls<before+8U) CHECK(pollOne(d,b,t).inProgress());
  const auto frames=b.frames;CHECK(d.pollOperation(t,3).inProgress());CHECK(b.frames==frames);
  CHECK(drain(d,b,t).ok());take(d,t);finishStop(d,b);
  // A cable wait releases bus ownership and permits the existing collector.
  Controller observing;Driver cable;ready(cable,observing);const auto running=start(cable,observing,settings(1));
  while(observing.triggers==0U)CHECK(pollOne(cable,observing,running).inProgress());
  CHECK(cable.operationSnapshot().phase==OperationPhase::WAIT);
  uint16_t rawId=0;CHECK(cable.readClause22(2,rawId,{1000000,1}).ok() && rawId==0x2000);
  EvidenceSnapshot evidence;CHECK(cable.acquireEvidence(evidenceBit(EvidenceSource::MISR1),evidence,{1000000,1}).ok());
  LinkSnapshot link;CHECK(cable.readLinkStatus(link,{1000000,10}).ok());
  CHECK(drain(cable,observing,running).ok());take(cable,running);
  const auto restoring=stop(cable,observing);while(observing.hardResets==0U)CHECK(pollOne(cable,observing,restoring).inProgress());
  const auto quietCalls=observing.calls;rawId=0xBEEF;
  CHECK(cable.readClause22(2,rawId,{1000000,1}).is(Err::BUSY));
  CHECK(rawId==0xBEEF && observing.calls==quietCalls);
  CHECK(drain(cable,observing,restoring).ok());take(cable,restoring);restored(cable,observing);
  // A reset indication read during the final BMCR proof withdraws readiness,
  // even when the same callback subsequently fails release or the deadline.
  // The first read of the final BMCR RMW is evidence too: its command bit must
  // not disappear into neutral composition, even if the following write fails.
  for(unsigned fault=0;fault<4;++fault) {
    Controller bus;Driver driver;ready(driver,bus);const auto begin=start(driver,bus,settings(1));
    CHECK(drain(driver,bus,begin).ok());take(driver,begin);
    const auto stopFirst=bus.frames,stopCall=bus.calls;
    const auto evidence=driver.evidenceSnapshot();
    bus.corruptFrame=static_cast<int>(stopFirst+61U);bus.corruptMask=0x8000;
    if(fault==1)bus.failFrame=static_cast<int>(stopFirst+62U);
    if(fault==2)bus.releaseCall=static_cast<int>(stopCall+34U);
    if(fault==3)bus.lateCall=static_cast<int>(stopCall+34U);
    const auto end=stop(driver,bus);CHECK(!drain(driver,bus,end).ok());const auto r=take(driver,end);
    CHECK(bus.trace[stopFirst+61U].kind==FrameKind::READ && bus.trace[stopFirst+61U].reg==0);
    CHECK(bus.trace[stopFirst+62U].kind==FrameKind::WRITE && bus.trace[stopFirst+62U].reg==0);
    CHECK(!driver.isInitialized() && r.cleanupOwed && !r.persistentRestored);
    CHECK(driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
    if(fault==0)CHECK(r.status.is(Err::INVALID_CONFIG));
    if(fault==1)CHECK(r.status.is(Err::MDIO_TIMEOUT) && r.status.detail==-25025);
    if(fault==2)CHECK(r.status.is(Err::MDIO_ERROR) && r.status.detail==-37);
    if(fault==3)CHECK(r.status.is(Err::OPERATION_TIMEOUT));
    const auto after=driver.evidenceSnapshot();
    for(unsigned i=0;i<static_cast<unsigned>(EvidenceSource::COUNT);++i)
      CHECK(after.samples[i].historyLost==evidence.samples[i].historyLost);
  }
  for(unsigned fault=0;fault<3;++fault) {
    Controller bus;Driver driver;ready(driver,bus);const auto begin=start(driver,bus,settings(1));
    CHECK(drain(driver,bus,begin).ok());take(driver,begin);
    const auto stopFirst=bus.frames,stopCall=bus.calls;
    bus.corruptFrame=static_cast<int>(stopFirst+63U);bus.corruptMask=0x8000;
    if(fault==1)bus.releaseCall=static_cast<int>(stopCall+35U);
    if(fault==2)bus.lateCall=static_cast<int>(stopCall+35U);
    const auto end=stop(driver,bus);CHECK(!drain(driver,bus,end).ok());const auto r=take(driver,end);
    CHECK(!driver.isInitialized() && r.cleanupOwed && !r.persistentRestored);
    if(fault==1)CHECK(r.status.is(Err::MDIO_ERROR) && r.status.detail==-37);
    if(fault==2)CHECK(r.status.is(Err::OPERATION_TIMEOUT));
    CHECK(driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
  for(unsigned fault=0;fault<3;++fault) {
    Controller bus;Driver driver;ready(driver,bus);const auto token=start(driver,bus,settings(1));
    while(!driver.operationSnapshot().temporaryMayBeActive) CHECK(pollOne(driver,bus,token).inProgress());
    if(fault==0)bus.lateCall=static_cast<int>(bus.calls);
    if(fault==1)bus.regressCall=static_cast<int>(bus.calls);
    if(fault==2)bus.overflowCall=static_cast<int>(bus.calls);
    CHECK(!pollOne(driver,bus,token).inProgress());const auto r=take(driver,token);
    CHECK(!r.status.ok() && r.cleanupOwed && r.temporaryMayBeActive);
  }
  Controller frozen;Driver stalled;ready(stalled,frozen);const auto token=start(stalled,frozen,settings(1));
  const auto old=frozen.calls;
  for(unsigned i=0;i<10 && stalled.operationSnapshot().state==OperationState::RUNNING;++i) stalled.pollOperation(token,0);
  CHECK(take(stalled,token).status.is(Err::OPERATION_TIMEOUT) && frozen.calls==old);
  Controller owned;Driver one,two;ready(one,owned);CHECK(two.bind(owned.config(2)).ok());owned.competitor=&two;
  const auto transaction=start(one,owned,settings(1));CHECK(drain(one,owned,transaction).ok());take(one,transaction);finishStop(one,owned);
  CHECK(owned.exclusions>0 && !owned.locked);
}

void interpretation() {
  CableResult r;r.recipe=CableRecipe::SNLA428;r.requestedMask=0x3F;r.complete=true;r.stale=false;
  const uint16_t rows[6][3]={{6,0x44,0},{0x0C,6,0x0800},{0,0,0},{0,0,0},{0,0,0},{0x78,0x11,0}};
  for(unsigned i=0;i<6;++i) {
    auto& s=r.samples[i];s.state=CableRunState::COMPLETE;s.freshnessObserved=s.triggerAccepted=true;
    s.cdcr={0x0102,true,false,true,0};s.index={rows[i][0],true,false,true,0};s.amplitude={rows[i][1],true,false,true,0};s.peak={rows[i][2],true,false,true,0};
  }
  CableDiagnosis result;CHECK(interpretCable(r,4.6,result).ok());
  CHECK(result.run==CableRun::RUN_5 && result.conclusion==CableConclusion::OPEN && result.distanceValid && std::abs(result.metres-100.0)<1e-9);
  r.samples[2].amplitude.raw=11;r.samples[2].index.raw=4;r.samples[2].peak.raw=0x0820;
  CHECK(interpretCable(r,4.6,result).ok() && result.run==CableRun::RUN_2 && result.conclusion==CableConclusion::SHORT);
  CHECK(result.rawBit11 && result.rawBit5 && std::abs(result.metres-8.0/9.2)<1e-9);
  r.samples[2].amplitude.raw=0x007F;CHECK(interpretCable(r,4.6,result).ok() && result.amplitudeAtLimit);
  r.samples[2].peak.raw=1;CHECK(interpretCable(r,4.6,result).ok() && result.conclusion==CableConclusion::SHORT && !result.rawBit11 && !result.rawBit5);
  for(auto& s:r.samples)s.amplitude.raw=0;
  r.samples[0].amplitude.raw=24;r.samples[1].amplitude.raw=24;
  for(unsigned i=2;i<6;++i)r.samples[i].amplitude.raw=10;
  CHECK(interpretCable(r,5.35,result).ok() && result.conclusion==CableConclusion::NO_PEAK && !result.distanceValid);
  r.samples[1].amplitude.raw=25;CHECK(interpretCable(r,5.35,result).ok() && result.run==CableRun::RUN_1B);
  for(unsigned invalid=0;invalid<8;++invalid) {
    auto bad=r;double delay=4.6;
    if(invalid==0)delay=0;
    if(invalid==1)delay=-1;
    if(invalid==2)delay=std::numeric_limits<double>::quiet_NaN();
    if(invalid==3)delay=std::numeric_limits<double>::infinity();
    if(invalid==4)bad.stale=true;
    if(invalid==5)bad.complete=false;
    if(invalid==6)bad.samples[1].index.valid=false;
    if(invalid==7)bad.samples[1].failed=true;
    CableDiagnosis sentinel;sentinel.metres=999;CHECK(!interpretCable(bad,delay,sentinel).ok() && sentinel.metres==999);
  }
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"matrix",matrix},{"raw",raw},{"admission",admission},{"capture",capture},
    {"failures",failures},{"lifecycle",lifecycle},{"timing",timing},{"interpretation",interpretation}};
  return test::runSuites(argc,argv,suites);
}
