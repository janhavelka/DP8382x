#include "support/BoundDriver.h"
#include "support/PartSelection.h"
#include <DP8382x/DP8382x.h>
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target, devad; uint64_t at; };

// Independent SNLS647I p26/27 recipe plus source-implied SNLS638C p80 fields.
// DEVAD 3/7 storage is separate from vendor 31 and direct C22; successful fake
// reads do not establish Q1 wire mapping or physical EEE interoperability.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<uint16_t,64> mmd3{},mmd7{};
  std::array<Wire,8192> trace{};
  uint16_t selector{0},address{0},id2{test::enhancedId2()};
  uint64_t now{100};
  unsigned jumpClockCall{0};
  uint64_t jumpClockTo{0};
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
    ext[0x030E]=0x8400;ext[0x04A1]=0x1000;ext[0x04D1]=0x0080;
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=test::declaredVariant;c.interface=id2==test::basicId2()?MacInterface::MII:MacInterface::RMII;c.phyAddress=phy;
    c.transport={this,execute,clock,{8,4,1}};return c;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    if (++bus.clocks == bus.jumpClockCall) { bus.now = bus.jumpClockTo; }
    return bus.now;
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
      if(devad==3)mmd3[target]=value;else mmd7[target]=static_cast<uint16_t>(value^(corruptAdvertisement?1U:0U));
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
      if(failed) { f.effectPossible=b.acceptedFailure;r.status={Err::MDIO_TIMEOUT,-29029,"Injected frame"};break; }
      f.completed=true;f.valueValid=q.kind==FrameKind::READ;
    }
    if(static_cast<int>(call)==b.releaseCall) { r.releaseStatus={Err::MDIO_ERROR,-37,"Injected release"};r.controllerReusable=false; }
    if(static_cast<int>(call)==b.lateCall)b.now=budget.deadlineUs;
    if(static_cast<int>(call)==b.regressCall)b.now=0;
    if(static_cast<int>(call)==b.invalidCall)r.status={Err::IN_PROGRESS,-29030,"Illegal terminal status"};
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
bool semantic(const Wire& frame) { return frame.kind==FrameKind::WRITE && frame.reg!=13 && !(frame.reg==14 && frame.target==14); }
unsigned writes(const Controller& b,unsigned first,uint16_t devad,uint16_t target) {
  unsigned count=0;for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==target && b.trace[i].devad==devad)++count;
  return count;
}
unsigned findWrite(const Controller& b,unsigned first,uint16_t devad,uint16_t target,uint16_t value) {
  for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==target && b.trace[i].devad==devad && b.trace[i].value==value)return i;
  CHECK(false);return first;
}
void literalMmd(const Controller& b,unsigned first,uint16_t devad,uint16_t address,FrameKind kind,uint16_t value=0) {
  CHECK(first+3U<b.frames);if(first+3U>=b.frames)return;
  const auto& a=b.trace[first];const auto& z=b.trace[first+3U];
  CHECK(a.kind==FrameKind::WRITE && a.reg==13 && a.value==devad);
  CHECK(b.trace[first+1U].kind==FrameKind::WRITE && b.trace[first+1U].reg==14 && b.trace[first+1U].value==address);
  CHECK(b.trace[first+2U].kind==FrameKind::WRITE && b.trace[first+2U].reg==13 && b.trace[first+2U].value==static_cast<uint16_t>(0x4000U|devad));
  CHECK(z.kind==kind && z.reg==14 && z.target==address && z.devad==devad);if(kind==FrameKind::WRITE)CHECK(z.value==value);
}

