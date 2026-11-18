#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include "../examples/portable/Pins.h"
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target; };

// Independent SNLS647I tables 8-42/43/49/75/76/85/89. Numeric oracles below
// come from the original PDF, not production masks. The fake models shared
// latch consumption, never physical light, electrical drive or pulse timing.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<Wire,8192> trace{};
  uint16_t selector{0},address{0},id2{0xA131},wakePin{0};
  uint64_t now{100},frameTime{2};
  unsigned frames{0},calls{0},clocks{0},releases{0},mutations{0},hard{0};
  int failFrame{-1},releaseCall{-1},corruptFrame{-1},lateCall{-1},regressCall{-1},invalidCall{-1},badReceiptCall{-1};
  uint16_t corruptMask{1};
  bool ledLoss{false},prbsError{false};
  bool acceptedFailure{true},locked{false},pendingWake{true},matchWhileEnabled{false},enhancedMii{false},clockOffStrap{false};
  Driver* competitor{nullptr}; unsigned excluded{0};
  Controller() { defaults(); }
  void defaults() {
    words.fill(0);ext.fill(0);words[0]=0x3100;words[4]=0x01E1;
    words[25]=0x8001;words[22]=0x0100;words[30]=0x0102;
    words[23]=(id2==0xA111 || enhancedMii)?0U:0x0020U;
    words[24]=0x0400;words[27]=0x007D;words[28]=0x05EE;ext[0x0025]=0x0041;ext[0x0460]=0x5665;ext[0x0303]=ext[0x0304]=ext[0x0305]=ext[0x0306]=8;
    ext[0x030E]=0x8400;ext[0x04A1]=0x1000;ext[0x04A0]=0x1081;ext[0x0308]=clockOffStrap?1U:0U;pendingWake=true;
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=Variant::DP83826I;c.interface=(id2==0xA111 || enhancedMii)?MacInterface::MII:MacInterface::RMII;c.phyAddress=phy;
    words[23]=static_cast<uint16_t>((words[23]&0xFFDFU)|(c.interface==MacInterface::RMII?0x0020U:0U));
    c.transport={this,execute,clock,{8,4,1}};return c;
  }
  static uint64_t clock(void* context) noexcept { auto& b=*static_cast<Controller*>(context);++b.clocks;return b.now; }
  uint16_t read(uint16_t target) {
    if(target==2)return 0x2000;
    if(target==3)return id2;
    if(target==0)words[0]&=0x7DFF;
    if(target==31)words[31]=0;
    if(target==1)ledLoss=false;
    if(target==0x0308)return static_cast<uint16_t>(clockOffStrap || (ext[0x0302]&0x0040U)!=0U?1U:0U);
    const auto value=target<32U?words[target]:ext[target];
    if(target==18 || target==19) { words[target]&=0x00FF;words[16]&=0xFF7F; }
    if(target==20) { words[20]&=0xFF00;words[16]&=0xF7FF; }
    if(target==21) { words[21]=0;words[16]&=0xDFFF; }
    if(target==23)words[23]&=0xFFF3; // Table 8-41 raw RC bits; no fault-polarity claim.
    if(target==0x04A1)ext[target]&=0xFF00;
    return value;
  }
  void write(uint8_t reg,uint16_t value) {
    if(reg==13) { CHECK(value==0x001F || value==0x401F);selector=value;return; }
    if(reg==14 && selector==0x001F) { address=value;return; }
    const uint16_t target=reg==14?address:uint16_t{reg};++mutations;
    if(target==31 && value==0x8000) { ++hard;defaults();return; }
    if(target==0 && value==0x8000) { words[0]=0x3100;words[24]=0x0400;words[25]=0x8001;return; }
    if(target==0x04A0) {
      if((value&0x0800U)==0U)pendingWake=false; // Original table 8-90 W0C.
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
    if(target==27 && (value&0x8000U)!=0U) { prbsError=false; words[27]=static_cast<uint16_t>(value&0x00FFU);return; }
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

PinSettings settings(uint8_t mask=0x0F) {
  PinSettings s;s.maintenanceAdmitted=s.macStopped=s.outputsQuiesced=s.led1ClockUnused=s.colUnused=true;
  s.availableMask=mask;
  for(unsigned i=0;i<4;++i)if((mask&(1U<<i))!=0U)s.pins[i].function=PinFunction::LED;
  return s;
}
PinResult configured(Driver& d,Controller& b,const PinSettings& s) {
  PinResult result;const auto status=d.configurePins(s,result,{1000000,2048});
  CHECK(status.ok() && result.status.ok() && result.verified && !b.locked);
  const auto view=d.pinsSnapshot();
  CHECK(view.desiredValid && view.trust==ConfigurationTrust::VERIFIED && !view.observedStale && view.timeValid==(result.transactions!=0U));
  CHECK(d.operationSnapshot().state==OperationState::IDLE && !d.operationSnapshot().cleanupOwed);
  return result;
}
bool semantic(const Wire& w) { return w.kind==FrameKind::WRITE && w.reg!=13 && !(w.reg==14 && w.target==14); }
unsigned writes(const Controller& b,unsigned from,uint16_t target) {
  unsigned count=0;for(unsigned i=from;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==target)++count;return count;
}
void vectors() {
  for(unsigned pin=0;pin<4;++pin)for(unsigned source=0;source<15;++source) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);auto s=settings(static_cast<uint8_t>(1U<<pin));
    s.pins[pin].source=static_cast<LedSource>(source);b.ext[0x25]=0xA581;b.ext[0x460]=0xABCD;
    configured(d,b,s);
    if(pin==0U)CHECK(b.ext[0x25]==static_cast<uint16_t>((0xA581U&0xFF86U)|(source<<3U)|1U));
    else { const unsigned shift=pin*4U;CHECK(b.ext[0x460]==static_cast<uint16_t>((0xABCDU&~(0xFU<<shift))|(source<<shift))); }
    CHECK((b.ext[0x460]&15U)==13U && (b.ext[0x25]&1U)==1U);
  }
  for(unsigned code=0;code<4;++code)for(bool bypass:{false,true})for(bool swap:{false,true}) {
    Controller b;Driver d;ready(d,b);auto s=settings(1);s.blinkOwned=s.stretchOwned=s.led0SwapOwned=true;
    s.blinkCode=static_cast<uint8_t>(code);s.stretchBypass=bypass;s.led0Swap=swap;
    b.words[24]=0xF96D;b.words[25]=0x7F7F;b.ext[0x25]=0xFD86;
    configured(d,b,s);CHECK((b.words[24]&0x0600U)==code*0x0200U);
    CHECK((b.words[24]&0xF96DU)==0xF96D && (b.words[25]&0xFF7FU)==0x7F7F);
    CHECK((b.words[25]&0x0080U)==(bypass?0x0080U:0U));
    CHECK(b.ext[0x25]==static_cast<uint16_t>(0xFD87U|(swap?0x0200U:0U)));
  }
  for(unsigned pin=0;pin<3;++pin)for(auto polarity:{LedPolarity::ACTIVE_LOW,LedPolarity::ACTIVE_HIGH})
    for(auto drive:{LedDrive::NORMAL,LedDrive::LOW,LedDrive::HIGH}) {
      Controller b;b.clockOffStrap=true;Driver d;ready(d,b);auto s=settings(static_cast<uint8_t>(1U<<pin));
      s.pins[pin].polarity=polarity;s.pins[pin].drive=drive;b.words[24]=0xF96D;b.ext[0x469]=0xFF88;
      configured(d,b,s);
      const bool high=polarity==LedPolarity::ACTIVE_HIGH,force=drive!=LedDrive::NORMAL,value=drive==LedDrive::HIGH;
      if(pin==0)CHECK(b.words[24]==static_cast<uint16_t>((0xF96DU&0xFF6DU)|(high?0x80U:0U)|(force?0x10U:0U)|(value?2U:0U)));
      else { const unsigned shift=pin==1?0U:4U;const uint16_t bits=static_cast<uint16_t>((high?4U:0U)|(force?1U:0U)|(value?2U:0U));
        CHECK(b.ext[0x469]==static_cast<uint16_t>(0xFF88U|(uint16_t{bits}<<shift))); }
      CHECK((b.ext[0x469]&0xFF88U)==0xFF88U);
    }
  // Source/value preparation precedes clock release and mux reassignment;
  // forced drive enables are the final writes. Literal words are PDF-derived.
  Controller b;Driver d;ready(d,b);auto s=settings();s.led1ClockOverride=true;
  s.blinkOwned=s.stretchOwned=true;s.blinkCode=3;s.stretchBypass=true;
  for(unsigned pin=0;pin<4;++pin) {
    s.pins[pin].source=LedSource::PRBS_ERROR;
    if(pin<3U) { s.pins[pin].polarity=LedPolarity::ACTIVE_HIGH;s.pins[pin].drive=LedDrive::HIGH; }
  }
  constexpr uint16_t addresses[]={0x25,0x460,0x19,0x18,0x469,0x302,0x304,0x305,0x18,0x469};
  constexpr uint16_t values[]={0x0071,0xEEE5,0x8081,0x0682,0x0066,0x0040,0x0008,0x0008,0x0692,0x0077};
  const auto start=b.frames;const auto result=configured(d,b,s);unsigned position=0;
  for(unsigned i=start;i<b.frames;++i)if(semantic(b.trace[i])) {
    CHECK(position<10U);
    if(position<10U)CHECK(b.trace[i].target==addresses[position] && b.trace[i].value==values[position]);
    ++position;
  }
  CHECK(position==10U && result.attemptedFrames==85U && result.transactions==24U);
}
void admission() {
  for(unsigned fault=0;fault<15;++fault) {
    Controller b;b.clockOffStrap=true;Driver d;if(fault!=0)ready(d,b);auto s=settings(1);
    if(fault==1)s.maintenanceAdmitted=false;
    if(fault==2)s.macStopped=false;
    if(fault==3)s.outputsQuiesced=false;
    if(fault==4)s.availableMask=0;
    if(fault==5)s.availableMask=0x80;
    if(fault==6)s.pins[0].source=static_cast<LedSource>(15);
    if(fault==7)s.pins[0].function=static_cast<PinFunction>(255);
    if(fault==8)s.pins[0].polarity=static_cast<LedPolarity>(255);
    if(fault==9)s.pins[0].drive=static_cast<LedDrive>(255);
    if(fault==10) { s.blinkOwned=true;s.blinkCode=4; }
    if(fault==11)s.pins[0].function=PinFunction::CLOCK;
    if(fault==12)s.pins[0].function=PinFunction::TX_ERROR;
    if(fault==13)s.pins[0].function=PinFunction::HIGH;
    if(fault==14) { s=settings(8);s.pins[3].drive=LedDrive::HIGH; }
    const auto frames=b.frames,clocks=b.clocks;PinResult out;out.attemptedFrames=999;
    CHECK(!d.configurePins(s,out,{1000000,2048}).ok());CHECK(b.frames==frames && b.clocks==clocks && out.attemptedFrames==999);
    CHECK(!d.pinsSnapshot().desiredValid);
  }
  for(unsigned pin=0;pin<4;++pin)for(bool overridePolarity:{false,true}) {
    Controller b;b.id2=0xA111;Driver d;ready(d,b);auto s=settings(static_cast<uint8_t>(1U<<pin));
    if(overridePolarity)s.pins[pin].polarity=LedPolarity::ACTIVE_LOW;
    const auto frames=b.frames;PinResult out;const auto status=d.configurePins(s,out,{1000000,2048});
    if(pin<2 && !overridePolarity)CHECK(status.ok());else CHECK(!status.ok() && b.frames==frames);
  }
}
void observedMode() {
  for(bool declaredMii:{false,true})for(bool useLed3:{false,true}) {
    if(declaredMii && useLed3)continue; // This combination rejects in pure validation.
    Controller b;b.enhancedMii=declaredMii;b.clockOffStrap=true;Driver d;ready(d,b);
    b.words[23]=declaredMii?0x002CU:0x000CU;
    auto s=settings(useLed3?8U:4U);s.colUnused=declaredMii;
    const auto first=b.frames,mutations=b.mutations;PinResult result;
    CHECK(!d.configurePins(s,result,{1000000,2048}).ok());
    CHECK(b.frames==first+1U && b.trace[first].reg==0x17 && b.trace[first].kind==FrameKind::READ);
    CHECK(b.mutations==mutations && !result.mutationPossible && !result.verified);
    const auto retained=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
    CHECK(retained.valid && retained.samples==1U && (retained.raw&0xCU)==0xCU && !retained.historyLost);
  }
  for(bool afterEffect:{false,true}) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);b.words[23]=0x006CU;
    const auto first=b.frames;b.failFrame=static_cast<int>(first);b.acceptedFailure=afterEffect;PinResult result;
    CHECK(d.configurePins(settings(8),result,{1000000,2048}).is(Err::MDIO_TIMEOUT));
    CHECK(b.frames==first+1U && !result.mutationPossible && !d.operationSnapshot().cleanupOwed);
    const auto retained=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
    CHECK(!retained.valid && retained.samples==0U && retained.historyLost==afterEffect);
    CHECK(b.words[23]==(afterEffect?0x0060U:0x006CU));
  }
  Controller b;b.clockOffStrap=true;Driver d;ready(d,b);configured(d,b,settings(8));
  b.words[23]=0;EvidenceSnapshot evidence;CHECK(d.acquireEvidence(evidenceBit(EvidenceSource::RCSR),evidence,{1000000,1}).ok());
  CHECK(d.pinsSnapshot().trust==ConfigurationTrust::DIRTY && d.pinsSnapshot().observedStale);
  b.words[23]=0x20;OperationToken token;
  CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());
  bool observed=false;
  for(unsigned n=0;n<128U && !observed;++n) {
    const auto first=b.frames;CHECK(poll(d,b,token).inProgress());
    for(unsigned i=first;i<b.frames;++i)observed|=b.trace[i].reg==0x17 && b.trace[i].kind==FrameKind::READ;
  }
  CHECK(observed);b.words[23]=0;
  CHECK(d.acquireEvidence(evidenceBit(EvidenceSource::RCSR),evidence,{1000000,1}).ok());
  const auto after=b.frames;const auto failed=take(d,token);
  CHECK(failed.status.is(Err::CONFIG_MISMATCH) && failed.mismatch.valid && failed.mismatch.registerAddress==0x17);
  CHECK(!failed.persistentRestored && b.frames==after);
  b.words[23]=0x20;CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());
  CHECK(drain(d,b,token).ok());CHECK(take(d,token).pins.trust==ConfigurationTrust::VERIFIED);
}
void routing() {
  observedMode();
  for(auto function:{PinFunction::COLLISION3,PinFunction::COLLISION5,PinFunction::COLLISION6,PinFunction::HIGH}) {
    Controller b;b.enhancedMii=true;Driver d;ready(d,b);auto s=settings(4);s.pins[2].function=function;
    b.ext[0x305]=0xA5A8;configured(d,b,s);const unsigned code=function==PinFunction::COLLISION3?3U:function==PinFunction::COLLISION5?5U:function==PinFunction::COLLISION6?6U:7U;
    CHECK(b.ext[0x305]==static_cast<uint16_t>(0xA5A8U|code));
  }
  for(bool colUnused:{false,true}) {
    Controller b;b.enhancedMii=true;Driver d;ready(d,b);auto s=settings(4);s.colUnused=colUnused;const auto start=b.frames;
    PinResult out;const auto status=d.configurePins(s,out,{1000000,2048});
    if(colUnused) { CHECK(status.ok());CHECK(writes(b,start,0x305)==1U && (b.ext[0x305]&7U)==0U); }
    else CHECK(!status.ok() && b.frames==start);
  }
  for(bool overrideClock:{false,true}) {
    Controller b;Driver d;ready(d,b);auto s=settings(2);s.led1ClockOverride=overrideClock;
    PinResult out;const auto start=b.frames;const auto status=d.configurePins(s,out,{1000000,2048});
    if(overrideClock)CHECK(status.ok() && (b.ext[0x302]&0x40U)!=0U && (b.ext[0x304]&7U)==0U);
    else { CHECK(!status.ok());for(unsigned i=start;i<b.frames;++i)CHECK(!semantic(b.trace[i])); }
  }
  for(unsigned fault=0;fault<4;++fault) {
    Controller b;b.clockOffStrap=true;b.enhancedMii=fault==0;Driver d;ready(d,b);auto s=settings(fault==0?8:2);
    if(fault==1)s.led1ClockUnused=false;
    if(fault==2)s.pins[1].function=PinFunction::TX_ERROR;
    if(fault==3)s.pins[1].function=PinFunction::CLOCK;
    const auto start=b.frames;PinResult out;CHECK(!d.configurePins(s,out,{1000000,2048}).ok());CHECK(b.frames==start);
  }
  // Every source owner keeps its mux. General pin configuration can share only
  // the identical already-owned function; it cannot silently enable IRQ/WoL.
  for(bool wakeOwner:{false,true}) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);OperationToken token;
    b.wakePin=wakeOwner?0x305U:0U;
    if(wakeOwner) { WakeSettings wake;wake.enabled=true;wake.route=WakeRoute::LED2;wake.pinAvailable=true;
      wake.maintenanceAdmitted=wake.receiveTrafficQuiesced=true;CHECK(d.startConfigureWake(wake,budget(),token).inProgress()); }
    else { InterruptSettings irq;irq.pin=InterruptPin::LED2;irq.maintenanceAdmitted=irq.pinAvailable=true;
      CHECK(d.startConfigureInterrupts(irq,budget(),token).inProgress()); }
    CHECK(drain(d,b,token).ok());take(d,token);
    auto s=settings(4);const auto frames=b.frames;PinResult out;CHECK(!d.configurePins(s,out,{1000000,2048}).ok() && b.frames==frames);
    s.pins[2].function=wakeOwner?PinFunction::WAKE:PinFunction::INTERRUPT;
    const auto sharedStart=b.frames;configured(d,b,s);
    CHECK(b.frames==sharedStart+1U && !b.matchWhileEnabled && b.pendingWake);
    s.pins[2].function=wakeOwner?PinFunction::INTERRUPT:PinFunction::WAKE;
    CHECK(!d.configurePins(s,out,{1000000,2048}).ok() && b.frames==sharedStart+1U);
    CHECK(!tryLed2(d,out,{1000000,128}).ok() && b.frames==sharedStart+1U);
    OperationToken rejected{777};
    if(wakeOwner) {
      auto moved=d.wakeSnapshot().desired;moved.route=WakeRoute::STATUS_ONLY;
      CHECK(!d.startConfigureWake(moved,budget(),rejected).ok());
    } else {
      auto moved=d.interruptSnapshot().desired;moved.pin=InterruptPin::INTR_PWRDN;
      CHECK(!d.startConfigureInterrupts(moved,budget(),rejected).ok());
    }
    CHECK(b.frames==sharedStart+1U && rejected.value==777);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    const auto replayStatus=drain(d,b,token);
    CHECK(replayStatus.ok());const auto restored=take(d,token);
    CHECK(restored.pins.trust==ConfigurationTrust::VERIFIED && !restored.cleanupOwed);
    CHECK((b.ext[0x305]&7U)==(wakeOwner?2U:4U));
    CHECK(!b.matchWhileEnabled && b.pendingWake);
  }
}
void failures() {
  Controller reference;reference.clockOffStrap=true;Driver ref;ready(ref,reference);auto s=settings();
  s.blinkOwned=s.stretchOwned=s.led0SwapOwned=true;s.blinkCode=3;s.stretchBypass=s.led0Swap=true;
  for(unsigned pin=0;pin<3;++pin) { s.pins[pin].drive=LedDrive::HIGH;s.pins[pin].polarity=LedPolarity::ACTIVE_HIGH; }
  const auto first=reference.frames,firstCall=reference.calls;const auto success=configured(ref,reference,s);
  const auto count=reference.frames-first,calls=reference.calls-firstCall;
  // Three four-frame prerequisites, six nine-frame extended RMW/readbacks,
  // three three-frame C22 RMW/readbacks and one RCSR mode read. No retries.
  CHECK(count==76U && calls==22U && success.attemptedFrames==76U && success.transactions==22U);
  for(unsigned phase=0;phase<count;++phase)for(bool accepted:{false,true}) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);const auto begin=b.frames;
    b.failFrame=static_cast<int>(begin+phase);b.acceptedFailure=accepted;PinResult out;
    const auto status=d.configurePins(s,out,{1000000,2048});
    CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail==-31031 && out.status.is(Err::MDIO_TIMEOUT));
    CHECK(b.frames==begin+phase+1U && out.attemptedFrames==phase+1U && b.releases==b.calls && !b.locked && !out.verified);
    CHECK(d.pinsSnapshot().trust!=ConfigurationTrust::VERIFIED && d.pinsSnapshot().observedStale);
    bool effect=false;for(unsigned i=0;i<phase;++i)effect|=semantic(reference.trace[first+i]);
    effect|=accepted && semantic(reference.trace[first+phase]);
    if(effect)CHECK(d.operationSnapshot().cleanupOwed && out.mutationPossible);
  }
  for(unsigned call=0;call<calls;++call) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);b.releaseCall=static_cast<int>(b.calls+call);PinResult out;
    CHECK(d.configurePins(s,out,{1000000,2048}).is(Err::MDIO_ERROR));CHECK(out.release.detail==-37 && !out.verified);
  }
  for(uint16_t address:{uint16_t{0x18},uint16_t{0x19},uint16_t{0x25},uint16_t{0x460},uint16_t{0x469}}) {
    unsigned last=first;
    for(unsigned i=first;i<reference.frames;++i)if(reference.trace[i].kind==FrameKind::READ && reference.trace[i].target==address)last=i;
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+last-first);
    b.corruptMask=address==0x18?0x0200:address==0x19?0x0080:address==0x25?0x0008:address==0x460?0x0010:1;
    PinResult out;CHECK(d.configurePins(s,out,{1000000,2048}).is(Err::CONFIG_MISMATCH));
    CHECK(out.mismatch.valid && !out.verified && d.operationSnapshot().cleanupOwed);
  }
  // A later RMW revisits LEDCR to enable forced drive. Its successful read
  // cannot erase an already-verified blink mismatch, even if that write fails.
  unsigned visits=0,revisit=0;
  for(unsigned i=first;i<reference.frames;++i)if(reference.trace[i].kind==FrameKind::READ && reference.trace[i].target==0x18) {
    if(++visits==3U)revisit=i-first;
  }
  CHECK(visits==4U);
  for(bool failWrite:{false,true}) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+revisit);b.corruptMask=0x0200;
    if(failWrite)b.failFrame=b.corruptFrame+1;
    PinResult out;const auto status=d.configurePins(s,out,{1000000,2048});
    CHECK(status.is(failWrite?Err::MDIO_TIMEOUT:Err::CONFIG_MISMATCH));
    CHECK(out.mismatch.valid && out.mismatch.registerAddress==0x18 && !out.verified && d.operationSnapshot().cleanupOwed);
    const auto stopped=b.frames;CHECK(!d.end().ok() && b.frames==stopped);
  }
}
void replay() {
  for(auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);auto s=settings();s.led1ClockOverride=true;
    s.pins[0].source=LedSource::PRBS_ERROR;s.pins[1].source=LedSource::MII_ERROR;s.pins[2].source=LedSource::LPI;s.pins[3].source=LedSource::AN100_FULL;
    s.blinkOwned=s.stretchOwned=true;s.blinkCode=3;s.stretchBypass=true;configured(d,b,s);
    OperationToken token;CHECK(d.startInitialize({reset},budget(),token).inProgress());CHECK(drain(d,b,token).ok());const auto out=take(d,token);
    CHECK(out.pins.trust==ConfigurationTrust::VERIFIED && d.pinsSnapshot().trust==ConfigurationTrust::VERIFIED && !out.cleanupOwed);
    CHECK((b.ext[0x25]&0x0079U)==0x0071 && (b.ext[0x460]&0xFFF0U)==0xABC0 && (b.words[24]&0x0600U)==0x0600);
    CHECK((b.ext[0x302]&0x40U)!=0U && (b.words[25]&0x0080U)!=0U);
    const auto writesBefore=b.mutations;CHECK(d.startVerify(budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);CHECK(b.mutations==writesBefore);
  }
  Controller reference;reference.clockOffStrap=true;Driver ref;ready(ref,reference);auto s=settings();configured(ref,reference,s);
  const auto begin=reference.frames;OperationToken token;CHECK(ref.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(ref,reference,token).ok());take(ref,token);
  const auto count=reference.frames-begin;
  for(unsigned phase=0;phase<count;++phase) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);configured(d,b,s);b.failFrame=static_cast<int>(b.frames+phase);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(!drain(d,b,token).ok());const auto out=take(d,token);
    CHECK(!out.persistentRestored);if(phase>0U)CHECK(out.cleanupOwed || !out.commandAccepted);
  }
  for(bool down:{false,true})for(bool sleep:{false,true}) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);configured(d,b,settings());PowerSettings p;
    p.maintenanceAdmitted=p.managementClockAvailable=p.clockLossApproved=true;p.request=down?PowerRequest::IEEE_POWER_DOWN:PowerRequest::NORMAL;p.activeSleep=sleep;
    CHECK(d.startConfigurePower(p,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(d.pinsSnapshot().trust==ConfigurationTrust::VERIFIED && d.powerSnapshot().requestedPowerDown==down);
  }
  // A permitted owner read between polls observes a contradiction in a pin
  // already verified during restoration. The retained mismatch must survive
  // all later successful rows and cannot become a trusted final profile.
  Controller changed;changed.clockOffStrap=true;Driver d;ready(d,changed);configured(d,changed,settings());
  CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());
  bool applied=false,verified=false;
  for(unsigned n=0;n<128U && !verified;++n) {
    const auto start=changed.frames;CHECK(poll(d,changed,token).inProgress());
    for(unsigned i=start;i<changed.frames;++i) {
      if(changed.trace[i].target!=0x25)continue;
      if(semantic(changed.trace[i]))applied=true;
      else if(applied && changed.trace[i].kind==FrameKind::READ)verified=true;
    }
  }
  CHECK(verified);changed.ext[0x25]^=0x0008;uint16_t raw=0;
  CHECK(d.readMmd(31,0x25,raw,{1000000,4}).ok());
  CHECK(!drain(d,changed,token).ok());const auto mismatch=take(d,token);
  CHECK(mismatch.mismatch.valid && mismatch.mismatch.registerAddress==0x25 && !mismatch.persistentRestored);
  CHECK(mismatch.cleanupOwed && d.pinsSnapshot().trust!=ConfigurationTrust::VERIFIED);
}
void clockDependencies() {
  // A pin-owned IO_CFG1 override is also an IRQ/WoL routing prerequisite.
  const auto setup=[](Driver& d,Controller& b,unsigned owner) {
    b.clockOffStrap=true;ready(d,b);OperationToken token;
    if(owner!=1U) {
      InterruptSettings irq;irq.pin=InterruptPin::LED1;irq.globalEnable=true;irq.misr2Mask=2;
      irq.maintenanceAdmitted=irq.pinAvailable=irq.led1ClockUnused=true;
      CHECK(d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    }
    if(owner!=0U) {
      WakeSettings wake;wake.enabled=true;wake.route=owner==1U?WakeRoute::LED1:WakeRoute::INTERRUPT;
      wake.maintenanceAdmitted=wake.receiveTrafficQuiesced=wake.pinAvailable=wake.led1ClockUnused=true;
      CHECK(d.startConfigureWake(wake,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    }
    auto s=settings(2);s.led1ClockOverride=true;
    s.pins[1].function=owner==1U?PinFunction::WAKE:PinFunction::INTERRUPT;
    configured(d,b,s);return s;
  };
  Controller reference;Driver ref;const auto profile=setup(ref,reference,0);
  const auto start=reference.frames;configured(ref,reference,profile);
  unsigned offsets[2]={0,0};
  for(unsigned i=start;i<reference.frames;++i) {
    if(reference.trace[i].target==0x302 && semantic(reference.trace[i]))offsets[0]=i-start;
    if(reference.trace[i].target==0x308 && reference.trace[i].kind==FrameKind::READ)offsets[1]=i-start;
  }
  CHECK(offsets[0]!=0U && offsets[1]>offsets[0]);
  for(unsigned owner=0;owner<3U;++owner)for(unsigned fault=0;fault<2U;++fault) {
    Controller b;Driver d;const auto s=setup(d,b,owner);
    const auto irqBefore=d.interruptSnapshot();const auto wakeBefore=d.wakeSnapshot();
    b.failFrame=static_cast<int>(b.frames+offsets[fault]);PinResult failed;
    CHECK(d.configurePins(s,failed,{1000000,2048}).is(Err::MDIO_TIMEOUT));
    CHECK(failed.mutationPossible && d.operationSnapshot().cleanupOwed);
    const auto frames=b.frames,clocks=b.clocks;
    for(unsigned reader=0;reader<2U;++reader) {
      if(owner!=1U) {
        const auto irq=d.interruptSnapshot();CHECK(irq.trust==ConfigurationTrust::DIRTY && irq.observedStale);
        CHECK(irq.observedAtUs==irqBefore.observedAtUs && irq.timeValid==irqBefore.timeValid);
      }
      if(owner!=0U) {
        const auto wake=d.wakeSnapshot();CHECK(wake.trust==ConfigurationTrust::DIRTY && wake.observedStale);
        CHECK(wake.observedAtUs==wakeBefore.observedAtUs && wake.timeValid==wakeBefore.timeValid);
      }
    }
    CHECK(b.frames==frames && b.clocks==clocks);b.failFrame=-1;
    if(owner!=0U) {
      WakeSnapshot live;CHECK(d.readWakeStatus(live,{1000000,33}).ok());
      CHECK(live.trust==ConfigurationTrust::DIRTY && live.observedStale);
    }
    OperationToken token;CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());
    CHECK(drain(d,b,token).ok());const auto restored=take(d,token);
    CHECK(restored.persistentRestored && !restored.cleanupOwed && restored.pins.trust==ConfigurationTrust::VERIFIED);
    if(owner!=1U)CHECK(restored.interrupts.trust==ConfigurationTrust::VERIFIED && d.interruptSnapshot().trust==ConfigurationTrust::VERIFIED);
    if(owner!=0U)CHECK(restored.wake.trust==ConfigurationTrust::VERIFIED && d.wakeSnapshot().trust==ConfigurationTrust::VERIFIED);
  }
}
void ownership() {
  clockDependencies();
  Controller b;b.clockOffStrap=true;Driver d,other;ready(d,b);CHECK(other.bind(b.config(2)).ok());b.competitor=&other;
  const auto begin=b.frames;configured(d,b,settings());CHECK(b.excluded==b.frames-begin);b.competitor=nullptr;CHECK(!other.pinsSnapshot().desiredValid);
  const auto calls=b.calls,clocks=b.clocks;const auto one=d.pinsSnapshot(),two=d.pinsSnapshot();
  CHECK(one.validMask==two.validMask && b.calls==calls && b.clocks==clocks);
  CHECK(d.modifyMmd(31,0x18,0x0002,0x0002,{1000000,5}).ok());
  CHECK(d.pinsSnapshot().trust!=ConfigurationTrust::VERIFIED && d.pinsSnapshot().observedStale);
  CHECK(d.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());CHECK(d.pinsSnapshot().desiredValid && d.pinsSnapshot().trust!=ConfigurationTrust::VERIFIED);
  OperationToken token;CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
  CHECK(d.pinsSnapshot().trust==ConfigurationTrust::VERIFIED);
  // Ownership is symmetric: later source setters cannot steal an LED pin.
  const auto before=b.frames;InterruptSettings irq;irq.pin=InterruptPin::LED2;
  irq.maintenanceAdmitted=irq.pinAvailable=true;OperationToken rejected{999};
  CHECK(!d.startConfigureInterrupts(irq,budget(),rejected).ok() && b.frames==before && rejected.value==999);
  WakeSettings wake;wake.enabled=true;wake.route=WakeRoute::LED2;wake.pinAvailable=true;
  wake.maintenanceAdmitted=wake.receiveTrafficQuiesced=true;
  CHECK(!d.startConfigureWake(wake,budget(),rejected).ok() && b.frames==before && rejected.value==999);
  for(unsigned prerequisite=0;prerequisite<3;++prerequisite) {
    Controller sample;sample.clockOffStrap=true;Driver phy;ready(phy,sample);configured(phy,sample,settings());
    const uint16_t address=prerequisite==0?0x303U:prerequisite==1?0x306U:0x308U;
    if(prerequisite==2)sample.clockOffStrap=false;else sample.ext[address]|=1;
    uint16_t raw=0;CHECK(phy.readMmd(31,address,raw,{1000000,4}).ok());
    CHECK(phy.pinsSnapshot().trust!=ConfigurationTrust::VERIFIED && phy.pinsSnapshot().observedStale);
  }
  for(bool wakeOwner:{false,true}) {
    Controller source;Driver phy;ready(phy,source);OperationToken operation;
    if(wakeOwner) {
      WakeSettings wakeSettings;wakeSettings.enabled=true;wakeSettings.route=WakeRoute::LED2;
      wakeSettings.pinAvailable=wakeSettings.maintenanceAdmitted=wakeSettings.receiveTrafficQuiesced=true;
      CHECK(phy.startConfigureWake(wakeSettings,budget(),operation).inProgress());
    } else {
      InterruptSettings irqSettings;irqSettings.pin=InterruptPin::LED2;
      irqSettings.maintenanceAdmitted=irqSettings.pinAvailable=true;
      CHECK(phy.startConfigureInterrupts(irqSettings,budget(),operation).inProgress());
    }
    CHECK(drain(phy,source,operation).ok());take(phy,operation);
    auto shared=settings(5);shared.pins[2].function=wakeOwner?PinFunction::WAKE:PinFunction::INTERRUPT;
    configured(phy,source,shared);const auto beforeFailure=phy.pinsSnapshot();CHECK(beforeFailure.timeValid);
    if(wakeOwner) {
      source.ext[0x305]^=1;uint16_t raw=0;CHECK(phy.readMmd(31,0x305,raw,{1000000,4}).ok());
      CHECK(phy.wakeSnapshot().trust==ConfigurationTrust::DIRTY);
    } else {
      // Identity 2, mux plus RCSR 5, test admission 1, then disable RMW's write.
      source.failFrame=static_cast<int>(source.frames+9U);
      CHECK(phy.startConfigureInterrupts(phy.interruptSnapshot().desired,budget(),operation).inProgress());
      CHECK(drain(phy,source,operation).is(Err::MDIO_TIMEOUT));const auto failed=take(phy,operation);
      CHECK(failed.mutationPossible && failed.cleanupOwed && phy.interruptSnapshot().trust==ConfigurationTrust::DIRTY);
      source.failFrame=-1;
    }
    const auto callbacks=source.calls,timestamps=source.clocks;
    const auto firstView=phy.pinsSnapshot(),secondView=phy.pinsSnapshot();
    CHECK(firstView.trust==ConfigurationTrust::DIRTY && secondView.trust==ConfigurationTrust::DIRTY && firstView.observedStale);
    CHECK(firstView.observedAtUs==beforeFailure.observedAtUs && firstView.timeValid==beforeFailure.timeValid);
    CHECK(source.calls==callbacks && source.clocks==timestamps);
    CHECK(phy.startInitialize({ResetKind::NONE},budget(),operation).inProgress());
    CHECK(drain(phy,source,operation).ok());const auto restored=take(phy,operation);
    CHECK(restored.persistentRestored && !restored.cleanupOwed && restored.pins.trust==ConfigurationTrust::VERIFIED);
    CHECK(phy.pinsSnapshot().trust==ConfigurationTrust::VERIFIED && !phy.pinsSnapshot().observedStale);
    CHECK(restored.pins.observedAtUs==phy.pinsSnapshot().observedAtUs);
  }
  // The no-wait direct call leaves the job slot free; a later cancellation
  // cannot undo its accepted pin writes or claim to restore physical state.
  CHECK(d.cancel(OperationToken{999}).is(Err::TOKEN_MISMATCH) || d.operationSnapshot().state==OperationState::IDLE);
}
void timing() {
  Controller reference;reference.clockOffStrap=true;Driver ref;ready(ref,reference);const auto first=reference.frames,callFirst=reference.calls;
  configured(ref,reference,settings());const auto frames=reference.frames-first,calls=reference.calls-callFirst;
  CHECK(frames==61U && calls==16U);
  for(uint32_t allowance:{0U,frames-1U,frames}) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);const auto start=b.frames;PinResult out;out.attemptedFrames=999;
    const auto status=d.configurePins(settings(),out,{1000000,allowance});
    if(allowance==frames)CHECK(status.ok() && out.attemptedFrames==frames);else CHECK(status.is(Err::OPERATION_TIMEOUT) && b.frames==start && out.attemptedFrames==999);
  }
  for(unsigned defect=0;defect<5;++defect) {
    Controller b;b.clockOffStrap=true;Driver d;ready(d,b);const auto call=static_cast<int>(b.calls+1U);
    if(defect==0)b.lateCall=call;
    if(defect==1)b.regressCall=call;
    if(defect==2)b.invalidCall=call;
    if(defect==3)b.badReceiptCall=call;
    if(defect==4)b.now=std::numeric_limits<uint64_t>::max();
    PinResult out;
    CHECK(!d.configurePins(settings(),out,{defect==4?std::numeric_limits<uint64_t>::max():1000000,2048}).ok());CHECK(!b.locked);
  }
  Controller frozen;frozen.clockOffStrap=true;frozen.frameTime=0;Driver d;ready(d,frozen);const auto start=frozen.calls;
  configured(d,frozen,settings());CHECK(frozen.calls==start+calls);
  OperationToken token;CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(poll(d,frozen,token,0).inProgress());
  const auto before=frozen.frames;CHECK(d.cancel(token).ok());CHECK(frozen.frames==before);take(d,token);
}
void collector() {
  Controller b;b.clockOffStrap=true;Driver d;ready(d,b);OperationToken token;LinkSettings link;link.advertisement.technologyMask=8;
  CHECK(d.startConfigure(link,{},budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
  auto s=settings(1);s.pins[0].source=LedSource::LINK_LOST;configured(d,b,s);
  b.ledLoss=true;const auto before=b.frames;const auto cached=d.pinsSnapshot();CHECK(cached.desiredValid && b.ledLoss && b.frames==before);
  EvidenceSnapshot e;CHECK(d.acquireEvidence(evidenceBit(EvidenceSource::BMSR),e,{1000000,1}).ok());CHECK(!b.ledLoss);
  s.pins[0].source=LedSource::PRBS_ERROR;configured(d,b,s);
  BistOptions bist;bist.loopback.maintenanceAdmitted=true;bist.observationUs=10;
  CHECK(d.startBist(bist,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
  b.prbsError=true;b.words[27]=0x037D;b.now+=100;
  const auto mutations=b.mutations;CHECK(d.startBistAcquire({},budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
  CHECK(!b.prbsError && b.mutations>mutations);
  // Changing LED settings never consumes MISR or RXFS evidence itself.
  Controller plain;plain.clockOffStrap=true;Driver other;ready(other,plain);plain.words[18]=0xA5FF;plain.words[19]=0x5AFF;plain.ext[0x4A1]=0x10E1;
  configured(other,plain,settings());CHECK(plain.words[18]==0xA5FF && plain.words[19]==0x5AFF && plain.ext[0x4A1]==0x10E1);
  Controller example;Driver application;ready(application,example);PinResult result;
  CHECK(applyLinkLed(application,result,{1000000,128}).ok());
  CHECK((example.ext[0x25]&0x0079U)==0x0041 && (example.words[24]&0x0600U)==0x0400);
  CHECK(tryLed2(application,result,{1000000,128}).ok());
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"vectors",vectors},{"admission",admission},{"routing",routing},{"failures",failures},
    {"replay",replay},{"ownership",ownership},{"timing",timing},{"collector",collector}};
  return test::runSuites(argc,argv,suites);
}
