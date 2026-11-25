#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include "../examples/portable/Interface.h"
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target, devad; uint64_t at; };

// Independent SNLS647I tables 8-19/28/29/35/41/73/84. The numeric
// assertions below come from the original tables; fake RC consumption is not
// proof of RCSR status polarity or physical clock/repeater operation.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<uint16_t,64> mmd3{},mmd7{};
  std::array<Wire,8192> trace{};
  uint16_t selector{0},address{0},id2{0xA131};
  Variant declared{Variant::DP83826I};
  MacInterface boardInterface{MacInterface::RMII};
  RmiiClock boardRole{RmiiClock::UNSPECIFIED};
  bool clockLossApproved{false};
  uint16_t sampledSor{0}, sampledRcsr{0};
  uint64_t now{100};
  unsigned delayedClock{0};
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
    words[25]=0x8001;words[22]=0x0100;words[30]=0x0102;words[23]=0x0061;
    ext[0x0303]=ext[0x0304]=ext[0x0305]=ext[0x0306]=8;
    if (declared == Variant::DP83825I) { ext[0x467] = sampledSor; words[23] = sampledRcsr; }
    ext[0x030E]=0x8400;ext[0x04A1]=0x1000;ext[0x04D1]=0x0080;
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=declared;c.interface=boardInterface;c.phyAddress=phy;
    c.rmiiClock=boardRole;c.clockLossApproved=clockLossApproved;
    c.transport={this,execute,clock,{8,4,1}};return c;
  }
  static uint64_t clock(void* context) noexcept {
    auto& b=*static_cast<Controller*>(context);
    if (++b.clocks==b.delayedClock) b.now+=240;
    return b.now;
  }
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
    if(declared==Variant::DP83825I && target==1)words[16]&=0xFFBF;
    if(target==0x0308 && (id2&15U)!=4U)return static_cast<uint16_t>((ext[0x0302]&0x0040U)!=0U?1U:0U);
    const auto value=target<32U?words[target]:ext[target];
    if(target==18 || target==19) { words[target]&=0x00FF;words[16]&=0xFF7F; }
    if(target==20) { words[20]&=0xFF00;words[16]&=0xF7FF; }
    if(target==21) { words[21]=0;words[16]&=0xDFFF; }
    if(target==23)words[23]&=0xFFF3;
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
    if(target==0 && value==0x8000) { words[0]=0x3100;words[9]=words[10]=words[17]=0;words[23]=declared==Variant::DP83825I?sampledRcsr:0x0061;return; }
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
      if(q.readIndex!=NO_READ_INDEX) {
        CHECK(q.readIndex<i && r.frames[q.readIndex].valueValid);
        const auto sampled=r.frames[q.readIndex].value;
        if(!q.acceptsRead(sampled)) { r.status=Status::Error(Err::CONFIG_MISMATCH,"Dependent write guard");break; }
        value=static_cast<uint16_t>((sampled&q.preserveMask)|q.value);
      }
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
  CHECK(d.bind(b.config()).ok());OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}
