#include "support/PartSelection.h"
#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include "../examples/portable/Quality.h"
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target, devad; uint64_t at; };

// Independent SNLS647I tables 8-28..30/35/44/51..53/72 original PDF literals.
// The fake models consumption, not physical latency, thresholds or SNR.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<uint16_t,64> mmd3{},mmd7{};
  std::array<Wire,8192> trace{};
  uint16_t selector{0},address{0},id2{test::enhancedId2()};
  Variant declared{test::declaredVariant};
  MacInterface boardInterface{MacInterface::MII};
  uint64_t now{100};
  unsigned frames{0},calls{0},clocks{0},releases{0},mutations{0},hard{0};
  std::array<unsigned,2048> firstFrame{};
  uint16_t capability{2}; bool corruptAdvertisement{false};
  bool peerPages{false}; // Inject one base event and the peer's final next page.
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
    ext[0x117]=0x8147;ext[0x131]=0x2284;ext[0x2A]=0x7998;ext[0x030E]=0x8400;ext[0x04A1]=0x1000;ext[0x04D1]=0x0080;
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=declared;c.interface=boardInterface;c.phyAddress=phy;
    if (isDp83822(declared)) c.dp83822Led1Mode=1;
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
    if (peerPages && ((target==0 && (value&0x0200U)!=0U) || target==7)) words[6]=0x000F;
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
        if (!q.acceptsRead(r.frames[q.readIndex].value)) {
          r.status=Status::Error(Err::CONFIG_MISMATCH,"Read prerequisite changed");
          break;
        }
        value=static_cast<uint16_t>((r.frames[q.readIndex].value&q.preserveMask)|q.value);
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
  b.words[23]=b.boardInterface==MacInterface::MII?0x0041:0x0061;
  CHECK(d.bind(b.config()).ok());OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}