void raw() {
  auto [b, d] = test::BoundDriver<Controller>{};
  b.mmd7[0x3C]=0x0002;b.mmd7[0x3D]=0x0002;
  const std::array<std::array<uint16_t,2>,3> allowed{{{{3,0x14}},{{7,0x3C}},{{7,0x3D}}}};
  for(const auto& pair:allowed) {
    uint16_t value=0xCAFE;const auto first=b.frames,calls=b.calls;
    CHECK(d.readMmd(pair[0],pair[1],value,{1000000,4}).ok());CHECK(value==2 && b.frames==first+4U && b.calls==calls+1U);
    literalMmd(b,first,pair[0],pair[1],FrameKind::READ);
  }
  for(uint32_t devad:{3U,7U})for(uint32_t address:{0U,1U,0x14U,0x16U,0x3CU,0x3DU,0x1001U,0x1014U,0x203CU,0x203DU,0x10000U}) {
    const auto first=b.frames,clocks=b.clocks;uint16_t sentinel=0xCAFE;
    const bool allowedRead=(devad==3U && address==0x14U)||(devad==7U && (address==0x3CU || address==0x3DU));
    if(!allowedRead) { CHECK(!d.readMmd(devad,address,sentinel,{1000000,64}).ok());CHECK(sentinel==0xCAFE); }
    CHECK(!d.writeMmd(devad,address,2,{1000000,64}).ok());
    CHECK(b.frames==first && b.clocks==clocks);
  }
}
void recipe() {
  // Original SNLS647I p26 literal rows, independently transcribed before source implementation.
  constexpr std::array<uint16_t,18> origAddress{{0x1F,0x3C,0x4D1,0x4D3,0x4DF,0x33E,0x33F,0x123,0x31B,0x466,0x4CF,0x416,0x4F5,0x4E0,0x31F,0x308,0x4F4,0}};
  constexpr std::array<uint16_t,18> origValue{{0x8000,2,0x008B,0x4F12,0x0180,0xA681,3,0x0800,0x8848,0xFE00,0x261D,0x1F30,0x2864,0xFFF2,0xFE36,0,0x0800,0x3300}};
  // SNLS783A pp. 27 and 28, including the duplicate row and trailing CLK word.
  constexpr uint16_t aAddress[]={31,60,0x4D1,0x4DF,0x33E,0x33F,0x4F5,0x4E0,0x31F,0x308,0x4F4,0x123,
    0x126,0x132,0x4D4,0x4CF,0x310,0x416,0x311,0x313,0x33A,0x33C,0x317,0x31B,0x4F5,0,0x466};
  constexpr uint16_t aValue[]={0x8000,2,0x008B,0x0180,0xA681,3,0x2864,0xFFF2,0xFE36,0,0x0800,0x0800,
    0x5D23,0x0183,0x3322,0x261D,0x76E1,0x1830,0x01FC,0x06E3,0x579B,0xEC00,0x1000,0x8848,0x2864,0x3300,0xFD00};
  const auto* address=test::aGrade()?aAddress:origAddress.data();
  const auto* value=test::aGrade()?aValue:origValue.data();
  Controller b;Driver d;ready(d,b);const auto first=b.frames;configured(d,b);
  unsigned index=findWrite(b,first,31,0x1F,0x8000);
  for(unsigned row=0;row<(test::aGrade()?27U:18U);++row) {
    while(index<b.frames && !semantic(b.trace[index]))++index;
    CHECK(index<b.frames);if(index>=b.frames)break;
    const auto& frame=b.trace[index];const uint16_t devad=row==1U?uint16_t{7}:uint16_t{31};
    CHECK(frame.target==address[row] && frame.value==value[row] && frame.devad==devad);
    if(row!=0U && address[row]!=0U)literalMmd(b,index-3U,devad,address[row],FrameKind::WRITE,value[row]);
    if(row==0U) {
      CHECK(index+1U<b.frames && b.trace[index+1U].at>=frame.at+2000U);
    }
    ++index;
  }
  CHECK(b.hard==1U && writes(b,first,31,0x123)==1U && writes(b,first,31,0x030E)==1U);
  CHECK(b.ext[0x123]==0x0800 && (b.ext[0x4D1]&0x1009U)==9U);
  CHECK((b.words[4]&0x8000U)==0U && (b.words[4]&0x01FFU)==0x0101U);
  CHECK((b.words[0]&0x1000U)!=0U && b.ext[0x030E]==0x4A40);
  const auto hard=findWrite(b,first,31,31,0x8000);
  bool capBefore=false,advCheck=false;
  for(unsigned i=first;i<hard;++i)capBefore|=b.trace[i].kind==FrameKind::READ && b.trace[i].devad==3 && b.trace[i].target==0x14;
  const auto firstVendor=findWrite(b,first,31,0x4D1,0x008B);
  for(unsigned i=hard;i<firstVendor;++i)advCheck|=b.trace[i].kind==FrameKind::READ && b.trace[i].devad==7 && b.trace[i].target==0x3C;
  CHECK(capBefore && advCheck);
  for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==0x4D1)CHECK((b.trace[i].value&0x1000U)==0U);
  // Neither bind nor default initialization starts EEE; only explicit policy does.
  Controller defaults;Driver ordinary;ready(ordinary,defaults);
  CHECK(!ordinary.eeeSnapshot().desiredValid && defaults.hard==0U && writes(defaults,0,31,0x4D1)==0U);
  for(unsigned i=0;i<defaults.frames;++i)CHECK(defaults.trace[i].devad==31U);
}
void settingsMatrix() {
  for(bool tx:{false,true})for(bool rx:{false,true})for(uint16_t adv:{uint16_t{0},uint16_t{2}}) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.bypassBit0=tx;s.bypassBit3=rx;s.advertisement=adv;configured(d,b,s);
    CHECK(b.mmd7[0x3C]==adv);
    const auto expected=static_cast<uint16_t>((tx?0U:1U)|(rx?0U:8U));
    CHECK((b.ext[0x4D1]&0x1009U)==expected && (b.ext[0x4D1]&0xEFF6U)==0x0082U);
    CHECK(d.eeeSnapshot().desired.bypassBit0==tx && d.eeeSnapshot().desired.bypassBit3==rx);
  }
  // 10BASE-Te remains a legal AN profile, without invented 10M EEE advertisement.
  Controller b;Driver d;ready(d,b);LinkSettings ten;ten.advertisement.technologyMask=2;
  OperationToken t;CHECK(d.startConfigure(ten,{},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
  configured(d,b);CHECK((b.words[4]&0x01E0U)==0x0040U && b.ext[0x030E]==0x4A40);
}
void conflicts();
void admission() {
  conflicts();
  for(uint16_t invalid:{uint16_t{1},uint16_t{3},uint16_t{4},uint16_t{0xFFFF}}) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.advertisement=invalid;OperationToken token{999};const auto calls=b.calls,clocks=b.clocks;
    CHECK(!d.startConfigureEee(s,budget(),token).inProgress());
    CHECK(token.value==999 && b.calls==calls && b.clocks==clocks && !d.eeeSnapshot().desiredValid);
  }
  for(unsigned which=0;which<5U;++which) {
    Controller b;Driver d;if(which!=0U)initialized(d,b);
    if(which>=2U) { LinkSettings p;p.advertisement.technologyMask=8;OperationToken t;CHECK(d.startConfigure(p,{},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t); }
    auto s=settings();if(which==2U)s.maintenanceAdmitted=false;
    if(which==3U) { LinkSettings p;p.mode=LinkMode::FORCED;p.forced={LinkSpeed::MBPS_100,Duplex::FULL};OperationToken t;CHECK(d.startConfigure(p,{},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t); }
    if(which==4U) { PowerSettings power;power.activeSleep=true;power.maintenanceAdmitted=power.managementClockAvailable=true;OperationToken t;CHECK(d.startConfigurePower(power,budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t); }
    OperationToken token{999};const auto calls=b.calls,clocks=b.clocks;CHECK(!d.startConfigureEee(s,budget(),token).inProgress());
    CHECK(token.value==999 && b.calls==calls && b.clocks==clocks && !d.eeeSnapshot().desiredValid);
  }
  for(uint16_t capability:{uint16_t{0},uint16_t{3},uint16_t{0xFFFF}}) {
    Controller b;Driver d;ready(d,b);b.capability=capability;const auto mutations=b.mutations,hard=b.hard;const auto t=configure(d,b);
    CHECK(!drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(b.mutations==mutations && b.hard==hard && !r.cleanupOwed && !r.mutationCompleted && !r.mutationPossible);
  }
  Controller b;Driver d;ready(d,b);b.corruptAdvertisement=true;const auto first=b.frames;const auto t=configure(d,b);
  CHECK(!drain(d,b,t).ok());const auto r=take(d,t);
  CHECK(b.hard==1U && r.cleanupOwed && !d.isInitialized() && writes(b,first,31,0x4D1)==0U);
}
void conflicts() {
  for (bool eeeFirst:{false,true}) {
    Controller bus; bus.id2=test::basicId2(); Driver driver; ready(driver,bus);
    PinSettings pins; pins.availableMask=2; pins.pins[1].function=PinFunction::LED;
    pins.maintenanceAdmitted=pins.macStopped=pins.outputsQuiesced=true;
    PinResult result; OperationToken token{999};
    if (eeeFirst) configured(driver,bus);
    else CHECK(driver.configurePins(pins,result,{1000000,2048}).ok());
    const auto frames=bus.frames, clocks=bus.clocks;
    const auto denied=eeeFirst?driver.configurePins(pins,result,{1000000,2048}):
                               driver.startConfigureEee(settings(),budget(),token);
    CHECK(denied.is(Err::INVALID_CONFIG) && token.value==999 && bus.frames==frames && bus.clocks==clocks);
  }
  for(bool eeeFirst:{false,true}) {
    Controller b;Driver d;ready(d,b);InterruptSettings irq;irq.pin=InterruptPin::LED1;
    irq.maintenanceAdmitted=irq.pinAvailable=irq.led1ClockUnused=true;
    if(eeeFirst)configured(d,b);
    else {
      b.ext[0x0308]=1;b.ext[0x0304]=4;OperationToken t;
      CHECK(d.startConfigureInterrupts(irq,budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    }
    const auto calls=b.calls,clocks=b.clocks;OperationToken sentinel{999};
    const auto rejected=eeeFirst?d.startConfigureInterrupts(irq,budget(),sentinel):d.startConfigureEee(settings(),budget(),sentinel);
    CHECK(!rejected.inProgress() && !rejected.ok());CHECK(sentinel.value==999 && b.calls==calls && b.clocks==clocks);
  }
  Controller b;Driver d;ready(d,b);configured(d,b);const auto calls=b.calls,clocks=b.clocks;OperationToken t{999};
  LoopbackOptions loop;loop.mode=LoopbackMode::DIGITAL;loop.maintenanceAdmitted=loop.allowHardResetExit=true;
  CHECK(!d.startLoopback(loop,budget(),t).inProgress());
  LinkSettings forced;forced.mode=LinkMode::FORCED;forced.forced={LinkSpeed::MBPS_100,Duplex::FULL};
  CHECK(!d.startConfigure(forced,{},budget(),t).inProgress());
  LinkSettings an;an.advertisement.technologyMask=8;ConfigureOptions pages;pages.nextPages.pageCount=1;pages.nextPages.pages[0].message=true;pages.nextPages.pages[0].data=1;
  CHECK(!d.startConfigure(an,pages,budget(),t).inProgress());
  PowerSettings power;power.request=PowerRequest::IEEE_POWER_DOWN;power.maintenanceAdmitted=power.managementClockAvailable=power.clockLossApproved=true;
  CHECK(!d.startConfigurePower(power,budget(),t).inProgress());
  CHECK(t.value==999 && b.calls==calls && b.clocks==clocks && b.ext[0x0123]==0x0800);
}
void staleness() {
  for(unsigned change=0;change<4U;++change) {
    Controller b;Driver d;ready(d,b);configured(d,b);b.mmd7[0x3D]=2;
    EeeSnapshot before;CHECK(d.readEeeStatus(before,{1000000,31}).ok());CHECK(before.negotiatedValid && before.negotiated);
    if(change==0U) {
      b.words[1]=0;b.words[16]=0;LinkSnapshot link;CHECK(d.readLinkStatus(link,{1000000,10}).ok());
    } else if(change==1U) {
      b.failFrame=static_cast<int>(b.frames);LinkSnapshot link;CHECK(d.readLinkStatus(link,{1000000,10}).is(Err::MDIO_TIMEOUT));
    } else if(change==2U) {
      const auto frames=b.frames,clocks=b.clocks;CHECK(d.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
      CHECK(b.frames==frames && b.clocks==clocks);
    } else {
      b.mmd7[0x3D]=0;uint16_t partner=0xCAFE;CHECK(d.readMmd(7,0x3D,partner,{1000000,4}).ok());CHECK(partner==0);
    }
    const auto after=d.eeeSnapshot();CHECK(after.stale && !after.negotiatedValid);
    CHECK(after.observedAtUs==before.observedAtUs && after.partner==before.partner);
  }
}
void status() {
  staleness();
  for(uint16_t capability:{uint16_t{0},uint16_t{2}})for(uint16_t local:{uint16_t{0},uint16_t{2}})for(uint16_t partner:{uint16_t{0},uint16_t{2}}) {
    Controller bus;Driver driver;ready(driver,bus);auto requested=settings();requested.advertisement=local;configured(driver,bus,requested);
    bus.capability=capability;bus.mmd7[0x3D]=partner;
    const auto calls=bus.calls,frames=bus.frames;EeeSnapshot out;CHECK(driver.readEeeStatus(out,{1000000,31}).ok());
    CHECK(bus.frames==frames+31U && bus.calls==calls+8U);
    if (test::aGrade()) {
      constexpr unsigned indices[]={0,1,2,3,28,29,30};
      constexpr unsigned registers[]={0,16,1,1,16,1,0};
      for(unsigned i=0;i<7U;++i) CHECK(bus.trace[frames+indices[i]].reg==registers[i]);
    }
    CHECK(out.valid && !out.stale && out.capabilityValid && out.advertisementValid && out.partnerValid && out.negotiatedValid);
    CHECK(out.capability==capability && out.advertisement==local && out.partner==partner);
    CHECK(out.negotiated==(capability==2 && local==2 && partner==2));
  }
  for(uint16_t invalid:{uint16_t{3},uint16_t{6},uint16_t{0xFFFF}}) {
    Controller bus;Driver driver;ready(driver,bus);configured(driver,bus);bus.mmd7[0x3D]=2;bus.capability=invalid;
    EeeSnapshot out;CHECK(driver.readEeeStatus(out,{1000000,31}).ok());
    CHECK(out.capabilityValid && out.capability==invalid && !out.negotiatedValid && !out.negotiated);
  }
  for(uint16_t bit:{uint16_t{0x0200},uint16_t{0x0400},uint16_t{0x4000}})for(bool changed:{false,true}) {
    Controller bus;Driver driver;ready(driver,bus);configured(driver,bus);bus.mmd7[0x3D]=2;
    bus.stickyRestart=bit==0x0200U;
    if(changed) { bus.changeBmcrFrame=static_cast<int>(bus.frames+30U);bus.bmcrChange=bit; }
    else bus.words[0]|=bit;
    EeeSnapshot out;out.partner=0xCAFE;const auto status=driver.readEeeStatus(out,{1000000,31});
    if(changed) { CHECK(status.is(Err::BUSY));CHECK(out.partner==0xCAFE && driver.eeeSnapshot().stale); }
    else { CHECK(status.ok());CHECK(!out.negotiatedValid && !out.negotiated); }
  }
  Controller b;Driver d;ready(d,b);configured(d,b);b.mmd7[0x3D]=2;
  for(uint16_t state:{uint16_t{0},uint16_t{0x0100},uint16_t{0x0200},uint16_t{0x0400},uint16_t{0x0800},uint16_t{0x0F00}}) {
    b.mmd3[1]=state;b.mmd3[0x16]=0xFFFF;EeeSnapshot out;CHECK(d.readEeeStatus(out,{1000000,128}).ok());
    CHECK(out.statusValid && out.status==state && out.wakeErrorsValid && out.wakeErrors==0xFFFF);
    const auto evidence=d.evidenceSnapshot();const auto& samples=evidence.samples;
    CHECK(samples[static_cast<unsigned>(EvidenceSource::EEE_STATUS)].raw==state);
    CHECK(samples[static_cast<unsigned>(EvidenceSource::EEE_ERRORS)].raw==0xFFFF);
    CHECK(b.mmd3[0x16]==0 && (b.mmd3[1]&0x0C00U)==0U);
  }
  for(uint16_t link:{uint16_t{0},uint16_t{4},uint16_t{0x20}}) {
    b.words[1]=link;b.words[16]=static_cast<uint16_t>(((link&4U)!=0U?5U:0U)|((link&0x20U)!=0U?0x10U:0U));EeeSnapshot out;CHECK(d.readEeeStatus(out,{1000000,128}).ok());CHECK(!out.negotiatedValid && !out.negotiated);
  }
  b.words[1]=0x24;b.words[16]=0x17;EeeSnapshot slow;CHECK(d.readEeeStatus(slow,{1000000,128}).ok());CHECK(!slow.negotiated);
  const auto calls=b.calls,clocks=b.clocks;const auto a=d.eeeSnapshot(),c=d.eeeSnapshot();CHECK(a.status==c.status && b.calls==calls && b.clocks==clocks);
}
void failures() {
  Controller reference;Driver ref;ready(ref,reference);const auto first=reference.frames;configured(ref,reference);const auto count=reference.frames-first;
  CHECK(count>=74U && count<512U);
  const auto restart=findWrite(reference,first,31,0,0x3300)-first;
  for(bool accepted:{false,true})for(unsigned phase=0;phase<count;++phase) {
    Controller b;Driver d;ready(d,b);const auto start=b.frames;b.failFrame=static_cast<int>(start+phase);b.acceptedFailure=accepted;const auto t=configure(d,b);
    CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));const auto r=take(d,t);
    CHECK(r.status.detail==-29029 && b.frames==start+phase+1U && b.calls==b.releases && !b.locked);
    CHECK(!r.persistentRestored && d.eeeSnapshot().trust!=ConfigurationTrust::VERIFIED);
    CHECK(r.eee.recipeRestartAttempted==(phase>=restart));
    CHECK(r.eee.recipeRestartAccepted==(phase>restart));
    CHECK(r.eee.recipeRestartPossible==(phase>restart || (phase==restart && accepted)));
    if(b.hard!=0U)CHECK(r.cleanupOwed && !d.isInitialized());
    if (test::aGrade() && phase>=restart && phase<=restart+4U) {
      // A row 26 is direct. All four selector/data cuts of row 27 occur
      // after the accepted AN restart. Detachment requires explicit abandonment.
      const auto calls=b.calls, clocks=b.clocks;
      CHECK(d.end().is(Err::BUSY));
      CHECK(d.abandonCleanup().ok() && d.end().ok());
      CHECK(r.eee.recipeRestartAccepted==(phase>restart));
      CHECK(r.eee.recipeRestartPossible==(phase>restart || accepted));
      CHECK(b.calls==calls && b.clocks==clocks);
    }

  }
  // Every release failure retains primary/release separation and consumes no retry.
  Controller trace;Driver ok;ready(ok,trace);const auto call=trace.calls;configured(ok,trace);const auto calls=trace.calls-call;
  for(unsigned phase=0;phase<calls;++phase) {
    Controller b;Driver d;ready(d,b);b.releaseCall=static_cast<int>(b.calls+phase);const auto t=configure(d,b);
    CHECK(drain(d,b,t).is(Err::MDIO_ERROR));const auto r=take(d,t);CHECK(r.status.detail==-37 && !r.persistentRestored);
  }
}
void acquisitionFailures() {
  {
    Controller b; Driver d; ready(d,b); configured(d,b);
    EeeSnapshot sample; CHECK(d.readEeeStatus(sample,{1000000,31}).ok());
    const auto retained=d.eeeSnapshot();
    b.badReceiptCall=static_cast<int>(b.calls);
    uint16_t id=0xCAFE; CHECK(!d.readClause22(2,id,{1000000,1}).ok());
    const auto frames=b.frames,clocks=b.clocks; sample.partner=0xCAFE;
    const auto status=d.readEeeStatus(sample,{1000000,31});
    CHECK(status.is(Err::BUSY) && status.detail==static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED));
    const auto cached=d.eeeSnapshot();
    CHECK(sample.partner==0xCAFE && cached.stale && !cached.negotiatedValid);
    CHECK(cached.lastStatus.code==status.code && cached.lastStatus.detail==status.detail);
    CHECK(cached.observedAtUs==retained.observedAtUs && cached.timeValid==retained.timeValid);
    CHECK(cached.partner==retained.partner && cached.status==retained.status);
    CHECK(b.frames==frames && b.clocks==clocks);
  }
  // A valid contradictory control word remains evidence even if the next
  // transaction fails; aggregate failure cannot preserve obsolete trust.
  for(bool control:{false,true})for(bool release:{false,true}) {
    Controller b;Driver d;ready(d,b);configured(d,b);
    if(control)b.ext[0x04D1]&=0xFFFE;else b.mmd7[0x3C]=0;
    if(release)b.releaseCall=static_cast<int>(b.calls+(control?6U:3U));
    else b.failFrame=static_cast<int>(b.frames+(control?28U:16U));
    EeeSnapshot sentinel;sentinel.partner=0xCAFE;
    CHECK(d.readEeeStatus(sentinel,{1000000,31}).is(release?Err::MDIO_ERROR:Err::MDIO_TIMEOUT));
    CHECK(sentinel.partner==0xCAFE && d.eeeSnapshot().trust==ConfigurationTrust::DIRTY && d.eeeSnapshot().stale);
    CHECK(control?d.eeeSnapshot().controlValid:d.eeeSnapshot().advertisementValid);
  }
  Controller reference;Driver ref;ready(ref,reference);configured(ref,reference);EeeSnapshot result;const auto start=reference.frames;
  CHECK(ref.readEeeStatus(result,{1000000,128}).ok());const auto count=reference.frames-start;CHECK(count<128U);
  for(unsigned phase=0;phase<count;++phase) {
    Controller b;Driver d;ready(d,b);configured(d,b);b.mmd3[1]=0x0F00;b.mmd3[0x16]=7;
    b.failFrame=static_cast<int>(b.frames+phase);const auto first=b.frames;EeeSnapshot sentinel;sentinel.partner=0xCAFE;
    CHECK(d.readEeeStatus(sentinel,{1000000,128}).is(Err::MDIO_TIMEOUT));CHECK(sentinel.partner==0xCAFE && b.frames==first+phase+1U);
    CHECK(d.eeeSnapshot().stale && !d.eeeSnapshot().negotiatedValid);
    const auto evidence=d.evidenceSnapshot();
    if (test::aGrade() && phase==2U) {
      const auto& mirror=evidence.samples[static_cast<unsigned>(EvidenceSource::PHYSTS)];
      CHECK(mirror.valid && mirror.raw==0x15U);
    }
    for(unsigned i=first;i<b.frames;++i) {
      if(b.trace[i].kind==FrameKind::READ && b.trace[i].devad==3 && (b.trace[i].target==1 || b.trace[i].target==0x16)) {
        const auto source=b.trace[i].target==1?EvidenceSource::EEE_STATUS:EvidenceSource::EEE_ERRORS;
        const auto& sample=evidence.samples[static_cast<unsigned>(source)];
        if(i+1U==b.frames)CHECK(sample.historyLost);else CHECK(sample.valid);
      }
    }
  }
}
void restoreFailures() {
  // Disable and every reset request share one finite HARD/replay. Inject every
  // frame independently so earlier accepted effects cannot become rollback.
  for(unsigned action=0;action<5U;++action) {
    const auto begin=[action](Driver& driver,OperationToken& token) {
      if(action==0U)return driver.startConfigureEee(settings(false),budget(),token);
      constexpr ResetKind resets[]={ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART};
      return driver.startInitialize({resets[action-1U]},budget(),token);
    };
    Controller reference;Driver ref;ready(ref,reference);configured(ref,reference);const auto first=reference.frames;OperationToken token;
    CHECK(begin(ref,token).inProgress());CHECK(drain(ref,reference,token).ok());take(ref,token);const auto count=reference.frames-first;
    for(unsigned phase=0;phase<count;++phase) {
      Controller b;Driver d;ready(d,b);configured(d,b);const auto start=b.frames,hard=b.hard;
      b.failFrame=static_cast<int>(start+phase);OperationToken t;CHECK(begin(d,t).inProgress());
      CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));const auto failed=take(d,t);
      CHECK(failed.status.detail==-29029 && !failed.persistentRestored && b.frames==start+phase+1U);
      if(b.hard>hard)CHECK(failed.cleanupOwed && !d.isInitialized());
    }
  }
  // Retained failure and restoration are different tokens/results.
  Controller b;Driver d;ready(d,b);const auto first=b.frames;const auto t=configure(d,b);
  for(unsigned i=0;i<128U && b.ext[0x0123]!=0x0800;++i) CHECK(poll(d,b,t).inProgress());
  CHECK(b.ext[0x0123]==0x0800);
  b.failFrame=static_cast<int>(b.frames);CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));const auto failed=take(d,t);
  CHECK(failed.cleanupOwed && writes(b,first,31,0x0123)==1U);b.failFrame=-1;OperationToken recovery;
  CHECK(d.startInitialize({ResetKind::HARD},budget(),recovery).inProgress());CHECK(drain(d,b,recovery).ok());const auto restored=take(d,recovery);
  CHECK(recovery.value!=t.value && restored.persistentRestored && !restored.cleanupOwed && failed.status.is(Err::MDIO_TIMEOUT));
}
void replay() {
  restoreFailures();
  {
    Controller b;Driver d;ready(d,b);InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=true;
    OperationToken irqToken;CHECK(d.startConfigureInterrupts(irq,budget(),irqToken).inProgress());CHECK(drain(d,b,irqToken).ok());take(d,irqToken);
    configured(d,b);const auto hard=b.hard;OperationToken t;CHECK(d.startInitialize({ResetKind::HARD},budget(),t).inProgress());
    for(unsigned n=0;n<128U && (b.hard==hard || d.eeeSnapshot().trust!=ConfigurationTrust::VERIFIED);++n)CHECK(poll(d,b,t).inProgress());
    CHECK(d.operationSnapshot().state==OperationState::RUNNING && b.hard==hard+1U);
    b.mmd7[0x3C]=0;uint16_t raw=0xCAFE;CHECK(d.readMmd(7,0x3C,raw,{1000000,4}).ok());CHECK(raw==0 && d.eeeSnapshot().trust==ConfigurationTrust::DIRTY);
    CHECK(!drain(d,b,t).ok());const auto result=take(d,t);
    CHECK(!result.persistentRestored && result.cleanupOwed && !d.isInitialized());
  }
  for(ResetKind reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
    Controller b;Driver d;ready(d,b);configured(d,b);const auto hard=b.hard;OperationToken t;
    CHECK(d.startInitialize({reset},budget(),t).inProgress());CHECK(drain(d,b,t).ok());const auto r=take(d,t);
    CHECK(r.reset==ResetKind::HARD && b.hard==hard+1U && r.persistentRestored);
    CHECK(d.eeeSnapshot().trust==ConfigurationTrust::VERIFIED && b.ext[0x123]==0x0800 && b.ext[0x030E]==0x4A40);
  }
  Controller b;Driver d;ready(d,b);configured(d,b);const auto first=b.frames,hard=b.hard;configured(d,b,settings(false));
  CHECK(b.hard==hard+1U && b.mmd7[0x3C]==0 && writes(b,first,31,0x123)==0U);
  CHECK(!d.eeeSnapshot().desired.enabled && (b.ext[0x4D1]&0x1009U)==0U);
  const auto after=b.frames;OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
  CHECK(writes(b,after,31,0x123)==0U && b.hard==hard+2U);
}
void recipeAdmission() {
  constexpr uint64_t deadline = 1000000;
  // Identity (2), SOR1/SOR2 (4 each), BMCR and PHYRCR (1 each) precede
  // capability. Only the remaining frames are available to the recipe.
  constexpr unsigned preflightFrames = 12;
  const unsigned requiredFrames=test::aGrade()?178U:75U;
  const unsigned requiredCalls=test::aGrade()?59U:21U;
  for (bool replay : {false, true}) {
    for (bool command : {false, true}) {
      for (unsigned slack : {0U, 1U}) {
        Controller bus;
        Driver driver;
        ready(driver, bus);
        OperationToken token;
        const auto allowance = budget(preflightFrames + requiredFrames, deadline);
        if (replay) {
          configured(driver, bus);
          CHECK(driver.startInitialize({ResetKind::NONE}, allowance, token).inProgress());
        } else {
          CHECK(driver.startConfigureEee(settings(), allowance, token).inProgress());
        }
        const auto first = bus.frames;
        for (unsigned i = 0; i < 5; ++i) { CHECK(poll(driver, bus, token).inProgress()); }
        CHECK(bus.frames == first + preflightFrames);
        if (command) {
          CHECK(poll(driver, bus, token).inProgress());
          literalMmd(bus, first + preflightFrames, 3, 0x14, FrameKind::READ);
        }
        const auto frames = bus.frames, calls = bus.calls, hard = bus.hard;
        CHECK(driver.operationSnapshot().remaining.maxFrames == requiredFrames - (command ? 4U : 0U));
        // 4 ticks + 4 us/frame + 8 us/callback + HARD's 2000 us wait.
        // A reserves the full re-identification, VOD, AN and EEE replay tail.
        bus.jumpClockCall = bus.clocks + 2;
        bus.jumpClockTo = deadline - (2004U + 4U * requiredFrames + 8U * requiredCalls - (command?24U:0U)) - slack;
        const auto status = poll(driver, bus, token);
        CHECK(bus.clocks >= bus.jumpClockCall);
        if (slack == 0U) {
          CHECK(status.is(Err::OPERATION_TIMEOUT) && status.detail == static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
          CHECK(bus.frames == frames && bus.calls == calls && bus.hard == hard);
          const auto result = take(driver, token);
          CHECK(!result.commandAccepted && !result.mutationAttempted && !result.persistentRestored);
          if (command) { CHECK(result.eee.capabilityValid && result.eee.capability == 2); }
        } else {
          CHECK(status.inProgress() && bus.calls == calls + 1);
          if (command) {
            CHECK(bus.frames == frames + 1 && bus.hard == hard + 1);
            CHECK(bus.trace[frames].kind == FrameKind::WRITE && bus.trace[frames].reg == 0x1F && bus.trace[frames].value == 0x8000);
          } else {
            CHECK(bus.frames == frames + 4 && bus.hard == hard);
            literalMmd(bus, frames, 3, 0x14, FrameKind::READ);
          }
          CHECK(driver.cancel(token).ok());
          take(driver, token);
        }
      }
    }
  }
  Controller bus;
  Driver driver;
  ready(driver, bus);
  OperationToken token;
  CHECK(driver.startConfigureEee(settings(), budget(preflightFrames + requiredFrames - 1U, deadline), token).inProgress());
  for (unsigned i = 0; i < 5; ++i) { CHECK(poll(driver, bus, token).inProgress()); }
  CHECK(driver.operationSnapshot().remaining.maxFrames == requiredFrames - 1U);
  const auto frames = bus.frames, calls = bus.calls;
  CHECK(poll(driver, bus, token, 3).inProgress()); // Yield before admitting the four-frame capability read.
  const auto status = poll(driver, bus, token, 4);
  CHECK(status.is(Err::OPERATION_TIMEOUT) && status.detail == static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
  CHECK(bus.frames == frames && bus.calls == calls && bus.hard == 0 && !take(driver, token).mutationAttempted);
}

void aOwnerAdmission() {
  if (!test::aGrade()) return;
  // At capability entry: 111/30 for recipe, checks and HARD poll, 10/3 to
  // identify and resample straps, 17/4 to apply EEE, 8/2 VOD, 24/18 link,
  // and 8/2 for final EEE verification. The leading preflight costs 12/5.
  for(unsigned owners=0;owners<8U;++owners) for(bool shortBudget:{false,true}) {
    Controller b; Driver d; ready(d,b); unsigned frames=190;
    OperationToken token;
    if ((owners&1U)!=0U) {
      PowerSettings power; power.maintenanceAdmitted=power.managementClockAvailable=true;
      CHECK(d.startConfigurePower(power,budget(),token).inProgress());
      CHECK(drain(d,b,token).ok()); take(d,token); frames+=9;
    }
    if ((owners&2U)!=0U) {
      AnalogSettings analog; analog.terminationOwned=analog.terminationOverrideAdmitted=true;
      analog.macStopped=analog.maintenanceAdmitted=analog.fixtureAdmitted=true;
      analog.mdiTermination=6;
      CHECK(d.startConfigureAnalog(analog,budget(),token).inProgress());
      CHECK(drain(d,b,token).ok()); take(d,token); frames+=14;
    }
    if ((owners&4U)!=0U) {
      QualitySettings quality; quality.tx10.owned=quality.partnerPreambleAdmitted=true;
      quality.maintenanceAdmitted=quality.macStopped=true; quality.tx10.squelchCode=3;
      CHECK(d.startConfigureQuality(quality,budget(),token).inProgress());
      CHECK(drain(d,b,token).ok()); take(d,token); frames+=19;
    }
    const auto first=b.frames, mutations=b.mutations;
    CHECK(d.startConfigureEee(settings(),budget(frames-(shortBudget?1U:0U)),token).inProgress());
    const auto status=drain(d,b,token);
    const auto r=take(d,token);
    if(shortBudget) {
      CHECK(status.is(Err::OPERATION_TIMEOUT) && b.frames==first+12 && b.mutations==mutations);
      CHECK(!r.eee.recipeRestartAttempted && !r.mutationPossible);
    } else {
      CHECK(status.ok() && r.persistentRestored && b.frames==first+frames);
      CHECK(r.eee.recipeRestartAccepted && r.eee.recipeRestartPossible);
      // A new failure before capability must not inherit the previous restart.
      b.failFrame=static_cast<int>(b.frames);
      CHECK(d.startConfigureEee(settings(false),budget(),token).inProgress());
      CHECK(drain(d,b,token).is(Err::MDIO_TIMEOUT));
      CHECK(!take(d,token).eee.recipeRestartPossible && r.eee.recipeRestartAccepted);
    }
  }
}

void ownershipTiming() {
  recipeAdmission();
  aOwnerAdmission();
  for(uint32_t allowance:{0U,73U}) {
    Controller bus;Driver driver;ready(driver,bus);const auto mutation=bus.mutations;OperationToken t{999};
    const auto result=driver.startConfigureEee(settings(),budget(allowance),t);
    if(result.inProgress()) { CHECK(!drain(driver,bus,t).ok());take(driver,t); }
    else CHECK(!result.ok());
    CHECK(bus.hard==0U && bus.mutations==mutation);
  }
  for(unsigned fault=0;fault<3U;++fault) {
    Controller bus;Driver driver;ready(driver,bus);const auto t=configure(driver,bus);
    if(fault==0U)bus.regressCall=static_cast<int>(bus.calls);
    if(fault==1U)bus.lateCall=static_cast<int>(bus.calls);
    if(fault==2U)bus.now=std::numeric_limits<uint64_t>::max();
    CHECK(!drain(driver,bus,t).ok());const auto r=take(driver,t);
    CHECK(!r.persistentRestored && bus.hard==0U);
    if(fault!=1U)CHECK(r.clockInvalid);
  }
  {
    Controller bus;Driver driver;ready(driver,bus);const auto t=configure(driver,bus);
    for(unsigned n=0;n<32U && bus.hard==0U;++n)CHECK(poll(driver,bus,t).inProgress());
    CHECK(bus.hard==1U);const auto floor=driver.operationSnapshot().busQuietUntilUs;
    CHECK(driver.cancel(t).ok());take(driver,t);const auto calls=bus.calls;
    uint16_t sentinel=0xCAFE;CHECK(driver.readMmd(3,0x14,sentinel,{1000000,4}).is(Err::BUSY));
    CHECK(sentinel==0xCAFE && bus.calls==calls && driver.operationSnapshot().busQuietUntilUs==floor);
    bus.now=floor;CHECK(driver.readMmd(3,0x14,sentinel,{1000000,4}).ok());CHECK(sentinel==2);
  }
  {
    Controller bus;Driver driver;ready(driver,bus);bus.frameTime=0;const auto t=configure(driver,bus);
    for(unsigned n=0;n<32U && bus.hard==0U;++n)CHECK(driver.pollOperation(t).inProgress());
    CHECK(bus.hard==1U);const auto calls=bus.calls;Status status{Err::IN_PROGRESS,0,"Pending"};
    for(unsigned n=0;n<16U && status.inProgress();++n)status=driver.pollOperation(t);
    CHECK(status.is(Err::OPERATION_TIMEOUT) && bus.calls==calls);CHECK(take(driver,t).cleanupOwed);
  }
  Controller b;Driver d,other;ready(d,b);CHECK(other.bind(b.config(2)).ok());b.competitor=&other;
  configured(d,b);CHECK(b.excluded>0U && b.releases==b.calls && !b.locked);b.competitor=nullptr;
  for(unsigned slice=0;slice<4U;++slice) {
    Controller bus;Driver driver;ready(driver,bus);const auto t=configure(driver,bus);
    if(slice>=2U)CHECK(poll(driver,bus,t).inProgress()); // identity precedes four-frame SOR
    const auto first=bus.frames;
    // The next complete transaction cannot be split by a short poll slice.
    CHECK(poll(driver,bus,t,slice).inProgress());CHECK(bus.frames==first);CHECK(driver.cancel(t).ok());take(driver,t);
  }
  Controller reference;Driver ref;ready(ref,reference);const auto base=reference.calls;configured(ref,reference);const auto calls=reference.calls-base;
  for(unsigned phase=0;phase<calls;++phase) {
    Controller bus;Driver driver;ready(driver,bus);const auto t=configure(driver,bus);
    for(unsigned n=0;n<phase && driver.operationSnapshot().state==OperationState::RUNNING;++n)CHECK(poll(driver,bus,t).inProgress());
    const auto frames=bus.frames,clocks=bus.clocks;CHECK(driver.cancel(t).ok());const auto r=take(driver,t);
    CHECK(r.status.is(Err::CANCELLED) && bus.frames==frames && bus.clocks==clocks && !r.persistentRestored);
    if(bus.hard!=0U)CHECK(r.cleanupOwed);
  }
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"raw",raw},{"recipe",recipe},{"settings",settingsMatrix},{"admission",admission},
    {"status",status},{"failures",failures},{"acquisition_failures",acquisitionFailures},{"replay",replay},{"ownership_timing",ownershipTiming}};
  return test::runVariantSuites(argc,argv,suites);
}
