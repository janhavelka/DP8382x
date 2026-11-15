#include "../examples/portable/Wake.h"
#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target; };

// Independent SNLS647I original tables8-90..97, pp91..93. RXFCFG1 W0C is
// modelled separately from its read word; the bit0 RH stability remains Q3.
// This fake proves protocol/evidence handling, never packet matching or MCU wake.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<Wire,8192> trace{};
  uint16_t selector{0},address{0},id2{0xA131},wakePin{0};
  uint64_t now{100},frameTime{2};
  unsigned frames{0},calls{0},clocks{0},releases{0},mutations{0},hard{0};
  int failFrame{-1},releaseCall{-1},corruptFrame{-1},lateCall{-1},regressCall{-1},invalidCall{-1},badReceiptCall{-1};
  uint16_t corruptMask{1};
  int injectFrame{-1}; uint16_t injectMisr2{0};
  bool acceptedFailure{true},locked{false},pendingWake{true},matchWhileEnabled{false},enhancedMii{false},clockOffStrap{false},w1cInterpretation{false};
  Driver* competitor{nullptr}; unsigned excluded{0};
  Controller() { defaults(); }
  void defaults() {
    words.fill(0);ext.fill(0);words[0]=0x3100;words[4]=0x01E1;
    words[25]=0x8001;words[22]=0x0100;words[30]=0x0102;
    ext[0x030E]=0x8400;ext[0x04A1]=0x1000;ext[0x04A0]=0x1081;ext[0x0308]=clockOffStrap?1U:0U;pendingWake=true;
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=Variant::DP83826I;c.interface=(id2==0xA111 || enhancedMii)?MacInterface::MII:MacInterface::RMII;c.phyAddress=phy;
    c.transport={this,execute,clock,{8,4,1}};return c;
  }
  static uint64_t clock(void* context) noexcept { auto& b=*static_cast<Controller*>(context);++b.clocks;return b.now; }
  uint16_t read(uint16_t target) {
    if(target==2)return 0x2000;
    if(target==3)return id2;
    if(target==0)words[0]&=0x7DFF;
    if(target==31)words[31]=0;
    const auto value=target<32U?words[target]:ext[target];
    if(target==18 || target==19) { words[target]&=0x00FF;words[16]&=0xFF7F; }
    if(target==20) { words[20]&=0xFF00;words[16]&=0xF7FF; }
    if(target==21) { words[21]=0;words[16]&=0xDFFF; }
    if(target==0x04A1)ext[target]&=0xFF00;
    return value;
  }
  void write(uint8_t reg,uint16_t value) {
    if(reg==13) { CHECK(value==0x001F || value==0x401F);selector=value;return; }
    if(reg==14 && selector==0x001F) { address=value;return; }
    const uint16_t target=reg==14?address:uint16_t{reg};++mutations;
    if(target==31 && value==0x8000) { ++hard;defaults();return; }
    if(target==0 && value==0x8000) { words[0]=0x3100;return; }
    if(target==0x04A0) {
      if(w1cInterpretation?((value&0x0800U)!=0U):((value&0x0800U)==0U))pendingWake=false; // Default original table8-90 W0C; alternate interpretation is Q3 stress only.
      ext[target]=static_cast<uint16_t>(value&0xF7FFU);
      if((value&0x0080U)!=0U)ext[0x04A1]|=0x1000; // Enabling selects WoL.
      return;
    }
    if(((target>=0x04A2U && target<=0x04A7U) || (wakePin!=0U && target==wakePin)) && (ext[0x04A0]&0x0081U)!=0U)matchWhileEnabled=true;
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
    for(uint8_t i=0;i<batch.frameCount;++i) {
      const auto& q=batch.frames[i];auto& f=r.frames[i];uint16_t value=q.value;
      if(q.readIndex!=NO_READ_INDEX) { CHECK(q.readIndex<i && r.frames[q.readIndex].valueValid);value=static_cast<uint16_t>((r.frames[q.readIndex].value&q.preserveMask)|q.value); }
      const uint16_t target=q.registerAddress==14 && b.selector==0x401F?b.address:uint16_t{q.registerAddress};
      CHECK(b.frames<b.trace.size());const auto frame=b.frames++;b.trace[frame]={q.kind,q.registerAddress,value,target};
      f.attempted=true;const bool failed=static_cast<int>(frame)==b.failFrame;
      if(!failed || b.acceptedFailure) {
        if(q.kind==FrameKind::WRITE)b.write(q.registerAddress,value);
        else { f.value=b.read(target);if(static_cast<int>(frame)==b.corruptFrame)f.value^=b.corruptMask; }
      }
      if(static_cast<int>(frame)==b.injectFrame) { b.words[19]|=b.injectMisr2; }
      b.now+=b.frameTime;
      if(b.competitor!=nullptr) {
        uint16_t sentinel=0xCAFE;CHECK(b.competitor->readMmd(31,0x0467,sentinel,budget).is(Err::BUSY));CHECK(sentinel==0xCAFE);++b.excluded;
      }
      if(failed) { f.effectPossible=b.acceptedFailure;r.status={Err::MDIO_TIMEOUT,-31031,"Injected frame"};break; }
      f.completed=true;f.valueValid=q.kind==FrameKind::READ;
    }
    if(static_cast<int>(call)==b.releaseCall) { r.releaseStatus={Err::MDIO_ERROR,-37,"Injected release"};r.controllerReusable=false; }
    if(static_cast<int>(call)==b.lateCall)b.now=budget.deadlineUs;
    if(static_cast<int>(call)==b.regressCall)b.now=0;
    if(static_cast<int>(call)==b.invalidCall)r.status={Err::IN_PROGRESS,-31032,"Illegal terminal status"};
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
  CHECK(d.bind(b.config()).ok());OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}

