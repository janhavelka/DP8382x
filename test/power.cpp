#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
struct Wire { FrameKind kind; uint8_t reg; uint16_t value, target; };

// Independent SNLS647I tables8-19/35/40 model: BMCR11 reads software OR pin.
// The fake deliberately keeps software intent and pin separate; it cannot prove
// physical clock availability, supply current or partner-dependent sleep timing.
struct Controller {
  std::array<uint16_t,32> words{};
  std::array<uint16_t,0x500> ext{};
  std::array<Wire,4096> trace{};
  uint16_t selector{0},address{0},id2{0xA131};
  uint64_t now{100};
  unsigned frames{0},calls{0},clocks{0},releases{0},mutations{0};
  int failFrame{-1},releaseCall{-1},corruptFrame{-1},lateCall{-1},regressCall{-1},invalidCall{-1},badReceiptCall{-1};
  uint16_t corruptMask{1};
  bool acceptedFailure{true},locked{false},powerPin{false};
  int poisonBmcrFrame{-1};
  uint64_t frameTime{2};
  Driver* competitor{nullptr}; unsigned excluded{0};
  Controller() { defaults(); }
  void defaults() {
    words.fill(0);ext.fill(0);words[0]=0x3100;words[4]=0x01E1;
    words[25]=0x8001;words[22]=0x0100;words[30]=0x0102;
    ext[0x030E]=0x8400;ext[0x04A1]=0x1000;
  }
  Config config(uint32_t phy=1) {
    Config c;c.variant=Variant::DP83826I;c.interface=id2==0xA111?MacInterface::MII:MacInterface::RMII;c.phyAddress=phy;
    c.transport={this,execute,clock,{8,4,1}};return c;
  }
  static uint64_t clock(void* context) noexcept { auto& b=*static_cast<Controller*>(context);++b.clocks;return b.now; }
  uint16_t read(uint16_t target) {
    if(target==2)return 0x2000;
    if(target==3)return id2;
    if(target==0) { words[0]&=0x7DFF;return static_cast<uint16_t>(words[0]|(powerPin?0x0800U:0U)); }
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
    if(target==31 && value==0x8000) { defaults();return; }
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
    CHECK(batch.frameCount<=budget.maxFrames);b.locked=true;const auto call=b.calls++;
    for(uint8_t i=0;i<batch.frameCount;++i) {
      const auto& q=batch.frames[i];auto& f=r.frames[i];uint16_t value=q.value;
      if(q.readIndex!=NO_READ_INDEX) { CHECK(q.readIndex<i && r.frames[q.readIndex].valueValid);value=static_cast<uint16_t>((r.frames[q.readIndex].value&q.preserveMask)|q.value); }
      const uint16_t target=q.registerAddress==14 && b.selector==0x401F?b.address:uint16_t{q.registerAddress};
      CHECK(b.frames<b.trace.size());const auto frame=b.frames++;b.trace[frame]={q.kind,q.registerAddress,value,target};
      f.attempted=true;const bool failed=static_cast<int>(frame)==b.failFrame;
      if(!failed || b.acceptedFailure) {
        if(q.kind==FrameKind::WRITE)b.write(q.registerAddress,value);
        else { f.value=b.read(target);if(static_cast<int>(frame)==b.corruptFrame)f.value^=b.corruptMask;
          if(static_cast<int>(frame)==b.poisonBmcrFrame && target==0)f.value=0xB700; }
      }
      b.now+=b.frameTime;
      if(b.competitor!=nullptr) {
        uint16_t sentinel=0xCAFE;CHECK(b.competitor->readMmd(31,0x0467,sentinel,budget).is(Err::BUSY));CHECK(sentinel==0xCAFE);++b.excluded;
      }
      if(failed) { f.effectPossible=b.acceptedFailure;r.status={Err::MDIO_TIMEOUT,-28028,"Injected frame"};break; }
      f.completed=true;f.valueValid=q.kind==FrameKind::READ;
    }
    if(static_cast<int>(call)==b.releaseCall) { r.releaseStatus={Err::MDIO_ERROR,-37,"Injected release"};r.controllerReusable=false; }
    if(static_cast<int>(call)==b.lateCall)b.now=budget.deadlineUs;
    if(static_cast<int>(call)==b.regressCall)b.now=0;
    if(static_cast<int>(call)==b.invalidCall)r.status={Err::IN_PROGRESS,-28029,"Illegal terminal status"};
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
PowerSettings settings(PowerRequest request=PowerRequest::NORMAL) {
  PowerSettings s;s.request=request;s.maintenanceAdmitted=true;s.managementClockAvailable=true;
  s.clockLossApproved=true;return s;
}
OperationToken configure(Driver& d,Controller& b,const PowerSettings& s) {
  OperationToken t{999};const auto calls=b.calls,clocks=b.clocks;
  CHECK(d.startConfigurePower(s,budget(),t).inProgress());
  CHECK(t.value!=999 && b.calls==calls && b.clocks==clocks);return t;
}
void configured(Driver& d,Controller& b,const PowerSettings& s=settings()) {
  const auto t=configure(d,b,s);CHECK(drain(d,b,t).ok());const auto r=take(d,t);
  CHECK(r.status.ok() && !r.cleanupOwed && r.persistentRestored && d.isInitialized());
  const auto observed=d.powerSnapshot();
  CHECK(observed.desiredValid && observed.trust==ConfigurationTrust::VERIFIED && !observed.observedStale);
}
unsigned writes(const Controller& b,unsigned first,uint16_t target) {
  unsigned count=0;for(unsigned i=first;i<b.frames;++i)if(b.trace[i].kind==FrameKind::WRITE && b.trace[i].target==target)++count;
  return count;
}
unsigned lastWrite(const Controller& b,unsigned first,uint16_t target) {
  unsigned index=first;for(unsigned i=first;i<b.frames;++i)if(b.trace[i].kind==FrameKind::WRITE && b.trace[i].target==target)index=i;
  CHECK(b.trace[index].kind==FrameKind::WRITE && b.trace[index].target==target);return index;
}
void policy() {
  for(bool basic:{false,true})for(bool enable:{false,true})for(bool awake:{false,true}) {
    Controller b;b.id2=basic?0xA111:0xA131;Driver d;ready(d,b);
    b.words[17]=0x010B;b.words[22]=awake?0x0100:0;auto s=settings();s.activeSleep=enable;
    const auto first=b.frames,calls=b.calls;configured(d,b,s);
    CHECK(b.frames==first+9U && b.calls==calls+7U);
    CHECK(b.words[17]==(enable?0x610B:0x010B));
    const auto p=d.powerSnapshot();CHECK(p.rawBiscr==(awake?0x0100:0));
    CHECK(p.requestedPowerDown==false && !p.observedBmcrPowerDown && p.timeValid);
    if(awake)CHECK(p.state==PowerState::NORMAL_INDICATED);
    else if(enable)CHECK(p.state==PowerState::ACTIVE_SLEEP_INDICATED);
    else CHECK(p.state==PowerState::UNKNOWN);
    CHECK(writes(b,first,0x030E)==0U && writes(b,first,0)==1U && writes(b,first,17)==1U);
    const auto cb=b.calls,ck=b.clocks;
    const auto a=d.powerSnapshot(),c=d.powerSnapshot();CHECK(a.rawBiscr==c.rawBiscr && b.calls==cb && b.clocks==ck);
    b.words[22]^=0x0100;PowerSnapshot live;CHECK(d.readPowerStatus(live,{1000000,3}).ok());
    CHECK(b.calls==cb+1U && live.rawBiscr==b.words[22]);
    CHECK(d.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::BISCR)].raw==b.words[22]);
  }
  // Invalid requests never change desired state, the output token, clocks or wire.
  for(unsigned which=0;which<6U;++which) {
    Controller b;Driver d;ready(d,b);auto s=settings();
    if(which==0U)s.request=static_cast<PowerRequest>(255);
    if(which==1U)s.request=PowerRequest::DEEP_POWER_DOWN;
    if(which==2U)s.maintenanceAdmitted=false;
    if(which==3U)s.managementClockAvailable=false;
    if(which==4U)s.disablePll=true;
    if(which==5U) { s.request=PowerRequest::IEEE_POWER_DOWN;s.clockLossApproved=false; }
    const auto cb=b.calls,ck=b.clocks;OperationToken t{999};
    const auto result=d.startConfigurePower(s,budget(),t);CHECK(!result.inProgress());
    if(which==1U)CHECK(result.is(Err::UNSUPPORTED) && result.detail==static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
    CHECK(t.value==999 && b.calls==cb && b.clocks==ck && !d.powerSnapshot().desiredValid);
  }
  Controller b;Driver d;ready(d,b);b.words[17]=4;const auto first=b.frames;
  const auto t=configure(d,b,settings());CHECK(!drain(d,b,t).ok());const auto r=take(d,t);
  CHECK(writes(b,first,17)==0U && writes(b,first,0)==0U && !r.cleanupOwed);
}
// OP-2 ordering distinguishes syntax, lifetime and source admission without I/O.
void auditAdmissionOrder() {
  Controller b;Driver d;auto deep=settings(PowerRequest::DEEP_POWER_DOWN);OperationToken token{999};
  auto malformed=budget();malformed.pollIntervalUs=0;
  auto status=d.startConfigurePower(deep,malformed,token);
  CHECK(status.is(Err::INVALID_PARAM) && status.detail==static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  CHECK(d.startConfigurePower(deep,budget(),token).is(Err::NOT_BOUND));
  CHECK(b.calls==0U && b.clocks==0U && token.value==999U);
  ready(d,b);const auto active=configure(d,b,settings());const auto calls=b.calls,clocks=b.clocks;
  status=d.startConfigurePower(deep,budget(),token);
  CHECK(status.is(Err::BUSY) && status.detail==static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
  CHECK(d.cancel(active).ok());
  status=d.startConfigurePower(deep,budget(),token);
  CHECK(status.is(Err::BUSY) && status.detail==static_cast<int32_t>(BusyDetail::RESULT_PENDING));
  take(d,active);
  status=d.startConfigurePower(deep,budget(0),token);
  CHECK(status.is(Err::UNSUPPORTED) && status.detail==static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
  CHECK(token.value==999U && b.calls==calls && b.clocks==clocks);
}
void transitions() {
  auditAdmissionOrder();
  for(bool pll:{false,true})for(bool sleep:{false,true}) {
    Controller b;Driver d;ready(d,b);b.words[0]=0x3500;b.words[17]=0x010B;
    auto down=settings(PowerRequest::IEEE_POWER_DOWN);down.disablePll=pll;down.activeSleep=sleep;
    auto first=b.frames;const auto calls=b.calls;configured(d,b,down);
    CHECK(b.frames==first+(pll?12U:9U) && b.calls==calls+(pll?9U:7U));
    CHECK(b.words[0]==0x3D00 && b.words[17]==static_cast<uint16_t>((pll?0x8000U:0U)|(sleep?0x6000U:0U)|0x010BU));
    CHECK(d.powerSnapshot().requestedPowerDown && d.powerSnapshot().observedBmcrPowerDown);
    CHECK(d.powerSnapshot().state==PowerState::POWER_DOWN_INDICATED);
    if(pll)CHECK(lastWrite(b,first,0)<lastWrite(b,first,17));
    auto normal=settings();normal.activeSleep=sleep;first=b.frames;
    configured(d,b,normal);CHECK(b.words[0]==0x3500 && (b.words[17]&0x8000U)==0U);
    CHECK(!d.powerSnapshot().requestedPowerDown && !d.powerSnapshot().observedBmcrPowerDown);
    CHECK(writes(b,first,0)==1U);
    if(pll)CHECK(lastWrite(b,first,17)<lastWrite(b,first,0));
  }
  // Toggling only active-sleep policy must never transiently enable a PLL
  // the owner deliberately disabled. PHYSCR7000 and8000 are separate fields.
  {
    Controller b;Driver d;ready(d,b);auto down=settings(PowerRequest::IEEE_POWER_DOWN);down.disablePll=true;
    configured(d,b,down);
    for(bool activeSleep:{true,false}) {
      down.activeSleep=activeSleep;const auto first=b.frames;configured(d,b,down);
      CHECK(writes(b,first,17)!=0U);
      for(unsigned i=first;i<b.frames;++i)if(b.trace[i].target==17U && b.trace[i].kind==FrameKind::WRITE) {
        CHECK((b.trace[i].value&0x8000U)!=0U);
      }
      CHECK((b.words[17]&0xF000U)==(activeSleep?0xE000:0x8000));
    }
  }
  // Poison commands only at the atomic BMCR read: exact independent table8-19
  // neutral compositions are B700 ->3D00 (down), B700 ->3500 (normal).
  for(bool down:{false,true}) {
    Controller b;Driver d;ready(d,b);b.words[0]=0x3500;
    const auto first=b.frames;b.poisonBmcrFrame=static_cast<int>(first+5U);
    const auto token=configure(d,b,settings(down?PowerRequest::IEEE_POWER_DOWN:PowerRequest::NORMAL));
    CHECK(drain(d,b,token).is(Err::INVALID_CONFIG));const auto result=take(d,token);
    CHECK(result.cleanupOwed && !d.isInitialized());
    CHECK(b.trace[lastWrite(b,first,0)].value==(down?0x3D00:0x3500));
  }
  // Hardware pin assertion does not become a persistent software request.
  Controller b;Driver d;ready(d,b);b.powerPin=true;const auto first=b.frames;
  const auto t=configure(d,b,settings());CHECK(drain(d,b,t).is(Err::CONFIG_MISMATCH));
  const auto failed=take(d,t);CHECK(failed.cleanupOwed && writes(b,first,0)==1U);
  CHECK((b.words[0]&0x0800U)==0U && !d.powerSnapshot().requestedPowerDown);
  CHECK(d.powerSnapshot().observedBmcrPowerDown && d.powerSnapshot().state==PowerState::WAKE_BLOCKED_OR_UNCONFIRMED);
  b.powerPin=false;PowerSnapshot live;CHECK(d.readPowerStatus(live,{1000000,3}).ok());
  CHECK(!live.observedBmcrPowerDown && d.operationSnapshot().cleanupOwed);
  OperationToken restore;CHECK(d.startInitialize({ResetKind::NONE},budget(),restore).inProgress());
  CHECK(drain(d,b,restore).ok());CHECK(!take(d,restore).cleanupOwed);
}
void failures() {
  // Every phase/frame of both entry and PLL-exit: a terminal failure stops
  // immediately and never retries. Already accepted writes create OP9 debt.
  for(bool exit:{false,true}) {
    Controller ref;Driver reference;ready(reference,ref);auto down=settings(PowerRequest::IEEE_POWER_DOWN);down.disablePll=true;
    if(exit)configured(reference,ref,down);
    const auto first=ref.frames,firstCall=ref.calls;configured(reference,ref,exit?settings():down);
    const auto count=ref.frames-first;
    for(unsigned fault=0;fault<count;++fault)for(bool accepted:{false,true}) {
      Controller b;Driver d;ready(d,b);if(exit)configured(d,b,down);
      const auto base=b.frames;b.failFrame=static_cast<int>(base+fault);b.acceptedFailure=accepted;
      const auto t=configure(d,b,exit?settings():down);const auto status=drain(d,b,t);
      CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail==-28028 && b.frames==base+fault+1U);
      const auto r=take(d,t);CHECK(!r.persistentRestored && d.powerSnapshot().trust!=ConfigurationTrust::VERIFIED);
      bool priorWrite=false;for(unsigned n=0;n<fault;++n)priorWrite|=ref.trace[first+n].kind==FrameKind::WRITE;
      if(priorWrite || (accepted && ref.trace[first+fault].kind==FrameKind::WRITE))CHECK(r.cleanupOwed && r.mutationPossible);
      CHECK(b.calls==b.releases && !b.locked);
    }
    // Controller-release failures and late/illegal receipts retain effects too.
    for(unsigned call=0;call<ref.calls-firstCall;++call) {
      Controller b;Driver d;ready(d,b);if(exit)configured(d,b,down);
      b.releaseCall=static_cast<int>(b.calls+call);const auto t=configure(d,b,exit?settings():down);
      CHECK(drain(d,b,t).is(Err::MDIO_ERROR));const auto r=take(d,t);
      CHECK(!r.persistentRestored && !r.failingBatch.release.ok());
    }
  }
  // Partial live observations retain returned words but not an ordinary aggregate.
  for(unsigned fault=0;fault<3U;++fault) {
    Controller b;Driver d;ready(d,b);b.words[0]=0x3500;b.words[17]=0x6108;b.words[22]=0x0100;
    b.failFrame=static_cast<int>(b.frames+fault);PowerSnapshot sentinel;sentinel.rawBmcr=0xCAFE;
    CHECK(d.readPowerStatus(sentinel,{1000000,3}).is(Err::MDIO_TIMEOUT));CHECK(sentinel.rawBmcr==0xCAFE);
    const auto partial=d.powerSnapshot();CHECK(partial.observedStale);
    if(fault>=1U)CHECK(partial.bmcrValid && partial.rawBmcr==0x3500);
    if(fault>=2U)CHECK(partial.physcrValid && partial.rawPhyscr==0x6108);
  }
  // A returned contradiction invalidates prior trust immediately, even if
  // a later word fails. An observed reset invalidates identity/readiness too.
  for(bool reset:{false,true})for(bool laterFailure:{false,true}) {
    Controller b;Driver d;ready(d,b);configured(d,b);const auto old=d.powerSnapshot();
    if(reset)b.poisonBmcrFrame=static_cast<int>(b.frames);else b.powerPin=true;
    if(laterFailure)b.failFrame=static_cast<int>(b.frames+1U);
    PowerSnapshot sentinel;sentinel.rawBmcr=0xCAFE;const auto status=d.readPowerStatus(sentinel,{1000000,3});
    if(laterFailure)CHECK(status.is(Err::MDIO_TIMEOUT));
    else if(reset)CHECK(status.is(Err::INVALID_CONFIG));else CHECK(status.ok());
    if(laterFailure || reset)CHECK(sentinel.rawBmcr==0xCAFE && d.powerSnapshot().observedStale && d.powerSnapshot().observedAtUs==old.observedAtUs);
    CHECK(d.powerSnapshot().trust!=ConfigurationTrust::VERIFIED);
    if(reset)CHECK(!d.isInitialized() && !d.capabilities().valid);
  }
  // Every supported verification field independently rejects a wrong literal.
  for(unsigned field=0;field<3U;++field) {
    Controller b;Driver d;ready(d,b);auto down=settings(PowerRequest::IEEE_POWER_DOWN);down.disablePll=true;
    const unsigned offset=field==0U?4U:(field==1U?7U:10U);
    b.corruptFrame=static_cast<int>(b.frames+offset);b.corruptMask=field==0U?0x1000:(field==1U?0x0800:0x8000);
    const auto token=configure(d,b,down);CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));const auto r=take(d,token);
    CHECK(r.mismatch.valid && r.cleanupOwed && !r.persistentRestored);
  }
  // A previously complete observation retains its old timestamp on a failed refresh.
  {
    Controller b;Driver d;ready(d,b);configured(d,b);const auto old=d.powerSnapshot();
    b.failFrame=static_cast<int>(b.frames+2U);PowerSnapshot output;output.rawBiscr=0xCAFE;
    CHECK(d.readPowerStatus(output,{1000000,3}).is(Err::MDIO_TIMEOUT));
    CHECK(output.rawBiscr==0xCAFE && d.powerSnapshot().observedAtUs==old.observedAtUs && d.powerSnapshot().observedStale);
  }
  // A stable-field mismatch survives a subsequent release error.
  Controller b;Driver d;ready(d,b);b.corruptFrame=static_cast<int>(b.frames+4U);b.corruptMask=0x1000;
  b.releaseCall=static_cast<int>(b.calls+3U);const auto t=configure(d,b,settings());
  CHECK(drain(d,b,t).is(Err::MDIO_ERROR));const auto result=take(d,t);
  CHECK(result.cleanupOwed && result.mismatch.valid);
}
void replay() {
  for(auto reset:{ResetKind::NONE,ResetKind::PCS,ResetKind::HARD,ResetKind::RESTART})for(bool pll:{false,true}) {
    Controller b;Driver d;ready(d,b);auto p=settings(PowerRequest::IEEE_POWER_DOWN);p.activeSleep=true;p.disablePll=pll;
    configured(d,b,p);OperationToken t;CHECK(d.startInitialize({reset},budget(),t).inProgress());
    const auto replayStatus=drain(d,b,t);
    if(!replayStatus.ok())std::fprintf(stderr,"reset=%u pll=%u code=%u detail=%d\n",unsigned(reset),unsigned(pll),unsigned(replayStatus.code),int(replayStatus.detail));
    CHECK(replayStatus.ok());const auto r=take(d,t);CHECK(r.persistentRestored && !r.cleanupOwed);
    CHECK(d.powerSnapshot().requestedPowerDown && d.powerSnapshot().trust==ConfigurationTrust::VERIFIED);
    CHECK((b.words[0]&0x0800U)!=0U && (b.words[17]&0xF000U)==(pll?0xE000:0x6000));
    const auto count=b.mutations;CHECK(d.startVerify(budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    CHECK(b.mutations==count); // selector writes do not count as target mutation.
  }
  // Failure in each HARD restoration power-tail frame preserves the reset's
  // earlier accepted effects and cannot publish initialized/verified success.
  {
    Controller reference;Driver ref;ready(ref,reference);auto desired=settings(PowerRequest::IEEE_POWER_DOWN);desired.disablePll=true;
    configured(ref,reference,desired);const auto begin=reference.frames;OperationToken token;
    CHECK(ref.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(ref,reference,token).ok());take(ref,token);
    const auto length=reference.frames-begin;CHECK(length>12U);
    for(unsigned offset=length-12U;offset<length;++offset) {
      Controller b;Driver d;ready(d,b);configured(d,b,desired);b.failFrame=static_cast<int>(b.frames+offset);
      CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());CHECK(drain(d,b,token).is(Err::MDIO_TIMEOUT));
      const auto failed=take(d,token);CHECK(failed.cleanupOwed && failed.mutationPossible && !failed.persistentRestored);
      CHECK(!d.isInitialized() && d.powerSnapshot().trust!=ConfigurationTrust::VERIFIED);
    }
  }
  // Every PCS frame, including pre-reset PLL enable and final replay, is
  // independently fail-injected. Earlier effects never disappear on failure.
  {
    Controller reference;Driver ref;ready(ref,reference);auto desired=settings(PowerRequest::IEEE_POWER_DOWN);desired.disablePll=true;
    configured(ref,reference,desired);const auto begin=reference.frames;OperationToken token;
    CHECK(ref.startInitialize({ResetKind::PCS},budget(),token).inProgress());CHECK(drain(ref,reference,token).ok());take(ref,token);
    const auto length=reference.frames-begin;
    for(unsigned offset=0;offset<length;++offset) {
      Controller b;Driver d;ready(d,b);configured(d,b,desired);const auto first=b.frames;b.failFrame=static_cast<int>(first+offset);
      CHECK(d.startInitialize({ResetKind::PCS},budget(),token).inProgress());CHECK(drain(d,b,token).is(Err::MDIO_TIMEOUT));
      const auto failed=take(d,token);CHECK(!failed.persistentRestored && b.frames==first+offset+1U);
      bool effect=false;for(unsigned i=0;i<=offset;++i)effect|=reference.trace[begin+i].kind==FrameKind::WRITE && reference.trace[begin+i].target!=13U && reference.trace[begin+i].target!=14U;
      if(effect)CHECK(failed.cleanupOwed && failed.mutationPossible);
    }
  }
  // Accepted PLL mutation followed by timeout leaves cached zero, actual one.
  // PCS preparation must read fresh controls, not trust that historical zero.
  {
    Controller b;Driver d;ready(d,b);auto desired=settings(PowerRequest::IEEE_POWER_DOWN);desired.disablePll=true;
    const auto failedToken=configure(d,b,desired);b.failFrame=static_cast<int>(b.frames+9U);
    CHECK(drain(d,b,failedToken).is(Err::MDIO_TIMEOUT));const auto failed=take(d,failedToken);
    CHECK(failed.cleanupOwed && (b.words[17]&0x8000U)!=0U && (d.powerSnapshot().rawPhyscr&0x8000U)==0U);
    b.failFrame=-1;const auto first=b.frames;OperationToken restore;
    CHECK(d.startInitialize({ResetKind::PCS},budget(),restore).inProgress());CHECK(drain(d,b,restore).ok());
    const auto restored=take(d,restore);CHECK(restored.persistentRestored && !restored.cleanupOwed);
    bool freshPllSeen=false,cleared=false,verified=false,reset=false;
    for(unsigned i=first;i<b.frames;++i) {
      const auto& frame=b.trace[i];
      if(frame.kind==FrameKind::READ && frame.target==17U) {
        if(cleared)verified=true;else freshPllSeen=true;
      }
      if(frame.kind==FrameKind::WRITE && frame.target==17U && (frame.value&0x8000U)==0U) {
        CHECK(freshPllSeen && !reset);cleared=true;
      }
      if(frame.kind==FrameKind::WRITE && frame.target==0U && frame.value==0x8000) {
        CHECK(freshPllSeen && cleared && verified);reset=true;break;
      }
    }
    CHECK(reset && (b.words[17]&0x8000U)!=0U && d.powerSnapshot().trust==ConfigurationTrust::VERIFIED);
  }
  // HARD/VOD effects survive a later power-admission failure. An external
  // stimulus observed at the replay tail is not permission to erase reset debt.
  {
    Controller b;Driver d;ready(d,b);auto desired=settings();desired.activeSleep=true;configured(d,b,desired);
    const auto first=b.frames;OperationToken token;CHECK(d.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    Status status{Err::IN_PROGRESS,0,"Pending"};bool injected=false;
    for(unsigned n=0;n<128U && status.inProgress();++n) {
      status=poll(d,b,token);
      if(!injected && writes(b,first,0x030E)!=0U) { b.words[17]|=4U;injected=true; }
    }
    CHECK(injected && status.is(Err::INVALID_CONFIG));const auto failed=take(d,token);
    CHECK(failed.commandAccepted && failed.mutationPossible && failed.cleanupOwed && !failed.persistentRestored);
    CHECK(b.ext[0x030E]==0x4A40 && writes(b,first,0x030E)==1U && writes(b,first,17)==0U);
    CHECK(!d.isInitialized() && d.powerSnapshot().trust!=ConfigurationTrust::VERIFIED);
    const auto calls=b.calls,clocks=b.clocks;CHECK(d.end().is(Err::BUSY));CHECK(b.calls==calls && b.clocks==clocks);
    b.words[17]&=0xFFFB;OperationToken restore;CHECK(d.startInitialize({ResetKind::NONE},budget(),restore).inProgress());
    CHECK(restore.value!=token.value && drain(d,b,restore).ok());CHECK(!take(d,restore).cleanupOwed);
    CHECK(failed.status.is(Err::INVALID_CONFIG) && failed.cleanupOwed);
  }
  // Existing profile application must preserve owned intent, not the pin OR.
  for(bool down:{false,true})for(bool assertPin:{false,true}) {
    Controller b;Driver d;ready(d,b);configured(d,b,settings(down?PowerRequest::IEEE_POWER_DOWN:PowerRequest::NORMAL));
    LinkSettings profile;profile.mode=LinkMode::AUTONEGOTIATED;profile.advertisement.technologyMask=8;
    OperationToken t;CHECK(d.startConfigure(profile,{},budget(),t).inProgress());
    bool asserted=false;Status status{Err::IN_PROGRESS,0,"Pending"};const auto first=b.frames;
    for(unsigned n=0;n<256U && status.inProgress();++n) {
      // Assert after the profile's first BMCR read and before its atomic update.
      if(assertPin && !asserted)for(unsigned i=first;i<b.frames;++i)if(b.trace[i].reg==0 && b.trace[i].kind==FrameKind::READ) { b.powerPin=true;asserted=true;break; }
      status=poll(d,b,t);
    }
    CHECK(!status.inProgress());const auto r=take(d,t);CHECK(status.ok() || (assertPin && !down && status.is(Err::CONFIG_MISMATCH)));
    CHECK(((b.words[0]&0x0800U)!=0U)==down && d.powerSnapshot().requestedPowerDown==down);
    if(assertPin && !down)CHECK(r.cleanupOwed);
    b.powerPin=false;CHECK(((b.read(0)&0x0800U)!=0U)==down);
  }
  // External reset/POR invalidates proof but preserves explicit replay intent.
  for(auto reason:{InvalidationReason::EXTERNAL_RESET,InvalidationReason::EXTERNAL_POWER_ON}) {
    Controller b;Driver d;ready(d,b);auto p=settings();p.activeSleep=true;configured(d,b,p);
    const auto calls=b.calls,clocks=b.clocks;CHECK(d.invalidateDeviceState(reason).ok());
    CHECK(b.calls==calls && b.clocks==clocks && d.powerSnapshot().observedStale && d.powerSnapshot().desired.activeSleep);
    b.defaults();OperationToken t;CHECK(d.startInitialize({ResetKind::NONE},budget(),t).inProgress());CHECK(drain(d,b,t).ok());take(d,t);
    CHECK((b.words[17]&0x7000U)==0x6000U);
  }
}
void auditInterleavedEvidence() {
  // OP14 permits exact identity reads between polls. A newly accepted mode
  // invalidates earlier context; the last BISCR read cannot restore it.
  for(unsigned phase=0;phase<7U;++phase) {
    Controller b;Driver d;ready(d,b);auto s=settings();s.activeSleep=true;const auto first=b.frames;
    const auto token=configure(d,b,s);
    for(unsigned n=0;n<phase;++n)CHECK(poll(d,b,token).inProgress());
    b.id2=0xA111;DeviceIdentity identity;CHECK(d.readIdentity(identity,{1000000,2}).ok());
    const auto calls=b.calls,clocks=b.clocks;
    const auto status=d.pollOperation(token);
    CHECK(status.is(Err::INVALID_CONFIG) && status.detail==static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
    CHECK(b.calls==calls && b.clocks==clocks && !d.isInitialized());
    const auto result=take(d,token);CHECK(!result.persistentRestored);
    CHECK(result.cleanupOwed==(writes(b,first,0)!=0U || writes(b,first,17)!=0U));
  }
  // An independently returned reset keeps its invalidation even if the next
  // live-read frame fails. Its raw transport status is separate from the
  // running operation invalidated by that read, as with public readIdentity.
  for(bool laterFailure:{false,true}) {
    Controller b;Driver d;ready(d,b);const auto token=configure(d,b,settings());
    for(unsigned n=0;n<6U;++n)CHECK(poll(d,b,token).inProgress());
    b.poisonBmcrFrame=static_cast<int>(b.frames);
    if(laterFailure)b.failFrame=static_cast<int>(b.frames+1U);
    PowerSnapshot sentinel;sentinel.rawBmcr=0xCAFE;
    const auto live=d.readPowerStatus(sentinel,{1000000,3});
    CHECK(live.is(laterFailure?Err::MDIO_TIMEOUT:Err::INVALID_CONFIG));
    if(laterFailure)CHECK(live.detail==-28028);
    CHECK(sentinel.rawBmcr==0xCAFE && !d.isInitialized());
    const auto calls=b.calls,clocks=b.clocks;const auto status=d.pollOperation(token);
    CHECK(status.is(Err::INVALID_CONFIG) && status.detail==static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED));
    CHECK(b.calls==calls && b.clocks==clocks);
    const auto result=take(d,token);CHECK(!result.persistentRestored && result.cleanupOwed);
  }
  // A later valid control word contradicts an already verified field. The
  // remaining successful state read cannot erase that evidence or restore trust.
  {
    Controller b;Driver d;ready(d,b);auto s=settings();s.activeSleep=true;const auto token=configure(d,b,s);
    for(unsigned n=0;n<6U;++n)CHECK(poll(d,b,token).inProgress());
    b.words[17]&=0x8FFF;PowerSnapshot observed;CHECK(d.readPowerStatus(observed,{1000000,3}).ok());
    CHECK(observed.rawPhyscr==0U && observed.trust==ConfigurationTrust::DIRTY);
    CHECK(!drain(d,b,token).ok());const auto result=take(d,token);
    CHECK(result.cleanupOwed && !result.persistentRestored && d.powerSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
  // The final PLL read also returns the active-sleep controls. A valid
  // contradicting bit in that same word must not be ignored by the PLL mask.
  {
    Controller b;Driver d;ready(d,b);auto s=settings(PowerRequest::IEEE_POWER_DOWN);s.disablePll=s.activeSleep=true;
    b.corruptFrame=static_cast<int>(b.frames+10U);b.corruptMask=0x6000;
    const auto token=configure(d,b,s);CHECK(drain(d,b,token).is(Err::CONFIG_MISMATCH));
    const auto result=take(d,token);CHECK(result.mismatch.valid && result.cleanupOwed && !result.persistentRestored);
    CHECK(d.powerSnapshot().rawPhyscr==0x8000 && d.powerSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
  // A power operation neither repairs nor erases another persistent owner's
  // uncertainty. RXFS is read through the same collector, invalidating IRQ trust.
  for(bool during:{false,true}) {
    Controller b;Driver d;ready(d,b);InterruptSettings irq;irq.maintenanceAdmitted=irq.pinAvailable=true;
    OperationToken irqToken;CHECK(d.startConfigureInterrupts(irq,budget(),irqToken).inProgress());
    CHECK(drain(d,b,irqToken).ok());take(d,irqToken);
    OperationToken token{999};
    if(during) { token=configure(d,b,settings());for(unsigned n=0;n<3U;++n)CHECK(poll(d,b,token).inProgress()); }
    b.ext[0x04A1]=0;EvidenceSnapshot evidence;
    CHECK(d.acquireEvidence(evidenceBit(EvidenceSource::RXFS),evidence,{1000000,4}).ok());
    CHECK(d.interruptSnapshot().trust==ConfigurationTrust::DIRTY);
    const auto calls=b.calls,clocks=b.clocks;
    if(during) {
      CHECK(!drain(d,b,token).ok());const auto result=take(d,token);
      CHECK(result.cleanupOwed && !result.persistentRestored);
    } else {
      CHECK(d.startConfigurePower(settings(),budget(),token).is(Err::INVALID_CONFIG));
      CHECK(token.value==999U && b.calls==calls && b.clocks==clocks);
    }
    CHECK(d.interruptSnapshot().trust==ConfigurationTrust::DIRTY);
  }
}
void ownership() {
  auditInterleavedEvidence();
  // Every cancellation boundary is silent and preserves accepted effects.
  for(unsigned phase=0;phase<7U;++phase) {
    Controller b;Driver d;ready(d,b);const auto first=b.frames;const auto t=configure(d,b,settings());
    for(unsigned n=0;n<phase;++n)CHECK(poll(d,b,t).inProgress());
    const auto calls=b.calls,clocks=b.clocks;CHECK(d.cancel(t).ok());CHECK(b.calls==calls && b.clocks==clocks);
    OperationToken other{999};CHECK(d.startConfigurePower(settings(),budget(),other).is(Err::BUSY));CHECK(other.value==999);
    const auto r=take(d,t);CHECK(r.status.is(Err::CANCELLED));
    if(writes(b,first,17)!=0U || writes(b,first,0)!=0U) {
      CHECK(r.cleanupOwed && d.end().is(Err::BUSY));
      CHECK(d.startConfigurePower(settings(),budget(),other).is(Err::BUSY));
      CHECK(d.startInitialize({ResetKind::NONE},budget(),other).inProgress());CHECK(drain(d,b,other).ok());take(d,other);
    }
    CHECK(d.end().ok());
  }
  // Cancel before each of the nine PLL entry/exit callbacks. Restoration
  // receives a separate token/budget/result and never replaces cancellation.
  for(bool exit:{false,true})for(unsigned phase=0;phase<9U;++phase) {
    Controller bus;Driver driver;ready(driver,bus);auto down=settings(PowerRequest::IEEE_POWER_DOWN);down.disablePll=true;
    if(exit)configured(driver,bus,down);
    const auto first=bus.frames;const auto token=configure(driver,bus,exit?settings():down);
    for(unsigned n=0;n<phase;++n)CHECK(poll(driver,bus,token).inProgress());
    const auto calls=bus.calls,clocks=bus.clocks;CHECK(driver.cancel(token).ok());CHECK(bus.calls==calls && bus.clocks==clocks);
    const auto cancelled=take(driver,token);CHECK(cancelled.status.is(Err::CANCELLED));
    const bool effect=writes(bus,first,0)!=0U || writes(bus,first,17)!=0U;
    CHECK(cancelled.cleanupOwed==effect);
    if(effect) {
      CHECK(driver.end().is(Err::BUSY));OperationToken denied{999};
      CHECK(driver.startInitialize({ResetKind::NONE},budget(0),denied).is(Err::OPERATION_TIMEOUT));
      CHECK(denied.value==999 && driver.operationSnapshot().cleanupOwed);
      CHECK(bus.calls==calls && bus.clocks==clocks);
    }
    OperationToken restore;CHECK(driver.startInitialize({ResetKind::NONE},budget(),restore).inProgress());
    CHECK(restore.value!=token.value && drain(driver,bus,restore).ok());const auto restored=take(driver,restore);
    CHECK(restored.persistentRestored && !restored.cleanupOwed && cancelled.status.is(Err::CANCELLED));
    CHECK(cancelled.cleanupOwed==effect && driver.powerSnapshot().requestedPowerDown==!exit);
  }
  Controller b;Driver d,other;ready(d,b);CHECK(other.bind(b.config(2)).ok());b.competitor=&other;
  const auto first=b.frames;configured(d,b);CHECK(b.excluded==b.frames-first && b.releases==b.calls);
  b.competitor=nullptr;
  const auto calls=b.calls,clocks=b.clocks;auto one=d.powerSnapshot();auto two=d.powerSnapshot();
  CHECK(one.rawBmcr==two.rawBmcr && b.calls==calls && b.clocks==clocks && !other.powerSnapshot().desiredValid);
  CHECK(d.end().ok() && b.calls==calls && b.clocks==clocks);
}
void timing() {
  Controller b;Driver d;ready(d,b);const auto t=configure(d,b,settings());
  CHECK(poll(d,b,t).inProgress());CHECK(poll(d,b,t).inProgress());const auto first=b.frames;
  CHECK(d.pollOperation(t,1).inProgress() && b.frames==first); // next complete RMW needs two frames.
  CHECK(drain(d,b,t).ok());take(d,t);
  for(unsigned kind=0;kind<4U;++kind) {
    Controller bus;Driver driver;ready(driver,bus);const auto firstCall=bus.calls;
    if(kind==0U)bus.lateCall=static_cast<int>(firstCall+2U);
    if(kind==1U)bus.regressCall=static_cast<int>(firstCall+2U);
    if(kind==2U)bus.invalidCall=static_cast<int>(firstCall+2U);
    if(kind==3U)bus.badReceiptCall=static_cast<int>(firstCall+2U);
    const auto token=configure(driver,bus,settings());CHECK(!drain(driver,bus,token).ok());const auto r=take(driver,token);
    CHECK(r.cleanupOwed && !r.persistentRestored);
    if(kind==1U)CHECK(driver.transportSnapshot().clockInvalid);
  }
  for(unsigned empty=0;empty<2U;++empty) {
    Controller bus;Driver driver;ready(driver,bus);const auto calls=bus.calls,clocks=bus.clocks;
    OperationToken token{999};CHECK(driver.startConfigurePower(settings(),budget(empty?2048:0,empty?0:1000000),token).is(Err::OPERATION_TIMEOUT));
    CHECK(token.value==999 && bus.calls==calls && bus.clocks==clocks);
  }
  Controller frozen;Driver stuck;ready(stuck,frozen);const auto token=configure(stuck,frozen,settings());
  const auto calls=frozen.calls;Status status{Err::IN_PROGRESS,0,"Pending"};
  for(unsigned n=0;n<12U && status.inProgress();++n)status=stuck.pollOperation(token,0);
  CHECK(status.is(Err::OPERATION_TIMEOUT) && frozen.calls==calls);take(stuck,token);
  // Even a locally exhausted refresh invalidates current aggregate freshness,
  // without replacing the caller's output or the old observation timestamp.
  {
    Controller bus;Driver driver;ready(driver,bus);configured(driver,bus);const auto old=driver.powerSnapshot();
    PowerSnapshot sentinel;sentinel.rawBmcr=0xCAFE;const auto calls=bus.calls;
    CHECK(driver.readPowerStatus(sentinel,{1000000,0}).is(Err::OPERATION_TIMEOUT));
    const auto stale=driver.powerSnapshot();CHECK(sentinel.rawBmcr==0xCAFE && bus.calls==calls);
    CHECK(stale.observedStale && stale.state==PowerState::UNKNOWN && stale.observedAtUs==old.observedAtUs);
  }
  // Zero ticks are valid clock values; lag supplies a conservative upper stamp.
  Controller zero;zero.now=0;zero.frameTime=0;Driver zeroDriver;ready(zeroDriver,zero);configured(zeroDriver,zero);
  CHECK(zeroDriver.powerSnapshot().timeValid && zeroDriver.powerSnapshot().observedAtUs==1U);
  Controller overflow;Driver nearEnd;ready(nearEnd,overflow);overflow.now=std::numeric_limits<uint64_t>::max()-1U;
  OperationToken end;CHECK(nearEnd.startConfigurePower(settings(),budget(64,std::numeric_limits<uint64_t>::max()),end).inProgress());
  const auto frames=overflow.frames;CHECK(!nearEnd.pollOperation(end).ok() && overflow.frames==frames);take(nearEnd,end);
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"policy",policy},{"transitions",transitions},{"failures",failures},
    {"replay",replay},{"ownership",ownership},{"timing",timing}};
  return test::runSuites(argc,argv,suites);
}