void ready(Driver& d,Controller& b) {
  initialized(d,b);LinkSettings profile;profile.advertisement.technologyMask=8;
  OperationToken t;CHECK(d.startConfigure(profile,{},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
}

QualitySettings settings(bool all=true) {
  QualitySettings s;s.maintenanceAdmitted=s.macStopped=s.partnerPreambleAdmitted=true;
  s.fastDrop.owned=true;s.fastDrop.causeMask=0x040F;
  if(all) {
    s.tx100.owned=s.tx100.scramblerBypass=s.tx100.rxErrorDuringIdle=true;s.tx100.fifoDepthCode=3;
    s.tx10.owned=s.tx10.lowerThreshold=s.tx10.nlpDisable=s.tx10.jabberDisable=s.tx10.shortPreamble=true;s.tx10.squelchCode=8;
    s.recoveryOwned=s.linkRecovery=true;
  }
  return s;
}
OperationToken start(Driver& d,Controller& b,const QualitySettings& s) {
  OperationToken t;const auto frames=b.frames,clocks=b.clocks;
  const auto status=d.startConfigureQuality(s,budget(),t);
  if(!status.inProgress())std::fprintf(stderr,"quality start %d/%d: %s\n",static_cast<int>(status.code),status.detail,status.msg);
  CHECK(status.inProgress() && b.frames==frames && b.clocks==clocks);return t;
}
OperationResult configured(Driver& d,Controller& b,const QualitySettings& s) {
  const auto t=start(d,b,s);const auto status=drain(d,b,t);
  if(!status.ok())std::fprintf(stderr,"quality finish %d/%d: %s\n",static_cast<int>(status.code),status.detail,status.msg);
  CHECK(status.ok());const auto r=take(d,t);
  CHECK(r.status.ok() && r.persistentRestored && !r.cleanupOwed);
  const auto q=d.qualitySnapshot();CHECK(q.desiredValid && q.trust==ConfigurationTrust::VERIFIED && !q.observedStale && q.timeValid);
  return r;
}
bool semantic(const Wire& w) { return w.kind==FrameKind::WRITE && w.reg!=13 && !(w.reg==14 && w.target==14); }

MacInterface p22Interface=MacInterface::MII;

void startP22(Driver& driver,Controller& bus) {
  bus.boardInterface=p22Interface;
  bus.id2=0xA240;
  bus.ext[0x0467]=p22Interface==MacInterface::MII ? 0x0FC3 :
      p22Interface==MacInterface::RMII ? 0x0FF3 : 0x0DC3;
  initialized(driver,bus);
  CHECK(driver.isInitialized() && bus.mutations==0);
}

OperationToken p22StartPolicy(Driver& driver,bool quality) {
  OperationToken token;
  if (quality) {
    QualitySettings target;
    target.maintenanceAdmitted=target.macStopped=target.fastDrop.owned=target.tx10.owned=true;
    target.fastDrop.causeMask=0x0409; target.tx10.squelchCode=5;
    CHECK(driver.startConfigureQuality(target,budget(),token).inProgress());
  } else {
    LinkSettings target;
    target.advertisement={15,true,false,true};
    target.crossover.swapPolarity=true;
    CHECK(driver.startConfigure(target,{},budget(),token).inProgress());
  }
  return token;
}

void p22InterruptPolicy(bool quality) {
  Controller baseline; Driver normal; startP22(normal,baseline);
  const auto first=baseline.frames,firstCall=baseline.calls;
  const auto good=p22StartPolicy(normal,quality);
  CHECK(drain(normal,baseline,good).ok() && take(normal,good).persistentRestored);
  const auto frames=baseline.frames-first,calls=baseline.calls-firstCall;
  CHECK(frames==(quality?13U:24U));
  unsigned firstWrite=0;
  while (firstWrite<frames && baseline.trace[first+firstWrite].kind==FrameKind::READ) ++firstWrite;
  CHECK(firstWrite<frames);
  for (unsigned cut=0;cut<frames;++cut) {
    Controller bus; Driver driver; startP22(driver,bus);
    bus.failFrame=static_cast<int>(bus.frames+cut);
    const auto token=p22StartPolicy(driver,quality);
    CHECK(drain(driver,bus,token).is(Err::MDIO_TIMEOUT));
    const auto failed=take(driver,token);
    CHECK(failed.status.detail==-33033 && !failed.persistentRestored);
    CHECK(failed.cleanupOwed==(cut>=firstWrite) && bus.calls==bus.releases);
    const auto count=bus.calls, clocks=bus.clocks;
    if (quality) CHECK(driver.qualitySnapshot().observedStale);
    else CHECK(driver.settingsSnapshot().observedStale);
    CHECK(count==bus.calls && clocks==bus.clocks);
  }
  for (unsigned cut=0;cut<calls;++cut) {
    Controller bus; Driver driver; startP22(driver,bus);
    bus.releaseCall=static_cast<int>(bus.calls+cut);
    const auto token=p22StartPolicy(driver,quality);
    CHECK(drain(driver,bus,token).is(Err::MDIO_ERROR));
    const auto failed=take(driver,token);
    CHECK(failed.failingBatch.release.detail==-37 && !failed.persistentRestored);
  }
  // A sampled copper declaration can be contradicted by a later CR2 read.
  // Neither policy issues its first write after that new observation.
  {
    Controller fx; Driver driver; startP22(driver,fx);
    fx.words[10]=0x4000;
    const auto token=p22StartPolicy(driver,quality);
    CHECK(!drain(driver,fx,token).ok());
    CHECK(!take(driver,token).cleanupOwed && fx.mutations==0);
  }
  Controller bus; Driver driver; startP22(driver,bus);
  auto token=p22StartPolicy(driver,quality);
  for (unsigned step=0;step<calls && !bus.mutations;++step) CHECK(poll(driver,bus,token).inProgress());
  CHECK(bus.mutations==1);
  const auto count=bus.calls;
  CHECK(driver.cancel(token).ok() && bus.calls==count);
  CHECK(take(driver,token).cleanupOwed);
  CHECK(driver.startInitialize({ResetKind::NONE},budget(),token).inProgress() && drain(driver,bus,token).ok());
  CHECK(take(driver,token).persistentRestored && !driver.operationSnapshot().cleanupOwed);
  if (quality) CHECK((bus.words[11]&0x040FU)==0x0409 && (bus.words[26]&0x1E00U)==0x0A00);
  else CHECK((bus.words[4]&0xADFFU)==0x25E1 && (bus.words[11]&0x0060U)==0x40);
}

void p22Copper() {
  // SNLS505H tables 8-1, 8-5..12, 8-16, 8-25/26. Inspect literal bus
  // targets and data, independently of command constants and register maps.
  Controller bus; Driver driver;
  startP22(driver,bus);
  if (!driver.isInitialized()) return;
  OperationToken token;
  LinkSettings profile;
  profile.crossover.robust=false;
  for (uint8_t mask=1;mask<=15;++mask) {
    profile.advertisement={mask,(mask&1U)!=0,(mask&2U)!=0,(mask&4U)!=0};
    profile.fastAutoNegotiation=profile.peerFastModeConfirmed=true;
    profile.fastAutoNegotiationCode=static_cast<uint8_t>(mask%3);
    profile.crossover.swapPair=(mask&1U)!=0;
    profile.crossover.swapPolarity=(mask&2U)!=0;
    profile.fastParallelDetect=(mask&8U)!=0;
    CHECK(driver.startConfigure(profile,{},budget(),token).inProgress());
    CHECK(drain(driver,bus,token).ok() && take(driver,token).persistentRestored);
    const auto snap=driver.settingsSnapshot();
    CHECK(snap.trust==ConfigurationTrust::VERIFIED && snap.observedValid);
    CHECK(snap.observed.advertisement.remoteFault==profile.advertisement.remoteFault);
    CHECK(snap.observed.crossover.swapPolarity==profile.crossover.swapPolarity &&
          snap.observed.crossover.swapPair==profile.crossover.swapPair);
    CHECK(bus.words[4]==static_cast<uint16_t>((static_cast<unsigned>(mask)<<5U)|1U|((mask&1U)?0x0400U:0U)|
          ((mask&2U)?0x0800U:0U)|((mask&4U)?0x2000U:0U)));
    CHECK((bus.words[9]&0x007C)==static_cast<uint16_t>(0x0010U|((mask%3U)<<2U)));
    CHECK((bus.words[10]&0x0060)==((mask&8U)?0x0040:0));
    CHECK((bus.words[11]&0x0060)==(((mask&1U)?0x0020:0)|((mask&2U)?0x0040:0)));
  }
  profile={};
  profile.advertisement.technologyMask=15;
  bus.peerPages=true; bus.words[5]=0xC101; bus.words[8]=0x7801;
  ConfigureOptions pages;
  pages.nextPages.pageCount=1; pages.nextPages.pages[0]={1,true,true};
  CHECK(driver.startConfigure(profile,pages,budget(),token).inProgress());
  CHECK(drain(driver,bus,token).ok());
  const auto page=take(driver,token);
  CHECK(page.nextPage.complete && page.nextPage.basePage==0xC101 && page.nextPage.receivedPages==1 &&
        page.nextPage.pages[0].valid && page.nextPage.pages[0].raw==0x7801 && bus.words[7]==0x3001);
  CHECK((bus.words[4]&0x8000U)==0U && (bus.words[10]&0x0040U)!=0U);
  const auto firstRestart=bus.frames;
  CHECK(driver.restartAutoNegotiation({1000000,3}).ok());
  CHECK(bus.frames-firstRestart==3U && bus.trace[firstRestart+1].kind==FrameKind::WRITE &&
        bus.trace[firstRestart+1].reg==0 && bus.trace[firstRestart+1].value==0x1200);
  pages.nextPages.pages[0]={4,true,false}; // Missing unformatted page after remote-fault message.
  const auto prior=bus.calls;
  CHECK(driver.startConfigure(profile,pages,budget(),token).is(Err::INVALID_CONFIG) && bus.calls==prior);

  for (unsigned invalid=0;invalid<4;++invalid) {
    auto request=profile;
    request.fastAutoNegotiation=true; request.peerFastModeConfirmed=true;
    if (invalid==0) request.fastAutoNegotiationCode=3;
    if (invalid==1) request.peerFastModeConfirmed=false;
    if (invalid==2) request.crossover.fast=true;
    if (invalid==3) { request.fastAutoNegotiation=false; request.fastAutoNegotiationCode=1; }
    const auto calls=bus.calls,clocks=bus.clocks;
    CHECK(driver.startConfigure(request,{},budget(),token).is(Err::INVALID_CONFIG));
    CHECK(bus.calls==calls && bus.clocks==clocks);
  }
  profile={}; profile.mode=LinkMode::FORCED; profile.crossover.robust=false;
  for (const auto speed : {LinkSpeed::MBPS_10,LinkSpeed::MBPS_100}) {
    for (const auto duplex : {Duplex::HALF,Duplex::FULL}) {
      profile.forced={speed,duplex};
      for (const auto route : {PairSelection::MDI,PairSelection::MDIX,PairSelection::AUTOMATIC}) {
        profile.crossover.pairSelection=route;
        profile.crossover.fast=profile.peerFastModeConfirmed=(route==PairSelection::AUTOMATIC && speed==LinkSpeed::MBPS_100);
        CHECK(driver.startConfigure(profile,{},budget(),token).inProgress());
        CHECK(drain(driver,bus,token).ok() && take(driver,token).persistentRestored);
        CHECK((bus.words[0]&0x3100U)==((speed==LinkSpeed::MBPS_100?0x2000U:0U)|(duplex==Duplex::FULL?0x100U:0U)));
        CHECK((bus.words[9]&0x7CU)==(profile.crossover.fast?0x40U:0U));
        CHECK((bus.words[25]&0xC000U)==(route==PairSelection::MDI?0U:route==PairSelection::MDIX?0x4000U:0xC000U));
      }
    }
  }
  QualitySettings quality;
  quality.maintenanceAdmitted=quality.macStopped=true;
  quality.fastDrop.owned=quality.tx10.owned=quality.tx100.owned=quality.recoveryOwned=true;
  quality.linkRecovery=quality.tx100.forceFarEndDrop=true;
  quality.tx10.lowerThreshold=quality.tx10.nlpDisable=quality.tx10.jabberDisable=true;
  for (unsigned causes=0;causes<32;++causes) {
    quality.fastDrop.causeMask=static_cast<uint16_t>((causes&15U)|((causes&16U)?0x0400U:0U));
    quality.tx10.squelchCode=static_cast<uint8_t>(causes%9U);
    bus.words[26]=0x0010; bus.words[16]|=0x1000;
    const auto first=bus.frames;
    configured(driver,bus,quality);
    CHECK((bus.words[10]&0x8000U)!=0 && (bus.words[9]&0x0080U)!=0);
    CHECK((bus.words[11]&0x040FU)==quality.fastDrop.causeMask);
    CHECK(bus.words[26]==static_cast<uint16_t>(0x2081U|((causes%9U)<<9U)));
    for (unsigned i=first;i<bus.frames;++i) {
      const auto& frame=bus.trace[i];
      CHECK(frame.reg!=13 && frame.reg!=14);
      if (frame.kind==FrameKind::WRITE)
        CHECK(frame.reg==0x1A || frame.reg==0x0A || frame.reg==9 || frame.reg==11);
      if (frame.kind==FrameKind::READ && frame.reg==0x1A)
        CHECK(i>first && bus.trace[i-1].kind==FrameKind::READ && bus.trace[i-1].reg==0x10);
    }
  }
  const auto calls=bus.calls,clocks=bus.clocks;
  for (uint8_t code=9;code<=15;++code) {
    quality.tx10.squelchCode=code;
    CHECK(driver.startConfigureQuality(quality,budget(),token).is(Err::INVALID_PARAM));
  }
  quality.tx10.squelchCode=0; quality.fastDrop.rawThresholdOverride=true;
  CHECK(driver.startConfigureQuality(quality,budget(),token).is(Err::UNSUPPORTED));
  profile.forced.speed=LinkSpeed::MBPS_10; profile.crossover.fast=profile.peerFastModeConfirmed=false;
  CHECK(driver.startConfigure(profile,{},budget(),token).is(Err::INVALID_CONFIG));
  CHECK(calls==bus.calls && clocks==bus.clocks);
  p22InterruptPolicy(false);
  p22InterruptPolicy(true);
}
unsigned writes(const Controller& b,unsigned first,uint16_t target) {
  unsigned n=0;for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i]) && b.trace[i].target==target)++n;return n;
}
void vectors() {
  for(unsigned bits=0;bits<32U;++bits) {
    Controller b;Driver d;ready(d,b);auto s=settings(false);
    s.fastDrop.causeMask=static_cast<uint16_t>((bits&15U)|((bits&16U)!=0U?0x0400U:0U));
    if (test::aGrade() && (bits & 0x16U)!=0U && (bits & 9U)==0U) {
      const auto frames=b.frames; OperationToken token;
      CHECK(d.startConfigureQuality(s,budget(),token).is(Err::INVALID_CONFIG) && frames==b.frames);
      continue;
    }
    b.words[11]=0xA170;const auto first=b.frames;configured(d,b,s);
    CHECK(b.words[11]==static_cast<uint16_t>(0xA170U|s.fastDrop.causeMask));
    CHECK(writes(b,first,11)==(bits==0U?1U:2U));
    CHECK(writes(b,first,0x117)==((bits&4U)!=0U?1U:0U));
    CHECK(writes(b,first,0x131)==((bits&16U)!=0U?1U:0U));
    if((bits&4U)!=0U)CHECK(b.ext[0x117]==0x0417U);
    if((bits&16U)!=0U)CHECK(b.ext[0x131]==0x2288U);
    const bool maxima=bits!=0U && ((bits&0x16U)==0U || (bits&9U)!=0U);
    CHECK(d.qualitySnapshot().fldMaximumApplicable==maxima);
  }
  for(bool high:{false,true})for(uint8_t fifo=0;fifo<4U;++fifo)for(uint8_t squelch=0;squelch<=8U;++squelch) {
    Controller b;Driver d;ready(d,b);auto s=settings();
    s.tx100.fifoDepthCode=fifo;s.tx100.scramblerBypass=s.tx100.rxErrorDuringIdle=high;
    s.tx10.squelchCode=squelch;s.tx10.lowerThreshold=s.tx10.nlpDisable=s.tx10.jabberDisable=s.tx10.shortPreamble=high;
    s.linkRecovery=high;b.words[9]=0xFF7F;b.words[10]=0xFFFB;b.words[17]=0x74FB;
    b.words[26]=0xC16E;b.ext[0x2A]=0xB998;const auto first=b.frames,calls=b.calls;configured(d,b,s);
    CHECK(b.frames-first==52U && b.calls-calls==19U);
    CHECK(b.words[17]==static_cast<uint16_t>(0x74FBU|(uint32_t{fifo}<<8)|(high?0x0800U:0U)));
    CHECK(b.words[10]==static_cast<uint16_t>(0xFFFBU|(high?4U:0U)));
    CHECK(b.words[26]==static_cast<uint16_t>(0xC16EU|(uint32_t{squelch}<<9)|(high?0x2081U:0U)));
    CHECK(b.ext[0x2A]==static_cast<uint16_t>(0xB998U|(high?0x4000U:0U)));
    CHECK(b.words[9]==static_cast<uint16_t>(0xFF7FU|(high?0x80U:0U)));
    CHECK(writes(b,first,0x30E)==0U && (b.ext[0x30E]&0xFFC0U)==0x4A40U);
    constexpr uint16_t ordered[]={11,0x117,0x131,17,10,26,0x2A,9,11};unsigned n=0;
    for(unsigned i=first;i<b.frames;++i)if(semantic(b.trace[i])) { CHECK(n<9U && b.trace[i].target==ordered[n]);++n; }
    CHECK(n==9U);
  }
  for(uint8_t code:{uint8_t{1},uint8_t{63}})for(uint8_t threshold:{uint8_t{1},uint8_t{63}})
    for(uint8_t descrambler:{uint8_t{0},uint8_t{63}}) {
      Controller b;Driver d;ready(d,b);auto s=settings(false);s.fastDrop.rawThresholdOverride=true;
      s.fastDrop.windowCode=code;s.fastDrop.errorThreshold=threshold;s.fastDrop.descramblerThreshold=descrambler;
      configured(d,b,s);CHECK(b.ext[0x117]==static_cast<uint16_t>((uint32_t{code}<<10)|(uint32_t{threshold}<<4)|7U));
      CHECK((b.ext[0x131]&0x3FU)==descrambler && !d.qualitySnapshot().fldMaximumApplicable);
    }
  // An unexpected reserved readback is preserved, without claiming the exact 0417 recipe.
  {
    Controller unexpected;Driver phy;ready(phy,unexpected);unexpected.ext[0x117]=0x0413;
    if (test::aGrade()) {
      const auto token=start(phy,unexpected,settings(false));
      CHECK(drain(phy,unexpected,token).is(Err::CONFIG_MISMATCH));
      CHECK(take(phy,token).mismatch.observed==0x0413U);
    } else { configured(phy,unexpected,settings(false)); }
    CHECK(unexpected.ext[0x117]==0x0413U && !phy.qualitySnapshot().fldMaximumApplicable);
  }
  // Source-defined zero-cause disable is admitted on RMII, without inventing a detector.
  Controller rmii;rmii.boardInterface=MacInterface::RMII;Driver d;ready(d,rmii);auto s=settings(false);s.fastDrop.causeMask=0;
  rmii.words[11]=0x046F;configured(d,rmii,s);CHECK(rmii.words[11]==0x0060U);
}
void prerequisiteEvidence();
void admission() {
  prerequisiteEvidence();
  for(unsigned fault=0;fault<16U;++fault) {
    Controller b;Driver d;if(fault!=0U)ready(d,b);auto s=settings();OperationToken token{777};auto allowance=budget();
    switch(fault) {
      case 1:s.maintenanceAdmitted=false;break;case 2:s.macStopped=false;break;
      case 3:s.partnerPreambleAdmitted=false;break;case 4:s.fastDrop.causeMask=0x10;break;
      case 5:s.fastDrop.windowCode=0;break;case 6:s.fastDrop.windowCode=64;break;
      case 7:s.fastDrop.errorThreshold=0;break;case 8:s.fastDrop.errorThreshold=64;break;
      case 9:s.fastDrop.descramblerThreshold=64;break;case 10:s.tx100.fifoDepthCode=4;break;
      case 11:s.tx10.squelchCode=9;break;case 12:s.tx10.squelchCode=15;break;
      case 13:allowance.io.maxFrames=0;break;case 14:allowance.pollIntervalUs=0;break;
      case 15:allowance.maxStalledPolls=0;break;default:break;
    }
    const auto frames=b.frames,clocks=b.clocks;
    CHECK(!d.startConfigureQuality(s,allowance,token).inProgress());
    CHECK(token.value==777U && b.frames==frames && b.clocks==clocks && !d.qualitySnapshot().desiredValid);
  }
  Controller b;b.boardInterface=MacInterface::RMII;Driver d;ready(d,b);const auto frames=b.frames,clocks=b.clocks;OperationToken token{777};
  CHECK(!d.startConfigureQuality(settings(),budget(),token).inProgress() && b.frames==frames && b.clocks==clocks);
  for(uint8_t reserved=9;reserved<=15;++reserved) {
    auto invalid=settings(); invalid.tx10.squelchCode=reserved;
    CHECK(d.startConfigureQuality(invalid,budget(),token).is(Err::INVALID_PARAM));
    CHECK(b.frames==frames && b.clocks==clocks);
  }
  // A contradictory live mode is rejected before a semantic write, after bounded evidence.
  Controller live;Driver phy;ready(phy,live);live.words[23]|=0x20;const auto mutations=live.mutations;
  token=start(phy,live,settings());CHECK(!drain(phy,live,token).ok());CHECK(!take(phy,token).mutationPossible && live.mutations==mutations);
  // Source-gated experimental threshold values require explicit opt-in.
  Controller raw;Driver rawPhy;ready(rawPhy,raw);auto s=settings(false);s.fastDrop.windowCode=63;
  const auto before=raw.frames;CHECK(!rawPhy.startConfigureQuality(s,budget(),token).inProgress() && raw.frames==before);
  configured(rawPhy,raw,settings());s=settings();s.tx10.owned=false;
  CHECK(!rawPhy.startConfigureQuality(s,budget(),token).inProgress()); // Cannot silently abandon saved fields.
}
void prerequisiteEvidence() {
  // A valid reset indication is retained before later frame/release failure.
  for (unsigned failure=0;failure<3U;++failure) {
    Controller b;Driver d;ready(d,b);const auto first=b.frames;
    b.corruptFrame=static_cast<int>(first);b.corruptMask=0x8000;
    if (failure==1U) { b.releaseCall=static_cast<int>(b.calls); }
    if (failure==2U) { b.failFrame=static_cast<int>(first+1U); }
    const auto token=start(d,b,settings(false));const auto status=drain(d,b,token);
    CHECK(failure==0U?status.is(Err::INVALID_CONFIG):failure==1U?status.is(Err::MDIO_ERROR):status.is(Err::MDIO_TIMEOUT));
    const auto result=take(d,token);
    CHECK(!d.isInitialized() && !d.capabilities().valid && d.identitySnapshot().stale);
    CHECK(!result.mutationPossible && !result.persistentRestored && writes(b,first,11)==0U);
  }
  // An observed mode contradicting persistent intent cannot remain VERIFIED.
  Controller b;Driver d;ready(d,b);b.words[0]=0x2100;const auto first=b.frames;
  const auto token=start(d,b,settings(false));CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));
  const auto result=take(d,token);
  CHECK(result.mismatch.valid && result.mismatch.registerAddress==0U && result.mismatch.mask==0x3100U);
  CHECK(d.settingsSnapshot().trust==ConfigurationTrust::DIRTY && d.settingsSnapshot().observedStale);
  CHECK(!result.persistentRestored && !result.mutationAttempted && writes(b,first,11)==0U);
}
void failures() {
  Controller ref;Driver reference;ready(reference,ref);const auto first=ref.frames,firstCall=ref.calls;configured(reference,ref,settings());
  const auto count=ref.frames-first,callbacks=ref.calls-firstCall;CHECK(count==52U && callbacks==19U);
  for(unsigned phase=0;phase<count;++phase)for(bool accepted:{false,true}) {
    Controller b;Driver d;ready(d,b);const auto startFrame=b.frames;b.failFrame=static_cast<int>(startFrame+phase);b.acceptedFailure=accepted;
    const auto token=start(d,b,settings());CHECK(drain(d,b,token).is(Err::MDIO_TIMEOUT));const auto r=take(d,token);
    CHECK(r.status.detail==-33033 && b.frames==startFrame+phase+1U && !r.persistentRestored);
    CHECK(b.calls==b.releases && !b.locked && d.qualitySnapshot().trust!=ConfigurationTrust::VERIFIED);
    bool effect=false;for(unsigned i=0;i<phase;++i)effect|=semantic(ref.trace[first+i]);effect|=accepted && semantic(ref.trace[first+phase]);
    if(effect)CHECK(r.mutationPossible && r.cleanupOwed && !d.end().ok());
  }
  for(unsigned call=0;call<callbacks;++call) {
    Controller b;Driver d;ready(d,b);b.releaseCall=static_cast<int>(b.calls+call);const auto token=start(d,b,settings());
    CHECK(drain(d,b,token).is(Err::MDIO_ERROR));const auto r=take(d,token);
    CHECK(r.failingBatch.release.detail==-37 && !r.persistentRestored && !b.locked);
  }
  constexpr uint16_t addresses[]={11,0x117,0x131,17,10,26,0x2A,9};
  constexpr uint16_t masks[]={1,0x10,1,0x100,4,1,0x4000,0x80};
  for(unsigned row=0;row<8U;++row) {
    unsigned last=first;for(unsigned i=first;i<ref.frames;++i)if(ref.trace[i].kind==FrameKind::READ && ref.trace[i].target==addresses[row])last=i;
    Controller b;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+last-first);b.corruptMask=masks[row];const auto token=start(d,b,settings());
    CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));const auto r=take(d,token);
    CHECK(r.mismatch.valid && r.mismatch.registerAddress==addresses[row] && r.cleanupOwed && !r.persistentRestored);
  }
  // Cancellation is bus-silent at every completed batch boundary, including before the first effect.
  for(unsigned completed=0;completed<callbacks;++completed) {
    Controller b;Driver d;ready(d,b);const auto token=start(d,b,settings());
    for(unsigned n=0;n<completed;++n)CHECK(poll(d,b,token).inProgress());
    const auto frames=b.frames,clocks=b.clocks;CHECK(d.cancel(token).ok() && b.frames==frames && b.clocks==clocks);
    const auto r=take(d,token);CHECK(!r.status.ok() && !r.persistentRestored);
    CHECK(r.cleanupOwed==(completed>=2U));
    CHECK(!d.qualitySnapshot().fldMaximumApplicable);
    OperationResult sentinel;sentinel.token.value=777;CHECK(!d.takeResult(token,sentinel).ok() && sentinel.token.value==777U);
  }
}
void replay() {
  for(auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART}) {
    Controller b;Driver d;ready(d,b);configured(d,b,settings());OperationToken token;
    CHECK(d.startInitialize({reset},budget(),token).inProgress());CHECK(drain(d,b,token).ok());const auto r=take(d,token);
    CHECK(r.persistentRestored && r.quality.trust==ConfigurationTrust::VERIFIED && !r.cleanupOwed && d.isInitialized());
    CHECK((b.words[11]&0x040FU)==0x040FU && b.ext[0x117]==0x0417U && (b.ext[0x131]&0x3FU)==8U);
    CHECK((b.words[17]&0x0B00U)==0x0B00U && (b.words[10]&4U)==4U && (b.words[26]&0x3E81U)==0x3081U);
    CHECK((b.words[9]&0x80U)==0x80U && (b.ext[0x2A]&0x4000U)!=0U && (b.ext[0x30E]&0xFFC0U)==0x4A40U);
    const auto mutations=b.mutations;CHECK(d.startVerify(budget(),token).inProgress());CHECK(drain(d,b,token).ok());take(d,token);
    CHECK(b.mutations==mutations && d.qualitySnapshot().trust==ConfigurationTrust::VERIFIED);
  }
  Controller ref;Driver reference;ready(reference,ref);configured(reference,ref,settings());OperationToken token;
  const auto first=ref.frames;CHECK(reference.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(reference,ref,token).ok());take(reference,token);
  const auto count=ref.frames-first;
  for(unsigned phase=0;phase<count;++phase) {
    Controller b;Driver d;ready(d,b);configured(d,b,settings());b.failFrame=static_cast<int>(b.frames+phase);
    CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(!drain(d,b,token).ok());const auto r=take(d,token);
    CHECK(!r.persistentRestored && !b.locked);
    b.failFrame=-1;CHECK(d.startInitialize({ResetKind::NONE},budget(),token).inProgress());CHECK(drain(d,b,token).ok());
    CHECK(take(d,token).persistentRestored && d.qualitySnapshot().trust==ConfigurationTrust::VERIFIED);
  }
  // Temporary far-end loopback changes the same FIFO field; stop restores
  // the persistent quality owner through the shared tail, not a second recipe.
  Controller loop;Driver loopPhy;ready(loopPhy,loop);configured(loopPhy,loop,settings());
  LoopbackOptions loopOptions;loopOptions.mode=LoopbackMode::REVERSE;loopOptions.speed=LinkSpeed::MBPS_100;
  loopOptions.maintenanceAdmitted=loopOptions.partnerReady=true;loopOptions.reverseFifoDepth=1;
  CHECK(loopPhy.startLoopback(loopOptions,budget(),token).inProgress());CHECK(drain(loopPhy,loop,token).ok());take(loopPhy,token);
  CHECK((loop.words[17]&0x0300U)==0x0100U);
  CHECK(loopPhy.stopLoopback(budget(),token).inProgress());CHECK(drain(loopPhy,loop,token).ok());take(loopPhy,token);
  CHECK((loop.words[17]&0x0300U)==0x0300U && loopPhy.qualitySnapshot().trust==ConfigurationTrust::VERIFIED);
  Controller power;Driver powerPhy;ready(powerPhy,power);configured(powerPhy,power,settings());
  PowerSettings powerSettings;powerSettings.maintenanceAdmitted=powerSettings.managementClockAvailable=powerSettings.clockLossApproved=true;
  powerSettings.request=PowerRequest::IEEE_POWER_DOWN;
  CHECK(powerPhy.startConfigurePower(powerSettings,budget(),token).inProgress());CHECK(drain(powerPhy,power,token).ok());take(powerPhy,token);
  powerSettings.request=PowerRequest::NORMAL;
  CHECK(powerPhy.startConfigurePower(powerSettings,budget(),token).inProgress());CHECK(drain(powerPhy,power,token).ok());take(powerPhy,token);
  CHECK(powerPhy.qualitySnapshot().trust==ConfigurationTrust::VERIFIED && (power.words[17]&0x0B00U)==0x0B00U);
  // A first verify mismatch remains diagnostic evidence when a later read fails.
  Controller oracle;Driver oraclePhy;ready(oraclePhy,oracle);configured(oraclePhy,oracle,settings());
  const auto verifyFirst=oracle.frames;CHECK(oraclePhy.startVerify(budget(),token).inProgress());CHECK(drain(oraclePhy,oracle,token).ok());take(oraclePhy,token);
  unsigned mismatchRead=verifyFirst;for(unsigned i=verifyFirst;i<oracle.frames;++i)if(oracle.trace[i].target==0x117 && oracle.trace[i].kind==FrameKind::READ) { mismatchRead=i;break; }
  Controller interrupted;Driver interruptedPhy;ready(interruptedPhy,interrupted);configured(interruptedPhy,interrupted,settings());
  interrupted.corruptFrame=static_cast<int>(interrupted.frames+mismatchRead-verifyFirst);interrupted.corruptMask=0x10;
  interrupted.failFrame=interrupted.corruptFrame+1;const auto writesBefore=interrupted.mutations;
  CHECK(interruptedPhy.startVerify(budget(),token).inProgress());CHECK(drain(interruptedPhy,interrupted,token).is(Err::MDIO_TIMEOUT));
  const auto interruptedResult=take(interruptedPhy,token);
  CHECK(interruptedResult.mismatch.valid && interruptedResult.mismatch.registerAddress==0x117 && interruptedResult.status.detail==-33033);
  CHECK(interrupted.mutations==writesBefore && interruptedPhy.qualitySnapshot().trust!=ConfigurationTrust::VERIFIED);
  Controller b;Driver d;ready(d,b);configured(d,b,settings());
  CHECK(d.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
  CHECK(d.qualitySnapshot().desiredValid && d.qualitySnapshot().observedStale && d.qualitySnapshot().trust!=ConfigurationTrust::VERIFIED && !d.qualitySnapshot().fldMaximumApplicable);
}
void collector() {
  Controller ref;Driver reference;ready(reference,ref);ref.words[26]=0x10;ref.words[16]|=0x1000;ref.words[15]=0x01F0;
  const auto first=ref.frames;configured(reference,ref,settings());
  const auto evidence=reference.evidenceSnapshot();const auto polarity=evidence.samples[static_cast<unsigned>(EvidenceSource::BTSCR)];
  CHECK(polarity.valid && polarity.samples==2U && (polarity.observedOnes&0x10U)!=0U && !polarity.historyLost);
  CHECK((evidence.samples[static_cast<unsigned>(EvidenceSource::PHYSTS)].observedOnes&0x1000U)!=0U);
  CHECK(evidence.samples[static_cast<unsigned>(EvidenceSource::FLDS)].eventBits==0x1F0U && ref.words[15]==0);
  unsigned read=0;for(unsigned i=first;i<ref.frames;++i)if(ref.trace[i].target==26 && ref.trace[i].kind==FrameKind::READ) { read=i-first;break; }
  for(bool afterEffect:{false,true}) {
    Controller b;Driver d;ready(d,b);b.words[26]=0x10;b.words[15]=0x10;b.words[16]|=0x1000;
    b.failFrame=static_cast<int>(b.frames+read);b.acceptedFailure=afterEffect;const auto token=start(d,b,settings());
    CHECK(!drain(d,b,token).ok());take(d,token);const auto samples=d.evidenceSnapshot();
    CHECK(samples.samples[static_cast<unsigned>(EvidenceSource::BTSCR)].historyLost==afterEffect);
    CHECK(samples.samples[static_cast<unsigned>(EvidenceSource::FLDS)].valid && samples.samples[static_cast<unsigned>(EvidenceSource::FLDS)].eventBits==0x10U);
  }
  Controller b;Driver d;ready(d,b);b.words[26]=0x10;b.failFrame=static_cast<int>(b.frames+read+1U);const auto token=start(d,b,settings());
  CHECK(!drain(d,b,token).ok());take(d,token);const auto saved=d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::BTSCR)];
  CHECK(saved.valid && saved.samples==1U && saved.raw==0x10U && !saved.historyLost);
  const auto calls=b.calls,clocks=b.clocks;d.qualitySnapshot();d.qualitySnapshot();d.evidenceSnapshot();CHECK(b.calls==calls && b.clocks==clocks);
  // Original PDF table 8-20p56: BMSR[1] is jabber evidence for 10BASE-T.
  // Table 8-34p64 mirrors it at PHYSTS[5]; its prose says PHYSTS reads do not
  // clear it despite the RC label. Retain raw evidence without inventing rearm.
  Controller jabber;Driver jabberPhy;ready(jabberPhy,jabber);
  jabber.words[0]=0;jabber.words[1]=0x0006;jabber.words[16]=0x0023;
  EvidenceSnapshot jabberView;const uint32_t jabberMask=evidenceBit(EvidenceSource::PHYSTS)|evidenceBit(EvidenceSource::BMSR);
  const auto jabberFirst=jabber.frames;
  CHECK(jabberPhy.acquireEvidence(jabberMask,jabberView,{1000000,2}).ok());
  CHECK(jabber.frames==jabberFirst+2U && jabber.trace[jabberFirst].target==0x10 && jabber.trace[jabberFirst+1U].target==1);
  CHECK((jabberView.samples[static_cast<unsigned>(EvidenceSource::BMSR)].raw&0x0002U)!=0U);
  CHECK((jabberView.samples[static_cast<unsigned>(EvidenceSource::PHYSTS)].raw&0x0020U)!=0U);
  const auto jabberCalls=jabber.calls,jabberClocks=jabber.clocks;
  const auto cached1=jabberPhy.evidenceSnapshot(),cached2=jabberPhy.evidenceSnapshot();
  CHECK(cached1.samples[static_cast<unsigned>(EvidenceSource::BMSR)].samples==cached2.samples[static_cast<unsigned>(EvidenceSource::BMSR)].samples);
  CHECK(jabber.calls==jabberCalls && jabber.clocks==jabberClocks);
  jabber.failFrame=static_cast<int>(jabber.frames+1U);jabberView.lastRequested=0xCAFE;
  CHECK(jabberPhy.acquireEvidence(jabberMask,jabberView,{1000000,2}).is(Err::MDIO_TIMEOUT) && jabberView.lastRequested==0xCAFEU);
  const auto partialJabber=jabberPhy.evidenceSnapshot();
  CHECK(partialJabber.samples[static_cast<unsigned>(EvidenceSource::PHYSTS)].samples==cached1.samples[static_cast<unsigned>(EvidenceSource::PHYSTS)].samples+1U);
  CHECK((partialJabber.samples[static_cast<unsigned>(EvidenceSource::PHYSTS)].raw&0x0020U)!=0U);
  CHECK(partialJabber.samples[static_cast<unsigned>(EvidenceSource::BMSR)].samples==cached1.samples[static_cast<unsigned>(EvidenceSource::BMSR)].samples);
  CHECK(partialJabber.samples[static_cast<unsigned>(EvidenceSource::BMSR)].historyLost);
  Controller simultaneous;Driver chip;ready(chip,simultaneous);
  simultaneous.words[18]=0xFF00;simultaneous.words[19]=0xFF00;simultaneous.words[20]=0xFF;simultaneous.words[21]=0xFFFF;simultaneous.words[15]=0x1F0;
  CounterSnapshot counters;CHECK(chip.collectEvents(counters,{1000000,8}).ok());
  CHECK(counters.valid && counters.misr1Events==0xFF00 && counters.misr2Events==0xFF00 && counters.fldCauses==0x1F0);
  CHECK(counters.falseCarrierLowerBound && counters.rawRecr==0xFFFF);
  const auto old=chip.evidenceSnapshot();SignalQualitySnapshot mse;CHECK(chip.readSignalQuality(mse,{1000000,4}).ok());
  const auto current=chip.evidenceSnapshot();CHECK(current.samples[static_cast<unsigned>(EvidenceSource::FCSCR)].samples==old.samples[static_cast<unsigned>(EvidenceSource::FCSCR)].samples);
  CHECK(current.samples[static_cast<unsigned>(EvidenceSource::RECR)].sampledSum==0U);
}
void ownership() {
  // Reset retention belongs to the common boundary, including a different
  // feature owner. IRQ must not continue from pre-reset evidence.
  for (bool alias : {false,true}) for (unsigned boundary : {1U,4U}) {
    Controller irqBus; Driver irqDriver; ready(irqDriver,irqBus);
    InterruptSettings irq; irq.maintenanceAdmitted=irq.pinAvailable=true;
    OperationToken token; CHECK(irqDriver.startConfigureInterrupts(irq,budget(),token).inProgress());
    for (unsigned step=0;step<boundary;++step) { CHECK(poll(irqDriver,irqBus,token).inProgress()); }
    irqBus.corruptFrame=static_cast<int>(irqBus.frames+(alias?3U:0U)); irqBus.corruptMask=0x8000;
    uint16_t raw=0xCAFE;
    CHECK((alias?irqDriver.readMmd(31,0,raw,{1000000,4}):irqDriver.readClause22(0,raw,{1000000,1})).ok());
    CHECK(raw==0x9000U && !irqDriver.isInitialized() && !irqDriver.capabilities().valid);
    const auto frames=irqBus.frames,mutations=irqBus.mutations;
    CHECK(irqDriver.operationSnapshot().state==OperationState::RESULT_PENDING);
    CHECK(irqDriver.pollOperation(token).is(Err::INVALID_CONFIG));
    const auto result=take(irqDriver,token);
    CHECK(result.status.detail==static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
    CHECK(!result.persistentRestored && result.cleanupOwed==result.mutationPossible);
    CHECK(irqBus.frames==frames && irqBus.mutations==mutations);
  }
  // An interleaved raw reset observation freezes the pass even when stable
  // mode bits still match. The consumed word remains valid to its caller.
  for (unsigned boundary=2;boundary<=3U;++boundary) {
    Controller interleaved;Driver phy;ready(phy,interleaved);
    const auto token=start(phy,interleaved,settings(false));
    for (unsigned step=0;step<boundary;++step) { CHECK(poll(phy,interleaved,token).inProgress()); }
    interleaved.corruptFrame=static_cast<int>(interleaved.frames);interleaved.corruptMask=0x8000;
    uint16_t word=0xCAFE;CHECK(phy.readClause22(0,word,{1000000,1}).ok() && word==0x9000U);
    const auto frames=interleaved.frames,mutations=interleaved.mutations;
    CHECK(phy.operationSnapshot().state==OperationState::RESULT_PENDING);
    CHECK(phy.pollOperation(token).is(Err::INVALID_CONFIG));
    const auto result=take(phy,token);
    CHECK(result.status.detail==static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
    CHECK(result.cleanupOwed && result.mutationCompleted && !result.persistentRestored);
    CHECK(!phy.isInitialized() && !phy.capabilities().valid);
    CHECK(interleaved.frames==frames && interleaved.mutations==mutations);
  }
  Controller b;Driver d,other;ready(d,b);CHECK(other.bind(b.config(2)).ok());b.competitor=&other;
  const auto first=b.frames;configured(d,b,settings());CHECK(b.excluded==b.frames-first);b.competitor=nullptr;
  CHECK(!other.qualitySnapshot().desiredValid);
  // Unrelated interface changes preserve CR2 idle detection, and vice versa.
  for(bool disabled:{false,true}) {
    InterfaceSettings i;i.interface=MacInterface::MII;i.referenceClockMHz=25;i.oddNibbleDisable=disabled;i.shortIpg=disabled;
    i.maintenanceAdmitted=i.macStopped=i.pinsAvailable=i.referenceClockStable=i.managementClockAvailable=i.clockChangeApproved=true;
    InterfaceResult result;CHECK(d.configureInterface(i,result,{1000000,2048}).ok());
    CHECK((b.words[10]&6U)==(disabled?6U:4U) && (b.ext[0x456]&8U)==(disabled?8U:0U));
    configured(d,b,settings());CHECK((b.words[10]&6U)==(disabled?6U:4U) && (b.ext[0x456]&8U)==(disabled?8U:0U));
  }
  b.words[9]^=0x80;uint16_t raw=0;CHECK(d.readClause22(9,raw,{1000000,1}).ok());CHECK(d.qualitySnapshot().trust==ConfigurationTrust::DIRTY);
  OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
  CHECK(d.qualitySnapshot().trust==ConfigurationTrust::VERIFIED);
  const auto profile=qualityMiiProfile();CHECK(profile.fastDrop.causeMask==9U);
  Controller example;Driver examplePhy;ready(examplePhy,example);CHECK(startMiiFastDrop(examplePhy,budget(),t).inProgress());
  CHECK(drain(examplePhy,example,t).ok());take(examplePhy,t);QualityViews views;CHECK(acquireQualityViews(examplePhy,views,{1000000,4}).ok());
  const auto calls=example.calls;const auto cached=cachedQualityViews(examplePhy);CHECK(cached.mse.valid && example.calls==calls);
}
void timing() {
  for(unsigned defect=0;defect<6U;++defect) {
    Controller b;Driver d;ready(d,b);const auto call=static_cast<int>(b.calls+3U);auto token=start(d,b,settings());
    if(defect==0)b.lateCall=call;
    if(defect==1)b.regressCall=call;
    if(defect==2)b.invalidCall=call;
    if(defect==3)b.badReceiptCall=call;
    if(defect==4)b.now=std::numeric_limits<uint64_t>::max();
    if(defect==5)b.frameTime=0;
    const auto status=drain(d,b,token);if(defect==5)CHECK(status.ok());else CHECK(!status.ok());take(d,token);
    CHECK(!b.locked && b.calls==b.releases);
  }
  Controller b;Driver d;ready(d,b);auto token=start(d,b,settings());
  const auto first=b.frames;CHECK(d.pollOperation(token,0).inProgress() && b.frames==first);
  CHECK(poll(d,b,token).inProgress());CHECK(poll(d,b,token).inProgress());CHECK(poll(d,b,token).inProgress()); // preflight, disable, verify
  const auto before=b.frames;CHECK(poll(d,b,token,4).inProgress() && b.frames==before); // Five-frame atomic MMD update.
  CHECK(drain(d,b,token).ok());take(d,token);
  Controller stalled;Driver stalledPhy;ready(stalledPhy,stalled);token=start(stalledPhy,stalled,settings());
  for(unsigned n=0;n<8U;++n)CHECK(stalledPhy.pollOperation(token,0).inProgress());
  CHECK(stalledPhy.pollOperation(token,0).is(Err::OPERATION_TIMEOUT));take(stalledPhy,token);
}
void raw() {
  Controller b;Driver d;ready(d,b);b.ext[0x218]=0xABCD;const auto first=b.frames,calls=b.calls;
  SignalQualitySnapshot out;CHECK(d.readSignalQuality(out,{1000000,4}).ok());
  CHECK(out.valid && !out.stale && out.rawMse==0xABCD && out.timeValid && b.frames-first==4U && b.calls-calls==1U);
  CHECK(b.trace[first].reg==13 && b.trace[first].value==0x001FU && b.trace[first+1U].reg==14 && b.trace[first+1U].value==0x0218U);
  CHECK(b.trace[first+2U].reg==13 && b.trace[first+2U].value==0x401FU && b.trace[first+3U].kind==FrameKind::READ);
  {
    Controller stopped;Driver stoppedPhy;ready(stoppedPhy,stopped);stopped.ext[0x218]=0x1234;
    SignalQualitySnapshot retained;CHECK(stoppedPhy.readSignalQuality(retained,{1000000,4}).ok());
    const auto time=retained.observedAtUs;stopped.badReceiptCall=static_cast<int>(stopped.calls);
    uint16_t rawId=0;CHECK(!stoppedPhy.readClause22(2,rawId,{1000000,1}).ok());
    const auto frames=stopped.frames,clocks=stopped.clocks;retained.rawMse=0xCAFE;
    const auto rejected=stoppedPhy.readSignalQuality(retained,{1000000,4});
    CHECK(rejected.is(Err::BUSY) && rejected.detail==static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED));
    const auto cached=stoppedPhy.signalQualitySnapshot();
    CHECK(retained.rawMse==0xCAFE && cached.rawMse==0x1234 && cached.stale && cached.observedAtUs==time);
    CHECK(cached.lastStatus.is(Err::BUSY) && stopped.frames==frames && stopped.clocks==clocks);
  }
  const auto saved=out;for(unsigned phase=0;phase<4U;++phase) {
    b.failFrame=static_cast<int>(b.frames+phase);out.rawMse=0xCAFE;CHECK(d.readSignalQuality(out,{1000000,4}).is(Err::MDIO_TIMEOUT));
    CHECK(out.rawMse==0xCAFE);const auto stale=d.signalQualitySnapshot();
    CHECK(stale.rawMse==0xABCD && stale.stale && stale.observedAtUs==saved.observedAtUs);b.failFrame=-1;
  }
  for(uint16_t value:{uint16_t{0},uint16_t{0xFFFF}}) { b.ext[0x218]=value;CHECK(d.readSignalQuality(out,{1000000,4}).ok() && out.rawMse==value && out.valid); }
  const auto frames=b.frames;out.rawMse=0xCAFE;CHECK(!d.readSignalQuality(out,{1000000,3}).ok() && b.frames==frames && out.rawMse==0xCAFE);
  CHECK(d.invalidateDeviceState(InvalidationReason::EXTERNAL_POWER_ON).ok());CHECK(d.signalQualitySnapshot().stale);
  Controller zero;zero.now=0;zero.frameTime=0;Driver zeroPhy;auto binding=zero.config();binding.transport.bounds.clockLagUs=0;
  CHECK(!zeroPhy.bind(binding).ok() && zero.frames==0U); // Missing clock-lag contract is rejected locally.
  binding.transport.bounds.clockLagUs=1;CHECK(zeroPhy.bind(binding).ok());DeviceIdentity identity;CHECK(zeroPhy.readIdentity(identity,{1000000,2}).ok());
  SignalQualitySnapshot zeroSample;CHECK(zeroPhy.readSignalQuality(zeroSample,{1000000,4}).ok() && zeroSample.timeValid && zeroSample.observedAtUs==1U);
  zero.now=std::numeric_limits<uint64_t>::max();zeroSample.rawMse=0xCAFE;
  CHECK(!zeroPhy.readSignalQuality(zeroSample,{zero.now,4}).ok() && zeroSample.rawMse==0xCAFE);
  Controller changed;Driver changedPhy;ready(changedPhy,changed);LinkSnapshot link;
  CHECK(changedPhy.readLinkStatus(link,{1000000,10}).ok());SignalQualitySnapshot sample;
  CHECK(changedPhy.readSignalQuality(sample,{1000000,4}).ok());const auto stamp=sample.observedAtUs;
  changed.words[1]=0;changed.words[16]=0;CHECK(changedPhy.readLinkStatus(link,{1000000,10}).ok());
  CHECK(changedPhy.signalQualitySnapshot().stale && changedPhy.signalQualitySnapshot().observedAtUs==stamp);
  // One ordinary event acquisition can expose next-page/polarity/counter evidence;
  // the MSE reader never performs an extra destructive read to render it.
  Controller view;Driver phy;ready(phy,view);view.words[6]=3;view.words[26]=0x10;
  EvidenceSnapshot evidence;CHECK(phy.acquireEvidence(evidenceBit(EvidenceSource::ANER)|evidenceBit(EvidenceSource::BTSCR),evidence,{1000000,8}).ok());
  CHECK((phy.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::ANER)].observedOnes&2U)!=0U);
  const auto acquired=view.frames;QualityViews views;CHECK(acquireQualityViews(phy,views,{1000000,4}).ok() && view.frames==acquired+4U);
  CHECK((views.events.samples[static_cast<unsigned>(EvidenceSource::BTSCR)].observedOnes&0x10U)!=0U);
}
template<Variant grade> void aQuality() {
  // SNLS783A tables 8-5/6, 8-31/34/36/53/54/88. Inject distinct
  // sampled straps and CR3 reset words; an explicit application request
  // can select a different cause set while retaining both mirror bits.
  struct StrapRow { uint16_t id,sor1,causes; };
  constexpr StrapRow rows[]={
    {0xA114,0,0x000D},{0xA114,0x0800,0x0001},{0xA134,0,0},
    {0xA134,0x0082,0x040F},{0xA134,0x0080,0x000D},{0xA134,0x0180,0x000C},
    {0xA134,0x0880,0x0001},{0xA134,0x0882,0x0403}
  };
  {
    Controller b; Driver d; b.declared=grade; b.id2=0xA114; initialized(d,b);
    b.ext[0x467]=0x0800; b.failFrame=static_cast<int>(b.frames+9);
    OperationToken init; CHECK(d.startInitialize({ResetKind::NONE},budget(),init).inProgress());
    CHECK(drain(d,b,init).is(Err::MDIO_TIMEOUT));
    const auto partial=take(d,init); const auto current=d.strapSnapshot();
    CHECK(partial.straps.sor1Valid && !partial.straps.sor2Valid && partial.straps.strap11High());
    CHECK(current.sor1Valid && current.strap11High() && !current.sor2Valid && !current.timeValid && current.stale);
  }
  for (const auto& row:rows) for (uint16_t mirrors:{uint16_t{0x20},uint16_t{0x40}}) {
    Controller b; Driver d; b.declared=grade; b.id2=row.id; b.ext[0x467]=row.sor1;
    b.words[11]=row.causes|mirrors; b.words[17]=row.id==0xA114?3:0; b.ext[0x131]=0x0288;
    initialized(d,b);
    const auto straps=d.strapSnapshot();
    CHECK(straps.sor1Valid && straps.sor1==row.sor1 && straps.strap11High()==((row.sor1&0x0800)!=0));
    const auto calls=b.calls,clocks=b.clocks; uint16_t initial=0;
    CHECK(d.readClause22(0x0B,initial,{1000000,1}).ok() && (initial&0x040F)==row.causes);
    CHECK(b.calls==calls+1 && b.clocks>clocks);
    auto s=settings(false); s.fastDrop.causeMask=1; auto result=configured(d,b,s);
    CHECK(b.words[11]==(mirrors|1) && (result.quality.raw[3]&3U)==(row.id==0xA114?3U:0U));
    CHECK(b.ext[0x131]==0x0288);
  }
  Controller ref; Driver driver; ref.declared=grade; ref.id2=0xA134; initialized(driver,ref);
  auto s=settings(false); s.recoveryOwned=s.linkRecovery=true;
  constexpr uint16_t dependentOnly[]={2,4,0x0400,0x0406};
  for (bool raw:{false,true}) for (uint16_t mask:dependentOnly) {
    auto denied=s; denied.fastDrop.rawThresholdOverride=raw; denied.fastDrop.causeMask=mask;
    const auto calls=ref.calls,clocks=ref.clocks; OperationToken t;
    CHECK(driver.startConfigureQuality(denied,budget(),t).is(Err::INVALID_CONFIG));
    CHECK(ref.calls==calls && ref.clocks==clocks && !driver.qualitySnapshot().desiredValid);
  }
  // Read the raw exact-part MSE word under its original address.
  SignalQualitySnapshot mse; ref.ext[0x218]=0xABCD;
  CHECK(driver.readSignalQuality(mse,{1000000,4}).ok() && mse.rawMse==0xABCD);
  const auto first=ref.frames,calls=ref.calls; ref.words[9]=0x0120; ref.words[11]=0x042F;
  ref.words[15]=0x01F0; ref.ext[0x131]=0x0288;
  auto result=configured(driver,ref,s);
  CHECK(ref.frames-first==32 && ref.calls-calls==11 && ref.words[9]==0x01A0);
  CHECK(ref.words[11]==0x042F && ref.ext[0x117]==0x0417 && ref.ext[0x131]==0x0288);
  CHECK(result.quality.fldMaximumApplicable);
  const auto flds=driver.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::FLDS)];
  CHECK(flds.samples==1 && flds.eventBits==0x01F0);
  constexpr uint16_t targets[]={11,0x117,0x131,9,11};
  constexpr uint16_t values[]={0x0020,0x0417,0x0288,0x01A0,0x042F};
  unsigned target=0;
  for (unsigned n=first;n<ref.frames;++n) if (semantic(ref.trace[n])) {
    CHECK(target<5);
    if (target<5) { CHECK(ref.trace[n].target==targets[target] && ref.trace[n].value==values[target]); }
    ++target;
  }
  CHECK(target==5);
  OperationToken verify; const auto beforeVerify=ref.mutations;
  CHECK(driver.startVerify(budget(),verify).inProgress() && drain(driver,ref,verify).ok());
  CHECK(take(driver,verify).persistentRestored && ref.mutations==beforeVerify);
  CHECK(ref.frames-ref.firstFrame[ref.calls-5]==15 && ref.trace[ref.frames-15].target==0);
  for (unsigned offset=0;offset<32;++offset) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; initialized(d,b);
    b.failFrame=static_cast<int>(b.frames+offset); const auto t=start(d,b,s);
    CHECK(drain(d,b,t).is(Err::MDIO_TIMEOUT));
    const auto failed=take(d,t);
    CHECK(!failed.persistentRestored && failed.cleanupOwed==(offset>=6) && b.calls==b.releases);
  }
  for (unsigned call=0;call<11;++call) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; initialized(d,b);
    b.releaseCall=static_cast<int>(b.calls+call); const auto t=start(d,b,s);
    CHECK(drain(d,b,t).is(Err::MDIO_ERROR));
    CHECK(take(d,t).failingBatch.release.detail==-37 && b.calls==b.releases);
  }
  for (unsigned fault=0;fault<4;++fault) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; initialized(d,b);
    OperationToken token; auto limit=budget();
    if (fault==0) limit.io.maxFrames=31;
    if (fault==1) limit.io.deadlineUs=b.now+217; // 1 us lag + 32*4 + 11*8; equality expires
    if (fault==2) b.words[23]|=0x20;           // independently observed RMII
    if (fault==3) b.locked=true;              // owner admission failure
    const auto mutations=b.mutations,frames=b.frames;
    CHECK(d.startConfigureQuality(s,limit,token).inProgress());
    if (fault==3) {
      CHECK(d.pollOperation(token).is(Err::BUSY)); b.locked=false;
      const auto busy=take(d,token);
      CHECK(!busy.mutationPossible && !busy.cleanupOwed && b.mutations==mutations);
      continue;
    }
    CHECK(!drain(d,b,token).ok());
    const auto failed=take(d,token); CHECK(!failed.persistentRestored);
    CHECK(b.mutations==mutations && !failed.cleanupOwed);
    if (fault<2) CHECK(b.frames==frames && failed.status.is(Err::OPERATION_TIMEOUT));
  }
  for (unsigned after=0;after<11;++after) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA114; initialized(d,b);
    const auto t=start(d,b,s);
    for (unsigned n=0;n<after;++n) CHECK(poll(d,b,t).inProgress());
    const auto callsBefore=b.calls,clocksBefore=b.clocks; CHECK(d.cancel(t).ok());
    const auto cancelled=take(d,t);
    CHECK(cancelled.status.is(Err::CANCELLED) && cancelled.cleanupOwed==(after>=2));
    d.qualitySnapshot(); d.strapSnapshot(); CHECK(b.calls==callsBefore && b.clocks==clocksBefore);
    if (after>=2) {
      OperationToken restore;
      CHECK(d.startInitialize({ResetKind::NONE},budget(),restore).inProgress() && drain(d,b,restore).ok());
      CHECK(take(d,restore).persistentRestored && !d.operationSnapshot().cleanupOwed);
    }
  }
  Controller wrongNibble; Driver wrong; wrongNibble.declared=grade; wrongNibble.id2=0xA134;
  initialized(wrong,wrongNibble); wrongNibble.ext[0x117]=6;
  const auto t=start(wrong,wrongNibble,s);
  CHECK(drain(wrong,wrongNibble,t).is(Err::CONFIG_MISMATCH));
  const auto failure=take(wrong,t);
  CHECK(failure.mismatch.valid && failure.mismatch.mask==0xFFFF && failure.mismatch.expected==0x0417);
  CHECK((wrongNibble.words[11]&0x040F)==0 && failure.cleanupOwed);
  for (unsigned polls:{5U,7U}) {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; initialized(d,b);
    const auto token=start(d,b,settings(false));
    for (unsigned i=0;i<polls;++i) CHECK(poll(d,b,token).inProgress());
    b.ext[0x117]=0x0416; uint16_t word=0;
    CHECK(d.readMmd(31,0x117,word,{1000000,4}).ok() && word==0x0416);
    const auto frames=b.frames;
    CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH) && b.frames==frames);
    const auto failed=take(d,token);
    CHECK(failed.mismatch.mask==0xFFFF && failed.mismatch.observed==word && failed.cleanupOwed);
    CHECK((b.words[11]&0x040F)==0 && !failed.persistentRestored);
  }
  {
    Controller b; Driver d; b.declared=grade; b.id2=0xA134; initialized(d,b);
    configured(d,b,settings(false));
    b.corruptFrame=static_cast<int>(b.frames+11); b.corruptMask=0x4000;
    OperationToken init; CHECK(d.startInitialize({ResetKind::NONE},budget(),init).inProgress());
    CHECK(drain(d,b,init).ok() && take(d,init).persistentRestored && d.strapSnapshot().sor2Valid);
    uint16_t word=0; b.ext[0x117]=0x0416;
    CHECK(d.readMmd(31,0x117,word,{1000000,4}).ok());
    CHECK(d.qualitySnapshot().trust==ConfigurationTrust::DIRTY && !d.qualitySnapshot().fldMaximumApplicable);
  }
}
} // namespace
int main(int argc,char** argv) {
  if (argc==4 && std::strcmp(argv[1],"p22_copper")==0) {
    if (std::strcmp(argv[2],"I")==0) test::declaredVariant=Variant::DP83822I;
    else if (std::strcmp(argv[2],"H")==0) test::declaredVariant=Variant::DP83822H;
    else if (std::strcmp(argv[2],"IF")==0) test::declaredVariant=Variant::DP83822IF;
    else if (std::strcmp(argv[2],"HF")==0) test::declaredVariant=Variant::DP83822HF;
    else return 2;
    if (std::strcmp(argv[3],"MII")==0) p22Interface=MacInterface::MII;
    else if (std::strcmp(argv[3],"RMII")==0) p22Interface=MacInterface::RMII;
    else if (std::strcmp(argv[3],"RGMII")==0) p22Interface=MacInterface::RGMII;
    else return 2;
    const test::Suite exact[]={{"p22_copper",p22Copper}};
    return test::runSuites(2,argv,exact);
  }
  const test::Suite suites[]={{"vectors",vectors},{"admission",admission},{"failures",failures},{"replay",replay},
    {"collector",collector},{"ownership",ownership},{"timing",timing},{"raw",raw},
    {"ai",aQuality<Variant::DP83826AI>},{"ae",aQuality<Variant::DP83826AE>}};
  return test::runVariantSuites(argc,argv,suites);
}