void ready(Driver& d,Controller& b) {
  initialized(d,b);LinkSettings profile;profile.advertisement.technologyMask=8;
  OperationToken t;CHECK(d.startConfigure(profile,{},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}
InterfaceSettings settings(MacInterface mode=MacInterface::RMII) {
  InterfaceSettings s;s.interface=mode;s.referenceClockMHz=25;
  s.maintenanceAdmitted=s.macStopped=s.pinsAvailable=true;
  s.referenceClockStable=s.managementClockAvailable=s.clockChangeApproved=true;
  return s;
}
InterfaceResult configured(Driver& d,Controller& b,const InterfaceSettings& s) {
  InterfaceResult out;const auto status=d.configureInterface(s,out,{1000000,2048});
  if(!status.ok())std::fprintf(stderr,"interface status %d detail %d: %s\n",static_cast<int>(status.code),status.detail,status.msg);
  CHECK(status.ok() && out.verified && out.status.ok() && !b.locked);
  const auto view=d.interfaceSnapshot();
  CHECK(view.desiredValid && view.trust==ConfigurationTrust::VERIFIED && !view.observedStale && view.timeValid);
  CHECK(view.modeValid && view.observedInterface==s.interface);
  CHECK(d.operationSnapshot().state==OperationState::IDLE && !d.operationSnapshot().cleanupOwed);
  return out;
}
bool semantic(const Wire& w) { return w.kind==FrameKind::WRITE && w.reg!=13 && !(w.reg==14 && w.target==14); }
unsigned writes(const Controller& b,unsigned start,uint16_t target) {
  unsigned n=0;for(unsigned i=start;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==target)++n;return n;
}
void vectors() {
  for(bool basic:{false,true})for(bool follower:{false,true})for(bool shift:{false,true})
    for(bool revision:{false,true})for(uint8_t code=0;code<4U;++code) {
      if(shift && !follower)continue;
      Controller b;if(basic)b.id2=0xA111;Driver d;ready(d,b);
      auto s=settings();s.rmiiClock=follower?RmiiClock::FOLLOWER_50MHZ:RmiiClock::LEADER_25MHZ;
      s.referenceClockMHz=follower?50:25;s.txClockShift=shift;s.revision10=revision;s.elasticityCode=code;
      b.words[23]=0xFE4C;
      const auto first=b.frames;configured(d,b,s);
      const auto expected=static_cast<uint16_t>(0xFE40U|0x0020U|(follower?0x0080U:0U)|(shift?0x0100U:0U)|(revision?0x0010U:0U)|code);
      CHECK(b.words[23]==expected && writes(b,first,0x17)==1U);
      CHECK((b.words[23]&0x0040U)!=0U && (b.words[23]&0x000CU)==0U);
    }
  for(unsigned bits=0;bits<16;++bits) {
    Controller b;b.boardInterface=MacInterface::MII;Driver d;ready(d,b);auto s=settings(MacInterface::MII);
    s.fastRxDv=(bits&1U)!=0U;s.oddNibbleDisable=(bits&2U)!=0U;
    s.collisionFullDuplex=(bits&4U)!=0U;s.shortIpg=(bits&8U)!=0U;
    b.words[9]=0xFFFD;b.words[10]=0xFFFD;b.words[17]=0x0FEB;b.words[23]=0x01FF;b.ext[0x456]=0xFFF7;
    configured(d,b,s);
    CHECK(b.words[9]==static_cast<uint16_t>(0xFFFDU|(s.fastRxDv?2U:0U)));
    CHECK(b.words[10]==static_cast<uint16_t>(0xFFFDU|(s.oddNibbleDisable?2U:0U)));
    CHECK(b.words[17]==static_cast<uint16_t>(0x0FEBU|(s.collisionFullDuplex?0x10U:0U)));
    CHECK(b.ext[0x456]==static_cast<uint16_t>(0xFFF7U|(s.shortIpg?8U:0U)));
    CHECK(b.words[23]==0x01D3U); // Only mode 20 and RC 0C clear in MII.
    CHECK((b.words[10]&4U)!=0U); // RX_ER-idle is not minimum IPG.
  }
  for(auto output:{ClockOutput::CLKOUT25,ClockOutput::LED1})for(bool isolate:{false,true}) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.clockOutput=output;s.isolationOwned=true;s.isolate=isolate;
    b.ext[0x302]=0xAEBF;b.words[0]=0x3101;const auto first=b.frames;
    const auto result=configured(d,b,s);
    CHECK(b.ext[0x302]==static_cast<uint16_t>((0xAEBFU&0xFEBFU)|(output==ClockOutput::LED1?0x0040U:0U)));
    CHECK((b.words[0]&0x0400U)==(isolate?0x0400U:0U));CHECK((b.words[0]&0x8200U)==0U);
    CHECK(result.attemptedFrames==39U && result.transactions==16U && b.frames-first==39U);
    for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==0)CHECK((b.trace[i].value&0x8200U)==0U);
  }
  for(bool isolate:{false,true}) {
    Controller b;Driver d;ready(d,b);PowerSettings p;p.request=PowerRequest::IEEE_POWER_DOWN;
    p.maintenanceAdmitted=p.managementClockAvailable=p.clockLossApproved=true;OperationToken token;
    CHECK(d.startConfigurePower(p,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    auto s=settings();s.isolationOwned=true;s.isolate=isolate;configured(d,b,s);
    CHECK((b.words[0]&0x0C00U)==(isolate?0x0C00U:0x0800U) && d.powerSnapshot().requestedPowerDown);
    p.request=PowerRequest::NORMAL;CHECK(d.startConfigurePower(p,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK((b.words[0]&0x0C00U)==(isolate?0x0400U:0U));
  }
  // With owned awake intent, the external power-pin OR must not be echoed
  // into a software request while isolation is applied or released.
  for(bool isolate:{false,true}) {
    Controller b;Driver d;ready(d,b);PowerSettings power;
    power.maintenanceAdmitted=power.managementClockAvailable=power.clockLossApproved=true;OperationToken token;
    CHECK(d.startConfigurePower(power,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    b.powerPin=true;auto s=settings();s.isolationOwned=true;s.isolate=isolate;const auto first=b.frames;configured(d,b,s);
    CHECK((b.words[0]&0x0800U)==0U && !d.powerSnapshot().requestedPowerDown);
    // This acquisition publishes its live OR readback in the interface view;
    // it does not perform the separate complete power-state acquisition.
    CHECK((d.interfaceSnapshot().raw[6]&0x0800U)!=0U);
    for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==0)CHECK((b.trace[i].value&0x0800U)==0U);
  }
}
void admission() {
  for(unsigned fault=0;fault<20;++fault) {
    Controller b;Driver d;if(fault!=0)ready(d,b);auto s=settings();
    switch(fault) {
      case 1:s.maintenanceAdmitted=false;break;case 2:s.macStopped=false;break;
      case 3:s.pinsAvailable=false;break;case 4:s.referenceClockStable=false;break;
      case 5:s.managementClockAvailable=false;break;case 6:s.clockChangeApproved=false;break;
      case 7:s.referenceClockMHz=50;break;case 8:s.interface=MacInterface::MII;break;
      case 9:s.interface=MacInterface::RGMII;break;case 10:s.txClockShift=true;break;
      case 11:s.elasticityCode=4;break;case 12:s.clockTolerancePpm=51;break;
      case 13:s.maxFrameBytes=0;break;case 14:s.repeater=true;break;
      case 15:s.collisionFullDuplex=true;break;case 16:s.clockOutput=ClockOutput::PLL;break;
      case 17:s.clockOutput=ClockOutput::RECOVERED;break;
      case 18:s.rmiiClock=static_cast<RmiiClock>(255);break;
      case 19:s.clockOutput=static_cast<ClockOutput>(255);break;
      default:break;
    }
    InterfaceResult out;out.attemptedFrames=999;const auto first=b.frames,clocks=b.clocks;
    CHECK(!d.configureInterface(s,out,{1000000,2048}).ok());
    CHECK(b.frames==first && b.clocks==clocks && out.attemptedFrames==999 && !d.interfaceSnapshot().desiredValid);
  }
  constexpr uint32_t maximum[]={16800,2400,7200,12000};
  for(uint8_t code=0;code<4U;++code)for(uint16_t ppm:{uint16_t{50},uint16_t{100}})for(bool beyond:{false,true}) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.elasticityCode=code;s.clockTolerancePpm=ppm;
    s.maxFrameBytes=maximum[code]/(ppm==50?1U:2U)+(beyond?1U:0U);const auto first=b.frames;InterfaceResult out;
    const auto status=d.configureInterface(s,out,{1000000,2048});
    if(beyond)CHECK(!status.ok() && b.frames==first);else CHECK(status.ok());
  }
  for(unsigned fault=0;fault<4;++fault) {
    Controller b;b.boardInterface=MacInterface::MII;Driver d;ready(d,b);auto s=settings(MacInterface::MII);
    if(fault==0)s.revision10=true;
    if(fault==1)s.elasticityCode=1;
    if(fault==2)s.rxDv=true;
    if(fault==3)s.rmiiClock=RmiiClock::FOLLOWER_50MHZ;
    const auto first=b.frames;InterfaceResult out;CHECK(!d.configureInterface(s,out,{1000000,2048}).ok() && b.frames==first);
  }
  for(bool isolate:{false,true}) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.isolationOwned=true;s.isolate=isolate;s.clockChangeApproved=false;
    const auto first=b.frames;InterfaceResult out;CHECK(!d.configureInterface(s,out,{1000000,2048}).ok() && b.frames==first);
  }
  Controller b;b.id2=0xA111;Driver d;ready(d,b);auto s=settings();s.clockOutput=ClockOutput::LED1;InterfaceResult out;
  const auto first=b.frames;CHECK(!d.configureInterface(s,out,{1000000,2048}).ok() && b.frames==first);
}
void failures() {
  Controller ref;Driver reference;ready(reference,ref);auto s=settings();s.isolationOwned=true;s.isolate=true;s.clockOutput=ClockOutput::LED1;
  s.fastRxDv=s.oddNibbleDisable=s.shortIpg=true;const auto first=ref.frames,firstCall=ref.calls;
  configured(reference,ref,s);const auto count=ref.frames-first,callbacks=ref.calls-firstCall;CHECK(count==39U && callbacks==16U);
  for(unsigned phase=0;phase<count;++phase)for(bool accepted:{false,true}) {
    Controller b;Driver d;ready(d,b);const auto start=b.frames;b.failFrame=static_cast<int>(start+phase);b.acceptedFailure=accepted;InterfaceResult out;
    CHECK(d.configureInterface(s,out,{1000000,2048}).is(Err::MDIO_TIMEOUT));
    CHECK(out.status.detail==-33033 && !out.verified && out.attemptedFrames==phase+1U && b.frames==start+phase+1U);
    CHECK(b.calls==b.releases && !b.locked && d.interfaceSnapshot().trust!=ConfigurationTrust::VERIFIED);
    bool effect=false;for(unsigned i=0;i<phase;++i)effect|=semantic(ref.trace[first+i]);effect|=accepted && semantic(ref.trace[first+phase]);
    if(effect)CHECK(out.mutationPossible && d.operationSnapshot().cleanupOwed && !d.end().ok());
  }
  for(unsigned call=0;call<callbacks;++call) {
    Controller b;Driver d;ready(d,b);b.releaseCall=static_cast<int>(b.calls+call);InterfaceResult out;
    CHECK(d.configureInterface(s,out,{1000000,2048}).is(Err::MDIO_ERROR));CHECK(out.release.detail==-37 && !out.verified);
  }
  constexpr uint16_t addresses[]={9,10,17,0x456,0x302,23,0};
  constexpr uint16_t masks[]={2,2,0x10,8,0x40,0x20,0x400};
  for(unsigned row=0;row<7;++row) {
    unsigned last=first;for(unsigned i=first;i<ref.frames;++i)if(ref.trace[i].kind==FrameKind::READ && ref.trace[i].target==addresses[row])last=i;
    Controller b;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+last-first);b.corruptMask=masks[row];InterfaceResult out;
    CHECK(d.configureInterface(s,out,{1000000,2048}).is(Err::CONFIG_MISMATCH));
    CHECK(out.mismatch.valid && out.mismatch.registerAddress==addresses[row] && !out.verified && d.operationSnapshot().cleanupOwed);
  }
  // A read of the software/pin power OR cannot establish unowned power intent.
  Controller powered;Driver d;ready(d,powered);powered.powerPin=true;const auto mutations=powered.mutations;InterfaceResult out;
  CHECK(!d.configureInterface(s,out,{1000000,2048}).ok() && powered.mutations==mutations && !d.operationSnapshot().cleanupOwed);
}
void replayReadiness() {
  // Mandatory VOD proof is not full initialization when owned pin/interface
  // restoration remains. Exercise both the no-link and link-profile paths.
  for (bool linkOwned:{false,true}) for (unsigned owners=0;owners<3U;++owners)
    for (auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART})
      for (unsigned outcome=0;outcome<3U;++outcome) {
        Controller b;Driver d;
        if (linkOwned) { ready(d,b); } else { initialized(d,b); }
        if (owners!=0U) { configured(d,b,settings()); }
        if (owners!=1U) {
          PinSettings pins;pins.availableMask=1;
          pins.maintenanceAdmitted=pins.macStopped=pins.outputsQuiesced=true;
          pins.pins[0].function=PinFunction::LED;PinResult result;
          CHECK(d.configurePins(pins,result,{1000000,2048}).ok());
        }
        OperationToken token;CHECK(d.startInitialize({reset},budget(),token).inProgress());
        bool entered=false,vodWritten=false;
        for (unsigned n=0;n<256U && !entered;++n) {
          const auto first=b.frames;CHECK(poll(d,b,token).inProgress());
          for (unsigned i=first;i<b.frames;++i) {
            const auto& frame=b.trace[i];
            if (semantic(frame) && frame.target==31U && frame.value==0x4000U) {
              CHECK(d.isInitialized()); // RESTART itself retains readiness (OP-13).
            }
            if (semantic(frame) && frame.target==0x030EU) { vodWritten=true; }
          }
          if (!vodWritten) { continue; }
          if (owners!=0U) {
            entered=b.frames==first+2U && b.trace[first].kind==FrameKind::READ &&
                b.trace[first].target==0U && b.trace[first+1U].target==0x11U;
          } else {
            for (unsigned i=first;i<b.frames;++i) {
              entered|=b.trace[i].kind==FrameKind::READ && b.trace[i].target==0x0303U;
            }
          }
        }
        CHECK(entered && !d.isInitialized());
        // A short slice cannot finish the outstanding owned restoration.
        const auto frames=b.frames;CHECK(d.pollOperation(token,0).inProgress());
        CHECK(b.frames==frames && !d.isInitialized());
        if (outcome==0U) {
          CHECK(drain(d,b,token).ok());const auto result=take(d,token);
          CHECK(result.persistentRestored && d.isInitialized());
        } else {
          if (outcome==1U) {
            b.failFrame=static_cast<int>(b.frames);CHECK(!poll(d,b,token).ok());
          } else { CHECK(d.cancel(token).ok()); }
          const auto result=take(d,token);
          CHECK(!result.persistentRestored && result.cleanupOwed && !d.isInitialized());
          b.failFrame=-1;CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());
          CHECK(drain(d,b,token).ok());CHECK(take(d,token).persistentRestored && d.isInitialized());
        }
      }
}
void replay() {
  replayReadiness();
  auto s=settings();s.fastRxDv=s.oddNibbleDisable=s.shortIpg=true;s.rxDv=true;s.clockOutput=ClockOutput::LED1;s.isolationOwned=true;s.isolate=true;
  for(auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
    Controller b;Driver d;ready(d,b);configured(d,b,s);OperationToken token;
    CHECK(d.startInitialize({reset},budget(),token).inProgress());CHECK(drain(d,b,token).ok());const auto result=take(d,token);
    CHECK(result.persistentRestored && !result.cleanupOwed && result.interfaceState.trust==ConfigurationTrust::VERIFIED);
    CHECK((b.words[9]&2U)!=0U && (b.words[10]&2U)!=0U && (b.ext[0x456]&8U)!=0U && (b.words[0]&0x400U)!=0U);
    CHECK((b.ext[0x302]&0x140U)==0x140 && (b.words[23]&0x1B3U)==0x20);
    const auto mutations=b.mutations;CHECK(d.startVerify(budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);CHECK(b.mutations==mutations);
  }
  Controller ref;Driver reference;ready(reference,ref);configured(reference,ref,s);OperationToken token;const auto first=ref.frames,firstCall=ref.calls;
  CHECK(reference.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(reference,ref,token).ok());take(reference,token);const auto count=ref.frames-first,callbacks=ref.calls-firstCall;
  for(unsigned phase=0;phase<count;++phase) {
    Controller b;Driver d;ready(d,b);configured(d,b,s);b.failFrame=static_cast<int>(b.frames+phase);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(!drain(d,b,token).ok());const auto result=take(d,token);
    CHECK(!result.persistentRestored);
    if(result.commandAccepted || result.mutationPossible)CHECK(result.interfaceState.trust!=ConfigurationTrust::VERIFIED);
  }
  for(unsigned boundary=0;boundary<callbacks;++boundary) {
    Controller b;Driver d;ready(d,b);configured(d,b,s);CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    for(unsigned step=0;step<boundary && d.operationSnapshot().state==OperationState::RUNNING;++step)poll(d,b,token);
    const auto frames=b.frames;CHECK(d.cancel(token).ok());CHECK(b.frames==frames);const auto result=take(d,token);
    CHECK(result.status.is(Err::CANCELLED));if(result.mutationPossible)CHECK(result.cleanupOwed);
  }
  for(bool down:{false,true}) {
    Controller b;Driver d;ready(d,b);configured(d,b,s);PowerSettings p;p.request=down?PowerRequest::IEEE_POWER_DOWN:PowerRequest::NORMAL;
    p.maintenanceAdmitted=p.managementClockAvailable=p.clockLossApproved=true;
    CHECK(d.startConfigurePower(p,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(d.interfaceSnapshot().trust==ConfigurationTrust::VERIFIED && (b.words[0]&0xC00U)==(down?0xC00U:0x400U));
  }
  for(unsigned owner=0;owner<3;++owner) {
    Controller b;Driver d;ready(d,b);auto clock=settings();clock.clockOutput=ClockOutput::LED1;configured(d,b,clock);
    PinSettings pins;pins.availableMask=2;pins.maintenanceAdmitted=pins.macStopped=pins.outputsQuiesced=pins.led1ClockUnused=true;
    if(owner==0)pins.pins[1].function=PinFunction::LED;
    else if(owner==1) {
      WakeSettings wake;wake.enabled=true;wake.route=WakeRoute::LED1;
      wake.maintenanceAdmitted=wake.receiveTrafficQuiesced=wake.pinAvailable=wake.led1ClockUnused=true;
      CHECK(d.startConfigureWake(wake,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
      pins.pins[1].function=PinFunction::WAKE;
    } else {
      InterruptSettings irq;irq.pin=InterruptPin::LED1;irq.maintenanceAdmitted=irq.pinAvailable=irq.led1ClockUnused=true;
      CHECK(d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
      pins.pins[1].function=PinFunction::INTERRUPT;
    }
    PinResult pinResult;CHECK(d.configurePins(pins,pinResult,{1000000,2048}).ok());
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(d,b,token).ok());const auto result=take(d,token);
    CHECK(result.persistentRestored && !result.cleanupOwed && result.interfaceState.trust==ConfigurationTrust::VERIFIED);
    CHECK(result.pins.trust==ConfigurationTrust::VERIFIED && (b.ext[0x302]&0x40U)!=0U);
    CHECK((b.ext[0x304]&7U)==(owner==0?0U:owner==1?2U:4U));
  }
  {
    Controller b;Driver d;ready(d,b);configured(d,b,s);PowerSettings power;
    power.request=PowerRequest::IEEE_POWER_DOWN;power.disablePll=true;
    power.maintenanceAdmitted=power.managementClockAvailable=power.clockLossApproved=true;
    CHECK(d.startConfigurePower(power,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(d.startVerify(budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(d.startInitialize({ResetKind::PCS},budget(),token).inProgress());CHECK(drain(d,b,token).ok());const auto result=take(d,token);
    CHECK(result.persistentRestored && result.interfaceState.trust==ConfigurationTrust::VERIFIED && (b.words[17]&0x8000U)!=0U);
  }
  // An independently consumed word can disprove an already verified row
  // between restoration polls. A later successful row cannot repair that proof.
  Controller changed;Driver phy;ready(phy,changed);configured(phy,changed,s);
  CHECK(phy.startInitialize({ResetKind::HARD},budget(),token).inProgress());
  bool applied=false,verified=false;
  for(unsigned n=0;n<128U && !verified;++n) {
    const auto start=changed.frames;CHECK(poll(phy,changed,token).inProgress());
    for(unsigned i=start;i<changed.frames;++i)if(changed.trace[i].target==9) {
      if(semantic(changed.trace[i]) && (changed.trace[i].value&2U)!=0U)applied=true;
      else if(applied && changed.trace[i].kind==FrameKind::READ)verified=true;
    }
  }
  CHECK(verified);changed.words[9]^=2;uint16_t raw=0;
  CHECK(phy.readClause22(9,raw,{1000000,1}).ok());const auto frozen=changed.frames;
  CHECK(!drain(phy,changed,token).ok());const auto result=take(phy,token);
  CHECK(result.mismatch.valid && result.mismatch.registerAddress==9 && !result.persistentRestored && result.cleanupOwed);
  CHECK(changed.frames==frozen && phy.interfaceSnapshot().trust!=ConfigurationTrust::VERIFIED);
}
void routeDependencies() {
  // A verified LED1 mux does not prove the independently owned clock route.
  // Exercise direct LEDs, IRQ, direct wake and wake through the IRQ consumer.
  const auto setup=[](Driver& d,Controller& b,unsigned owner) {
    ready(d,b);auto clock=settings();clock.clockOutput=ClockOutput::LED1;configured(d,b,clock);
    OperationToken token;
    if(owner==1U || owner==3U) {
      InterruptSettings irq;irq.pin=InterruptPin::LED1;irq.globalEnable=true;irq.misr2Mask=2;
      irq.maintenanceAdmitted=irq.pinAvailable=irq.led1ClockUnused=true;
      CHECK(d.startConfigureInterrupts(irq,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    }
    if(owner>=2U) {
      WakeSettings wake;wake.enabled=true;wake.route=owner==2U?WakeRoute::LED1:WakeRoute::INTERRUPT;
      wake.maintenanceAdmitted=wake.receiveTrafficQuiesced=wake.pinAvailable=wake.led1ClockUnused=true;
      CHECK(d.startConfigureWake(wake,budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    }
    PinSettings pins;pins.availableMask=2;pins.maintenanceAdmitted=pins.macStopped=pins.outputsQuiesced=pins.led1ClockUnused=true;
    pins.pins[1].function=owner==0U?PinFunction::LED:owner==2U?PinFunction::WAKE:PinFunction::INTERRUPT;
    PinResult pin;CHECK(d.configurePins(pins,pin,{1000000,2048}).ok());
  };
  auto clock=settings();clock.clockOutput=ClockOutput::LED1;
  Controller reference;Driver ref;setup(ref,reference,0);const auto start=reference.frames;configured(ref,reference,clock);
  unsigned offsets[2]={0,0};
  for(unsigned i=start;i<reference.frames;++i) {
    if(reference.trace[i].target==0x0302 && semantic(reference.trace[i]))offsets[0]=i-start;
    if(reference.trace[i].target==0x0308 && reference.trace[i].kind==FrameKind::READ)offsets[1]=i-start;
  }
  CHECK(offsets[0]!=0U && offsets[1]>offsets[0]);
  for(unsigned owner=0;owner<4U;++owner)for(unsigned fault=0;fault<3U;++fault) {
    Controller b;Driver d;setup(d,b,owner);const auto pinsBefore=d.pinsSnapshot();
    const auto irqBefore=d.interruptSnapshot();const auto wakeBefore=d.wakeSnapshot();OperationToken token;
    OperationResult cancelled;
    if(fault<2U) {
      b.failFrame=static_cast<int>(b.frames+offsets[fault]);InterfaceResult failed;
      CHECK(d.configureInterface(clock,failed,{1000000,2048}).is(Err::MDIO_TIMEOUT));
      CHECK(failed.mutationPossible && d.operationSnapshot().cleanupOwed);
    } else {
      CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());bool routeWritten=false;
      for(unsigned n=0;n<512U && !routeWritten;++n) {
        const auto first=b.frames;CHECK(poll(d,b,token).inProgress());
        for(unsigned i=first;i<b.frames;++i)if(b.trace[i].target==0x0302 && semantic(b.trace[i]))routeWritten=true;
      }
      CHECK(routeWritten);const auto frames=b.frames;CHECK(d.cancel(token).ok() && b.frames==frames);cancelled=take(d,token);
      CHECK(cancelled.status.is(Err::CANCELLED) && cancelled.cleanupOwed);
      CHECK(cancelled.pins.trust==ConfigurationTrust::DIRTY && cancelled.pins.observedStale);
      if(owner==1U || owner==3U)CHECK(cancelled.interrupts.trust==ConfigurationTrust::DIRTY && cancelled.interrupts.observedStale);
      if(owner>=2U)CHECK(cancelled.wake.trust==ConfigurationTrust::DIRTY && cancelled.wake.observedStale);
    }
    const auto frames=b.frames,clocks=b.clocks;
    for(unsigned reader=0;reader<2U;++reader) {
      const auto pins=d.pinsSnapshot();CHECK(pins.trust==ConfigurationTrust::DIRTY && pins.observedStale);
      if(fault<2U)CHECK(pins.observedAtUs==pinsBefore.observedAtUs && pins.timeValid==pinsBefore.timeValid);
      if(owner==1U || owner==3U) {
        const auto irq=d.interruptSnapshot();CHECK(irq.trust==ConfigurationTrust::DIRTY && irq.observedStale);
        if(fault<2U)CHECK(irq.observedAtUs==irqBefore.observedAtUs && irq.timeValid==irqBefore.timeValid);
      }
      if(owner>=2U) {
        const auto wake=d.wakeSnapshot();CHECK(wake.trust==ConfigurationTrust::DIRTY && wake.observedStale);
        if(fault<2U)CHECK(wake.observedAtUs==wakeBefore.observedAtUs && wake.timeValid==wakeBefore.timeValid);
      }
    }
    CHECK(b.frames==frames && b.clocks==clocks);b.failFrame=-1;
    if(owner>=2U) {
      const auto first=b.frames,calls=b.calls,mutations=b.mutations;WakeSnapshot live;
      CHECK(d.readWakeStatus(live,{1000000,33}).ok());
      CHECK(live.controlValid && live.rxfsValid && live.misr2Valid && live.credentialValidMask==0x3FU);
      CHECK(live.trust==ConfigurationTrust::DIRTY && live.observedStale);
      CHECK(d.wakeSnapshot().trust==ConfigurationTrust::DIRTY && d.wakeSnapshot().observedStale);
      CHECK(b.frames==first+33U && b.calls==calls+8U && b.mutations==mutations);
      CHECK(d.interfaceSnapshot().trust==ConfigurationTrust::DIRTY && d.operationSnapshot().cleanupOwed);
    }
    CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());CHECK(drain(d,b,token).ok());const auto repaired=take(d,token);
    CHECK(repaired.persistentRestored && repaired.pins.trust==ConfigurationTrust::VERIFIED && d.pinsSnapshot().trust==ConfigurationTrust::VERIFIED);
    if(owner==1U || owner==3U)CHECK(repaired.interrupts.trust==ConfigurationTrust::VERIFIED && d.interruptSnapshot().trust==ConfigurationTrust::VERIFIED);
    if(owner>=2U)CHECK(repaired.wake.trust==ConfigurationTrust::VERIFIED && d.wakeSnapshot().trust==ConfigurationTrust::VERIFIED);
  }
}
void ownership() {
  routeDependencies();
  Controller b;Driver d,other;ready(d,b);CHECK(other.bind(b.config(2)).ok());b.competitor=&other;
  const auto first=b.frames;configured(d,b,settings());CHECK(b.excluded==b.frames-first);b.competitor=nullptr;
  const auto calls=b.calls,clocks=b.clocks;const auto a=d.interfaceSnapshot(),c=d.interfaceSnapshot();
  CHECK(a.validMask==c.validMask && b.calls==calls && b.clocks==clocks && !other.interfaceSnapshot().desiredValid);
  b.words[9]^=2;uint16_t raw=0;CHECK(d.readClause22(9,raw,{1000000,1}).ok());
  CHECK(d.interfaceSnapshot().trust==ConfigurationTrust::DIRTY && d.interfaceSnapshot().observedStale);
  CHECK(d.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
  CHECK(d.interfaceSnapshot().desiredValid && !d.interfaceSnapshot().modeValid);
  OperationToken token;CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
  CHECK(d.interfaceSnapshot().trust==ConfigurationTrust::VERIFIED);
  for(unsigned owner=0;owner<3;++owner) {
    Controller bus;Driver phy;ready(phy,bus);auto profile=settings();profile.clockOutput=ClockOutput::CLKOUT25;configured(phy,bus,profile);
    const auto before=bus.frames;
    if(owner==0) {
      PinSettings pin;pin.maintenanceAdmitted=pin.macStopped=pin.outputsQuiesced=pin.led1ClockUnused=true;pin.availableMask=2;
      pin.pins[1].function=PinFunction::LED;pin.led1ClockOverride=true;PinResult result;
      CHECK(!phy.configurePins(pin,result,{1000000,2048}).ok());
    } else if(owner==1) {
      WakeSettings wake;wake.route=WakeRoute::LED1;wake.maintenanceAdmitted=wake.receiveTrafficQuiesced=wake.pinAvailable=wake.led1ClockUnused=true;
      CHECK(!phy.startConfigureWake(wake,budget(),token).inProgress());
    } else {
      InterruptSettings irq;irq.pin=InterruptPin::LED1;irq.maintenanceAdmitted=irq.pinAvailable=irq.led1ClockUnused=true;
      CHECK(!phy.startConfigureInterrupts(irq,budget(),token).inProgress());
    }
    CHECK(bus.frames==before);
  }
  Controller pins;Driver pinDriver;ready(pinDriver,pins);PinSettings pin;
  pin.maintenanceAdmitted=pin.macStopped=pin.outputsQuiesced=pin.led1ClockUnused=true;pin.availableMask=2;pin.led1ClockOverride=true;pin.pins[1].function=PinFunction::LED;PinResult result;
  CHECK(pinDriver.configurePins(pin,result,{1000000,2048}).ok());auto profile=settings();profile.clockOutput=ClockOutput::CLKOUT25;
  const auto before=pins.frames;InterfaceResult interfaceResult;CHECK(!pinDriver.configureInterface(profile,interfaceResult,{1000000,2048}).ok() && pins.frames==before);
  Controller example;Driver exampleDriver;ready(exampleDriver,example);CHECK(configureStoppedRmii(exampleDriver,interfaceResult,{1000000,2048}).ok());
  const auto noFrames=example.frames;CHECK(!rejectLiveInterfaceChange(exampleDriver,interfaceResult,{1000000,2048}).ok() && example.frames==noFrames);
  for(bool eeeFirst:{false,true}) {
    Controller bus;Driver phy;ready(phy,bus);EeeSettings eee;eee.enabled=eee.maintenanceAdmitted=true;
    auto conflicting=settings();conflicting.clockOutput=ClockOutput::LED1;
    if(eeeFirst) {
      CHECK(phy.startConfigureEee(eee,budget(),token).inProgress());CHECK(drain(phy,bus,token).ok());take(phy,token);
      const auto start=bus.frames;InterfaceResult rejected;
      CHECK(!phy.configureInterface(conflicting,rejected,{1000000,2048}).ok() && bus.frames==start);
      conflicting.clockOutput=ClockOutput::UNOWNED;conflicting.isolationOwned=conflicting.isolate=true;
      CHECK(!phy.configureInterface(conflicting,rejected,{1000000,2048}).ok() && bus.frames==start);
    } else {
      configured(phy,bus,conflicting);const auto start=bus.frames;
      CHECK(!phy.startConfigureEee(eee,budget(),token).inProgress() && bus.frames==start);
    }
  }
  for(bool interfaceFirst:{false,true}) {
    Controller bus;bus.boardInterface=MacInterface::MII;Driver phy;ready(phy,bus);
    bus.words[23]&=0xFFDFU; // Actual MII strap/mode agrees with this pin-routing case.
    auto collision=settings(MacInterface::MII);collision.collisionFullDuplex=true;
    PinSettings led;led.pins[2].function=PinFunction::LED;led.availableMask=4;
    led.maintenanceAdmitted=led.macStopped=led.outputsQuiesced=led.colUnused=true;PinResult pinResult;
    if(interfaceFirst) {
      configured(phy,bus,collision);const auto start=bus.frames;
      CHECK(!phy.configurePins(led,pinResult,{1000000,2048}).ok() && bus.frames==start);
    } else {
      CHECK(phy.configurePins(led,pinResult,{1000000,2048}).ok());const auto start=bus.frames;InterfaceResult rejected;
      CHECK(!phy.configureInterface(collision,rejected,{1000000,2048}).ok() && bus.frames==start);
    }
  }
}
void collector() {
  Controller ref;Driver reference;ready(reference,ref);ref.words[23]=0x006D;const auto first=ref.frames;configured(reference,ref,settings());
  unsigned read=first;for(unsigned i=first;i<ref.frames;++i)if(ref.trace[i].target==23 && ref.trace[i].kind==FrameKind::READ) { read=i-first;break; }
  const auto sample=reference.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
  CHECK(sample.valid && sample.samples==2U && (sample.observedOnes&0xCU)==0xCU && sample.sampledSum==0 && !sample.historyLost);
  for(bool afterEffect:{false,true}) {
    Controller b;Driver d;ready(d,b);b.words[23]=0x006D;b.failFrame=static_cast<int>(b.frames+read);b.acceptedFailure=afterEffect;InterfaceResult out;
    CHECK(d.configureInterface(settings(),out,{1000000,2048}).is(Err::MDIO_TIMEOUT));
    const auto retained=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
    CHECK(retained.historyLost==afterEffect && !retained.valid && retained.samples==0U);
  }
  Controller b;Driver d;ready(d,b);b.words[23]=0x006D;b.failFrame=static_cast<int>(b.frames+read+1U);InterfaceResult out;
  CHECK(d.configureInterface(settings(),out,{1000000,2048}).is(Err::MDIO_TIMEOUT));
  const auto retained=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
  CHECK(retained.valid && retained.samples==1U && retained.raw==0x006D && !retained.historyLost);
  const auto calls=b.calls,clocks=b.clocks;d.interfaceSnapshot();d.evidenceSnapshot();CHECK(b.calls==calls && b.clocks==clocks);
  for(unsigned failedRead:{read,read+2U}) {
    Controller known;Driver phy;ready(phy,known);configured(phy,known,settings());const auto old=phy.interfaceSnapshot();
    known.failFrame=static_cast<int>(known.frames+failedRead);InterfaceResult partial;
    CHECK(phy.configureInterface(settings(),partial,{1000000,2048}).is(Err::MDIO_TIMEOUT));
    const auto stale=phy.interfaceSnapshot();CHECK(!stale.modeValid && stale.observedStale && stale.observedAtUs==old.observedAtUs);
    CHECK(stale.observedInterface==old.observedInterface); // Retained word is historical, never a new mode claim.
  }
}
void timing() {
  Controller ref;Driver reference;ready(reference,ref);const auto first=ref.frames,firstCall=ref.calls;configured(reference,ref,settings());
  const auto frames=ref.frames-first,calls=ref.calls-firstCall;CHECK(frames==32U && calls==13U);
  for(uint32_t allowance:{0U,frames-1U,frames}) {
    Controller b;Driver d;ready(d,b);const auto start=b.frames;InterfaceResult out;out.attemptedFrames=999;
    const auto status=d.configureInterface(settings(),out,{1000000,allowance});
    if(allowance==frames)CHECK(status.ok());else CHECK(status.is(Err::OPERATION_TIMEOUT) && out.attemptedFrames==999 && b.frames==start);
  }
  for(uint64_t margin:{uint64_t{0},uint64_t{233},uint64_t{234}}) {
    Controller b;Driver d;ready(d,b);const auto start=b.frames;InterfaceResult out;out.attemptedFrames=999;
    // 13 callbacks*8us +32 frames*4us +1us clock lag =233us.
    const auto status=d.configureInterface(settings(),out,{b.now+margin,32});
    if(margin==234U)CHECK(status.ok());else CHECK(status.is(Err::OPERATION_TIMEOUT) && b.frames==start && out.attemptedFrames==999);
  }
  for(unsigned defect=0;defect<6;++defect) {
    Controller b;Driver d;ready(d,b);const auto call=static_cast<int>(b.calls+2U);
    if(defect==0)b.lateCall=call;
    if(defect==1)b.regressCall=call;
    if(defect==2)b.invalidCall=call;
    if(defect==3)b.badReceiptCall=call;
    if(defect==4)b.now=std::numeric_limits<uint64_t>::max();
    if(defect==5)b.frameTime=0;
    const auto firstFrames=b.frames;InterfaceResult out;const auto status=d.configureInterface(settings(),out,{defect==4?b.now:1000000,2048});
    if(defect==5)CHECK(status.ok() && b.frames-firstFrames==frames);else CHECK(!status.ok());
    CHECK(b.calls==b.releases && !b.locked);
  }
  Controller b;Driver d;ready(d,b);configured(d,b,settings());OperationToken token;CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());
  bool found=false;
  for(unsigned n=0;n<128U && !found;++n) {
    const auto start=b.frames;CHECK(poll(d,b,token).inProgress());
    for(unsigned i=start;i<b.frames;++i)if(b.trace[i].target==0x456 && semantic(b.trace[i]))found=true;
  }
  CHECK(found);const auto before=b.frames;CHECK(poll(d,b,token,3).inProgress() && b.frames==before); // Four-frame MMD readback cannot split.
  CHECK(drain(d,b,token).ok());take(d,token);
}
void repeater() {
  Controller leftBus,rightBus;Driver left,right;ready(left,leftBus);ready(right,rightBus);
  auto leader=settings();leader.rxDv=leader.repeater=leader.repeaterWiringAdmitted=true;
  auto follower=leader;follower.rmiiClock=RmiiClock::FOLLOWER_50MHZ;follower.referenceClockMHz=50;
  configured(left,leftBus,leader);configured(right,rightBus,follower);
  CHECK((leftBus.words[23]&0xA0U)==0x20U && (rightBus.words[23]&0xA0U)==0xA0U);
  CHECK((leftBus.ext[0x302]&0x100U)!=0U && (rightBus.ext[0x302]&0x100U)!=0U);
  // Two followers may share one application-established reference. No core
  // object claims a pair is ready or rolls back the successful first side.
  configured(left,leftBus,follower);rightBus.failFrame=static_cast<int>(rightBus.frames+4U);InterfaceResult partial;
  CHECK(right.configureInterface(follower,partial,{1000000,2048}).is(Err::MDIO_TIMEOUT));
  CHECK(left.interfaceSnapshot().trust==ConfigurationTrust::VERIFIED && right.interfaceSnapshot().trust!=ConfigurationTrust::VERIFIED);
  for(bool wiring:{false,true}) {
    Controller b;Driver d;ready(d,b);auto rejected=settings();rejected.repeater=true;rejected.repeaterWiringAdmitted=wiring;
    const auto before=b.frames;InterfaceResult out;CHECK(!d.configureInterface(rejected,out,{1000000,2048}).ok() && b.frames==before);
  }
}
template<Variant grade> void aInterface() {
  {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; ready(d,b);
    auto s=settings(); s.clockOutput=ClockOutput::PLL; s.isolationOwned=true;
    const auto calls=b.calls;
    b.delayedClock=b.clocks+2;
    InterfaceResult out; out.attemptedFrames=999;
    CHECK(d.configureInterface(s,out,{b.now+314,44}).is(Err::OPERATION_TIMEOUT));
    CHECK(b.calls==calls && out.attemptedFrames==999 && !d.interfaceSnapshot().desiredValid);
    CHECK(!d.operationSnapshot().cleanupOwed);
    s.observePin31=true; b.ext[0x308]=0x8881; configured(d,b,s);
    s.clockOutput=ClockOutput::UNOWNED; s.observePin31=false; configured(d,b,s);
    OperationToken init;
    CHECK(d.startInitialize({ResetKind::HARD},budget(),init).inProgress() && drain(d,b,init).ok());
    const auto replay=take(d,init).interfaceState;
    CHECK(!replay.observedStale && (replay.validMask&0x180U)==0U);
    CHECK(replay.raw[8]==0x29 && replay.raw[7]==0x8881 && b.ext[0x303]==8 && b.ext[0x308]==0);
  }
  // SNLS783A fig. 8-13 and tables 8-13, 8-74/75/79/89. SOR1 is
  // physical strap evidence, SOR2 records startup mode and RCSR is live.
  struct Startup { uint16_t id, sor1, sor2; bool allowed; };
  constexpr Startup startup[]={
    {0xA114,0,0,true},{0xA114,0x10,0x10,true},{0xA114,0x400,0,false},
    {0xA114,0x410,0x18,true},{0xA114,0x410,0x10,false},
    {0xA134,0,0,true},{0xA134,0x100,0,true},{0xA134,2,0,true},
    {0xA134,0x102,0x10,true},{0xA134,0x122,0x18,true},{0xA134,0x100,0x10,false}
  };
  for (const auto& row:startup) {
    Controller b; Driver d; b.declared=grade; b.id2=row.id;
    b.ext[0x467]=row.sor1; b.ext[0x468]=row.sor2; ready(d,b);
    const auto straps=d.strapSnapshot();
    CHECK(straps.sor1Valid && straps.sor2Valid && straps.sor1==row.sor1 && straps.sor2==row.sor2);
    auto s=settings(); s.isolationOwned=true; s.clockOutput=ClockOutput::PLL;
    s.shortIpg=s.oddNibbleDisable=s.repeater=s.rxDv=s.repeaterWiringAdmitted=true;
    b.ext[0x302]=0x8040; b.ext[0x303]=0xA5FE; b.words[23]=0x4C;
    const auto calls=b.calls,clocks=b.clocks;
    InterfaceResult out;
    const auto status=d.configureInterface(s,out,{1000000,44});
    if (!row.allowed) {
      CHECK(status.is(Err::INVALID_CONFIG) && b.calls==calls && b.clocks==clocks);
      continue;
    }
    CHECK(status.ok() && out.transactions==17 && out.attemptedFrames==44);
    CHECK(b.ext[0x302]==0x8140 && b.ext[0x303]==0xA5E9 && b.words[23]==0x60);
    CHECK((b.ext[0x456]&8U)!=0 && (b.words[10]&2U)!=0);
    CHECK(out.interfaceState.raw[8]==0xA5E9 && (out.interfaceState.validMask&0x180U)==0x100U);
    const auto rcsr=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
    CHECK((rcsr.observedOnes&0xCU)==0xCU && (rcsr.observedZeros&0xCU)==0xCU);
    OperationToken verify; const auto before=b.mutations;
    CHECK(d.startVerify(budget(),verify).inProgress() && drain(d,b,verify).ok());
    CHECK(take(d,verify).persistentRestored && b.mutations==before);
    CHECK(b.frames-b.firstFrame[b.calls-9]==19 && b.trace[b.frames-19].target==0);
  }
  auto profile=settings(); profile.clockOutput=ClockOutput::RECOVERED;
  profile.isolationOwned=true; profile.observePin31=true;
  for (bool statusBit:{false,true}) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; ready(d,b);
    b.ext[0x303]=0x5AFE; b.ext[0x308]=statusBit?0x8001:0x8000;
    InterfaceResult out;
    const auto calls=b.calls;
    CHECK(d.configureInterface(profile,out,{1000000,48}).is(Err::OPERATION_TIMEOUT) && b.calls==calls);
    b.words[16]&=0xFFFE;
    CHECK(d.configureInterface(profile,out,{1000000,49}).is(Err::INVALID_CONFIG) && out.attemptedFrames==3 && !out.mutationPossible);
    b.words[16]|=1;
    out=configured(d,b,profile);
    CHECK(out.attemptedFrames==49 && out.transactions==18 && out.interfaceState.raw[8]==0x5AF1);
    CHECK((out.interfaceState.validMask&0x180)==0x180 && out.interfaceState.raw[7]==b.ext[0x308]);
    const auto frames=b.frames,clocks=b.clocks;
    d.interfaceSnapshot(); d.strapSnapshot(); CHECK(b.frames==frames && b.clocks==clocks);
    for (auto output:{ClockOutput::CLKOUT25,ClockOutput::LED1}) {
      auto denied=profile; denied.clockOutput=output;
      CHECK(d.configureInterface(denied,out,{1000000,49}).is(Err::INVALID_CONFIG) && b.frames==frames && b.clocks==clocks);
    }
    OperationToken token; const auto mutations=b.mutations;
    CHECK(d.startVerify(budget(),token).inProgress() && drain(d,b,token).ok());
    CHECK(take(d,token).interfaceState.trust==ConfigurationTrust::VERIFIED && b.mutations==mutations);
    b.ext[0x303]^=0x0008;
    CHECK(d.startVerify(budget(),token).inProgress() && drain(d,b,token).is(Err::CONFIG_MISMATCH));
    CHECK(take(d,token).mismatch.registerAddress==0x0303 && b.mutations==mutations);
  }
  // The last 13 frames in the 49-frame pass are pin 30 RMW/readback and
  // passive pin 31 status. Every failure there must retain earlier writes.
  for (unsigned offset=0;offset<49;++offset) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; ready(d,b);
    b.failFrame=static_cast<int>(b.frames+offset);
    InterfaceResult out;
    CHECK(d.configureInterface(profile,out,{1000000,49}).is(Err::MDIO_TIMEOUT));
    CHECK(out.attemptedFrames==offset+1 && out.completedFrames==offset && !out.verified && b.calls==b.releases);
    CHECK(d.operationSnapshot().cleanupOwed==(offset>=4));
  }
  for (unsigned call=0;call<18;++call) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; ready(d,b);
    b.releaseCall=static_cast<int>(b.calls+call);
    InterfaceResult out;
    CHECK(d.configureInterface(profile,out,{1000000,49}).is(Err::MDIO_ERROR));
    CHECK(!out.verified && out.release.detail==-37 && b.releases==b.calls);
  }
  for (bool cancelAfterPin:{false,true}) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; ready(d,b);
    auto pll=profile; pll.clockOutput=ClockOutput::PLL; pll.observePin31=false;
    configured(d,b,pll);
    OperationToken token; const auto before=b.frames;
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    if (cancelAfterPin) {
      for (unsigned i=0;i<128 && !writes(b,before,0x0303);++i) CHECK(poll(d,b,token).inProgress());
      CHECK(writes(b,before,0x0303)==1);
    }
    const auto frame=b.frames,clocks=b.clocks; CHECK(d.cancel(token).ok());
    const auto cancelled=take(d,token);
    CHECK(cancelled.status.is(Err::CANCELLED) && cancelled.cleanupOwed==cancelAfterPin);
    CHECK(b.frames==frame && b.clocks==clocks);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress() && drain(d,b,token).ok());
    const auto replay=take(d,token);
    CHECK(replay.persistentRestored && replay.interfaceState.trust==ConfigurationTrust::VERIFIED);
    CHECK((replay.interfaceState.raw[8]&0x003FU)==0x29 && !replay.cleanupOwed);
  }
  for (bool follower:{false,true}) for (bool revision:{false,true}) for (unsigned ppm:{50U,100U}) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA114; ready(d,b);
    auto s=settings(); s.elasticityCode=1; s.maxFrameBytes=ppm==50?2400:1200;
    s.clockTolerancePpm=static_cast<uint16_t>(ppm); s.revision10=revision;
    s.rmiiClock=follower?RmiiClock::FOLLOWER_50MHZ:RmiiClock::LEADER_25MHZ;
    s.referenceClockMHz=follower?50:25; s.txClockShift=follower; configured(d,b,s);
    CHECK((b.words[23]&0x1B3U)==(0x21U|(follower?0x180U:0U)|(revision?0x10U:0U)));
    const auto calls=b.calls,clocks=b.clocks; InterfaceResult out;
    ++s.maxFrameBytes;
    CHECK(d.configureInterface(s,out,{1000000,44}).is(Err::INVALID_CONFIG) && b.calls==calls && b.clocks==clocks);
    --s.maxFrameBytes; s.observePin31=true;
    CHECK(d.configureInterface(s,out,{1000000,48}).is(Err::INVALID_CONFIG) && b.calls==calls && b.clocks==clocks);
  }
}
// SNLS638C tables 7-3, 7-6, 7-19, 7-26 and 7-80. Treat both role
// observations as independent inputs and both FIFO flags as consumed evidence.
void p25Board(Controller& b, bool follower, bool sor, bool rcsr) {
  b.declared=Variant::DP83825I; b.id2=0xA140;
  b.boardRole=follower ? RmiiClock::FOLLOWER_50MHZ : RmiiClock::LEADER_25MHZ;
  b.sampledSor=sor ? 0x0008 : 0;
  b.sampledRcsr=rcsr ? 0x00ED : 0x006D;
  b.defaults();
}
void p25StartupFailures() {
  // Initialization without reset. Stop after each wire frame with uncertain
  // consumption and after each complete callback with a failed release.
  for (bool follower : {false,true}) {
    unsigned totalFrames=0, totalCalls=0, fifoFrame=0;
    {
      Controller b; Driver d;
      p25Board(b,follower,follower,follower);
      initialized(d,b);
      totalFrames=b.frames; totalCalls=b.calls;
      for (unsigned i=0;i<b.frames;++i) if (b.trace[i].target==23) fifoFrame=i;
      CHECK(fifoFrame>1 && totalFrames>fifoFrame && b.trace[fifoFrame].kind==FrameKind::READ);
    }
    for (bool release : {false,true}) for (unsigned cut=0;cut<(release?totalCalls:totalFrames);++cut) {
      Controller b; Driver d;
      p25Board(b,follower,follower,follower);
      CHECK(d.bind(b.config()).ok());
      if (release) b.releaseCall=static_cast<int>(cut);
      else b.failFrame=static_cast<int>(cut);
      OperationToken token;
      CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());
      CHECK(drain(d,b,token).is(release?Err::MDIO_ERROR:Err::MDIO_TIMEOUT));
      const auto terminal=take(d,token);
      CHECK(!d.healthSnapshot().initialized && b.releases==b.calls && writes(b,0,0x030E)==0);
      CHECK(!terminal.mutationPossible && !terminal.cleanupOwed);
      const auto sample=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
      const bool fifoReturned=release?b.frames>fifoFrame:cut>fifoFrame;
      CHECK(sample.valid==fifoReturned && terminal.straps.rcsrValid==fifoReturned);
      if (fifoReturned) CHECK((sample.observedOnes&12U)==12U && terminal.straps.rcsr==b.sampledRcsr);
      if (!release && cut==fifoFrame) CHECK(sample.historyLost && (b.words[23]&12U)==0);
    }
  }
}
void p25InterruptedStrapRecapture() {
  // HARD reloads distinct strap words so a mixed capture cannot pass as fresh.
  for (bool failRcsr : {false, true}) for (bool failRelease : {false, true}) {
    Controller bus;
    Driver driver;
    p25Board(bus, false, false, false);
    bus.ext[0x467] = 0x0001;
    bus.sampledSor = failRcsr && failRelease ? 0x0002 : 0x000A;
    bus.sampledRcsr = 0x00E5;
    CHECK(driver.bind(bus.config()).ok());
    OperationToken token;
    CHECK(driver.startInitialize({ResetKind::HARD}, budget(), token).inProgress());
    for (unsigned i = 0; i < 32; ++i) {
      if (!poll(driver, bus, token).inProgress()) break;
      if (bus.hard == 1 && bus.trace[bus.frames - 1].target == 3) break;
    }
    CHECK(bus.hard == 1 && bus.trace[bus.frames - 1].target == 3);
    if (failRcsr) CHECK(poll(driver, bus, token).inProgress());
    if (failRelease) bus.releaseCall = static_cast<int>(bus.calls);
    else bus.failFrame = static_cast<int>(bus.frames + (failRcsr ? 0U : 3U));

    const auto status = drain(driver, bus, token);
    const auto result = take(driver, token);
    CHECK(status.is(failRelease ? Err::MDIO_ERROR : Err::MDIO_TIMEOUT));
    CHECK(result.commandAccepted && result.cleanupOwed && !driver.isInitialized());
    CHECK(result.straps.stale && !result.straps.timeValid);
    const bool sorReturned = failRcsr || failRelease;
    const bool rcsrReturned = failRcsr && failRelease;
    CHECK(result.straps.sor1Valid && result.straps.sor1 == (sorReturned ? bus.sampledSor : 0x0001));
    CHECK(result.straps.rcsrValid && result.straps.rcsr == (rcsrReturned ? bus.sampledRcsr : 0x006D));
    const auto current = driver.strapSnapshot();
    CHECK(current.stale && !current.timeValid);
    CHECK(current.sor1Valid == sorReturned && current.rcsrValid == rcsrReturned);
    CHECK(result.mismatch.valid == sorReturned);
    if (sorReturned) {
      CHECK(result.mismatch.registerAddress == (rcsrReturned ? 0x17 : 0x0467));
      CHECK(result.mismatch.mask == (rcsrReturned ? 0x80 : 8) && result.mismatch.expected == 0);
      CHECK(result.mismatch.observed == (rcsrReturned ? bus.sampledRcsr : bus.sampledSor));
    }
  }
}
void p25Core() {
  p25InterruptedStrapRecapture();
  p25StartupFailures();
  for (bool declaredFollower : {false,true}) for (bool sor : {false,true}) for (bool rcsr : {false,true}) {
    Controller b; Driver d;
    p25Board(b,declaredFollower,sor,rcsr);
    CHECK(d.bind(b.config()).ok());
    OperationToken token;
    CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());
    const auto status=drain(d,b,token);
    const auto result=take(d,token);
    CHECK(status.ok()==(declaredFollower==sor && sor==rcsr));
    CHECK(result.straps.sor1Valid && result.straps.sor1==b.sampledSor && !result.straps.sor2Valid);
    CHECK(result.straps.rcsrValid && result.straps.rcsr==b.sampledRcsr);
    const auto fifo=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
    CHECK(fifo.valid && (fifo.observedOnes&0x000C)==0x000C && (b.words[23]&0x000C)==0);
    CHECK(writes(b,0,0x17)==0 && writes(b,0,0x030E)==0);
    const auto calls=b.calls, clocks=b.clocks;
    d.strapSnapshot(); d.healthSnapshot(); d.interfaceSnapshot();
    CHECK(b.calls==calls && b.clocks==clocks);
  }
  for (bool follower : {false,true}) {
    Controller b; Driver d;
    p25Board(b,follower,follower,follower);
    ready(d,b);
    LinkSettings profile;
    for (uint8_t modes=1;modes<=15;++modes) {
      profile.advertisement.technologyMask=modes;
      b.words[4]|=0x2000; // Local remote-fault advertisement is prohibited.
      OperationToken token;
      CHECK(d.startConfigure(profile,{},budget(),token).inProgress());
      CHECK(drain(d,b,token).ok() && take(d,token).status.ok());
      CHECK((b.words[4]&0x21E0U)==(uint32_t{modes}<<5U));
    }
    for (auto speed:{LinkSpeed::MBPS_10,LinkSpeed::MBPS_100}) for (auto duplex:{Duplex::HALF,Duplex::FULL}) {
      profile={}; profile.mode=LinkMode::FORCED; profile.forced={speed,duplex}; profile.crossover.robust=false;
      OperationToken token;
      CHECK(d.startConfigure(profile,{},budget(),token).inProgress());
      CHECK(drain(d,b,token).ok() && take(d,token).status.ok());
      CHECK((b.words[0]&0x3100U)==((speed==LinkSpeed::MBPS_100?0x2000U:0U)|(duplex==Duplex::FULL?0x100U:0U)));
      CHECK((b.words[9]&0x20U)==0);
    }
    for (auto kind:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
      OperationToken token;
      CHECK(d.startInitialize({kind},budget(),token).inProgress());
      CHECK(drain(d,b,token).ok());
      const auto result=take(d,token);
      CHECK(result.persistentRestored && result.straps.rcsrValid && !result.straps.stale);
      CHECK(writes(b,0,0x030E)==0 && b.ext[0x030E]==0x8400);
    }
    LinkSnapshot link{};
    b.words[16]|=0x0040;
    const auto before=b.frames;
    CHECK(d.readLinkStatus(link,{1000000,10}).ok());
    constexpr uint16_t order[]={16,1,1,0,4,5,6,25,16,1};
    CHECK(b.frames-before==10);
    for (unsigned i=0;i<10;++i) CHECK(b.trace[before+i].target==order[i]);
    CHECK((link.rawPhystsFirst&0x40U)!=0 && (link.rawPhystsFinal&0x40U)==0);
    const auto calls=b.calls,clocks=b.clocks;
    uint16_t untouched=0xBEEF;
    for (uint32_t reg:{12U,15U,18U,19U,23U}) {
      CHECK(!d.readClause22(reg,untouched,{1000000,1}).ok() && untouched==0xBEEF);
      CHECK(!d.readMmd(31,reg,untouched,{1000000,4}).ok() && untouched==0xBEEF);
    }
    CHECK(!d.writeClause22(4,0x2001,{1000000,1}).ok());
    CHECK(!d.writeMmd(31,4,0x2001,{1000000,4}).ok());
    CHECK(b.calls==calls && b.clocks==clocks);
  }
}
#include "p25_profiles.inc"
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"p25",p25Core},{"p25_profiles",p25Profiles},{"vectors",vectors},{"admission",admission},{"failures",failures},{"replay",replay},
    {"ownership",ownership},{"collector",collector},{"timing",timing},{"repeater",repeater},
    {"ai",aInterface<Variant::DP83826AI>},{"ae",aInterface<Variant::DP83826AE>}};
  return test::runSuites(argc,argv,suites);
}