WakeSettings settings(bool enabled=true) {
  WakeSettings s;s.enabled=enabled;s.secureOn=enabled;s.maintenanceAdmitted=s.receiveTrafficQuiesced=true;
  const uint8_t mac[]={0x11,0x22,0x33,0x44,0x55,0x66},password[]={0x2A,0x2B,0x2C,0x2D,0x2E,0x2F};
  for(unsigned i=0;i<6U;++i) { s.mac[i]=mac[i];s.password[i]=password[i]; }
  return s;
}
OperationToken configure(Driver& d,Controller& b,const WakeSettings& s) {
  b.wakePin=s.route==WakeRoute::LED1?0x0304:s.route==WakeRoute::LED2?0x0305:
      s.route==WakeRoute::INTERRUPT?(d.interruptSnapshot().desired.pin==InterruptPin::LED1?0x0304:
      d.interruptSnapshot().desired.pin==InterruptPin::LED2?0x0305:0):0;
  OperationToken t{999};const auto calls=b.calls,clocks=b.clocks;
  CHECK(d.startConfigureWake(s,budget(),t).inProgress());
  CHECK(t.value!=999 && b.calls==calls && b.clocks==clocks);return t;
}
OperationResult configured(Driver& d,Controller& b,const WakeSettings& s=settings()) {
  const auto t=configure(d,b,s);CHECK(drain(d,b,t).ok());const auto r=take(d,t);
  CHECK(r.status.ok() && !r.cleanupOwed && r.persistentRestored && d.isInitialized());
  const auto view=d.wakeSnapshot();CHECK(view.desiredValid && view.trust==ConfigurationTrust::VERIFIED);
  return r;
}
bool semantic(const Wire& w) { return w.kind==FrameKind::WRITE && w.reg!=13 && !(w.reg==14 && w.target==14); }
unsigned writes(const Controller& b,unsigned from,uint16_t target) {
  unsigned count=0;for(unsigned i=from;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==target)++count;return count;
}
const EvidenceSample& sample(const EvidenceSnapshot& snapshot,EvidenceSource source) { return snapshot.samples[static_cast<unsigned>(source)]; }
void harvestBeforeWrites(const Controller& b,unsigned first) {
  bool rxfs=false,misr=false;
  for(unsigned i=first;i<b.frames;++i) {
    const auto& w=b.trace[i];
    if(w.kind==FrameKind::READ && w.target==0x04A1)rxfs=true;
    if(w.kind==FrameKind::READ && w.target==0x0013)misr=true;
    if(semantic(w) && w.target==0x04A0) { CHECK(rxfs && misr);rxfs=misr=false; }
  }
}
void vectors() {
  {
    Controller b;Driver d;ready(d,b);const auto first=b.frames,calls=b.calls;configured(d,b);
    // Identity2 + control4 + six comparisons24 + disable14 + six writes24 +
    // six verifies24 + enable14 + selector4/drain1 =111, in28 owned callbacks.
    CHECK(b.frames==first+111U && b.calls==calls+28U);
    const auto next=b.frames,nextCalls=b.calls;configured(d,b);
    CHECK(b.frames==next+35U && b.calls==nextCalls+10U);
  }

  for(bool basic:{false,true})for(bool crc:{false,true})for(bool level:{false,true})for(bool secure:{false,true})for(uint8_t pulse=0;pulse<4U;++pulse) {
    Controller b;b.id2=basic?0xA111:0xA131;Driver d;ready(d,b);auto s=settings();s.crcGate=crc;s.levelMode=level;s.secureOn=secure;s.pulseWidth=pulse;
    b.ext[0x04A0]|=0xE01E;const auto first=b.frames;configured(d,b,s);
    const uint16_t expected=static_cast<uint16_t>((crc?0x1000U:0U)|(level?0x0100U:0U)|(uint16_t{pulse}<<9U)|0x0081U|(secure?0x0020U:0U));
    CHECK((b.ext[0x04A0]&0x17A1U)==expected && (b.ext[0x04A0]&0xE01EU)==0xE01E);
    CHECK(b.ext[0x04A2]==0x2211 && b.ext[0x04A3]==0x4433 && b.ext[0x04A4]==0x6655);
    CHECK(b.ext[0x04A5]==0x2B2A && b.ext[0x04A6]==0x2D2C && b.ext[0x04A7]==0x2F2E);
    CHECK(!b.matchWhileEnabled && b.pendingWake);harvestBeforeWrites(b,first);
    const auto view=d.wakeSnapshot();CHECK(view.enabled && view.magicMatch && view.secureOn==secure && view.crcGate==crc && view.levelMode==level && view.pulseWidth==pulse);
    CHECK(view.rawMac[0]==0x2211 && view.rawPassword[2]==0x2F2E && view.credentialValidMask==0x3F);
    for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==0x04A0)CHECK((b.trace[i].value&0x0800U)!=0U);
    const auto calls=b.calls,clocks=b.clocks;const auto one=d.wakeSnapshot(),two=d.wakeSnapshot();
    CHECK(one.rawControl==two.rawControl && b.calls==calls && b.clocks==clocks);
  }
  for(uint8_t octet:{uint8_t{0},uint8_t{0xFF}}) {
    Controller b;Driver d;ready(d,b);auto s=settings();for(unsigned i=0;i<6U;++i)s.mac[i]=s.password[i]=octet;
    configured(d,b,s);for(unsigned i=0;i<6U;++i)CHECK(b.ext[0x04A2U+i]==(octet==0U?0U:0xFFFFU));
  }
  {
    Controller b;Driver d;ready(d,b);configured(d,b);auto changed=settings();changed.mac[0]=0xAA;changed.password[5]=0xBB;
    const auto first=b.frames;configured(d,b,changed);harvestBeforeWrites(b,first);
    CHECK(b.ext[0x04A2]==0x22AA && b.ext[0x04A7]==0xBB2E && !b.matchWhileEnabled);
    const auto mutations=b.mutations;configured(d,b,changed);CHECK(b.mutations==mutations);
  }
  {
    Controller b;Driver d;ready(d,b);b.ext[0x04A0]|=0x0040;configured(d,b);
    CHECK((b.ext[0x04A0]&0x0040U)!=0U);const auto mutations=b.mutations;configured(d,b);CHECK(b.mutations==mutations);
  }
  {
    // Q3 alternate acknowledgement stress: even if hardware clears on one,
    // pre-write acquisition retains the earlier chip indication and IRQ evidence.
    Controller b;Driver d;ready(d,b);b.w1cInterpretation=true;b.ext[0x04A1]=0x10E1;b.words[19]=0x0202;configured(d,b);
    const auto evidence=d.evidenceSnapshot();CHECK(!b.pendingWake);
    CHECK((sample(evidence,EvidenceSource::RXFS).eventBits&0x00E1U)==0x00E1U && (sample(evidence,EvidenceSource::MISR2).eventBits&0x0200U)!=0U);
  }
  Controller b;Driver d;ready(d,b);CHECK(b.ext[0x04A0]==0x1081); // Initialization is not WoL-off.
  WakeSnapshot resetView;CHECK(d.readWakeStatus(resetView,{1000000,33}).ok());
  CHECK(!resetView.desiredValid && resetView.enabled && resetView.magicMatch && resetView.trust==ConfigurationTrust::UNKNOWN);

  configured(d,b);const auto first=b.frames;auto off=settings(false);configured(d,b,off);
  CHECK((b.ext[0x04A0]&0x00A1U)==0U && b.pendingWake);harvestBeforeWrites(b,first);
  const auto stable=b.frames,mutations=b.mutations;configured(d,b,off);
  CHECK(b.mutations==mutations && writes(b,stable,0x04A0)==0U); // Complete unchanged profile skips writes.
}
void admission() {
  for(unsigned fault=0;fault<9U;++fault) {
    Controller b;Driver d;if(fault!=0U)ready(d,b);auto s=settings();
    if(fault==1U)s.maintenanceAdmitted=false;
    if(fault==2U)s.receiveTrafficQuiesced=false;
    if(fault==3U)s.pulseWidth=4;
    if(fault==4U)s.route=static_cast<WakeRoute>(255);
    if(fault==5U)s.route=WakeRoute::LED0;
    if(fault==6U)s.route=WakeRoute::LED3;
    if(fault==7U)s.route=WakeRoute::INTERRUPT;
    if(fault==8U) { s.route=WakeRoute::LED1;s.pinAvailable=false; }
    const auto calls=b.calls,clocks=b.clocks;OperationToken token{999};const auto before=d.wakeSnapshot();
    CHECK(!d.startConfigureWake(s,budget(),token).inProgress());CHECK(token.value==999 && b.calls==calls && b.clocks==clocks);
    CHECK(d.wakeSnapshot().desiredValid==before.desiredValid);
  }
  Controller b;Driver d;ready(d,b);OperationToken token{999};
  CHECK(d.acknowledgeWake(budget(),token).is(Err::INVALID_CONFIG));CHECK(token.value==999);
  for(unsigned empty=0;empty<2U;++empty) {
    const auto calls=b.calls,clocks=b.clocks;
    CHECK(d.startConfigureWake(settings(),budget(empty?2048:0,empty?0:1000000),token).is(Err::OPERATION_TIMEOUT));
    CHECK(token.value==999 && b.calls==calls && b.clocks==clocks);
  }
  auto pulse=settings();pulse.levelMode=false;configured(d,b,pulse);
  const auto calls=b.calls;CHECK(!d.acknowledgeWake(budget(),token).inProgress() && b.calls==calls);
}
void routeChanges();
void routing() {
  for(bool basic:{false,true}) {
    Controller b;b.id2=basic?0xA111:0xA131;b.enhancedMii=!basic;Driver d;ready(d,b);auto wake=settings();
    wake.route=basic?WakeRoute::LED1:WakeRoute::LED2;wake.pinAvailable=wake.colUnused=true;
    configured(d,b,wake);CHECK((b.ext[basic?0x0304U:0x0305U]&7U)==2U);
  }

  // A declared available pin is programmed only after detector disable; its
  // explicit narrow mux ownership is replayed after a resetting operation.
  for(bool led1:{false,true}) {
    Controller b;Driver d;ready(d,b);b.ext[0x0308]=1;auto wake=settings();
    wake.route=led1?WakeRoute::LED1:WakeRoute::LED2;wake.pinAvailable=wake.led1ClockUnused=wake.colUnused=true;
    const auto target=led1?uint16_t{0x0304}:uint16_t{0x0305};b.ext[target]=0xA5A4;
    const auto first=b.frames;configured(d,b,wake);CHECK(b.ext[target]==0xA5A2);
    CHECK(writes(b,first,target)==1U && !b.matchWhileEnabled);
    CHECK(d.wakeSnapshot().pinControlValid && d.wakeSnapshot().rawPinControl==0xA5A2);
    WakeSnapshot view;CHECK(d.readWakeStatus(view,{1000000,33}).ok());CHECK(!view.pinControlValid && view.rawPinControl==0xA5A2);

  }

  routeChanges();
  for(auto route:{WakeRoute::LED1,WakeRoute::LED2}) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.route=route;s.pinAvailable=s.led1ClockUnused=s.colUnused=true;
    b.ext[0x0304]=2;b.ext[0x0305]=2;b.ext[0x0308]=1;configured(d,b,s);
    CHECK(writes(b,0,0x0304)==0U && writes(b,0,0x0305)==0U); // Already selected pins need no write.
  }
  for(unsigned fault=1;fault<3U;++fault) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.route=WakeRoute::LED1;s.pinAvailable=true;s.led1ClockUnused=true;
    b.ext[0x0304]=fault==0U?4:2;b.ext[0x0308]=1;
    if(fault==1U)s.led1ClockUnused=false;
    if(fault==2U)b.ext[0x0308]=0x0200;
    OperationToken t;const auto before=b.mutations;auto status=d.startConfigureWake(s,budget(),t);
    if(status.inProgress())status=drain(d,b,t);
    CHECK(!status.ok() && b.mutations==before);
    if(d.operationSnapshot().state==OperationState::RESULT_PENDING)take(d,t);
  }
  for(bool basic:{false,true}) {
    Controller b;b.id2=basic?0xA111:0xA131;b.enhancedMii=!basic;Driver d;ready(d,b);
    auto s=settings();s.route=WakeRoute::LED2;s.pinAvailable=true;s.colUnused=false;
    const auto calls=b.calls,clocks=b.clocks;OperationToken token{999};
    CHECK(!d.startConfigureWake(s,budget(),token).inProgress());CHECK(token.value==999 && b.calls==calls && b.clocks==clocks);
  }
  // Existing IRQ policy owns pin21 routing; a polarity epoch cannot be silently relabelled.
  for(bool polarity:{false,true}) {
    Controller b;Driver d;ready(d,b);InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=true;
    irq.globalEnable=true;irq.misr2Mask=2;irq.sharedSource=polarity?InterruptSharedSource::POLARITY:InterruptSharedSource::WAKE;
    OperationToken t;CHECK(d.startConfigureInterrupts(irq,budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    auto s=settings();s.route=WakeRoute::INTERRUPT;s.pinAvailable=true;const auto calls=b.calls;
    if(polarity) { CHECK(!d.startConfigureWake(s,budget(),t).inProgress());CHECK(b.calls==calls); }
    else { configured(d,b,s);CHECK(d.interruptSnapshot().selector==InterruptSharedSource::WAKE); }
  }
}
void routeChanges() {
  {
    Controller b;Driver d;ready(d,b);InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=true;
    irq.globalEnable=irq.powerDownInputSafe=true;irq.outputEnable=false;irq.misr2Mask=2;OperationToken token;
    CHECK(d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    auto wake=settings();wake.route=WakeRoute::INTERRUPT;wake.pinAvailable=true;const auto calls=b.calls,clocks=b.clocks;token.value=999;
    CHECK(!d.startConfigureWake(wake,budget(),token).inProgress());CHECK(token.value==999 && b.calls==calls && b.clocks==clocks && !d.wakeSnapshot().desiredValid);
  }

  for(bool enabled:{false,true})for(auto route:{WakeRoute::LED1,WakeRoute::LED2}) {
    Controller b;Driver d;ready(d,b);auto wake=settings(enabled);wake.route=route;wake.pinAvailable=wake.led1ClockUnused=wake.colUnused=true;
    b.ext[0x0304]=2;b.ext[0x0305]=2;b.ext[0x0308]=1;configured(d,b,wake);
    InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=irq.led1ClockUnused=irq.colUnused=true;
    irq.pin=route==WakeRoute::LED1?InterruptPin::LED1:InterruptPin::LED2;
    const auto calls=b.calls,clocks=b.clocks;OperationToken token{999};
    CHECK(!d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(b.calls==calls && b.clocks==clocks && token.value==999);
    CHECK(!d.interruptSnapshot().desiredValid);
  }
  for(bool enabled:{false,true})for(unsigned field=0;field<4U;++field) {
    Controller b;Driver d;ready(d,b);InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=true;
    irq.globalEnable=true;irq.misr2Mask=2;OperationToken token;
    CHECK(d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    auto wake=settings(enabled);wake.route=WakeRoute::INTERRUPT;wake.pinAvailable=true;configured(d,b,wake);
    if(field==0U)irq.globalEnable=false;
    if(field==1U)irq.misr2Mask=0;
    if(field==2U) { irq.outputEnable=false;irq.powerDownInputSafe=true; }
    if(field==3U)irq.sharedSource=InterruptSharedSource::POLARITY;
    const auto calls=b.calls,clocks=b.clocks;token.value=999;
    CHECK(!d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(b.calls==calls && b.clocks==clocks && token.value==999);
    const auto retained=d.interruptSnapshot().desired;
    CHECK(retained.globalEnable && retained.misr2Mask==2 && retained.outputEnable && retained.sharedSource==InterruptSharedSource::WAKE);
  }
  for(bool enabled:{false,true}) {
    Controller b;Driver d;ready(d,b);LinkSettings link;link.advertisement.technologyMask=8;OperationToken token;
    CHECK(d.startConfigure(link,{},budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    auto wake=settings(enabled);wake.route=WakeRoute::LED1;wake.pinAvailable=wake.led1ClockUnused=true;
    b.ext[0x0304]=2;b.ext[0x0308]=1;configured(d,b,wake);
    EeeSettings eee;eee.enabled=eee.maintenanceAdmitted=true;const auto calls=b.calls,clocks=b.clocks;
    CHECK(!d.startConfigureEee(eee,budget(),token).inProgress());CHECK(b.calls==calls && b.clocks==clocks);
    CHECK(!d.eeeSnapshot().desiredValid);
  }
}
void acquisition() {
  // Repeated operation reads have the same per-word validity as live capture:
  // a failed requested word keeps its raw history but loses current validity.
  for(unsigned source=0;source<3U;++source)for(unsigned phase=0;phase<4U;++phase) {
    Controller b;Driver d;ready(d,b);auto wake=settings();
    if(source==2U) { wake.route=WakeRoute::LED2;wake.pinAvailable=true; }
    configured(d,b,wake);const auto previous=d.wakeSnapshot();
    const auto token=configure(d,b,wake);
    CHECK(poll(d,b,token).inProgress()); // Identity, then control or selected pin.
    if(source==1U)CHECK(poll(d,b,token).inProgress()); // Control, then first MAC word.
    b.failFrame=static_cast<int>(b.frames+phase);
    CHECK(drain(d,b,token).is(Err::MDIO_TIMEOUT));const auto result=take(d,token);
    const auto view=d.wakeSnapshot();CHECK(view.observedStale);
    if(source==0U)CHECK(!view.controlValid && !result.wake.controlValid && view.rawControl==previous.rawControl);
    if(source==1U)CHECK((view.credentialValidMask&1U)==0U && (result.wake.credentialValidMask&1U)==0U && view.rawMac[0]==previous.rawMac[0]);
    if(source==2U)CHECK(!view.pinControlValid && !result.wake.pinControlValid && view.rawPinControl==previous.rawPinControl);
  }
  {
    Controller b; Driver d; ready(d,b); configured(d,b);
    WakeSnapshot snapshot; CHECK(d.readWakeStatus(snapshot,{1000000,33}).ok());
    const auto stamp=snapshot.observedAtUs; const auto calls=b.calls;
    snapshot.rawControl=0xCAFE;
    CHECK(d.readWakeStatus(snapshot,{1000000,0}).is(Err::OPERATION_TIMEOUT));
    CHECK(snapshot.rawControl==0xCAFE && b.calls==calls);
    CHECK(d.wakeSnapshot().observedStale && d.wakeSnapshot().observedAtUs==stamp &&
          d.wakeSnapshot().lastStatus.is(Err::OPERATION_TIMEOUT));
  }

  for(unsigned method=0;method<3U;++method) {
    Controller b;Driver d;ready(d,b);configured(d,b);b.ext[0x04A1]=0x0001;
    if(method==1U) { EvidenceSnapshot evidence;CHECK(d.acquireEvidence(evidenceBit(EvidenceSource::RXFS),evidence,{1000000,4}).ok()); }
    else {
      if(method==2U)b.failFrame=static_cast<int>(b.frames+32U);
      WakeSnapshot view;const auto status=d.readWakeStatus(view,{1000000,33});
      CHECK(method==2U?status.is(Err::MDIO_TIMEOUT):status.ok());
    }
    CHECK(d.wakeSnapshot().trust==ConfigurationTrust::DIRTY && d.wakeSnapshot().rawRxfs==1);
    const auto calls=b.calls;OperationToken token;PowerSettings power;power.maintenanceAdmitted=power.managementClockAvailable=true;
    CHECK(!d.startConfigurePower(power,budget(),token).inProgress() && b.calls==calls);
  }

  Controller reference;Driver ref;ready(ref,reference);configured(ref,reference);
  const auto first=reference.frames,calls=reference.calls;WakeSnapshot complete;CHECK(ref.readWakeStatus(complete,{1000000,128}).ok());
  const auto count=reference.frames-first;CHECK(count==33U && reference.calls-calls==8U);
  {
    Controller b;Driver d;ready(d,b);configured(d,b);b.ext[0x04A2]^=1;uint16_t raw=0;
    CHECK(d.readMmd(31,0x04A2,raw,{1000000,4}).ok());CHECK(raw==0x2210 && d.wakeSnapshot().trust==ConfigurationTrust::DIRTY);
    InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=true;OperationToken token{999};const auto before=b.calls;
    CHECK(!d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(b.calls==before && token.value==999);
  }
  for(unsigned phase=0;phase<count;++phase) {
    Controller b;Driver d;ready(d,b);configured(d,b);b.ext[0x04A1]=0x10E1;b.words[19]=0xA502;
    const auto baseline=d.wakeSnapshot();b.failFrame=static_cast<int>(b.frames+phase);WakeSnapshot sentinel;sentinel.rawControl=0xCAFE;
    CHECK(d.readWakeStatus(sentinel,{1000000,128}).is(Err::MDIO_TIMEOUT));CHECK(sentinel.rawControl==0xCAFE);
    const auto view=d.wakeSnapshot();CHECK(view.observedStale && view.observedAtUs==baseline.observedAtUs);
    const auto target=reference.trace[first+phase].target;const auto evidence=d.evidenceSnapshot();
    if(reference.trace[first+phase].kind==FrameKind::READ && target==0x04A1)CHECK(sample(evidence,EvidenceSource::RXFS).historyLost);
    if(reference.trace[first+phase].kind==FrameKind::READ && target==0x0013)CHECK(sample(evidence,EvidenceSource::MISR2).historyLost);
    if(phase==32U)CHECK(view.rxfsValid && view.rawRxfs==0x10E1 && (sample(evidence,EvidenceSource::RXFS).eventBits&0xE1U)==0xE1U);

  }
  Controller b;Driver d;ready(d,b);configured(d,b);b.ext[0x04A1]=0x10E1;b.words[19]=0xA502;
  WakeSnapshot view;CHECK(d.readWakeStatus(view,{1000000,128}).ok());
  CHECK(view.rxfsValid && view.misr2Valid && view.rawRxfs==0x10E1 && view.rawMisr2==0xA502);
  const auto history=d.evidenceSnapshot();CHECK((sample(history,EvidenceSource::RXFS).eventBits&0x00E1U)==0x00E1U);
  CHECK((sample(history,EvidenceSource::MISR2).eventBits&0xA500U)==0xA500U);
  const auto reads=b.calls,clocks=b.clocks;const auto one=d.wakeSnapshot(),two=d.wakeSnapshot();
  CHECK(one.rawRxfs==two.rawRxfs && b.calls==reads && b.clocks==clocks);
  CHECK(d.readWakeStatus(view,{1000000,128}).ok());CHECK(view.rawRxfs==0x1000 && (view.rawMisr2&0xFF00U)==0U);
  b.injectFrame=static_cast<int>(b.frames+31U);b.injectMisr2=0x0200;
  CHECK(d.readWakeStatus(view,{1000000,33}).ok());CHECK(view.rawRxfs==0x1000 && (view.rawMisr2&0x0200U)!=0U);

}
void failures() {
  for(bool led1:{false,true}) {
    Controller reference;Driver ref;ready(ref,reference);reference.ext[0x0308]=1;auto wake=settings();
    wake.route=led1?WakeRoute::LED1:WakeRoute::LED2;wake.pinAvailable=wake.led1ClockUnused=wake.colUnused=true;
    const auto first=reference.frames;configured(ref,reference,wake);const auto count=reference.frames-first;
    for(unsigned phase=0;phase<count;++phase) {
      Controller b;Driver d;ready(d,b);b.ext[0x0308]=1;const auto start=b.frames;b.failFrame=static_cast<int>(start+phase);
      const auto token=configure(d,b,wake);CHECK(drain(d,b,token).is(Err::MDIO_TIMEOUT));const auto result=take(d,token);
      CHECK(b.frames==start+phase+1U && !result.persistentRestored && !b.matchWhileEnabled);
    }
    const auto target=led1?uint16_t{0x0304}:uint16_t{0x0305};unsigned last=first;
    for(unsigned i=first;i<reference.frames;++i)if(reference.trace[i].kind==FrameKind::READ && reference.trace[i].target==target)last=i;
    Controller b;Driver d;ready(d,b);b.ext[0x0308]=1;b.corruptFrame=static_cast<int>(b.frames+last-first);b.corruptMask=1;
    const auto token=configure(d,b,wake);CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));const auto result=take(d,token);
    CHECK(result.mismatch.valid && result.cleanupOwed && !b.matchWhileEnabled);
  }

  for(bool ack:{false,true}) {
    Controller reference;Driver ref;ready(ref,reference);if(ack)configured(ref,reference);
    const auto first=reference.frames,firstCall=reference.calls;OperationToken token;
    if(ack)CHECK(ref.acknowledgeWake(budget(),token).inProgress());else token=configure(ref,reference,settings());
    CHECK(drain(ref,reference,token).ok());take(ref,token);const auto count=reference.frames-first;
    CHECK(count<160U);
    for(unsigned phase=0;phase<count;++phase)for(bool accepted:{false,true}) {
      Controller b;Driver d;ready(d,b);if(ack)configured(d,b);const auto start=b.frames;
      b.failFrame=static_cast<int>(start+phase);b.acceptedFailure=accepted;
      if(ack)CHECK(d.acknowledgeWake(budget(),token).inProgress());else token=configure(d,b,settings());
      const auto status=drain(d,b,token);CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail==-31031);
      const auto result=take(d,token);CHECK(!result.status.ok() && b.frames==start+phase+1U && b.releases==b.calls && !b.locked);
      bool priorMutation=false;for(unsigned i=0;i<phase;++i)priorMutation|=semantic(reference.trace[first+i]);
      if(priorMutation || (accepted && semantic(reference.trace[first+phase])))CHECK(result.cleanupOwed);
      if(!ack)CHECK(d.wakeSnapshot().trust!=ConfigurationTrust::VERIFIED);
    }
    for(unsigned call=0;call<reference.calls-firstCall;++call) {
      Controller b;Driver d;ready(d,b);if(ack)configured(d,b);b.releaseCall=static_cast<int>(b.calls+call);
      if(ack)CHECK(d.acknowledgeWake(budget(),token).inProgress());else token=configure(d,b,settings());
      CHECK(!drain(d,b,token).ok());CHECK(!take(d,token).failingBatch.release.ok());
    }
  }
  // Every final comparison independently rejects the wrong returned literal.
  for(uint16_t target:{uint16_t{0x04A0},uint16_t{0x04A2},uint16_t{0x04A3},uint16_t{0x04A4},uint16_t{0x04A5},uint16_t{0x04A6},uint16_t{0x04A7}}) {
    Controller reference;Driver ref;ready(ref,reference);const auto first=reference.frames;configured(ref,reference);
    unsigned last=first;for(unsigned i=first;i<reference.frames;++i)if(reference.trace[i].kind==FrameKind::READ && reference.trace[i].target==target)last=i;
    Controller b;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+last-first);b.corruptMask=1;
    const auto token=configure(d,b,settings());CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));const auto result=take(d,token);
    CHECK(result.mismatch.valid && result.cleanupOwed && !result.persistentRestored);
  }
  {
    Controller b;Driver d;ready(d,b);configured(d,b);b.corruptFrame=static_cast<int>(b.frames+19U);b.corruptMask=1;
    OperationToken token;CHECK(d.acknowledgeWake(budget(),token).inProgress());CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));
    const auto result=take(d,token);CHECK(result.mismatch.valid && result.cleanupOwed && !b.pendingWake);
  }
  Controller b;Driver d;ready(d,b);configured(d,b);b.ext[0x04A1]=0x10E1;b.words[19]=0x0202;
  const auto start=b.frames,firstCall=b.calls;OperationToken token;CHECK(d.acknowledgeWake(budget(),token).inProgress());CHECK(drain(d,b,token).ok());
  const auto result=take(d,token);CHECK(result.status.ok() && !b.pendingWake);harvestBeforeWrites(b,start);
  CHECK(writes(b,start,0x04A0)==1U && b.frames==start+25U && b.calls==firstCall+7U);
  for(unsigned i=start;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==0x04A0)CHECK((b.trace[i].value&0x0800U)==0U);
}
void combinedRoutes(Driver& d,Controller& b,bool wakeLed1) {
  b.clockOffStrap=true;b.ext[0x0308]=1;ready(d,b);const uint16_t irqAddress=wakeLed1?uint16_t{0x0305}:uint16_t{0x0304};
  b.ext[irqAddress]=4;InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=irq.led1ClockUnused=irq.colUnused=irq.globalEnable=true;
  irq.misr2Mask=2;irq.pin=wakeLed1?InterruptPin::LED2:InterruptPin::LED1;OperationToken token;
  CHECK(d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
  auto wake=settings();wake.route=wakeLed1?WakeRoute::LED1:WakeRoute::LED2;wake.pinAvailable=wake.led1ClockUnused=wake.colUnused=true;configured(d,b,wake);
}
void replay() {
  for(bool wakeLed1:{false,true}) {
    Controller reference;Driver ref;combinedRoutes(ref,reference,wakeLed1);OperationToken token;
    const auto first=reference.frames,firstCall=reference.calls;CHECK(ref.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    CHECK(drain(ref,reference,token).ok());take(ref,token);
    CHECK((reference.ext[0x0304]&7U)==(wakeLed1?2U:4U) && (reference.ext[0x0305]&7U)==(wakeLed1?4U:2U));
    const uint16_t irqAddress=wakeLed1?uint16_t{0x0305}:uint16_t{0x0304};
    unsigned writeIndex=first;for(unsigned i=first;i<reference.frames;++i)if(semantic(reference.trace[i]) && reference.trace[i].target==irqAddress)writeIndex=i;
    CHECK(writeIndex>=first+4U);
    // The IRQ tail's new whole RMW5 + verify4 are bounded independently of WoL.
    for(unsigned phase=writeIndex-4U;phase<=writeIndex+4U;++phase) {
      Controller b;Driver d;combinedRoutes(d,b,wakeLed1);b.failFrame=static_cast<int>(b.frames+phase-first);
      CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(d,b,token).is(Err::MDIO_TIMEOUT));
      const auto result=take(d,token);CHECK(result.cleanupOwed && !result.persistentRestored);
    }
    for(unsigned boundary=0;boundary<reference.calls-firstCall;++boundary) {
      Controller b;Driver d;combinedRoutes(d,b,wakeLed1);CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());
      for(unsigned turn=0;turn<boundary;++turn)CHECK(poll(d,b,token).inProgress());
      const auto frames=b.frames,clocks=b.clocks;CHECK(d.cancel(token).ok());CHECK(b.frames==frames && b.clocks==clocks);
      CHECK(take(d,token).status.is(Err::CANCELLED));
    }
  }

  for(bool irqRoute:{false,true})for(bool led1:{false,true}) {
    Controller b;b.clockOffStrap=true;b.ext[0x0308]=1;Driver d;ready(d,b);OperationToken token;
    const uint16_t address=led1?uint16_t{0x0304}:uint16_t{0x0305};
    if(irqRoute) {
      b.ext[address]=4;InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=irq.led1ClockUnused=irq.colUnused=irq.globalEnable=true;
      irq.misr2Mask=2;irq.pin=led1?InterruptPin::LED1:InterruptPin::LED2;
      CHECK(d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    }
    auto wake=settings();wake.pinAvailable=wake.led1ClockUnused=wake.colUnused=true;
    wake.route=irqRoute?WakeRoute::INTERRUPT:led1?WakeRoute::LED1:WakeRoute::LED2;configured(d,b,wake);
    CHECK((b.ext[address]&7U)==(irqRoute?4U:2U));
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK((b.ext[address]&7U)==(irqRoute?4U:2U) && d.wakeSnapshot().trust==ConfigurationTrust::VERIFIED && !b.matchWhileEnabled);
  }

  for(bool down:{false,true}) {
    Controller b;Driver d;ready(d,b);configured(d,b);PowerSettings power;
    power.maintenanceAdmitted=power.managementClockAvailable=power.clockLossApproved=true;
    power.request=down?PowerRequest::IEEE_POWER_DOWN:PowerRequest::NORMAL;OperationToken token;
    CHECK(d.startConfigurePower(power,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(b.ext[0x04A2]==0x2211 && b.ext[0x04A7]==0x2F2E && (b.ext[0x04A0]&0x17A1U)==0x11A1);
    CHECK(d.wakeSnapshot().trust==ConfigurationTrust::VERIFIED && d.powerSnapshot().requestedPowerDown==down);
  }

  for(auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
    Controller b;Driver d;ready(d,b);configured(d,b);OperationToken token;
    CHECK(d.startInitialize({reset},budget(),token).inProgress());CHECK(drain(d,b,token).ok());const auto result=take(d,token);
    CHECK(result.persistentRestored && !result.cleanupOwed && d.wakeSnapshot().trust==ConfigurationTrust::VERIFIED);
    CHECK(b.ext[0x04A2]==0x2211 && b.ext[0x04A7]==0x2F2E && (b.ext[0x04A0]&0x17A1U)==0x11A1);
    CHECK(!b.matchWhileEnabled && b.hard==(reset==ResetKind::HARD?1U:0U));
    const auto mutations=b.mutations;CHECK(d.startVerify(budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);CHECK(b.mutations==mutations);
  }
  for(bool routed:{false,true}) {
    auto wake=settings();if(routed) { wake.route=WakeRoute::LED1;wake.pinAvailable=wake.led1ClockUnused=true; }
    Controller reference;reference.clockOffStrap=true;reference.ext[0x0308]=1;Driver ref;ready(ref,reference);configured(ref,reference,wake);OperationToken token;
    const auto start=reference.frames;CHECK(ref.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(ref,reference,token).ok());take(ref,token);
    const auto count=reference.frames-start;
    for(unsigned phase=0;phase<count;++phase) {
      Controller b;b.clockOffStrap=true;b.ext[0x0308]=1;Driver d;ready(d,b);configured(d,b,wake);b.failFrame=static_cast<int>(b.frames+phase);
      CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(!drain(d,b,token).ok());const auto r=take(d,token);
      CHECK(!r.persistentRestored);
      bool acceptedMutation=false;for(unsigned i=0;i<=phase;++i)acceptedMutation|=semantic(reference.trace[start+i]);
      if(acceptedMutation)CHECK(r.cleanupOwed);
    }
  }
}
Status ownerTurns(Driver& d,Controller& b,WakeAttempt& attempt,SharedMdioQuiet& quiet) {
  Status status{Err::IN_PROGRESS,0,"Pending"};
  for(unsigned turn=0;turn<512U && status.inProgress();++turn) {
    const auto state=d.operationSnapshot();
    if(state.nextPollValid && b.now<state.nextPollAtUs)b.now=state.nextPollAtUs;
    if(state.busQuietValid && b.now<state.busQuietUntilUs)b.now=state.busQuietUntilUs;
    status=pollInitialization(d,attempt,quiet);
  }
  CHECK(!status.inProgress() && attempt.resultAvailable);return status;
}
void ownerExample() {
  Controller b;Driver d;ready(d,b);SharedMdioQuiet quiet;WakeAttempt policy;
  CHECK(beginWakePolicy(d,settings(),budget(),policy).inProgress());CHECK(ownerTurns(d,b,policy,quiet).ok());
  b.ext[0x04A1]=0x10E1;b.words[19]=0x0202;WakeSnapshot view;
  CHECK(acquireWakeView(d,view,{1000000,33}).ok());const auto calls=b.calls,clocks=b.clocks;
  CHECK(cachedWakeView(d).rawRxfs==0x10E1 && cachedWakeView(d).misr2Valid && b.calls==calls && b.clocks==clocks);
  WakeAttempt acknowledgement;CHECK(beginWakeAcknowledgement(d,budget(),acknowledgement).inProgress());
  CHECK(ownerTurns(d,b,acknowledgement,quiet).ok() && !b.pendingWake);
  b.pendingWake=true;b.failFrame=static_cast<int>(b.frames+15U);WakeAttempt failed;
  CHECK(beginWakeAcknowledgement(d,budget(),failed).inProgress());CHECK(ownerTurns(d,b,failed,quiet).is(Err::MDIO_TIMEOUT));
  CHECK(!b.pendingWake && failed.result.cleanupOwed && policy.result.status.ok() && acknowledgement.result.status.ok());
}
void ownership() {
  for(bool late:{false,true}) {
    Controller b;Driver d;ready(d,b);const auto token=configure(d,b,settings());
    for(unsigned turn=0;turn<24U;++turn)CHECK(poll(d,b,token).inProgress());
    b.ext[0x04A2]^=1;
    if(late)b.lateCall=static_cast<int>(b.calls);else b.releaseCall=static_cast<int>(b.calls);
    uint16_t sentinel=0xCAFE;const auto status=d.readMmd(31,0x04A2,sentinel,{1000000,4});
    CHECK(status.is(late?Err::OPERATION_TIMEOUT:Err::MDIO_ERROR) && sentinel==0xCAFE);
    if(!late)CHECK(status.detail==-37);
    const auto frames=b.frames;CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));
    const auto result=take(d,token);
    CHECK(result.mismatch.valid && result.mismatch.observed==0x2210 && result.cleanupOwed && b.frames==frames);
    CHECK((b.ext[0x04A0]&0x0080U)==0U); // Known-wrong match was never enabled.
  }
  // A consumed contradiction must freeze the operation before another poll can
  // enable a match already known to be wrong. Cover every post-credential phase.
  for(unsigned boundary=18;boundary<28U;++boundary) {
    Controller b;Driver d;ready(d,b);const auto token=configure(d,b,settings());
    for(unsigned turn=0;turn<boundary;++turn)CHECK(poll(d,b,token).inProgress());
    b.ext[0x04A2]^=1;uint16_t raw=0;
    CHECK(d.readMmd(31,0x04A2,raw,{1000000,4}).ok());
    CHECK(d.operationSnapshot().state==OperationState::RESULT_PENDING);
    const auto frames=b.frames,mutations=b.mutations;
    CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));
    const auto result=take(d,token);
    CHECK(result.mismatch.valid && result.cleanupOwed && b.frames==frames && b.mutations==mutations);
  }
  for(bool ack:{false,true}) {
    Controller b;Driver d;ready(d,b);OperationToken token;if(ack) { configured(d,b);CHECK(d.acknowledgeWake(budget(),token).inProgress()); }
    else token=configure(d,b,settings());
    for(unsigned turn=0;turn<(ack?4U:18U);++turn)CHECK(poll(d,b,token).inProgress());
    b.ext[0x04A2]^=1;uint16_t raw=0;CHECK(d.readMmd(31,0x04A2,raw,{1000000,4}).ok());
    const auto mutations=b.mutations;CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));const auto result=take(d,token);
    CHECK(result.mismatch.valid && result.cleanupOwed && d.wakeSnapshot().trust==ConfigurationTrust::DIRTY && b.mutations==mutations);
  }
  {
    Controller b;Driver d;ready(d,b);const auto token=configure(d,b,settings());CHECK(poll(d,b,token).inProgress());
    b.id2=0xA111;DeviceIdentity id;CHECK(d.readIdentity(id,{1000000,2}).ok());CHECK(id.mode==OperatingMode::BASIC && !d.isInitialized());const auto mutations=b.mutations;
    CHECK(!drain(d,b,token).ok());take(d,token);CHECK(b.mutations==mutations);
  }

  ownerExample();
  for(bool routed:{false,true}) {
    auto wake=settings();if(routed) { wake.route=WakeRoute::LED1;wake.pinAvailable=wake.led1ClockUnused=true; }
    Controller reference;reference.clockOffStrap=true;reference.ext[0x0308]=1;Driver ref;ready(ref,reference);const auto first=reference.calls;configured(ref,reference,wake);
    const auto calls=reference.calls-first;
    for(unsigned boundary=0;boundary<calls;++boundary) {
      Controller b;b.clockOffStrap=true;b.ext[0x0308]=1;Driver d;ready(d,b);const auto token=configure(d,b,wake);
      for(unsigned i=0;i<boundary;++i)CHECK(poll(d,b,token).inProgress());
      const auto frames=b.frames,clocks=b.clocks;CHECK(d.cancel(token).ok());CHECK(b.frames==frames && b.clocks==clocks);
      const auto cancelled=take(d,token);CHECK(cancelled.status.is(Err::CANCELLED));
      if(cancelled.cleanupOwed) { CHECK(d.end().is(Err::BUSY));OperationToken restore;
        CHECK(d.startInitialize({ResetKind::HARD},budget(),restore).inProgress());CHECK(drain(d,b,restore).ok());take(d,restore); }
    }
  }
  Controller b;Driver d,other;ready(d,b);CHECK(other.bind(b.config(2)).ok());b.competitor=&other;
  const auto start=b.frames;configured(d,b);CHECK(b.excluded==b.frames-start);b.competitor=nullptr;
  CHECK(!other.wakeSnapshot().desiredValid);
}
void timing() {
  {
    Controller b;b.now=0;b.frameTime=0;Driver d;ready(d,b);configured(d,b);
    CHECK(d.wakeSnapshot().timeValid && d.wakeSnapshot().observedAtUs==1U);
  }
  {
    Controller b;Driver d;ready(d,b);b.now=std::numeric_limits<uint64_t>::max();OperationToken token;
    CHECK(d.startConfigureWake(settings(),budget(111,std::numeric_limits<uint64_t>::max()),token).inProgress());
    const auto frames=b.frames;CHECK(!d.pollOperation(token).ok());CHECK(b.frames==frames && d.transportSnapshot().clockInvalid);take(d,token);
  }

  for(uint32_t allowance:{110U,111U}) {
    Controller b;Driver d;ready(d,b);OperationToken token;
    CHECK(d.startConfigureWake(settings(),budget(allowance),token).inProgress());const auto status=drain(d,b,token);const auto result=take(d,token);
    if(allowance==111U)CHECK(status.ok());else CHECK(status.is(Err::OPERATION_TIMEOUT) && result.cleanupOwed);
  }

  Controller b;Driver d;ready(d,b);const auto token=configure(d,b,settings());
  const auto frames=b.frames;CHECK(d.pollOperation(token,0).inProgress() && b.frames==frames);
  CHECK(drain(d,b,token).ok());take(d,token);
  for(unsigned defect=0;defect<4U;++defect) {
    Controller bus;Driver driver;ready(driver,bus);const auto call=static_cast<int>(bus.calls+1U);
    if(defect==0U)bus.lateCall=call;
    if(defect==1U)bus.regressCall=call;
    if(defect==2U)bus.invalidCall=call;
    if(defect==3U)bus.badReceiptCall=call;
    const auto t=configure(driver,bus,settings());CHECK(!drain(driver,bus,t).ok());take(driver,t);
  }
  Controller frozen;Driver stuck;ready(stuck,frozen);const auto t=configure(stuck,frozen,settings());const auto calls=frozen.calls;
  Status status{Err::IN_PROGRESS,0,"Pending"};for(unsigned i=0;i<12U && status.inProgress();++i)status=stuck.pollOperation(t,0);
  CHECK(status.is(Err::OPERATION_TIMEOUT) && frozen.calls==calls);take(stuck,t);
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"vectors",vectors},{"admission",admission},{"routing",routing},{"acquisition",acquisition},
    {"failures",failures},{"replay",replay},{"ownership",ownership},{"timing",timing}};
  return test::runSuites(argc,argv,suites);
}
