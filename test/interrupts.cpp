#include "support/PartSelection.h"
#include <DP8382x/DP8382x.h>
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
MacInterface p22Interface{MacInterface::MII};
bool p22ClearsEnables{false};
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target; };

// Independent literal SNLS647I model. MISR reads clear only their event byte;
// RXFS reads consume its status, while whole-word selector writes do not read it.
// The model proves protocol ownership, not an MCU edge or physical pin behavior.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<Wire,4096> trace{};
  std::array<unsigned,1024> callFirst{};
  uint16_t selector{0},address{0},id2{test::enhancedId2()};
  uint64_t now{100};
  unsigned frames{0},calls{0},clocks{0},releases{0},mutations{0},hard{0};
  int failFrame{-1},releaseCall{-1},corruptFrame{-1},lateCall{-1},regressCall{-1},invalidCall{-1},badReceiptCall{-1};
  uint16_t corruptMask{1};
  int injectFrame{-1}; uint16_t injectMisr1{0},injectMisr2{0};
  int identityChangeFrame{-1}; unsigned mutationsAtIdentityChange{0};
  bool acceptedFailure{true},locked{false},enhancedMii{false},clearEnables{false};
  uint16_t wakeErrors{0}, lpiStatus{0}, eeePartner{0};
  Driver* competitor{nullptr}; unsigned excluded{0};
  Controller() {
    if (isDp83822(test::declaredVariant)) { id2=0xA240; clearEnables=p22ClearsEnables; }
    defaults();
  }
  void defaults() {
    words.fill(0);ext.fill(0);words[0]=0x3100;words[4]=0x01E1;
    words[25]=0x8001;words[22]=0x0100;words[30]=0x0102;
    words[23]=(id2==test::basicId2() || enhancedMii)?0U:0x0020U;
    ext[0x030E]=test::aGrade()?0x4A40:0x8400;ext[0x04A1]=0x1000;
    if (id2==0xA240) {
      ext[0x0467]=p22Interface==MacInterface::MII?0x0FC3:p22Interface==MacInterface::RMII?0x0FE3:0x0DC3;
      ext[0x04A1]=0;
    }
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=test::declaredVariant;c.interface=(id2==test::basicId2() || enhancedMii)?MacInterface::MII:MacInterface::RMII;c.phyAddress=phy;
    if (isDp83822(c.variant)) { c.interface=p22Interface; c.dp83822Led1Mode=1; }
    c.transport={this,execute,clock,{8,4,1}};return c;
  }
  static uint64_t clock(void* context) noexcept { auto& b=*static_cast<Controller*>(context);++b.clocks;return b.now; }
  uint16_t read(uint16_t target) {
    if(target==2)return 0x2000;
    if(target==3)return id2;
    if(target==0)words[0]&=0x7DFF;
    if(target==31)words[31]=0;
    const auto value=target<32U?words[target]:ext[target];
    if(target==18 || target==19) { words[target]&=clearEnables?0:0x00FF;words[16]&=0xFF7F; }
    if(target==20) { words[20]&=0xFF00;words[16]&=0xF7FF; }
    if(target==21) { words[21]=0;words[16]&=0xDFFF; }
    if(target==23)words[23]&=0xFFF3;
    if(target==22 && id2==0xA240)words[22]&=0xFBFF;
    if(target==0x04A1)ext[target]&=0xFF00;
    return value;
  }
  void write(uint8_t reg,uint16_t value) {
    if(reg==13) { CHECK(value==0x001F || value==0x401F || value==3 || value==0x4003 || value==7 || value==0x4007);selector=value;return; }
    if(reg==14 && (selector==0x001F || selector==3 || selector==7)) { address=value;return; }
    const uint16_t target=reg==14?address:uint16_t{reg};++mutations;
    if(target==31 && value==0x8000) { ++hard;defaults();return; }
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
    CHECK(batch.frameCount<=budget.maxFrames);b.locked=true;const auto call=b.calls++;
    CHECK(call<b.callFirst.size());b.callFirst[call]=b.frames;
    for(uint8_t i=0;i<batch.frameCount;++i) {
      const auto& q=batch.frames[i];auto& f=r.frames[i];uint16_t value=q.value;
      if(q.readIndex!=NO_READ_INDEX) { CHECK(q.readIndex<i && r.frames[q.readIndex].valueValid);value=static_cast<uint16_t>((r.frames[q.readIndex].value&q.preserveMask)|q.value); }
      const uint16_t target=q.registerAddress==14 && b.selector==0x401F?b.address:uint16_t{q.registerAddress};
      CHECK(b.frames<b.trace.size());const auto frame=b.frames++;b.trace[frame]={q.kind,q.registerAddress,value,target};
      if(static_cast<int>(frame)==b.identityChangeFrame) { b.id2=test::basicId2();b.mutationsAtIdentityChange=b.mutations; }
      f.attempted=true;const bool failed=static_cast<int>(frame)==b.failFrame;
      if(!failed || b.acceptedFailure) {
        if(q.kind==FrameKind::WRITE)b.write(q.registerAddress,value);
        else if (q.registerAddress==14 && b.selector==0x4003) {
            CHECK(b.address==0 || b.address==1 || b.address==0x0016);
            if (b.address==1) { f.value=b.lpiStatus; }
            else if (b.address==0x0016) { f.value=b.wakeErrors; b.wakeErrors=0; }
        }
          else if (q.registerAddress==14 && b.selector==0x4007) { CHECK(b.address==0x003D); f.value=b.eeePartner; }
        else { f.value=b.read(target);if(static_cast<int>(frame)==b.corruptFrame)f.value^=b.corruptMask; }
      }
      if(static_cast<int>(frame)==b.injectFrame) { b.words[18]|=b.injectMisr1;b.words[19]|=b.injectMisr2; }
      b.now+=2;
      if(b.competitor!=nullptr) {
        uint16_t sentinel=0xCAFE;CHECK(b.competitor->readMmd(31,0x0467,sentinel,budget).is(Err::BUSY));CHECK(sentinel==0xCAFE);++b.excluded;
      }
      if(failed) { f.effectPossible=b.acceptedFailure;r.status={Err::MDIO_TIMEOUT,-27027,"Injected frame"};break; }
      f.completed=true;f.valueValid=q.kind==FrameKind::READ;
    }
    if(static_cast<int>(call)==b.releaseCall) { r.releaseStatus={Err::MDIO_ERROR,-37,"Injected release"};r.controllerReusable=false; }
    if(static_cast<int>(call)==b.lateCall)b.now=budget.deadlineUs;
    if(static_cast<int>(call)==b.regressCall)b.now=0;
    if(static_cast<int>(call)==b.invalidCall)r.status={Err::IN_PROGRESS,-27028,"Illegal terminal status"};
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
void ready(Driver& d,Controller& b) {
  b.words[23]=b.config().interface==MacInterface::RMII?0x0020U:0U;
  CHECK(d.bind(b.config()).ok());OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}
InterruptSettings settings() {
  InterruptSettings s;s.misr1Mask=0xFF;s.misr2Mask=0xFF;s.globalEnable=true;
  s.maintenanceAdmitted=true;s.pinAvailable=true;return s;
}
OperationToken configure(Driver& d,Controller& b,const InterruptSettings& s) {
  OperationToken t{999};const auto calls=b.calls,clocks=b.clocks;
  CHECK(d.startConfigureInterrupts(s,budget(),t).inProgress());
  CHECK(t.value!=999 && b.calls==calls && b.clocks==clocks);return t;
}
void configured(Driver& d,Controller& b,const InterruptSettings& s=settings()) {
  const auto t=configure(d,b,s);CHECK(drain(d,b,t).ok());const auto r=take(d,t);
  CHECK(r.status.ok() && !r.cleanupOwed && d.isInitialized());
  const auto snapshot=d.interruptSnapshot();
  CHECK(snapshot.desiredValid && snapshot.trust==ConfigurationTrust::VERIFIED && !snapshot.observedStale);
}
InterruptServiceBudget serviceBudget(uint32_t frames=5,uint8_t calls=2,uint8_t samples=5) {
  return {{1000000,frames},calls,samples};
}
const EvidenceSample& misr2(const EvidenceSnapshot& s) { return s.samples[static_cast<unsigned>(EvidenceSource::MISR2)]; }

void policy() {
  {
    Controller b;Driver d;ready(d,b);const auto frames=b.frames,calls=b.calls;
    configured(d,b);
    // Pin 21: identity, control admission, disable/drain, whole MMD selector
    // write/readback, crossing drain, two masks, output/global and three verifies.
    CHECK(b.frames==frames+26U && b.calls==calls+14U);
  }
  for(bool basic:{false,true})for(bool activeLow:{false,true})for(bool enabled:{false,true})for(bool output:{false,true}) {
    Controller b;b.id2=basic?test::basicId2():test::enhancedId2();Driver d;ready(d,b);b.words[17]=0xA5F1;
    b.words[18]=0xA520;b.words[19]=0x5A02;
    auto s=settings();s.activeLow=activeLow;s.globalEnable=enabled;s.outputEnable=output;s.powerDownInputSafe=true;
    s.misr1Mask=0x20;s.misr2Mask=0x02;
    const auto first=b.frames;configured(d,b,s);
    const uint16_t mask=basic?0x000A:0x000B;
    const uint16_t expected=static_cast<uint16_t>((activeLow?8U:0U)|(enabled?2U:0U)|(basic?1U:(output?1U:0U)));
    CHECK(b.words[17]==static_cast<uint16_t>((0xA5F1U&~mask)|expected));
    CHECK((b.words[18]&0x00FFU)==0x20 && (b.words[19]&0x00FFU)==2);
    const auto retained=d.evidenceSnapshot();
    CHECK((retained.samples[static_cast<unsigned>(EvidenceSource::MISR1)].eventBits&0xA500U)==0xA500U);
    CHECK((misr2(retained).eventBits&0x5A00U)==0x5A00U);
    CHECK(d.interruptSnapshot().basicStrapInterrupt==basic);
    for(unsigned i=first;i<b.frames;++i) {
      CHECK(b.trace[i].target!=0x0304 && b.trace[i].target!=0x0305);
      if(b.trace[i].kind==FrameKind::WRITE && b.trace[i].target==17)CHECK((b.trace[i].value&4U)==0U);
    }
    const auto calls=b.calls,clocks=b.clocks;
    CHECK(d.interruptSnapshot().physcrValid && d.interruptSnapshot().misr1Valid && d.interruptSnapshot().misr2Valid);
    CHECK(d.interruptSnapshot().rxfsValid && b.calls==calls && b.clocks==clocks);
  }
  for(unsigned bank=0;bank<2U;++bank)for(unsigned bit=0;bit<8U;++bit) {
    Controller b;Driver d;ready(d,b);auto s=settings();
    s.misr1Mask=static_cast<uint16_t>(bank==0U?(1U<<bit):0U);
    s.misr2Mask=static_cast<uint16_t>(bank==1U?(1U<<bit):0U);
    configured(d,b,s);CHECK((b.words[18]&0xFFU)==s.misr1Mask && (b.words[19]&0xFFU)==s.misr2Mask);
  }
}

void service() {
  for(unsigned bank=0;bank<2U;++bank)for(unsigned bit=0;bit<8U;++bit) {
    Controller b;Driver d;ready(d,b);configured(d,b);
    const uint16_t event=static_cast<uint16_t>(0x0100U<<bit);
    b.words[18]=bank==0U?static_cast<uint16_t>(event|0x20U):0x20;
    b.words[19]=bank==1U?static_cast<uint16_t>(event|2U):2;
    b.words[20]=0xFF;b.words[21]=0x8123;b.words[16]=0; // PHYSTS 7 is deliberately clear.
    const auto first=b.frames,calls=b.calls;InterruptServiceSnapshot out;
    CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
    const unsigned counters=bank==0U && bit<2U?1U:0U;
    CHECK(b.frames==first+3U+counters && b.calls==calls+1U+counters);
    CHECK(b.trace[first].target==16 && b.trace[first+1U].target==18 && b.trace[first+2U].target==19);
    CHECK(out.valid && !out.stale && out.morePossiblyPending);
    CHECK((out.misr1.raw&0xFF00U)==(bank==0U?event:0U));
    CHECK((out.misr2.raw&0xFF00U)==(bank==1U?event:0U));
    CHECK(out.fcscrRequested==(bank==0U && bit==1U));CHECK(out.recrRequested==(bank==0U && bit==0U));
    if(out.fcscrRequested)CHECK(out.fcscr.raw==0xFF && out.fcscr.counterLowerBound);
    if(out.recrRequested)CHECK(out.recr.raw==0x8123 && out.recr.sampledSum==0U);
    const auto cachedCalls=b.calls,cachedClocks=b.clocks;
    CHECK(d.interruptServiceSnapshot().misr1.raw==out.misr1.raw);
    CHECK(d.interruptServiceSnapshot().misr2.raw==out.misr2.raw);
    CHECK(b.calls==cachedCalls && b.clocks==cachedClocks);
    CHECK(d.serviceInterrupts(out,serviceBudget()).ok() && !out.morePossiblyPending);
  }
  Controller b;Driver d;ready(d,b);b.words[18]=0xFF00;b.words[19]=0xFF00;b.words[20]=0x80;b.words[21]=0xFF;
  const auto first=b.frames;InterruptServiceSnapshot out;
  CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
  CHECK(b.frames==first+5U && out.fcscrRequested && out.recrRequested);
  // New MISR1 evidence after its read remains in hardware for the next bounded pass.
  b.injectFrame=static_cast<int>(b.frames+1U);b.injectMisr1=0x2000;b.injectMisr2=0x8000;
  CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
  CHECK((out.misr1.raw&0xFF00U)==0 && (out.misr2.raw&0xFF00U)==0x8000);
  CHECK((b.words[18]&0x2000U)!=0U);b.injectFrame=-1;
  CHECK(d.serviceInterrupts(out,serviceBudget()).ok() && (out.misr1.raw&0x2000U)!=0U);
  for(unsigned bound=0;bound<3U;++bound) {
    Controller empty;Driver driver;ready(driver,empty);auto allowance=serviceBudget();
    if(bound==0U)allowance.maxTransactions=1;
    if(bound==1U)allowance.maxSamples=3;
    if(bound==2U)allowance.io.maxFrames=3;
    CHECK(driver.serviceInterrupts(out,allowance).ok());
    CHECK(out.morePossiblyPending && (out.misr1.raw&0xFF00U)==0U && (out.misr2.raw&0xFF00U)==0U);
  }
  for(unsigned point=0;point<5U;++point) {
    Controller bus;Driver driver;ready(driver,bus);bus.words[18]=0x0300;
    bus.injectFrame=static_cast<int>(bus.frames+point);bus.injectMisr1=0x2000;bus.injectMisr2=0x8000;
    CHECK(driver.serviceInterrupts(out,serviceBudget()).ok());
    CHECK((out.misr1.raw&0x2000U)==(point==0U?0x2000U:0U));
    CHECK((out.misr2.raw&0x8000U)==(point<2U?0x8000U:0U));
    CHECK((bus.words[18]&0x2000U)==(point>0U?0x2000U:0U));
    CHECK((bus.words[19]&0x8000U)==(point>1U?0x8000U:0U));
    bus.injectFrame=-1;CHECK(driver.serviceInterrupts(out,serviceBudget()).ok());
    CHECK((out.misr1.raw&0x2000U)==(point>0U?0x2000U:0U));
    CHECK((out.misr2.raw&0x8000U)==(point>1U?0x8000U:0U));
  }
}

void attribution() {
  Controller b;Driver d;ready(d,b);InterruptServiceSnapshot out;
  b.words[19]=0x0200;CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
  CHECK(out.misr2.sharedSource==InterruptSharedSource::UNKNOWN && out.misr2.sharedAmbiguous);
  auto s=settings();s.sharedSource=InterruptSharedSource::POLARITY;configured(d,b,s);
  b.words[19]=0x0200;CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
  CHECK(out.misr2.sharedSource==InterruptSharedSource::POLARITY && out.misr2.polaritySeen);
  const auto prior=out.misr2;s.sharedSource=InterruptSharedSource::WAKE;
  // Pending old-source evidence must be consumed before selector change.
  b.words[19]=0x0200;const auto first=b.frames;configured(d,b,s);
  unsigned selectorWrite=b.frames;
  for(unsigned i=first;i<b.frames;++i)if(b.trace[i].kind==FrameKind::WRITE && b.trace[i].target==0x04A1) { selectorWrite=i;CHECK(b.trace[i].value==0x1000);break; }
  CHECK(selectorWrite<b.frames);bool drained=false;
  for(unsigned i=first;i<selectorWrite;++i)if(b.trace[i].kind==FrameKind::READ && b.trace[i].target==19)drained=true;
  CHECK(drained && prior.sharedSource==InterruptSharedSource::POLARITY);
  b.words[19]=0x0200;CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
  CHECK(out.misr2.sharedSource==InterruptSharedSource::WAKE && out.misr2.wakeSeen && out.misr2.polaritySeen);
  // Events introduced at the selector write belong to an uncertain crossing.
  Controller reference;Driver ref;ready(ref,reference);configured(ref,reference,settings());
  const auto start=reference.frames;auto polarity=settings();polarity.sharedSource=InterruptSharedSource::POLARITY;configured(ref,reference,polarity);
  unsigned offset=0;
  for(unsigned i=start;i<reference.frames;++i)if(reference.trace[i].kind==FrameKind::WRITE && reference.trace[i].target==0x04A1) { offset=i-start;break; }
  CHECK(offset>0U);
  Controller crossed;Driver cross;ready(cross,crossed);configured(cross,crossed);CHECK(cross.clearEvidence().ok());
  crossed.injectFrame=static_cast<int>(crossed.frames+offset);crossed.injectMisr2=0x0200;
  configured(cross,crossed,polarity);const auto history=cross.evidenceSnapshot();
  CHECK(misr2(history).sharedAmbiguous && !misr2(history).polaritySeen && !misr2(history).wakeSeen);
  CHECK(cross.interruptSnapshot().selector==InterruptSharedSource::POLARITY);
  // A contradictory selector read invalidates attribution before returning a
  // readback mismatch or a later callback-release error. Raw collector reads
  // use exactly the same rule as verify-only operations.
  for(bool raw:{false,true})for(bool release:{false,true}) {
    Controller bus;Driver driver;ready(driver,bus);configured(driver,bus);bus.ext[0x04A1]=0;
    if(raw) {
      if(release)bus.releaseCall=static_cast<int>(bus.calls);
      EvidenceSnapshot captured;
      CHECK(driver.acquireEvidence(evidenceBit(EvidenceSource::RXFS),captured,{1000000,4}).ok()==!release);
    } else {
      Controller model;Driver baseline;ready(baseline,model);configured(baseline,model);OperationToken verify;
      const auto firstCall=model.calls;CHECK(baseline.startVerify(budget(),verify).inProgress());CHECK(drain(baseline,model,verify).ok());take(baseline,verify);
      unsigned rxfsCall=firstCall;
      for(unsigned c=firstCall;c<model.calls;++c) {
        const auto end=c+1U<model.calls?model.callFirst[c+1U]:model.frames;
        for(unsigned f=model.callFirst[c];f<end;++f)if(model.trace[f].kind==FrameKind::READ && model.trace[f].target==0x04A1)rxfsCall=c;
      }
      if(release)bus.releaseCall=static_cast<int>(bus.calls+rxfsCall-firstCall);
      CHECK(driver.startVerify(budget(),verify).inProgress());
      CHECK(drain(driver,bus,verify).is(release?Err::MDIO_ERROR:Err::CONFIG_MISMATCH));
      CHECK(take(driver,verify).mismatch.valid);
    }
    CHECK(driver.interruptSnapshot().selector==InterruptSharedSource::UNKNOWN);
    CHECK(driver.interruptSnapshot().trust==ConfigurationTrust::DIRTY && driver.interruptSnapshot().observedStale);
    if(!release) {
      bus.words[19]=0x0200;CHECK(driver.serviceInterrupts(out,serviceBudget()).ok());
      CHECK(out.misr2.sharedSource==InterruptSharedSource::UNKNOWN && out.misr2.sharedAmbiguous && !out.misr2.wakeSeen);
    }
  }
}

void serviceFailures() {
  for(unsigned fail=0;fail<5U;++fail) {
    Controller b;Driver d;ready(d,b);b.words[18]=0x0300;b.words[19]=0xA500;b.words[20]=0x80;b.words[21]=0xFF;
    const auto first=b.frames;b.failFrame=static_cast<int>(first+fail);
    InterruptServiceSnapshot out;out.rawPhysts=0xCAFE;
    CHECK(d.serviceInterrupts(out,serviceBudget()).is(Err::MDIO_TIMEOUT));CHECK(out.rawPhysts==0xCAFE);
    CHECK(b.frames==first+fail+1U && b.releases==b.calls);
    const auto evidence=d.evidenceSnapshot();
    if(fail>1U)CHECK(evidence.samples[static_cast<unsigned>(EvidenceSource::MISR1)].raw==0x0300);
    if(fail>2U)CHECK(misr2(evidence).raw==0xA500);
    constexpr EvidenceSource sources[]={EvidenceSource::PHYSTS,EvidenceSource::MISR1,EvidenceSource::MISR2,EvidenceSource::FCSCR,EvidenceSource::RECR};
    CHECK(evidence.samples[static_cast<unsigned>(sources[fail])].historyLost);
    CHECK(d.interruptServiceSnapshot().stale && d.interruptServiceSnapshot().morePossiblyPending);
    b.failFrame=-1;CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
    CHECK(d.evidenceSnapshot().samples[static_cast<unsigned>(sources[fail])].historyLost);
  }
  for(unsigned bound=0;bound<3U;++bound) {
    Controller b;Driver d;ready(d,b);b.words[18]=0x0300;InterruptServiceSnapshot out;out.rawPhysts=0xCAFE;
    auto allowance=serviceBudget();if(bound==0U)allowance.maxTransactions=1;if(bound==1U)allowance.maxSamples=3;if(bound==2U)allowance.io.maxFrames=3;
    const auto first=b.frames;CHECK(!d.serviceInterrupts(out,allowance).ok());
    CHECK(out.rawPhysts==0xCAFE && b.frames<=first+3U && d.interruptServiceSnapshot().morePossiblyPending);
    CHECK(d.evidenceSnapshot().pendingInterruptCounters==3U);
    const auto next=b.frames;CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
    CHECK(b.frames==next+5U && out.fcscrRequested && out.recrRequested && (out.misr1.raw&0xFF00U)==0U);
    CHECK(d.evidenceSnapshot().pendingInterruptCounters==0U);
  }
  for(unsigned action=0;action<3U;++action) {
    Controller b;Driver d;ready(d,b);b.words[18]=0x0300;b.words[20]=0x91;b.words[21]=0x8123;
    InterruptServiceSnapshot out;b.failFrame=static_cast<int>(b.frames+3U);
    CHECK(d.serviceInterrupts(out,serviceBudget()).is(Err::MDIO_TIMEOUT));
    CHECK(d.evidenceSnapshot().pendingInterruptCounters==3U);
    CHECK(d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::FCSCR)].historyLost);
    b.failFrame=-1;
    if(action==0U) {
      const auto first=b.frames;CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
      CHECK(b.frames==first+5U && out.fcscrRequested && out.recrRequested && out.fcscr.historyLost);
    } else if(action==1U) {
      CounterSnapshot counters;CHECK(d.collectEvents(counters,{1000000,6}).ok());
      CHECK(counters.falseCarrierLowerBound);
    } else { CHECK(d.clearEvidence().ok()); }
    CHECK(d.evidenceSnapshot().pendingInterruptCounters==0U);
  }
  for(unsigned fault=0;fault<4U;++fault) {
    Controller b;Driver d;ready(d,b);const auto calls=b.calls;
    if(fault==0U)b.lateCall=static_cast<int>(calls);
    if(fault==1U)b.regressCall=static_cast<int>(calls);
    if(fault==2U)b.invalidCall=static_cast<int>(calls);
    if(fault==3U)b.badReceiptCall=static_cast<int>(calls);
    b.words[18]=0x2000;InterruptServiceSnapshot out;out.rawPhysts=0xCAFE;
    CHECK(!d.serviceInterrupts(out,serviceBudget()).ok() && out.rawPhysts==0xCAFE);
    CHECK(b.calls==calls+1U && d.interruptServiceSnapshot().stale && d.interruptServiceSnapshot().morePossiblyPending);
    if(fault>=2U)CHECK(d.transportSnapshot().violation!=TransportViolation::NONE);
  }
}

