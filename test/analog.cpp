#include "support/Check.h"
#include "support/PartSelection.h"
#include <DP8382x/DP8382x.h>

#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target, devad; uint64_t at; };

// Independent SNLS647I tables 8-73/79..83/86 original PDF literals.
// The fake models consumption, not physical latency, thresholds or SNR.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<uint16_t,64> mmd3{},mmd7{};
  std::array<Wire,8192> trace{};
  uint16_t selector{0},address{0},id2{0xA131};
  MacInterface boardInterface{MacInterface::MII};
  uint64_t now{100};
  unsigned frames{0},calls{0},clocks{0},releases{0},mutations{0},hard{0};
  std::array<unsigned,2048> firstFrame{};
  uint16_t capability{2}; bool corruptAdvertisement{false};
  int failFrame{-1},releaseCall{-1},corruptFrame{-1},lateCall{-1},regressCall{-1},invalidCall{-1},badReceiptCall{-1};
  uint16_t corruptMask{1};
  bool acceptedFailure{true},locked{false},powerPin{false},stickyRestart{false};
  int poisonBmcrFrame{-1},changeBmcrFrame{-1}; uint16_t bmcrChange{0};
  uint64_t frameTime{2};
  Driver* competitor{nullptr}; unsigned excluded{0};
  Controller() { defaults(); }
  void defaults() {
    words.fill(0);ext.fill(0);mmd3.fill(0);mmd7.fill(0);words[0]=0x3100;words[4]=0x01E1;
    words[1]=0x0024;words[5]=0x0101;words[6]=1;words[16]=0x15;
    words[25]=0x8001;words[22]=0x0100;words[30]=0x0102;words[23]=boardInterface==MacInterface::MII?0x0041:0x0061;
    ext[0x0303]=ext[0x0304]=ext[0x0305]=ext[0x0306]=8;
    ext[0x30B]=0x3C00;ext[0x30C]=0x0410;ext[0x461]=0x10;ext[0x117]=0x8147;ext[0x131]=0x2284;ext[0x2A]=0x7998;ext[0x030E]=0x8400;ext[0x04A1]=0x1000;ext[0x04D1]=0x0080;
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=test::declaredVariant;c.interface=boardInterface;c.phyAddress=phy;
    c.transport={this,execute,clock,{8,4,1}};return c;
  }
  static uint64_t clock(void* context) noexcept { auto& b=*static_cast<Controller*>(context);++b.clocks;return b.now; }
  uint16_t read(uint16_t target,uint16_t devad=31) {
    if(devad==3) {
      CHECK(target<64U);if(target>=64U)return 0xFFFF;
      if(target==0x14)return capability;
      const auto value=mmd3[target];if(target==1)mmd3[target]&=0xF3FF;if(target==0x16)mmd3[target]=0;
      return value;
    }
    if(devad==7) { CHECK(target<64U);return target<64U?mmd7[target]:uint16_t{0xFFFF}; }
    if(target==2)return 0x2000;
    if(target==3)return id2;
    if(target==0) { words[0]&=stickyRestart?0x7FFFU:0x7DFFU;return static_cast<uint16_t>(words[0]|(powerPin?0x0800U:0U)); }
    if(target==31)words[31]=0;
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
    if(reg==13) { CHECK((value&0xBFE0U)==0U && ((value&31U)==3U || (value&31U)==7U || (value&31U)==31U));selector=value;return; }
    if(reg==14 && (selector&0x4000U)==0U) { address=value;return; }
    const uint16_t target=reg==14?address:uint16_t{reg};++mutations;
    const uint16_t devad=reg==14?static_cast<uint16_t>(selector&31U):uint16_t{31};
    if(devad==3 || devad==7) {
      CHECK(target<64U);if(target>=64U)return;
      if(devad==3)mmd3[target]=value;else mmd7[target]=static_cast<uint16_t>(value^(corruptAdvertisement?1U:0U));
      return;
    }
    if(target==31 && value==0x8000) { ++hard;defaults();return; }
    if(target==0 && value==0x8000) { words[0]=0x3100;words[9]=words[10]=words[17]=0;words[23]=boardInterface==MacInterface::MII?0x0041:0x0061;return; }
    if(target==26)CHECK((value&0x0010U)==0U);
    if(target==23)CHECK((value&0x000CU)==0U);
    if(target==18 || target==19) { CHECK((value&0xFF00U)==0U);words[target]=static_cast<uint16_t>((words[target]&0xFF00U)|value);return; }
    if(target==0x04A1) {
      CHECK(value==0x1000 || value==0);
      ext[target]=static_cast<uint16_t>((ext[target]&0x00FFU)|value);return;
    }
    if(target==0x30B || target==0x30C) {
      const auto vod1=target==0x30B?value:ext[0x30B],vod2=target==0x30C?value:ext[0x30C];
      const unsigned mdix=((vod1&0x3000U)>>8U)|(vod2>>12U);
      CHECK(mdix>=0x28U && mdix<=0x38U); // Also checks accepted-error and restoration prefixes.
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
      if(static_cast<int>(frame)==b.changeBmcrFrame)b.words[0]^=b.bmcrChange;
      f.attempted=true;const bool failed=static_cast<int>(frame)==b.failFrame;
      if(!failed || b.acceptedFailure) {
        if(q.kind==FrameKind::WRITE)b.write(q.registerAddress,value);
        else { f.value=b.read(target,devad);if(static_cast<int>(frame)==b.corruptFrame)f.value^=b.corruptMask;
          if(static_cast<int>(frame)==b.poisonBmcrFrame && target==0)f.value=0xB700; }
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
  CHECK(d.bind(b.config()).ok());OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());
  CHECK(take(d,t).identity.declaredVariant == test::declaredVariant);
}
void ready(Driver& d,Controller& b) {
  initialized(d,b);LinkSettings profile;profile.advertisement.technologyMask=8;
  OperationToken t;CHECK(d.startConfigure(profile,{},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}

AnalogSettings settings(bool all=true) {
  AnalogSettings s;s.maintenanceAdmitted=s.macStopped=s.fixtureAdmitted=true;s.amplitudeOwned=true;
  s.minusMdi=0x28;s.minusMdix=0x2F;s.plusMdi=0x18;s.plusMdix=8;s.zeroCode=63;
  if(all) { s.terminationOwned=s.terminationOverrideAdmitted=s.macSlewOwned=s.ioSlewOwned=true;
    s.mdiTermination=15;s.mdixTermination=0;s.mdiTerminationEnable=false;s.macSlewCode=0x11;s.ioSlewCode=1; }
  return s;
}
OperationToken start(Driver& d,Controller& b,const AnalogSettings& s) {
  OperationToken t;const auto frames=b.frames,clocks=b.clocks;const auto status=d.startConfigureAnalog(s,budget(),t);
  if(!status.inProgress())std::fprintf(stderr,"analog start %d/%d: %s\n",static_cast<int>(status.code),status.detail,status.msg);
  CHECK(status.inProgress() && b.frames==frames && b.clocks==clocks);return t;
}
OperationResult configured(Driver& d,Controller& b,const AnalogSettings& s) {
  const auto t=start(d,b,s);const auto status=drain(d,b,t);
  if(!status.ok())std::fprintf(stderr,"analog finish %d/%d: %s\n",static_cast<int>(status.code),status.detail,status.msg);
  CHECK(status.ok());const auto r=take(d,t);
  CHECK(r.status.ok() && r.persistentRestored && !r.cleanupOwed);
  const auto a=d.analogSnapshot();CHECK(a.desiredValid && a.trust==ConfigurationTrust::VERIFIED && !a.observedStale && a.timeValid);
  return r;
}
bool semantic(const Wire& w) { return w.kind==FrameKind::WRITE && w.reg!=13 && !(w.reg==14 && w.target==14); }
unsigned writes(const Controller& b,unsigned first,uint16_t target) {
  unsigned n=0;for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==target)++n;return n;
}
void vectors() {
  // Every legal old/new MDIX-minus pair, checking every actual write prefix.
  for(unsigned old=0x28;old<=0x38;++old)for(unsigned target=0x28;target<=0x38;++target) {
    Controller b;Driver d;ready(d,b);auto s=settings(false);s.minusMdix=static_cast<uint8_t>(target);
    b.ext[0x30B]=static_cast<uint16_t>(0xCC00U|((old/16U)*0x1000U));
    b.ext[0x30C]=static_cast<uint16_t>(0x0410U|((old%16U)*0x1000U));
    auto word1=b.ext[0x30B],word2=b.ext[0x30C];const auto first=b.frames;configured(d,b,s);
    unsigned dependent=0;
    for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i])) {
      const auto& w=b.trace[i];CHECK(w.target==0x30B || w.target==0x30C);
      if(w.target==0x30B)word1=w.value;else word2=w.value;
      const unsigned current=((word1&0x3000U)/0x1000U)*16U+(word2/0x1000U);
      CHECK(current>=0x28U && current<=0x38U);++dependent;
      CHECK(i+4U<b.frames && b.trace[i+4U].kind==FrameKind::READ && b.trace[i+4U].target==w.target);
      if(dependent==1U && old/16U!=target/16U)CHECK(w.target==0x30C && w.value==0x8410U);
    }
    CHECK(dependent==(old/16U==target/16U?2U:3U));
    CHECK(b.ext[0x30B]==static_cast<uint16_t>(0xCA3FU|((target/16U)*0x1000U)));
    CHECK(b.ext[0x30C]==static_cast<uint16_t>(0x0218U|((target%16U)*0x1000U)));
    CHECK(b.frames-first==(dependent==2U?26U:34U));CHECK(writes(b,first,0x30E)==0U);
  }
  for(uint8_t minus:{uint8_t{0x28},uint8_t{0x38}})for(uint8_t plus:{uint8_t{8},uint8_t{0x18}}) {
    Controller b;Driver d;ready(d,b);auto s=settings(false);s.minusMdi=s.minusMdix=minus;s.plusMdi=s.plusMdix=plus;
    configured(d,b,s);CHECK(((b.ext[0x30B]>>6U)&63U)==minus && (b.ext[0x30B]&63U)==63U);
    CHECK(((b.ext[0x30C]>>6U)&63U)==plus && (b.ext[0x30C]&63U)==plus);
  }
  for(unsigned code=0;code<16U;++code)for(unsigned enable=0;enable<4U;++enable) {
    Controller b;Driver d;ready(d,b);AnalogSettings s;s.maintenanceAdmitted=s.macStopped=s.fixtureAdmitted=true;
    s.terminationOwned=s.terminationOverrideAdmitted=true;s.mdiTermination=static_cast<uint8_t>(code);s.mdixTermination=static_cast<uint8_t>(15U-code);
    s.mdiTerminationEnable=(enable&1U)!=0U;s.mdixTerminationEnable=(enable&2U)!=0U;b.ext[0x30E]|=0x3F;
    configured(d,b,s);CHECK(b.ext[0x30E]==static_cast<uint16_t>((code*0x1000U)+((15U-code)*0x80U)+
        ((enable&1U)!=0U?0x800U:0U)+((enable&2U)!=0U?0x40U:0U)+0x3FU));
  }
  for(unsigned slow=0;slow<2U;++slow)for(unsigned fast=0;fast<2U;++fast) {
    Controller b;Driver d;ready(d,b);AnalogSettings s;s.maintenanceAdmitted=s.macStopped=s.fixtureAdmitted=true;
    s.macSlewOwned=s.ioSlewOwned=true;s.macSlewCode=static_cast<uint16_t>(0x10U+slow);s.ioSlewCode=static_cast<uint8_t>(fast);
    b.ext[0x461]=0xFFE0;b.ext[0x302]=0x05A5;configured(d,b,s);
    CHECK(b.ext[0x461]==static_cast<uint16_t>(0xFFF0U+slow));CHECK(b.ext[0x302]==static_cast<uint16_t>(0x05A5U+fast*0x4000U));
  }
}
void admission() {
  for(unsigned fault=0;fault<22U;++fault) {
    Controller b;Driver d;if(fault!=0U)ready(d,b);auto s=settings();OperationToken token{777};auto allowance=budget();
    switch(fault) {
      case 1:s.maintenanceAdmitted=false;break;case 2:s.macStopped=false;break;case 3:s.fixtureAdmitted=false;break;
      case 4:s.terminationOverrideAdmitted=false;break;case 5:s.minusMdi=0x27;break;case 6:s.minusMdix=0x39;break;
      case 7:s.plusMdi=7;break;case 8:s.plusMdix=0x19;break;case 9:s.zeroCode=64;break;
      case 10:s.mdiTermination=16;break;case 11:s.mdixTermination=16;break;case 12:s.macSlewCode=0x0400;break;
      case 13:s.macSlewCode=0;break;case 14:s.ioSlewCode=2;break;case 15:s.receiverGainOwned=true;break;
      case 16:allowance.io.maxFrames=0;break;case 17:allowance.io.deadlineUs=0;break;
      case 18:allowance.pollIntervalUs=0;break;case 19:allowance.maxStalledPolls=0;break;
      case 20:s.amplitudeOwned=false;break;case 21:s.receiverGainCode=32;break;default:break;
    }
    const auto frames=b.frames,clocks=b.clocks;CHECK(!d.startConfigureAnalog(s,allowance,token).inProgress());
    CHECK(token.value==777U && b.frames==frames && b.clocks==clocks && !d.analogSnapshot().desiredValid);
  }
  Controller b;Driver d;ready(d,b);configured(d,b,settings());auto s=settings(false);const auto first=b.frames;OperationToken token;
  CHECK(!d.startConfigureAnalog(s,budget(),token).inProgress() && b.frames==first);
  for(uint16_t invalid:{uint16_t{0x2000},uint16_t{0x3F00}}) {
    Controller bad;Driver phy;ready(phy,bad);bad.ext[0x30B]=static_cast<uint16_t>(invalid&0x3000U);bad.ext[0x30C]=static_cast<uint16_t>((invalid&0x0F00U)<<4U);
    const auto mutations=bad.mutations;const auto t=start(phy,bad,settings(false));CHECK(!drain(phy,bad,t).ok());
    const auto r=take(phy,t);CHECK(!r.mutationPossible && r.cleanupOwed && mutations==bad.mutations);
    CHECK(phy.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(phy,bad,token).ok());CHECK(take(phy,token).persistentRestored);
  }
  for(uint16_t state:{uint16_t{0x0800},uint16_t{0x0400},uint16_t{0x4000},uint16_t{0x0080}}) {
    Controller live;Driver phy;ready(phy,live);live.words[0]|=state;const auto mutations=live.mutations;
    const auto t=start(phy,live,settings(false));CHECK(!drain(phy,live,t).ok());CHECK(!take(phy,t).mutationPossible && live.mutations==mutations);
  }
  // Valid reset evidence invalidates previous identity/configuration even if
  // that same transaction reports a later ownership-release failure.
  for(bool releaseFailure:{false,true}) {
    Controller reset;Driver phy;ready(phy,reset);reset.corruptFrame=static_cast<int>(reset.frames);reset.corruptMask=0x8000;
    if(releaseFailure)reset.releaseCall=static_cast<int>(reset.calls);
    const auto mutations=reset.mutations;const auto t=start(phy,reset,settings(false));const auto status=drain(phy,reset,t);
    CHECK(status.is(releaseFailure?Err::MDIO_ERROR:Err::INVALID_CONFIG));
    CHECK(!phy.isInitialized() && !phy.capabilities().valid && reset.mutations==mutations);
    CHECK(!take(phy,t).persistentRestored);
  }
  Controller drift;Driver driftPhy;ready(driftPhy,drift);drift.words[0]^=0x1000;const auto mutations=drift.mutations;
  const auto driftToken=start(driftPhy,drift,settings(false));CHECK(drain(driftPhy,drift,driftToken).is(Err::CONFIG_MISMATCH));
  CHECK(driftPhy.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED && drift.mutations==mutations);
  CHECK(take(driftPhy,driftToken).mismatch.valid);
}
void failures() {
  Controller ref;Driver reference;ready(reference,ref);const auto first=ref.frames,firstCall=ref.calls;configured(reference,ref,settings());
  const auto count=ref.frames-first,callbacks=ref.calls-firstCall;CHECK(count==70U && callbacks==18U);
  for(unsigned phase=0;phase<count;++phase)for(bool accepted:{false,true}) {
    Controller b;Driver d;ready(d,b);const auto startFrame=b.frames;b.failFrame=static_cast<int>(startFrame+phase);b.acceptedFailure=accepted;
    const auto t=start(d,b,settings());CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));const auto r=take(d,t);
    CHECK(r.status.detail==-33033 && b.frames==startFrame+phase+1U && !r.persistentRestored);
    CHECK(b.calls==b.releases && !b.locked && d.analogSnapshot().trust!=ConfigurationTrust::VERIFIED);
    bool effect=false;for(unsigned i=0;i<phase;++i)effect|=semantic(ref.trace[first+i]);effect|=accepted && semantic(ref.trace[first+phase]);
    if(effect)CHECK(r.mutationPossible && r.cleanupOwed && !d.end().ok());
    if(phase>=2U && phase<10U)CHECK(r.cleanupOwed && !r.mutationPossible); // Current split code not established.
  }
  for(unsigned call=0;call<callbacks;++call) {
    Controller b;Driver d;ready(d,b);b.releaseCall=static_cast<int>(b.calls+call);const auto t=start(d,b,settings());
    CHECK(drain(d,b,t).is(Err::MDIO_ERROR));const auto r=take(d,t);CHECK(r.failingBatch.release.detail==-37 && !r.persistentRestored && !b.locked);
  }
  for(unsigned i=first;i<ref.frames;++i)if(semantic(ref.trace[i])) {
    Controller b;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+i-first+4U);
    b.corruptMask=ref.trace[i].target==0x302?uint16_t{0x4000}:ref.trace[i].target==0x30E?uint16_t{0x40}:uint16_t{1};
    const auto t=start(d,b,settings());CHECK(drain(d,b,t).is(Err::CONFIG_MISMATCH));const auto r=take(d,t);
    CHECK(r.mismatch.valid && r.mismatch.registerAddress==ref.trace[i].target && r.cleanupOwed && !r.persistentRestored);
    CHECK(b.frames==static_cast<unsigned>(b.corruptFrame)+1U);
  }
  for(unsigned completed=0;completed<callbacks;++completed) {
    Controller b;Driver d;ready(d,b);const auto t=start(d,b,settings());
    for(unsigned n=0;n<completed;++n)CHECK(poll(d,b,t).inProgress());
    const auto frames=b.frames,clocks=b.clocks;CHECK(d.cancel(t).ok() && b.frames==frames && b.clocks==clocks);
    const auto r=take(d,t);CHECK(!r.status.ok() && !r.persistentRestored && r.cleanupOwed==(completed>=7U));
    OperationResult sentinel;sentinel.token.value=777;CHECK(!d.takeResult(t,sentinel).ok() && sentinel.token.value==777U);
  }
}
// Each later owner must take over the shared slot after the analog pass ends.
void downstreamOwners(Driver& d,Controller& b,unsigned mask) {
  OperationToken token;
  if ((mask&1U)!=0U) {
    PinSettings pins;pins.maintenanceAdmitted=pins.macStopped=pins.outputsQuiesced=true;
    pins.availableMask=1;pins.pins[0].function=PinFunction::LED;pins.pins[0].source=LedSource::LINK_ACTIVITY;
    PinResult result;CHECK(d.configurePins(pins,result,{1000000,1024}).ok());
  }
  if ((mask&2U)!=0U) {
    WakeSettings wake;wake.enabled=true;wake.maintenanceAdmitted=wake.receiveTrafficQuiesced=true;
    wake.mac[0]=0x11;wake.mac[5]=0x66;
    CHECK(d.startConfigureWake(wake,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
  }
  if ((mask&4U)!=0U) {
    InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=irq.globalEnable=true;
    irq.misr1Mask=1;irq.misr2Mask=2;
    CHECK(d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
  }
  if ((mask&8U)!=0U) {
    PowerSettings power;power.maintenanceAdmitted=power.managementClockAvailable=true;
    CHECK(d.startConfigurePower(power,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
  }
}
void downstreamReplay() {
  for (unsigned mask:{1U,2U,4U,8U,15U}) {
    Controller b;Driver d;ready(d,b);downstreamOwners(d,b,mask);configured(d,b,settings());
    OperationToken token;CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    CHECK(drain(d,b,token).ok());const auto restored=take(d,token);
    CHECK(restored.persistentRestored && !restored.cleanupOwed && d.isInitialized());
    CHECK(d.analogSnapshot().trust==ConfigurationTrust::VERIFIED);
    if ((mask&1U)!=0U)CHECK(d.pinsSnapshot().trust==ConfigurationTrust::VERIFIED);
    if ((mask&2U)!=0U)CHECK(d.wakeSnapshot().trust==ConfigurationTrust::VERIFIED);
    if ((mask&4U)!=0U)CHECK(d.interruptSnapshot().trust==ConfigurationTrust::VERIFIED);
    if ((mask&8U)!=0U)CHECK(d.powerSnapshot().trust==ConfigurationTrust::VERIFIED);
    const auto mutations=b.mutations;
    CHECK(d.startVerify(budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(b.mutations==mutations && d.isInitialized());
  }
  Controller oracle;Driver reference;ready(reference,oracle);downstreamOwners(reference,oracle,15);configured(reference,oracle,settings());
  OperationToken token;const auto startFrames=oracle.frames,startCalls=oracle.calls;
  CHECK(reference.startInitialize({ResetKind::HARD},budget(),token).inProgress());
  bool invalidated=false;unsigned handoffFrames=0,handoffCalls=0;
  for (unsigned n=0;n<512U;++n) {
    const auto status=poll(reference,oracle,token);
    if (reference.analogSnapshot().trust!=ConfigurationTrust::VERIFIED)invalidated=true;
    if (invalidated && handoffFrames==0U && reference.analogSnapshot().trust==ConfigurationTrust::VERIFIED) {
      CHECK(status.inProgress());handoffFrames=oracle.frames-startFrames;handoffCalls=oracle.calls-startCalls;
    }
    if (!status.inProgress()) { CHECK(status.ok());break; }
  }
  CHECK(handoffFrames!=0U);take(reference,token);
  // Every downstream frame failure keeps the whole restoration incomplete,
  // even though analog's independently verified words remain valid evidence.
  for (unsigned frame=handoffFrames;frame<oracle.frames-startFrames;++frame) {
    Controller b;Driver d;ready(d,b);downstreamOwners(d,b,15);configured(d,b,settings());
    b.failFrame=static_cast<int>(b.frames+frame);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(d,b,token).is(Err::MDIO_TIMEOUT));
    const auto failed=take(d,token);
    CHECK(!failed.persistentRestored && failed.cleanupOwed && !d.isInitialized());
    CHECK(b.calls==b.releases && !b.locked);
  }
  for (unsigned boundary:{handoffCalls-1U,handoffCalls,handoffCalls+1U}) {
    Controller b;Driver d;ready(d,b);downstreamOwners(d,b,15);configured(d,b,settings());
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    for (unsigned n=0;n<boundary;++n)CHECK(poll(d,b,token).inProgress());
    const auto frames=b.frames,clocks=b.clocks;CHECK(d.cancel(token).ok());
    CHECK(b.frames==frames && b.clocks==clocks);
    const auto cancelled=take(d,token);CHECK(cancelled.status.is(Err::CANCELLED) && !cancelled.persistentRestored && cancelled.cleanupOwed && !d.isInitialized());
  }
}
void replay() {
  downstreamReplay();
  for(auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
    Controller b;Driver d;ready(d,b);configured(d,b,settings());OperationToken t;const auto first=b.frames;
    CHECK(d.startInitialize({reset},budget(),t).inProgress());CHECK(drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(r.persistentRestored && r.analog.trust==ConfigurationTrust::VERIFIED && !r.cleanupOwed && d.isInitialized());
    CHECK((b.ext[0x30B]&0x3FFFU)==0x2A3FU && b.ext[0x30C]==0xF218 && (b.ext[0x30E]&0xFFC0U)==0xF040);
    bool mandatory=false;for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==0x30E) {
      if(!mandatory) { CHECK(b.trace[i].value==0x4A40); } mandatory=true;
    }
    CHECK(mandatory);const auto mutations=b.mutations;CHECK(d.startVerify(budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);CHECK(b.mutations==mutations);
  }
  Controller ref;Driver reference;ready(reference,ref);configured(reference,ref,settings());OperationToken t;
  const auto first=ref.frames;CHECK(reference.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(drain(reference,ref,t).ok());take(reference,t);
  for(unsigned phase=0;phase<ref.frames-first;++phase) {
    Controller b;Driver d;ready(d,b);configured(d,b,settings());b.failFrame=static_cast<int>(b.frames+phase);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(!drain(d,b,t).ok());CHECK(!take(d,t).persistentRestored && !b.locked);
    b.failFrame=-1;CHECK(d.startInitialize({ResetKind::HARD},budget(),t).inProgress());CHECK(drain(d,b,t).ok());CHECK(take(d,t).persistentRestored);
  }
  Controller oracle;Driver oraclePhy;ready(oraclePhy,oracle);configured(oraclePhy,oracle,settings());const auto verifyFirst=oracle.frames;
  CHECK(oraclePhy.startVerify(budget(),t).inProgress());CHECK(drain(oraclePhy,oracle,t).ok());take(oraclePhy,t);
  unsigned mismatch=verifyFirst;for(unsigned i=verifyFirst;i<oracle.frames;++i)if(oracle.trace[i].target==0x30B && oracle.trace[i].kind==FrameKind::READ) { mismatch=i;break; }
  Controller b;Driver d;ready(d,b);configured(d,b,settings());b.corruptFrame=static_cast<int>(b.frames+mismatch-verifyFirst);b.failFrame=b.corruptFrame+1;
  const auto mutations=b.mutations;CHECK(d.startVerify(budget(),t).inProgress());CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));const auto r=take(d,t);
  CHECK(r.mismatch.valid && r.mismatch.registerAddress==0x30B && r.status.detail==-33033 && b.mutations==mutations);
  // Changing a link profile verifies explicit termination instead of adding
  // another mandatory initialization write or silently undoing analog intent.
  Controller link;Driver linkPhy;ready(linkPhy,link);configured(linkPhy,link,settings());const auto linkFirst=link.frames;
  LinkSettings profile;profile.advertisement.technologyMask=1;
  CHECK(linkPhy.startConfigure(profile,{},budget(),t).inProgress());CHECK(drain(linkPhy,link,t).ok());take(linkPhy,t);
  CHECK(writes(link,linkFirst,0x30E)==0U && linkPhy.analogSnapshot().trust==ConfigurationTrust::VERIFIED);
  Controller loop;Driver loopPhy;ready(loopPhy,loop);configured(loopPhy,loop,settings());
  LoopbackOptions options;options.mode=LoopbackMode::REVERSE;options.speed=LinkSpeed::MBPS_100;
  options.maintenanceAdmitted=options.partnerReady=true;options.reverseFifoDepth=1;
  CHECK(loopPhy.startLoopback(options,budget(),t).inProgress());CHECK(drain(loopPhy,loop,t).ok());take(loopPhy,t);
  CHECK(loopPhy.stopLoopback(budget(),t).inProgress());CHECK(drain(loopPhy,loop,t).ok());take(loopPhy,t);
  CHECK(loopPhy.analogSnapshot().trust==ConfigurationTrust::VERIFIED && (loop.ext[0x30E]&0xFFC0U)==0xF040U);
}
void ownership() {
  Controller b;Driver d,other;ready(d,b);CHECK(other.bind(b.config(2)).ok());b.competitor=&other;
  const auto first=b.frames;configured(d,b,settings());CHECK(b.excluded==b.frames-first);b.competitor=nullptr;CHECK(!other.analogSnapshot().desiredValid);
  const auto frames=b.frames,clocks=b.clocks;const auto a=d.analogSnapshot(),a2=d.analogSnapshot();
  CHECK(a.raw==a2.raw && b.frames==frames && b.clocks==clocks);
  uint16_t sentinel=0xCAFE;CHECK(!d.writeMmd(31,0x30B,0,{1000000,4}).ok());CHECK(!d.writeMmd(31,0x461,0x0400,{1000000,4}).ok());
  CHECK(d.readMmd(31,0x30B,sentinel,{1000000,4}).ok());CHECK(d.analogSnapshot().trust==ConfigurationTrust::VERIFIED);
  b.ext[0x30B]^=1;CHECK(d.readMmd(31,0x30B,sentinel,{1000000,4}).ok());CHECK(d.analogSnapshot().trust!=ConfigurationTrust::VERIFIED);
  CHECK(d.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());CHECK(d.analogSnapshot().desiredValid && d.analogSnapshot().observedStale);
  for(unsigned boundary=0;boundary<18U;++boundary) {
    Controller reset;Driver phy;ready(phy,reset);const auto t=start(phy,reset,settings());
    for(unsigned n=0;n<boundary;++n)CHECK(poll(phy,reset,t).inProgress());
    const auto before=reset.frames,beforeClocks=reset.clocks;
    CHECK(phy.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
    CHECK(reset.frames==before && reset.clocks==beforeClocks && !phy.isInitialized());
    CHECK(!phy.pollOperation(t,16).inProgress() && reset.frames==before);
    const auto result=take(phy,t);CHECK(!result.persistentRestored && phy.analogSnapshot().observedStale);
  }
  // Independent reset evidence interrupts retained composition even when the
  // owned BMCR mode bits still match. The raw read itself remains publishable.
  for (unsigned boundary:{3U,7U,9U})for (unsigned resetKind=0;resetKind<3U;++resetKind) {
    Controller reset;Driver phy;ready(phy,reset);const auto t=start(phy,reset,settings());
    for (unsigned n=0;n<boundary;++n)CHECK(poll(phy,reset,t).inProgress());
    const auto mutations=reset.mutations,frames=reset.frames;
    const uint16_t address=resetKind==0U?uint16_t{0}:uint16_t{0x1F};
    reset.corruptFrame=static_cast<int>(frames);reset.corruptMask=resetKind==2U?0x4000:0x8000;
    uint16_t raw=0xCAFE;
    CHECK(phy.readClause22(address,raw,{1000000,1}).ok());
    CHECK(raw==(resetKind==0U?0x9000U:resetKind==1U?0x8000U:0x4000U));
    CHECK(phy.operationSnapshot().state==OperationState::RESULT_PENDING);
    CHECK(phy.pollOperation(t,MAX_BATCH_FRAMES).is(Err::INVALID_CONFIG));
    CHECK(reset.frames==frames+1U && reset.mutations==mutations);
    const auto result=take(phy,t);
    CHECK(result.status.detail==static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
    CHECK(!result.persistentRestored && result.cleanupOwed==(boundary>=7U));
    CHECK(result.mutationPossible==(boundary>=7U));
  }
  // Each admitted read compares against snapshot, bridge or completed state,
  // never against an un-applied destination. Drift invalidates saved composition.
  for(unsigned boundary:{3U,6U,7U,8U,9U})for(uint16_t address:{uint16_t{0x30B},uint16_t{0x30C}}) {
    Controller observed;Driver phy;ready(phy,observed);const auto t=start(phy,observed,settings());
    for(unsigned n=0;n<boundary;++n)CHECK(poll(phy,observed,t).inProgress());
    uint16_t raw=0;const auto unchangedMutations=observed.mutations;
    CHECK(phy.readMmd(31,address,raw,{1000000,4}).ok());CHECK(phy.operationSnapshot().state==OperationState::RUNNING);
    observed.ext[address]^=1U;CHECK(phy.readMmd(31,address,raw,{1000000,4}).ok());
    CHECK(phy.operationSnapshot().state==OperationState::RESULT_PENDING && observed.mutations==unchangedMutations);
    const auto result=take(phy,t);CHECK(result.status.is(Err::CONFIG_MISMATCH) && result.mismatch.valid && !result.persistentRestored);
  }
}
void timing() {
  Controller b;Driver d;ready(d,b);const auto t=start(d,b,settings());const auto first=b.frames;
  CHECK(d.pollOperation(t,0).inProgress() && b.frames==first);CHECK(d.pollOperation(t,1).inProgress() && b.frames==first);
  CHECK(poll(d,b,t,2).inProgress() && b.frames==first+2U);const auto second=b.frames;
  CHECK(d.pollOperation(t,3).inProgress() && b.frames==second);CHECK(drain(d,b,t).ok());take(d,t);
  for(unsigned mode=0;mode<4U;++mode) {
    Controller timed;Driver phy;ready(phy,timed);OperationToken token;auto allowance=budget();
    if(mode==0U)allowance.io.maxFrames=25;
    if(mode==1U)allowance.io.deadlineUs=timed.now+20U;
    if(mode==2U)timed.regressCall=static_cast<int>(timed.calls+1U);
    if(mode==3U)timed.lateCall=static_cast<int>(timed.calls+6U);
    CHECK(phy.startConfigureAnalog(settings(),allowance,token).inProgress());CHECK(!drain(phy,timed,token).ok());CHECK(!take(phy,token).persistentRestored && !timed.locked);
  }
  Controller frozen;Driver frozenPhy;ready(frozenPhy,frozen);frozen.frameTime=0;const auto token=start(frozenPhy,frozen,settings());
  Status status;for(unsigned n=0;n<32U;++n) { status=frozenPhy.pollOperation(token,0);if(!status.inProgress())break; }
  CHECK(!status.inProgress());take(frozenPhy,token);
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"vectors",vectors},{"admission",admission},{"failures",failures},{"replay",replay},{"ownership",ownership},{"timing",timing}};
  return test::runVariantSuites(argc,argv,suites);
}
