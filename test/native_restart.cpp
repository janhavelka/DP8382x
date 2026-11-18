#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include <array>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;

// Independent literal BMCR/identity/VOD model. This checks terminal ownership
// and receipts, not negotiation completion or physical timing.
struct Bus {
  std::array<uint16_t, 32> words{};
  uint16_t selector{0}, address{0}, vod{0x8400}, written{0};
  uint64_t now{100};
  unsigned calls{0};
  int fail{-1};
  bool late{false}, releaseFailure{false}, holdCommand{false}, holdReset{false};
  bool malformed{false}, effect{true};
  Driver* reentrant{nullptr};
  unsigned rejectedReentries{0};
  MdioBatch last{};
  Bus() { words[0]=0x3100; words[4]=0x01E1; words[25]=0x8001; }
  static uint64_t clock(void* context) noexcept { return static_cast<Bus*>(context)->now; }
  Config config() noexcept {
    Config c; c.variant=Variant::DP83826I; c.interface=MacInterface::RMII; c.phyAddress=1;
    c.transport={this,execute,clock,{8,4,1}}; return c;
  }
  uint16_t read(uint8_t reg) noexcept {
    if (reg==2) { return 0x2000; }
    if (reg==3) { return 0xA131; }
    if (reg==0 && !holdCommand) { words[0]&=0xFDFF; }
    if (reg==0 && !holdReset) { words[0]&=0x7FFF; }
    if (reg==14 && selector==0x401F) {
      if (address==0x030E) { return vod; }
      if (address==0x0467) { return 0x1234; }
      if (address==0x0468) { return 0x5678; }
    }
    return words[reg];
  }
  void write(uint8_t reg,uint16_t word) noexcept {
    if (reg==13) { selector=word; }
    else if (reg==14 && selector==0x001F) { address=word; }
    else if (reg==14 && selector==0x401F && address==0x030E) { vod=word; }
    else { words[reg]=word; }
    if (reg==0) { written=word; }
  }
  static BatchResult execute(void* context,const MdioBatch& batch,const IoBudget& budget) noexcept {
    auto& bus=*static_cast<Bus*>(context); ++bus.calls; bus.last=batch;
    if (bus.reentrant!=nullptr) {
      CHECK(bus.reentrant->restartAutoNegotiation(budget).is(Err::BUSY));
      ++bus.rejectedReentries;
    }
    BatchResult result; result.status=Status::Ok(); result.releaseStatus=Status::Ok(); result.controllerReusable=true;
    for (uint8_t i=0;i<batch.frameCount;++i) {
      const auto& request=batch.frames[i]; auto& frame=result.frames[i]; frame.attempted=true;
      uint16_t word=request.value;
      if (request.readIndex!=NO_READ_INDEX) {
        word=static_cast<uint16_t>(word|(result.frames[request.readIndex].value&request.preserveMask));
      }
      if (bus.fail!=i || bus.effect) {
        if (request.kind==FrameKind::WRITE) { bus.write(request.registerAddress,word); }
        else { frame.value=bus.read(request.registerAddress); }
      }
      bus.now+=2;
      if (bus.fail==i) {
        frame.effectPossible=bus.effect;
        result.status=Status::Error(Err::MDIO_TIMEOUT,"Injected terminal failure",-702); break;
      }
      frame.completed=true; frame.valueValid=request.kind==FrameKind::READ;
    }
    if (bus.late) { bus.now=budget.deadlineUs; }
    if (bus.releaseFailure) {
      result.releaseStatus=Status::Error(Err::MDIO_ERROR,"Release failure",-901); result.controllerReusable=false;
    }
    if (bus.malformed) { result.frames[1].valueValid=true; }
    return result;
  }
};
OperationBudget budget() { return {{1000000,512},10,16}; }
void finish(Driver& driver,Bus& bus,OperationToken token) {
  for (unsigned i=0;i<200;++i) {
    const auto status=driver.pollOperation(token);
    if (!status.inProgress()) { CHECK(status.ok()); OperationResult result; CHECK(driver.takeResult(token,result).ok()); return; }
    bus.now+=10;
  }
  CHECK(false);
}
void ready(Driver& driver,Bus& bus,bool forced=false) {
  CHECK(driver.bind(bus.config()).ok()); OperationToken token;
  CHECK(driver.startInitialize({},budget(),token).inProgress()); finish(driver,bus,token);
  LinkSettings profile; profile.advertisement.technologyMask=0x0F;
  if (forced) { profile.mode=LinkMode::FORCED; profile.advertisement={}; profile.forced={LinkSpeed::MBPS_100,Duplex::FULL}; }
  CHECK(driver.startConfigure(profile,{},budget(),token).inProgress()); finish(driver,bus,token);
}
void vectors() {
  Bus bus; Driver driver; ready(driver,bus); bus.holdCommand=true;
  const auto calls=bus.calls; const auto health=driver.healthSnapshot();
  bus.words[0]=0x12A5; // Restart already high; legal preserved low bits survive.
  CHECK(driver.restartAutoNegotiation({1000000,3}).ok());
  CHECK(bus.calls==calls+1 && bus.last.frameCount==3);
  CHECK(bus.last.frames[1].preserveMask==0x44FF && bus.last.frames[1].value==0x1200);
  CHECK(bus.written==0x12A5); // Neither reset nor pin-derived power is echoed.
  CHECK(driver.settingsSnapshot().trust==ConfigurationTrust::VERIFIED);
  CHECK(driver.operationSnapshot().state==OperationState::IDLE);
  CHECK(!driver.operationSnapshot().cleanupOwed);
  CHECK(driver.healthSnapshot().totalSuccess==health.totalSuccess+1);
  CHECK(driver.restartAutoNegotiation({1000000,3}).ok()); // No hidden token/result.
  bus.reentrant=&driver;
  CHECK(driver.restartAutoNegotiation({1000000,3}).ok());
  CHECK(bus.rejectedReentries==1);
}
void admission() {
  Bus bus; Driver driver;
  CHECK(driver.restartAutoNegotiation({1000000,3}).is(Err::NOT_BOUND));
  CHECK(driver.bind(bus.config()).ok());
  CHECK(driver.restartAutoNegotiation({1000000,3}).is(Err::NOT_INITIALIZED));
  ready(driver,bus); const auto calls=bus.calls;
  CHECK(driver.restartAutoNegotiation({1000000,2}).is(Err::OPERATION_TIMEOUT));
  CHECK(driver.restartAutoNegotiation({0,3}).is(Err::OPERATION_TIMEOUT));
  CHECK(bus.calls==calls);
  OperationToken token; CHECK(driver.startVerify(budget(),token).inProgress());
  CHECK(driver.restartAutoNegotiation({1000000,3}).is(Err::BUSY));
  CHECK(driver.cancel(token).ok());
  CHECK(driver.restartAutoNegotiation({1000000,3}).detail==static_cast<int32_t>(BusyDetail::RESULT_PENDING));
  Bus forcedBus; Driver forced; ready(forced,forcedBus,true);
  const auto forcedCalls=forcedBus.calls;
  CHECK(forced.restartAutoNegotiation({1000000,3}).is(Err::INVALID_CONFIG)); CHECK(forcedBus.calls==forcedCalls);
}
void failures() {
  for (int index=0;index<3;++index) {
    Bus bus; Driver driver; ready(driver,bus); bus.fail=index;
    const auto status=driver.restartAutoNegotiation({1000000,3});
    CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail==-702);
    CHECK(driver.operationSnapshot().cleanupOwed==(index!=0));
    if (index!=0) {
      CHECK(driver.settingsSnapshot().trust==ConfigurationTrust::DIRTY);
      const auto calls=bus.calls;
      CHECK(driver.restartAutoNegotiation({1000000,3}).detail==static_cast<int32_t>(BusyDetail::CLEANUP_REQUIRED));
      CHECK(bus.calls==calls);
    }
  }
  for (unsigned fault=0;fault<7;++fault) {
    Bus bus; Driver driver; ready(driver,bus);
    if (fault==0) { bus.late=true; }
    if (fault==1) { bus.releaseFailure=true; }
    if (fault==2) { bus.words[0]=0x9000; bus.holdReset=true; }
    if (fault==3) { bus.words[0]=0x1800; } // External power pin must not latch software PD.
    if (fault==4) { bus.malformed=true; }
    if (fault==5) { bus.words[0]=0x3000; } // Mode drift is retained even if write restores it.
    if (fault==6) { bus.words[0]=0x5400; } // Loopback/isolation are preserved, never blessed.
    CHECK(!driver.restartAutoNegotiation({1000000,3}).ok());
    CHECK(driver.operationSnapshot().cleanupOwed);
    CHECK((bus.written&0x8800U)==0U);
    if (fault==2) { CHECK(!driver.isInitialized()); CHECK(!driver.capabilities().valid); }
  }
  Bus bus; Driver driver; ready(driver,bus); bus.fail=1; bus.effect=false;
  CHECK(driver.restartAutoNegotiation({1000000,3}).is(Err::MDIO_TIMEOUT));
  CHECK(!driver.operationSnapshot().cleanupOwed);
  CHECK(driver.settingsSnapshot().trust==ConfigurationTrust::DIRTY);
}
void basicControl() {
  // Independently enumerate BMCR 13/12/8. Maintenance never rewrites desired
  // settings, blesses the result or echoes RESET/RESTART/pin-derived power.
  for (uint16_t value : {0x0000,0x0100,0x1000,0x1100,0x2000,0x2100,0x3000,0x3100}) {
    Bus bus; Driver driver; ready(driver,bus); bus.holdCommand=true;
    bus.words[0]=0x1AA5;
    const auto calls=bus.calls;
    CHECK(driver.modifyBasicControl(0x3100,value,{1000000,2}).ok());
    CHECK(bus.calls==calls+1 && bus.last.frameCount==2);
    CHECK(bus.last.frames[1].preserveMask==0x44FF);
    CHECK(bus.written==static_cast<uint16_t>(value|0x00A5));
    CHECK(!driver.isInitialized() && !driver.capabilities().valid);
    CHECK(driver.settingsSnapshot().desired.mode==LinkMode::AUTONEGOTIATED);
    CHECK(driver.settingsSnapshot().trust!=ConfigurationTrust::VERIFIED);
  }
  Bus bus; Driver driver; ready(driver,bus); const auto calls=bus.calls;
  for (uint32_t mask : {0U,0x8000U,0x0200U,0x0800U,0x40U,0x10000U})
    CHECK(driver.modifyBasicControl(mask,0,{1000000,2}).is(Err::INVALID_PARAM));
  CHECK(driver.modifyBasicControl(0x1000,0x2000,{1000000,2}).is(Err::INVALID_PARAM));
  CHECK(driver.modifyBasicControl(0x1000,0x10000,{1000000,2}).is(Err::INVALID_PARAM));
  CHECK(driver.modifyBasicControl(0x1000,0,{1000000,1}).is(Err::OPERATION_TIMEOUT));
  CHECK(bus.calls==calls && driver.isInitialized());
  for (int index=0;index<2;++index) {
    Bus failed; Driver target; ready(target,failed); failed.fail=index;
    CHECK(target.modifyBasicControl(0x1000,0,{1000000,2}).is(Err::MDIO_TIMEOUT));
    CHECK(target.isInitialized()==(index==0));
  }
  for (unsigned fault=0;fault<2;++fault) {
    Bus failed; Driver target; ready(target,failed);
    failed.late=fault==0; failed.releaseFailure=fault==1;
    CHECK(!target.modifyBasicControl(0x1000,0,{1000000,2}).ok());
    CHECK(!target.isInitialized());
  }
}
} // namespace
int main(int argc,char** argv) {
  const test::Suite suites[]={{"vectors",vectors},{"admission",admission},{"failures",failures},{"basic_control",basicControl}};
  return test::runSuites(argc,argv,suites);
}