void failures() {
  Controller reference;Driver ref;ready(ref,reference);const auto first=reference.frames,firstCall=reference.calls;
  configured(ref,reference);const auto count=reference.frames-first,calls=reference.calls-firstCall;CHECK(count<128 && calls<64);
  for(unsigned phase=0;phase<count;++phase) {
    Controller b;Driver d;ready(d,b);const auto frames=b.frames;b.failFrame=static_cast<int>(frames+phase);
    const auto t=configure(d,b,settings());CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));const auto result=take(d,t);
    CHECK(result.status.detail==-27027 && b.frames==frames+phase+1U);
    CHECK(d.interruptSnapshot().trust!=ConfigurationTrust::VERIFIED && !result.persistentRestored);
    if(result.mutationPossible || result.mutationCompleted)CHECK(result.cleanupOwed);
  }
  for(unsigned phase=0;phase<calls;++phase)for(bool cancel:{false,true}) {
    Controller b;Driver d;ready(d,b);const auto t=configure(d,b,settings());
    if(cancel) {
      for(unsigned n=0;n<phase;++n)CHECK(poll(d,b,t).inProgress());
      const auto frames=b.frames,clocks=b.clocks;CHECK(d.cancel(t).ok());CHECK(b.frames==frames && b.clocks==clocks);
    }else { b.releaseCall=static_cast<int>(b.calls+phase);CHECK(drain(d,b,t).is(Err::MDIO_ERROR)); }
    const auto result=take(d,t);CHECK(result.status.is(cancel?Err::CANCELLED:Err::MDIO_ERROR));
    CHECK(d.interruptSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
  // Corrupt only the final stable readback of each owned register. A prior
  // valid mismatch must remain visible even if release from that read fails.
  for(uint16_t target:{uint16_t{17},uint16_t{18},uint16_t{19},uint16_t{0x04A1}}) {
    unsigned chosen=first;
    for(unsigned i=first;i<reference.frames;++i)if(reference.trace[i].target==target && reference.trace[i].kind==FrameKind::READ)chosen=i;
    CHECK(chosen>first);unsigned chosenCall=firstCall;
    while(chosenCall+1U<reference.calls && reference.callFirst[chosenCall+1U]<=chosen)++chosenCall;
    for(bool release:{false,true}) {
      Controller b;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+chosen-first);
      b.corruptMask=target==0x04A1?0x1000:uint16_t{1};
      if(release)b.releaseCall=static_cast<int>(b.calls+chosenCall-firstCall);
      const auto t=configure(d,b,settings());CHECK(drain(d,b,t).is(release?Err::MDIO_ERROR:Err::CONFIG_MISMATCH));
      const auto result=take(d,t);CHECK(result.mismatch.valid && result.cleanupOwed && !result.persistentRestored);
    }
  }
}

void liveModeRouting() {
  // No PinSettings owner exists: each direct source must verify actual mode.
  for (bool mii:{false,true}) for (unsigned outcome=0;outcome<4U;++outcome) {
    Controller b;b.enhancedMii=mii;Driver d;ready(d,b);auto s=settings();
    s.pin=InterruptPin::LED2; s.pinAvailable=true;s.colUnused=mii;
    b.words[23]=static_cast<uint16_t>((mii?0U:0x0020U)|0x000CU);
    if (outcome==1U) { b.words[23]^=0x0020U; }
    const auto first=b.frames,calls=b.calls,mutations=b.mutations;
    if (outcome==2U) { b.failFrame=static_cast<int>(first+6U); }
    const auto token=configure(d,b,s);
    if (outcome==0U || outcome==3U) {
      CHECK(drain(d,b,token).ok());take(d,token);
      CHECK(b.frames==first+40U && b.calls==calls+17U);
      const auto view=d.interruptSnapshot();CHECK(view.trust==ConfigurationTrust::VERIFIED);
      const auto evidence=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
      CHECK(evidence.valid && (evidence.raw&0x000CU)==0x000CU && !evidence.historyLost);
      CHECK((b.words[23]&0x000CU)==0U);
      if (outcome==3U) {
        b.words[23]^=0x0020U;EvidenceSnapshot retained;
        CHECK(d.acquireEvidence(evidenceBit(EvidenceSource::RCSR),retained,{1000000,1}).ok());
        CHECK(d.interruptSnapshot().trust==ConfigurationTrust::DIRTY && d.interruptSnapshot().observedStale);
      }
    } else {
      CHECK(!drain(d,b,token).ok());const auto result=take(d,token);
      CHECK(b.mutations==mutations && !result.mutationPossible && !result.cleanupOwed);
      CHECK(b.frames==first+7U && b.calls==calls+2U);
      const auto evidence=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
      if (outcome==2U) { CHECK(result.status.is(Err::MDIO_TIMEOUT) && evidence.historyLost && !evidence.valid); }
      else { CHECK(evidence.valid && !evidence.historyLost && (evidence.raw&0x000CU)==0x000CU); }
    }
  }
  // A second collector observation between the admission read and first
  // mutation revokes the mode proof; the operation must not keep writing.
  Controller b;Driver d;ready(d,b);auto s=settings();s.pin=InterruptPin::LED2; s.pinAvailable=true;
  const auto token=configure(d,b,s);CHECK(poll(d,b,token).inProgress());CHECK(poll(d,b,token).inProgress());
  const auto mutations=b.mutations;b.words[23]=0;EvidenceSnapshot evidence;
  CHECK(d.acquireEvidence(evidenceBit(EvidenceSource::RCSR),evidence,{1000000,1}).ok());
  CHECK(!drain(d,b,token).ok());take(d,token);CHECK(b.mutations==mutations);
}
void pins() {
  liveModeRouting();
  for(unsigned invalid=0;invalid<7U;++invalid) {
    Controller b;Driver d;ready(d,b);auto s=settings();
    if(invalid==0U)s.misr1Mask=0x100;
    if(invalid==1U)s.misr2Mask=0x8000;
    if(invalid==2U)s.sharedSource=InterruptSharedSource::UNKNOWN;
    if(invalid==3U)s.maintenanceAdmitted=false;
    if(invalid==4U)s.pinAvailable=false;
    if(invalid==5U)s.pin=static_cast<InterruptPin>(255);
    if(invalid==6U)s.outputEnable=false;
    const auto calls=b.calls,clocks=b.clocks;OperationToken token{999};CHECK(!d.startConfigureInterrupts(s,budget(),token).inProgress());
    CHECK(token.value==999 && b.calls==calls && b.clocks==clocks);
  }
  for(bool led1:{false,true})for(bool validMux:{false,true}) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.pin=led1?InterruptPin::LED1:InterruptPin::LED2;
    s.led1ClockUnused=true;s.colUnused=true;b.ext[0x0308]=1;b.ext[led1?0x0304:0x0305]=validMux?4:0;
    const auto first=b.frames;const auto t=configure(d,b,s);CHECK(drain(d,b,t).ok());take(d,t);
    bool disabled=false;unsigned muxWrites=0;
    for(unsigned i=first;i<b.frames;++i) {
      const auto& frame=b.trace[i];if(frame.kind!=FrameKind::WRITE)continue;
      if(frame.target==0x11 && (frame.value&2U)==0U)disabled=true;
      if(frame.target==(led1?0x0304:0x0305)) { CHECK(disabled && frame.value==4U);++muxWrites; }
    }
    CHECK(muxWrites==(validMux?0U:1U) && (b.ext[led1?0x0304:0x0305]&7U)==4U);
    b.ext[led1?0x0304:0x0305]=0;const auto writes=b.mutations;OperationToken verify;
    CHECK(d.startVerify(budget(),verify).inProgress());CHECK(drain(d,b,verify).is(Err::INVALID_CONFIG));take(d,verify);
    CHECK(b.mutations==writes); // Verification never silently repairs a mux.
  }
  Controller basic;basic.id2=test::basicId2();Driver d;ready(d,basic);auto s=settings();s.pin=InterruptPin::LED2;
  const auto calls=basic.calls;OperationToken token;CHECK(!d.startConfigureInterrupts(s,budget(),token).inProgress() && basic.calls==calls);
  for(auto route:{InterruptPin::LED1,InterruptPin::LED2}) {
    Controller bus;Driver driver;ready(driver,bus);auto denied=settings();denied.pin=route;denied.pinAvailable=false;
    denied.led1ClockUnused=denied.colUnused=true;
    const auto count=bus.calls,clocks=bus.clocks;OperationToken sentinel{999};
    CHECK(driver.startConfigureInterrupts(denied,budget(),sentinel).is(Err::INVALID_CONFIG));
    CHECK(sentinel.value==999 && bus.calls==count && bus.clocks==clocks);
  }
  // Stage 32's source-owned route mutation is one complete five-frame RMW,
  // then four-frame readback; fail every new phase before/after acceptance.
  Controller reference;Driver model;ready(model,reference);auto led=settings();led.pin=InterruptPin::LED2;
  reference.ext[0x305]=0xA5A8;const auto first=reference.frames;configured(model,reference,led);
  CHECK(reference.ext[0x305]==0xA5AC);
  unsigned targetWrite=0;
  for(unsigned i=first;i<reference.frames;++i) {
    if(reference.trace[i].kind==FrameKind::WRITE && reference.trace[i].target==0x305)targetWrite=i;
  }
  CHECK(targetWrite>=first+4U);
  for(unsigned phase=targetWrite-4U;phase<=targetWrite+4U;++phase)for(bool accepted:{false,true}) {
    Controller bus;Driver driver;ready(driver,bus);bus.ext[0x305]=0xA5A8;const auto start=bus.frames;
    bus.failFrame=static_cast<int>(start+phase-first);bus.acceptedFailure=accepted;
    const auto t=configure(driver,bus,led);CHECK(drain(driver,bus,t).is(Err::MDIO_TIMEOUT));const auto result=take(driver,t);
    CHECK(bus.frames==start+phase-first+1U && result.cleanupOwed && !result.persistentRestored);
    CHECK(driver.interruptSnapshot().trust!=ConfigurationTrust::VERIFIED && !bus.locked);
  }
}

void stimulus() {
  Controller b;Driver d;ready(d,b);configured(d,b);OperationToken token;
  const auto calls=b.calls,clocks=b.clocks;CHECK(d.startInterruptTest(budget(),token).inProgress());CHECK(b.calls==calls && b.clocks==clocks);
  CHECK(drain(d,b,token).ok());const auto started=take(d,token);
  CHECK((b.words[17]&4U)!=0U && d.interruptSnapshot().testMayBeActive && started.cleanupOwed);
  InterruptServiceSnapshot observed;CHECK(d.serviceInterrupts(observed,serviceBudget()).ok() && !observed.morePossiblyPending);
  CHECK(d.stopInterruptTest(budget(),token).inProgress());CHECK(drain(d,b,token).ok());const auto stopped=take(d,token);
  CHECK((b.words[17]&4U)==0U && !d.interruptSnapshot().testMayBeActive && !stopped.cleanupOwed);
  CHECK(started.cleanupOwed && started.status.ok());
  for(bool stable:{false,true})for(bool release:{false,true}) {
    Controller model;Driver baseline;ready(baseline,model);configured(baseline,model);OperationToken t;
    const auto first=model.frames,firstCall=model.calls;CHECK(baseline.startInterruptTest(budget(),t).inProgress());
    CHECK(drain(baseline,model,t).ok());take(baseline,t);
    Controller bus;Driver driver;ready(driver,bus);configured(driver,bus);
    bus.corruptFrame=static_cast<int>(bus.frames+model.frames-first-1U);bus.corruptMask=stable?2:4;
    if(release)bus.releaseCall=static_cast<int>(bus.calls+model.calls-firstCall-1U);
    CHECK(driver.startInterruptTest(budget(),t).inProgress());
    CHECK(drain(driver,bus,t).is(release?Err::MDIO_ERROR:Err::CONFIG_MISMATCH));const auto failed=take(driver,t);
    CHECK(failed.mismatch.valid && failed.cleanupOwed && driver.interruptSnapshot().testMayBeActive);
    CHECK(driver.interruptSnapshot().trust==(stable?ConfigurationTrust::DIRTY:ConfigurationTrust::VERIFIED));
  }
  for(bool stopping:{false,true}) {
    Controller reference;Driver ref;ready(ref,reference);configured(ref,reference);OperationToken t;
    if(stopping) { CHECK(ref.startInterruptTest(budget(),t).inProgress());CHECK(drain(ref,reference,t).ok());take(ref,t); }
    const auto first=reference.frames,firstCall=reference.calls;
    CHECK((stopping?ref.stopInterruptTest(budget(),t):ref.startInterruptTest(budget(),t)).inProgress());
    CHECK(drain(ref,reference,t).ok());take(ref,t);const auto frameCount=reference.frames-first,callCount=reference.calls-firstCall;
    for(unsigned phase=0;phase<frameCount;++phase) {
      Controller bus;Driver driver;ready(driver,bus);configured(driver,bus);
      if(stopping) { CHECK(driver.startInterruptTest(budget(),t).inProgress());CHECK(drain(driver,bus,t).ok());take(driver,t); }
      const auto frames=bus.frames;bus.failFrame=static_cast<int>(frames+phase);
      CHECK((stopping?driver.stopInterruptTest(budget(),t):driver.startInterruptTest(budget(),t)).inProgress());
      CHECK(drain(driver,bus,t).is(Err::MDIO_TIMEOUT));const auto r=take(driver,t);CHECK(bus.frames==frames+phase+1U);
      if(stopping || r.mutationPossible || r.mutationCompleted)CHECK(r.cleanupOwed && driver.interruptSnapshot().testMayBeActive);
    }
    for(unsigned phase=0;phase<callCount;++phase) {
      Controller bus;Driver driver;ready(driver,bus);configured(driver,bus);
      if(stopping) { CHECK(driver.startInterruptTest(budget(),t).inProgress());CHECK(drain(driver,bus,t).ok());take(driver,t); }
      CHECK((stopping?driver.stopInterruptTest(budget(),t):driver.startInterruptTest(budget(),t)).inProgress());
      for(unsigned n=0;n<phase;++n)CHECK(poll(driver,bus,t).inProgress());
      const auto frames=bus.frames;CHECK(driver.cancel(t).ok() && bus.frames==frames);CHECK(take(driver,t).status.is(Err::CANCELLED));
    }
  }
}

void replay() {
  {
    Controller b;Driver d;ready(d,b);auto alternate=settings();alternate.pin=InterruptPin::LED1;
    alternate.led1ClockUnused=true;b.ext[0x0308]=1;b.ext[0x0304]=4;configured(d,b,alternate);
    const auto first=b.frames;OperationToken token;
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    CHECK(drain(d,b,token).is(Err::INVALID_CONFIG));const auto result=take(d,token);
    CHECK(result.cleanupOwed && !result.persistentRestored && !d.isInitialized());
    CHECK(d.interruptSnapshot().trust!=ConfigurationTrust::VERIFIED && b.ext[0x0304]==0);
    bool resetSeen=false;
    for(unsigned i=first;i<b.frames;++i) {
      const auto& frame=b.trace[i];
      if(frame.kind==FrameKind::WRITE && frame.target==31 && frame.value==0x8000)resetSeen=true;
      if(resetSeen && frame.kind==FrameKind::WRITE) {
        CHECK(frame.target!=17 && frame.target!=18 && frame.target!=19 && frame.target!=0x04A1);
        CHECK(frame.target!=0x0304 && frame.target!=0x0308);
      }
    }
    CHECK(resetSeen); // Reset removed clock-release evidence; IRQ cannot repair the clock owner.
  }
  for(auto kind:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.misr1Mask=0x20;s.misr2Mask=2;s.sharedSource=InterruptSharedSource::POLARITY;configured(d,b,s);
    OperationToken token;CHECK(d.startInitialize({kind},budget(),token).inProgress());CHECK(drain(d,b,token).ok());const auto r=take(d,token);
    CHECK(r.persistentRestored && d.interruptSnapshot().trust==ConfigurationTrust::VERIFIED);
    CHECK((b.words[17]&0x000BU)==0x000BU && (b.words[18]&0xFFU)==0x20 && (b.words[19]&0xFFU)==2 && (b.ext[0x04A1]&0x1000U)==0);
    const auto writes=b.mutations;CHECK(d.startVerify(budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);CHECK(b.mutations==writes);
  }
  Controller reference;Driver ref;ready(ref,reference);configured(ref,reference);OperationToken t;
  const auto first=reference.frames;CHECK(ref.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(drain(ref,reference,t).ok());take(ref,t);
  unsigned tail=first;
  for(unsigned i=first;i<reference.frames;++i)if(reference.trace[i].kind==FrameKind::READ && reference.trace[i].target==17) { tail=i;break; }
  CHECK(tail>first);
  for(unsigned phase=tail;phase<reference.frames;++phase) {
    Controller b;Driver d;ready(d,b);configured(d,b);b.failFrame=static_cast<int>(b.frames+phase-first);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));const auto failed=take(d,t);
    CHECK(!failed.persistentRestored && failed.cleanupOwed && !d.isInitialized());
    b.failFrame=-1;CHECK(d.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(drain(d,b,t).ok());
    CHECK(take(d,t).persistentRestored && d.interruptSnapshot().trust==ConfigurationTrust::VERIFIED);
    CHECK(failed.status.is(Err::MDIO_TIMEOUT));
  }
  // A later IRQ-tail identity sample cannot validate settings proved under the
  // earlier mode. Inject the BASIC model during either frame of that final batch.
  for(unsigned kind=0;kind<4U;++kind)for(unsigned word=0;word<2U;++word) {
    const auto initializeProfile=[](Driver& driver,Controller& bus) {
      ready(driver,bus);LinkSettings profile;profile.advertisement.technologyMask=LINK_100_FULL;OperationToken token;
      CHECK(driver.startConfigure(profile,budget(),token).inProgress());CHECK(drain(driver,bus,token).ok());take(driver,token);configured(driver,bus);
    };
    const auto start=[&](Driver& driver,OperationToken& token) {
      if(kind<2U)return driver.startInitialize({kind==0U?ResetKind::NONE:ResetKind::HARD},budget(),token);
      if(kind==2U)return driver.startVerify(budget(),token);
      LinkSettings profile;profile.advertisement.technologyMask=LINK_10_FULL;
      return driver.startConfigure(profile,budget(),token);
    };
    Controller model;Driver baseline;initializeProfile(baseline,model);OperationToken token;
    const auto first=model.frames;CHECK(start(baseline,token).inProgress());CHECK(drain(baseline,model,token).ok());take(baseline,token);
    unsigned identity=first;
    for(unsigned i=first;i<model.frames;++i)if(model.trace[i].kind==FrameKind::READ && model.trace[i].target==2)identity=i;
    CHECK(identity>first && model.trace[identity+1U].target==3);
    Controller bus;Driver driver;initializeProfile(driver,bus);const auto initial=bus.frames;
    bus.identityChangeFrame=static_cast<int>(initial+identity-first+word);
    CHECK(start(driver,token).inProgress());const auto status=drain(driver,bus,token);
    CHECK(status.is(Err::INVALID_CONFIG) && status.detail==static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
    const auto result=take(driver,token);
    CHECK(bus.frames==initial+identity-first+2U && bus.mutations==bus.mutationsAtIdentityChange);
    CHECK(!driver.isInitialized() && !result.persistentRestored && driver.settingsSnapshot().trust==ConfigurationTrust::DIRTY);
    CHECK(result.identity.id1Valid && result.identity.id2Valid && result.identity.id1==0x2000 && result.identity.id2==test::basicId2());
    CHECK(result.identity.mode==OperatingMode::BASIC && driver.interruptSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
}

// Fresh prompt 27/28 audit: a valid prefix and an observation between owner
// turns can withdraw earlier proof; a later successful read cannot restore it.
void auditRegressions() {
  for(bool stopping:{false,true}) {
    Controller b;Driver d;ready(d,b);configured(d,b);OperationToken t;
    if(stopping) { CHECK(d.startInterruptTest(budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t); }
    CHECK((stopping?d.stopInterruptTest(budget(),t):d.startInterruptTest(budget(),t)).inProgress());
    CHECK(poll(d,b,t).inProgress()); // identity
    if(!stopping)CHECK(poll(d,b,t).inProgress()); // test preflight
    b.words[17]&=0xFFFD; // observed global-enable contradiction at RMW read
    b.failFrame=static_cast<int>(b.frames+1U);b.acceptedFailure=false;
    CHECK(poll(d,b,t).is(Err::MDIO_TIMEOUT));const auto result=take(d,t);
    const auto snapshot=d.interruptSnapshot();
    CHECK(result.status.detail==-27027 && snapshot.rawPhyscr==b.words[17]);
    CHECK(snapshot.physcrValid && snapshot.observedStale && snapshot.trust==ConfigurationTrust::DIRTY);
    CHECK(result.mismatch.valid && !result.persistentRestored);
  }
  {
    Controller b;Driver d;ready(d,b);configured(d,b);OperationToken t;
    CHECK(d.startInterruptTest(budget(),t).inProgress());CHECK(poll(d,b,t).inProgress());
    b.words[17]&=0xFFFD;b.releaseCall=static_cast<int>(b.calls);
    CHECK(poll(d,b,t).is(Err::MDIO_ERROR));const auto result=take(d,t);
    CHECK(result.mismatch.valid && result.status.detail==-37);
    CHECK(d.interruptSnapshot().rawPhyscr==b.words[17] && d.interruptSnapshot().trust==ConfigurationTrust::DIRTY);
  }
  {
    Controller b;Driver d;ready(d,b);const auto t=configure(d,b,settings());
    for(unsigned i=0;i<13U;++i)CHECK(poll(d,b,t).inProgress()); // before final PHYSCR verification
    b.corruptFrame=static_cast<int>(b.frames);b.corruptMask=0x8000;
    PowerSnapshot sentinel;sentinel.rawBmcr=0xCAFE;
    CHECK(d.readPowerStatus(sentinel,{1000000,3}).is(Err::INVALID_CONFIG));CHECK(sentinel.rawBmcr==0xCAFE);
    const auto frames=b.frames;CHECK(!poll(d,b,t).ok());const auto result=take(d,t);
    CHECK(b.frames==frames && !d.isInitialized() && d.identitySnapshot().stale);
    CHECK(result.cleanupOwed && !result.persistentRestored && d.interruptSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
  {
    Controller b;Driver d;ready(d,b);auto desired=settings();desired.sharedSource=InterruptSharedSource::POLARITY;
    const auto t=configure(d,b,desired);
    for(unsigned i=0;i<6U;++i)CHECK(poll(d,b,t).inProgress()); // selector verified; crossing drain pending
    b.ext[0x04A1]=0x1000;EvidenceSnapshot raw;
    CHECK(d.acquireEvidence(evidenceBit(EvidenceSource::RXFS),raw,{1000000,4}).ok());
    CHECK(d.operationSnapshot().state==OperationState::RESULT_PENDING);
    const auto frames=b.frames,calls=b.calls;
    CHECK(poll(d,b,t).is(Err::CONFIG_MISMATCH));
    CHECK(b.frames==frames && b.calls==calls);const auto result=take(d,t);
    CHECK(result.cleanupOwed && !result.persistentRestored);
    CHECK(d.interruptSnapshot().selector==InterruptSharedSource::UNKNOWN && d.interruptSnapshot().trust==ConfigurationTrust::DIRTY);
    b.words[19]|=0x0200;InterruptServiceSnapshot observed;
    CHECK(d.serviceInterrupts(observed,serviceBudget()).ok());
    CHECK(observed.misr2.sharedAmbiguous && !observed.misr2.polaritySeen);
  }
  const auto ownPower=[](Driver& driver,Controller& bus) {
    PowerSettings power;power.maintenanceAdmitted=power.managementClockAvailable=power.clockLossApproved=true;
    OperationToken t;CHECK(driver.startConfigurePower(power,budget(),t).inProgress());CHECK(drain(driver,bus,t).ok());take(driver,t);
  };
  for(unsigned phase=0;phase<3U;++phase) {
    Controller b;Driver d;ready(d,b);configured(d,b);ownPower(d,b);OperationToken t;
    if(phase==1U) {
      t=configure(d,b,settings());for(unsigned i=0;i<13U;++i)CHECK(poll(d,b,t).inProgress());
    } else if(phase==2U) {
      CHECK(d.startInterruptTest(budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    }
    b.words[17]|=0x6000;PowerSnapshot observed;CHECK(d.readPowerStatus(observed,{1000000,3}).ok());
    CHECK(d.powerSnapshot().trust==ConfigurationTrust::DIRTY);
    if(phase==0U) {
      const auto frames=b.frames,clocks=b.clocks;OperationToken sentinel{999};
      CHECK(d.startConfigureInterrupts(settings(),budget(),sentinel).is(Err::INVALID_CONFIG));
      CHECK(d.startInterruptTest(budget(),sentinel).is(Err::INVALID_CONFIG));
      CHECK(sentinel.value==999 && b.frames==frames && b.clocks==clocks);
    } else {
      if(phase==2U)CHECK(d.stopInterruptTest(budget(),t).inProgress());
      CHECK(!drain(d,b,t).ok());const auto result=take(d,t);
      CHECK(result.cleanupOwed && !result.persistentRestored && d.powerSnapshot().trust==ConfigurationTrust::DIRTY);
      if(phase==2U)CHECK((b.words[17]&4U)==0U); // owning stop still clears its stimulus
    }
  }
  // OP2: syntax first, then binding/slot, then source/capability permissions,
  // and only then zero execution allowance. All rejected calls are silent.
  {
    Driver unbound;OperationToken t{999};auto denied=settings();denied.maintenanceAdmitted=false;
    CHECK(unbound.startConfigureInterrupts(denied,budget(0),t).is(Err::NOT_BOUND));
    auto invalid=denied;invalid.misr1Mask=0x100;
    CHECK(unbound.startConfigureInterrupts(invalid,budget(),t).is(Err::INVALID_PARAM));
    auto badBudget=budget();badBudget.pollIntervalUs=0;
    CHECK(unbound.startConfigureInterrupts(denied,badBudget,t).is(Err::INVALID_PARAM));CHECK(t.value==999);
    Controller b;Driver d;ready(d,b);const auto active=configure(d,b,settings());
    CHECK(drain(d,b,active).ok());const auto frames=b.frames,clocks=b.clocks;
    CHECK(d.startConfigureInterrupts(denied,budget(0),t).is(Err::BUSY));
    CHECK(d.startConfigureInterrupts(denied,badBudget,t).is(Err::INVALID_PARAM));
    CHECK(t.value==999 && b.frames==frames && b.clocks==clocks);take(d,active);
  }
}

void ownership() {
  auditRegressions();
  Controller b;Driver d,other;ready(d,b);CHECK(other.bind(b.config(2)).ok());b.competitor=&other;
  configured(d,b);InterruptServiceSnapshot out;CHECK(d.serviceInterrupts(out,serviceBudget()).ok());
  CHECK(b.excluded>0U && b.releases==b.calls && !b.locked);
  b.competitor=nullptr;OperationToken token=configure(d,b,settings());const auto first=b.frames;
  CHECK(poll(d,b,token,0).inProgress() && b.frames==first);CHECK(d.cancel(token).ok());take(d,token);
  // A short poll slice cannot split the selector/address/data transaction.
  Controller reference;Driver ref;ready(ref,reference);const auto base=reference.calls;configured(ref,reference);
  unsigned selectCall=base,writeCall=base;
  for(unsigned i=base;i<reference.calls;++i) {
    const auto end=i+1U<reference.calls?reference.callFirst[i+1U]:reference.frames;
    for(unsigned n=reference.callFirst[i];n<end;++n) {
      if(reference.trace[n].kind==FrameKind::WRITE && reference.trace[n].target==0x04A1)selectCall=i;
      if(reference.trace[n].kind==FrameKind::WRITE && reference.trace[n].target==17)writeCall=i;
    }
  }
  CHECK(selectCall>base && writeCall>base);
  Controller sliced;Driver slice;ready(slice,sliced);const auto start=sliced.calls;
  const auto current=configure(slice,sliced,settings());
  while(sliced.calls<start+selectCall-base)CHECK(poll(slice,sliced,current).inProgress());
  const auto before=sliced.frames;CHECK(poll(slice,sliced,current,3).inProgress() && sliced.frames==before);
  CHECK(drain(slice,sliced,current).ok());take(slice,current);
  for(unsigned fault=0;fault<4U;++fault) {
    Controller bus;Driver driver;ready(driver,bus);const auto target=static_cast<int>(bus.calls+writeCall-base);
    if(fault==0U)bus.lateCall=target;
    if(fault==1U)bus.regressCall=target;
    if(fault==2U)bus.invalidCall=target;
    if(fault==3U)bus.badReceiptCall=target;
    const auto t=configure(driver,bus,settings());CHECK(!drain(driver,bus,t).ok());const auto failed=take(driver,t);
    CHECK(failed.cleanupOwed && !failed.persistentRestored && driver.interruptSnapshot().trust!=ConfigurationTrust::VERIFIED);
    if(fault>=2U)CHECK(driver.transportSnapshot().violation!=TransportViolation::NONE);
  }
  Controller expired;Driver deadline;ready(deadline,expired);OperationToken t;const auto count=expired.frames;
  CHECK(deadline.startConfigureInterrupts(settings(),budget(2048,expired.now),t).inProgress());
  CHECK(deadline.pollOperation(t).is(Err::OPERATION_TIMEOUT) && expired.frames==count);take(deadline,t);
  OperationToken unchanged{999};const auto clocks=expired.clocks;
  const auto noFrames=deadline.startConfigureInterrupts(settings(),budget(0),unchanged);
  CHECK(noFrames.is(Err::OPERATION_TIMEOUT) && noFrames.detail==static_cast<int32_t>(TimeoutDetail::FRAME_BUDGET));
  CHECK(unchanged.value==999 && expired.frames==count && expired.clocks==clocks);
  const auto noDeadline=deadline.startConfigureInterrupts(settings(),budget(2048,0),unchanged);
  CHECK(noDeadline.is(Err::OPERATION_TIMEOUT) && noDeadline.detail==static_cast<int32_t>(TimeoutDetail::DEADLINE));
  CHECK(unchanged.value==999 && expired.frames==count && expired.clocks==clocks);
  Controller regressed;Driver broken;ready(broken,regressed);regressed.regressCall=static_cast<int>(regressed.calls);
  const auto active=configure(broken,regressed,settings());CHECK(!drain(broken,regressed,active).ok());take(broken,active);
  const auto cb=regressed.calls,ck=regressed.clocks;
  const auto clockFailure=broken.startConfigureInterrupts(settings(),budget(),unchanged);
  CHECK(clockFailure.is(Err::INVALID_CONFIG) && clockFailure.detail==static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  CHECK(unchanged.value==999 && regressed.calls==cb && regressed.clocks==ck);
}

void aRoutes() {
  CHECK(test::aGrade());
  constexpr InterruptPin pins[]={InterruptPin::LED0,InterruptPin::LED1,InterruptPin::LED2,InterruptPin::LED3};
  // An independent observation can withdraw the clock proof before or after
  // mux preflight. A planned mux repair cannot restore the clock prerequisite.
  for (unsigned polls:{2U,3U}) {
    Controller bus; Driver driver; ready(driver,bus);
    auto policy=settings(); policy.pin=InterruptPin::LED1; policy.led1ClockUnused=true;
    bus.ext[0x308]=1; bus.ext[0x304]=0;
    const auto token=configure(driver,bus,policy);
    for (unsigned i=0;i<polls;++i) CHECK(poll(driver,bus,token).inProgress());
    bus.ext[0x308]=0; uint16_t raw=0xFFFF;
    CHECK(driver.readMmd(31,0x308,raw,{1000000,4}).ok() && raw==0);
    const auto frames=bus.frames;
    const auto receipt=take(driver,token);
    CHECK(receipt.status.is(Err::CONFIG_MISMATCH) && receipt.mismatch.valid);
    CHECK(receipt.mismatch.registerAddress==0x308 && receipt.mismatch.mask==1 && !receipt.mutationPossible);
    CHECK(driver.interruptSnapshot().trust==ConfigurationTrust::DIRTY && bus.frames==frames && bus.ext[0x304]==0);
  }
  // Mask/apply/verify needs 26 on pin21. Pin29 adds a 5-frame preflight
  // and adds a 9-frame RMW/readback only for a live mux mismatch.
  for (bool matching:{false,true}) for (bool exact:{false,true}) {
    Controller bus; bus.enhancedMii=true; Driver driver; ready(driver,bus);
    auto policy=settings(); policy.pin=InterruptPin::LED3; policy.crsUnused=true;
    bus.ext[0x306]=matching?4:3;
    const uint32_t required=matching?31U:40U;
    const auto first=bus.frames,mutations=bus.mutations;
    OperationToken t; CHECK(driver.startConfigureInterrupts(policy,budget(required-(exact?0U:1U)),t).inProgress());
    const auto status=drain(driver,bus,t); const auto receipt=take(driver,t);
    if (exact) CHECK(status.ok() && bus.frames==first+required);
    else CHECK(status.is(Err::OPERATION_TIMEOUT) && bus.mutations==mutations && !receipt.mutationPossible);
  }
  for (unsigned pin=0;pin<4;++pin) {
    for (bool basic:{false,true}) {
      Controller b; b.id2=basic?0xA114:0xA134; b.enhancedMii=true; b.ext[0x308]=1;
      Driver d; ready(d,b);
      const auto address=static_cast<uint16_t>(0x303U+pin);
      b.ext[address]=0xA5E3; auto irq=settings(); irq.pin=pins[pin];
      irq.led1ClockUnused=irq.colUnused=irq.crsUnused=true;
      const auto first=b.frames; OperationToken t;
      if (basic && pin>=2U) {
        CHECK(!d.startConfigureInterrupts(irq,budget(),t).inProgress() && b.frames==first); continue;
      }
      if (pin>=2U) {
        auto denied=irq;
        if (pin==2) denied.colUnused=false; else denied.crsUnused=false;
        CHECK(!d.startConfigureInterrupts(denied,budget(),t).inProgress() && b.frames==first);
      }
      configured(d,b,irq);
      CHECK(b.ext[address]==0xA5E4 && (b.words[18]&0xFF)==0xFF && (b.words[19]&0xFF)==0xFF);
      unsigned disabled=b.frames,mux=b.frames,enabled=b.frames;
      for (unsigned n=first;n<b.frames;++n) {
        if (b.trace[n].kind!=FrameKind::WRITE) continue;
        if (b.trace[n].target==address) mux=n;
        if (b.trace[n].target==17 && disabled==b.frames && !(b.trace[n].value&2U)) disabled=n;
        if (b.trace[n].target==17 && (b.trace[n].value&2U)) enabled=n;
      }
      CHECK(disabled<mux && mux<enabled);
      WakeSettings wake; wake.enabled=wake.maintenanceAdmitted=wake.receiveTrafficQuiesced=wake.pinAvailable=true;
      wake.crsUnused=wake.colUnused=wake.led1ClockUnused=true;
      constexpr WakeRoute routes[]={WakeRoute::LED0,WakeRoute::LED1,WakeRoute::LED2,WakeRoute::LED3};
      wake.route=routes[pin];
      const auto old=b.frames;
      CHECK(!d.startConfigureWake(wake,budget(),t).inProgress() && b.frames==old);
      b.ext[address]=0xA5E2; uint16_t raw=0;
      CHECK(d.readMmd(31,address,raw,{1000000,4}).ok() && raw==0xA5E2);
      CHECK(d.interruptSnapshot().trust==ConfigurationTrust::DIRTY);
    }
  }
}
void p22PassiveEvidence() {
  // Tables 8-22/27/138. No generator command or capture write accompanies
  // these observations. The LPI received/status fields are plain R on this part.
  const auto diagnostic=evidenceBit(EvidenceSource::BISCR)|evidenceBit(EvidenceSource::BICSR1)|
                        evidenceBit(EvidenceSource::EEE_STATUS);
  for (int cut=-1;cut<6;++cut) {
    Controller b; Driver d; ready(d,b); b.words[22]=0x3F00; b.words[27]=0xFE7D; b.lpiStatus=0x0E40;
    const auto start=b.frames, writes=b.mutations;
    uint16_t raw=0xA55A;
    for (uint8_t reg : {uint8_t{22},uint8_t{27}}) {
      CHECK(!d.readClause22(reg,raw,{1000000,1}).ok() && !d.readMmd(31,reg,raw,{1000000,4}).ok());
    }
    CHECK(!d.readMmd(3,1,raw,{1000000,4}).ok() && raw==0xA55A && b.frames==start);
    if (cut>=0) b.failFrame=static_cast<int>(start)+cut;
    EvidenceSnapshot out;
    const auto status=d.acquireEvidence(diagnostic,out,{1000000,6});
    CHECK(status.ok()==(cut<0) && b.mutations==writes);
    const auto e=d.evidenceSnapshot();
    const auto& checker=e.samples[static_cast<unsigned>(EvidenceSource::BISCR)];
    const auto& count=e.samples[static_cast<unsigned>(EvidenceSource::BICSR1)];
    const auto& lpi=e.samples[static_cast<unsigned>(EvidenceSource::EEE_STATUS)];
    if (cut>=0) {
      CHECK(status.detail==-27027 && b.frames==start+static_cast<unsigned>(cut)+1);
      CHECK(!out.samples[static_cast<unsigned>(EvidenceSource::EEE_STATUS)].valid);
      CHECK(checker.historyLost==(cut==0));
      CHECK(!count.historyLost && !lpi.historyLost);
    }
    if (cut<0 || cut>=1) CHECK(checker.raw==0x3F00 && checker.eventBits==0x0400 && (b.words[22]&0x0400)==0);
    if (cut<0 || cut>=2) CHECK(count.raw==0xFE7D && count.sampledSum==0 && b.words[27]==0xFE7D);
    if (cut<0) {
      CHECK(lpi.raw==0x0E40 && lpi.eventBits==0 && lpi.observedOnes==0x0E40 && b.lpiStatus==0x0E40);
      CHECK(b.trace[start].reg==22 && b.trace[start+1].reg==27 && b.trace[start+2].value==3 &&
            b.trace[start+3].value==1 && b.trace[start+4].value==0x4003 && b.trace[start+5].reg==14);
    }
  }
  // Four-frame C22-MMD transactions use different DEVADs for the vendor
  // FIFO alias and the standard partner word. Cut each selector/data frame.
  for (bool fifo : {false,true}) for (int cut=-1;cut<4;++cut) {
    Controller b; Driver d; ready(d,b); b.eeePartner=2; b.words[23]=0x006C;
    const auto start=b.frames, calls=b.calls, writes=b.mutations;
    if (cut>=0) b.failFrame=static_cast<int>(start)+cut;
    EvidenceSnapshot e; uint16_t word=0xA55A;
    auto status=fifo ? d.acquireEvidence(evidenceBit(EvidenceSource::RCSR),e,{1000000,4}) :
                       d.readMmd(7,0x003D,word,{1000000,4});
    const auto size=cut<0?4U:static_cast<unsigned>(cut)+1;
    CHECK(status.ok()==(cut<0) && b.frames==start+size && b.calls==calls+1 && b.mutations==writes);
    const uint16_t request[]={uint16_t(fifo?31:7),uint16_t(fifo?0x0017:0x003D),uint16_t(fifo?0x401F:0x4007)};
    for (unsigned i=0;i<size;++i) {
      const auto& w=b.trace[start+i];
      CHECK(w.reg==(i%2==0?13:14) && w.kind==(i==3?FrameKind::READ:FrameKind::WRITE));
      if (i<3) CHECK(w.value==request[i]);
    }
    if (fifo) {
      const auto sample=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
      CHECK(sample.historyLost==(cut==3));
      if (cut<0) CHECK(sample.raw==0x006C && sample.eventBits==0x000C);
    } else { CHECK(word==(cut<0?2:0xA55A)); }
  }
}

void p22Collector() {
  for (const auto variant : {Variant::DP83822I, Variant::DP83822H, Variant::DP83822IF, Variant::DP83822HF}) {
    test::declaredVariant=variant;
    for (const auto interface : {MacInterface::MII, MacInterface::RMII, MacInterface::RGMII}) {
      for (bool clears : {false,true}) {
        p22Interface=interface; p22ClearsEnables=clears;
        p22PassiveEvidence();
        failures();
        serviceFailures();
        for (unsigned bank=0;bank<2;++bank) for (unsigned bit=0;bit<8;++bit) {
          Controller maskBus; Driver masks; ready(masks,maskBus);
          auto s=settings();
          s.misr1Mask=static_cast<uint16_t>(bank==0?1U<<bit:0);
          s.misr2Mask=static_cast<uint16_t>(bank==1?1U<<bit:0);
          configured(masks,maskBus,s);
          CHECK(maskBus.words[18]==s.misr1Mask && maskBus.words[19]==s.misr2Mask);
          const auto writes=maskBus.mutations; OperationToken verify;
          CHECK(masks.startVerify(budget(),verify).inProgress());
          const auto verdict=drain(masks,maskBus,verify);
          CHECK(verdict.is(Err::INVALID_CONFIG));
          const auto proof=take(masks,verify);
          CHECK(proof.status.detail==static_cast<int32_t>(ConfigDetail::SOURCE_GATE) && !proof.persistentRestored);
          CHECK(proof.interrupts.trust==ConfigurationTrust::DIRTY && !proof.interrupts.misr1EnablesKnown &&
                !proof.interrupts.misr2EnablesKnown && maskBus.mutations==writes);
        }
        Controller b; Driver d;
        ready(d,b);
        auto policy=settings();
        for (auto bad : {InterruptPin::LED0,InterruptPin::LED1,InterruptPin::LED2,InterruptPin::LED3}) {
          OperationToken t; policy.pin=bad; const auto calls=b.calls, clocks=b.clocks;
          CHECK(d.startConfigureInterrupts(policy,budget(),t).is(Err::UNSUPPORTED));
          CHECK(calls==b.calls && clocks==b.clocks);
        }
        policy.pin=InterruptPin::INTR_PWRDN;
        policy.sharedSource=clears?InterruptSharedSource::POLARITY:InterruptSharedSource::WAKE;
        policy.misr1Mask=0xA5; policy.misr2Mask=0x5A; policy.activeLow=clears;
        b.words[19]=0x0200; b.ext[0x04A1]=0x00FF;
        const auto first=b.frames;
        configured(d,b,policy);
        bool selectorWritten=false;
        for (unsigned i=first;i<b.frames;++i) {
          const auto& w=b.trace[i];
          if (w.kind==FrameKind::WRITE && w.target==0x04A1) {
            CHECK(w.value==(clears?0:0x1000)); selectorWritten=true;
          }
          CHECK(w.target<0x0303 || w.target>0x0306);
        }
        CHECK(selectorWritten && d.interruptSnapshot().selector==policy.sharedSource);
        CHECK(misr2(d.evidenceSnapshot()).sharedAmbiguous);
        CHECK((b.words[17]&0xF)==(clears?0xB:3));
        CHECK(b.words[18]==0xA5 && b.words[19]==0x5A);
        CHECK(d.interruptSnapshot().misr1EnablesKnown && d.interruptSnapshot().misr2EnablesKnown);
        OperationToken t;
        CHECK(d.startInterruptTest(budget(),t).inProgress() && drain(d,b,t).ok());
        CHECK(take(d,t).temporaryMayBeActive && (b.words[17]&4U)!=0);
        CHECK(d.stopInterruptTest(budget(),t).inProgress() && drain(d,b,t).ok());
        CHECK(!take(d,t).temporaryMayBeActive && (b.words[17]&4U)==0);
        for (unsigned bank=0;bank<2;++bank) for (unsigned bit=0;bit<8;++bit) {
          const auto event=static_cast<uint16_t>(0x100U<<bit);
          b.words[18]=static_cast<uint16_t>((bank==0?event:0)|0xA5);
          b.words[19]=static_cast<uint16_t>((bank==1?event:0)|0x5A);
          b.words[20]=0xABFF; b.words[21]=0xFFFF;
          InterruptServiceSnapshot s;
          CHECK(d.serviceInterrupts(s,serviceBudget()).ok());
          CHECK(s.fcscrRequested==(bank==0 && bit==1));
          CHECK(s.recrRequested==(bank==0 && bit==0));
          CHECK((s.misr1.raw&0xFF00)==(bank==0?event:0) && (s.misr2.raw&0xFF00)==(bank==1?event:0));
          if (s.fcscrRequested) { CHECK(s.fcscr.sampledSum==255 && s.fcscr.counterSaturated); }
          if (s.recrRequested) { CHECK(s.recr.sampledSum==65535 && s.recr.counterSaturated); }
          if (bank==1 && bit==1) { CHECK(clears?s.misr2.polaritySeen:s.misr2.wakeSeen); }
        }
        const auto irq=d.interruptSnapshot();
        CHECK(irq.trust==ConfigurationTrust::DIRTY && !irq.misr1EnablesKnown && !irq.misr2EnablesKnown);
        CHECK(irq.desired.misr1Mask==0xA5 && irq.desired.misr2Mask==0x5A);
        CHECK(irq.rawMisr1==0x00A5 && irq.rawMisr2==0x805A);
        CHECK(b.words[18]==(clears?0:0xA5) && b.words[19]==(clears?0:0x5A));
        b.words[15]=0xA5F5; CounterSnapshot counters;
        CHECK(d.collectEvents(counters,{1000000,6}).ok() && counters.fldCauses==0x01F0);
        b.words[23]=0x000C; b.wakeErrors=65535; b.ext[0x04A1]|=0x00FF; EvidenceSnapshot e;
        const auto start=b.frames;
        uint16_t raw=0xA55A;
        CHECK(!d.readClause22(23,raw,{1000000,1}).ok() && !d.readMmd(31,23,raw,{1000000,4}).ok() && raw==0xA55A);
        const auto sources=evidenceBit(EvidenceSource::RCSR)|evidenceBit(EvidenceSource::RXFS)|evidenceBit(EvidenceSource::EEE_ERRORS);
        CHECK(d.acquireEvidence(sources,e,{b.now+65,12}).is(Err::OPERATION_TIMEOUT));
        CHECK(d.acquireEvidence(sources,e,{1000000,11}).is(Err::OPERATION_TIMEOUT));
        CHECK(b.frames==start);
        CHECK(d.acquireEvidence(sources,e,{1000000,12}).ok());
        CHECK(e.samples[static_cast<unsigned>(EvidenceSource::RCSR)].eventBits==0x000C);
        CHECK(e.samples[static_cast<unsigned>(EvidenceSource::RXFS)].eventBits==0x00E3);
        CHECK(e.samples[static_cast<unsigned>(EvidenceSource::EEE_ERRORS)].sampledSum==65535);
        CHECK(e.samples[static_cast<unsigned>(EvidenceSource::EEE_ERRORS)].counterSaturated);
        const uint16_t requests[3][3]={{0x001F,0x0017,0x401F},{0x001F,0x04A1,0x401F},{0x0003,0x0016,0x4003}};
        for (unsigned n=0;n<3;++n) for (unsigned i=0;i<4;++i) {
          const auto& frame=b.trace[start+4*n+i];
          CHECK(frame.reg==(i%2==0?13:14) && frame.kind==(i<3?FrameKind::WRITE:FrameKind::READ));
          if (i<3) { CHECK(frame.value==requests[n][i]); }
        }
        const auto samples=e.samples[static_cast<unsigned>(EvidenceSource::MISR1)].samples;
        b.words[18]=0x8101; b.words[19]=0x4200;
        if (clears) b.failFrame=static_cast<int>(b.frames+2);
        else b.releaseCall=static_cast<int>(b.calls);
        InterruptServiceSnapshot sentinel;
        CHECK(!d.serviceInterrupts(sentinel,serviceBudget()).ok() && !sentinel.valid);
        const auto partial=d.evidenceSnapshot();
        CHECK(partial.samples[static_cast<unsigned>(EvidenceSource::MISR1)].samples==samples+1);
        CHECK(partial.samples[static_cast<unsigned>(EvidenceSource::MISR1)].raw==0x8101);
        CHECK(d.interruptServiceSnapshot().stale && d.interruptSnapshot().trust==ConfigurationTrust::DIRTY);
      }
    }
  }
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"p22_collector",p22Collector},{"a_routes",aRoutes},{"policy",policy},{"service",service},{"attribution",attribution},
    {"service_failures",serviceFailures},{"failures",failures},{"pins",pins},{"stimulus",stimulus},{"replay",replay},{"ownership",ownership}};
  return test::runVariantSuites(argc,argv,suites);
}
