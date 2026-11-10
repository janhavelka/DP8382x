#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include "../examples/portable/LinkStatus.h"

#include <array>
#include <cstdint>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;

// Independent literal SNLS647I controller: tables 8-19..26, 8-34 and 8-43.
// Script positions are physical reads, including destructive reads. No device
// masks or implementation decode helpers are used to construct expected values.
struct Controller {
  std::array<uint16_t, 32> words{};
  std::array<uint8_t, 2048> trace{};
  std::array<uint8_t, 512> sizes{};
  uint16_t selector{0}, address{0}, vod{0x8400};
  uint16_t finalStatusXor{0}, finalBasicXor{0};
  uint64_t now{100}, frameTime{2};
  unsigned frames{0}, calls{0}, clocks{0}, sampleIndex{0}, exclusions{0}, writes{0};
  int failFrame{-1}, lateCall{-1}, regressCall{-1}, releaseFailCall{-1};
  int changeAt{-1};
  bool sampling{false}, firstLow{false}, down{false}, locked{false}, stamps{false};
  Driver* competitor{nullptr};
  Controller() { words[0] = 0x3100; words[4] = 0x0101; words[5] = 0x0101; words[6] = 1; }
  Config config(uint32_t phy = 1) noexcept {
    Config result;
    result.variant = Variant::DP83826I;
    result.interface = MacInterface::RMII;
    result.phyAddress = phy;
    result.transport = {this, execute, clock, {8, 4, 1}};
    return result;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    ++bus.clocks;
    return bus.now;
  }
  uint16_t read(uint8_t reg) noexcept {
    if (reg == 2) return 0x2000;
    if (reg == 3) return 0xA131;
    if (reg == 0) words[0] = static_cast<uint16_t>(words[0] & 0x7DFFU);
    if (reg == 31) words[31] = 0;
    if (reg == 14 && selector == 0x401F) {
      if (address == 0x030E) return vod;
      if (address == 0x0467) return 0x1234;
      if (address == 0x0468) return 0x5678;
    }
    if (sampling) {
      const auto position = sampleIndex++;
      if (static_cast<int>(position) == changeAt) down = !down;
      if (reg == 1) return static_cast<uint16_t>((words[1] &
          ((down || (firstLow && position == 0)) ? 0xFFFBU : 0xFFFFU)) ^
          (position == 8 ? finalBasicXor : 0));
      if (reg == 16) return static_cast<uint16_t>((words[16] & (down ? 0xFFFEU : 0xFFFFU)) ^
          (position == 9 ? finalStatusXor : 0));
    }
    const auto value = words[reg];
    if (reg == 6) words[6] = static_cast<uint16_t>(value & 0xFFFDU);
    return value;
  }
  void write(uint8_t reg, uint16_t value) noexcept {
    ++writes;
    if (reg == 13) selector = value;
    else if (reg == 14 && selector == 0x001F) address = value;
    else if (reg == 14 && selector == 0x401F && address == 0x030E) vod = value;
    else words[reg] = value;
  }
  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget& budget) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    BatchResult result;
    result.releaseStatus = Status::Ok();
    result.controllerReusable = true;
    if (bus.locked) {
      result.status = {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), "Owned"};
      return result;
    }
    bus.locked = true;
    result.status = Status::Ok();
    const auto call = bus.calls++;
    CHECK(call < bus.sizes.size());
    bus.sizes[call] = batch.frameCount;
    CHECK(batch.frameCount <= budget.maxFrames);
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& frame = batch.frames[i];
      auto value = frame.value;
      if (frame.readIndex != 0xFF) {
        CHECK(frame.readIndex < i && result.frames[frame.readIndex].valueValid);
        value = static_cast<uint16_t>((result.frames[frame.readIndex].value & frame.preserveMask) | value);
      }
      CHECK(bus.frames < bus.trace.size());
      bus.trace[bus.frames] = frame.registerAddress;
      const bool fail = static_cast<int>(bus.frames++) == bus.failFrame;
      auto& receipt = result.frames[i];
      receipt.attempted = true;
      if (frame.kind == FrameKind::WRITE) bus.write(frame.registerAddress, value);
      else receipt.value = bus.read(frame.registerAddress);
      bus.now += bus.frameTime;
      if (bus.competitor) {
        uint16_t sentinel = 0xDEAD;
        CHECK(bus.competitor->readClause22(2, sentinel, budget).is(Err::BUSY));
        CHECK(sentinel == 0xDEAD);
        ++bus.exclusions;
      }
      if (fail) {
        receipt.effectPossible = true;
        result.status = {Err::MDIO_TIMEOUT, -19019, "Injected link timeout"};
        break;
      }
      receipt.completed = true;
      receipt.valueValid = frame.kind == FrameKind::READ;
      receipt.timeValid = bus.stamps;
      receipt.completedAtUs = bus.stamps ? bus.now : 0;
    }
    if (static_cast<int>(call) == bus.lateCall) bus.now = budget.deadlineUs;
    if (static_cast<int>(call) == bus.regressCall) bus.now = 0;
    if (static_cast<int>(call) == bus.releaseFailCall) {
      result.releaseStatus = {Err::MDIO_ERROR, -19020, "Release failed"};
      result.controllerReusable = false;
    }
    bus.locked = false;
    return result;
  }
  void sample() noexcept {
    sampling = true; sampleIndex = 0;
  }
};

OperationBudget operationBudget() { return {{1000000, 512}, 10, 8}; }
Status drain(Driver& driver, Controller& bus, OperationToken token) {
  Status result{Err::IN_PROGRESS, 0, "Pending"};
  for (unsigned i = 0; i < 256 && result.inProgress(); ++i) {
    const auto snapshot = driver.operationSnapshot();
    if (snapshot.nextPollValid && bus.now < snapshot.nextPollAtUs) bus.now = snapshot.nextPollAtUs;
    result = driver.pollOperation(token);
    CHECK(!bus.locked);
  }
  CHECK(!result.inProgress());
  return result;
}
void initialize(Driver& driver, Controller& bus) {
  CHECK(driver.bind(bus.config()).ok());
  OperationToken token;
  CHECK(driver.startInitialize({ResetKind::NONE}, operationBudget(), token).inProgress());
  CHECK(drain(driver, bus, token).ok());
  OperationResult result;
  CHECK(driver.takeResult(token, result).ok());
}
void configure(Driver& driver, Controller& bus, const LinkSettings& settings) {
  bus.sampling = false;
  OperationToken token;
  CHECK(driver.startConfigure(settings, operationBudget(), token).inProgress());
  CHECK(drain(driver, bus, token).ok());
  OperationResult result;
  CHECK(driver.takeResult(token, result).ok());
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
}
void up(Controller& bus, uint16_t physts = 0x0015) {
  bus.words[1] = static_cast<uint16_t>(0x0004U | ((physts & 0x0010U) ? 0x0020U : 0U));
  bus.words[16] = physts;
  bus.words[6] = 1;
  bus.down = false;
}
Status refresh(Driver& driver, Controller& bus, LinkSnapshot& output,
               IoBudget budget = {1000000, 10}) {
  bus.sample();
  return driver.readLinkStatus(output, budget);
}
const EvidenceSample& evidence(const EvidenceSnapshot& value, EvidenceSource source) {
  return value.samples[static_cast<uint8_t>(source)];
}

void readPlan() {
  Controller bus; Driver driver; initialize(driver, bus); up(bus);
  bus.firstLow = true;
  bus.words[6] = 3;
  const auto first = bus.frames;
  const auto call = bus.calls;
  const auto started = bus.now;
  const auto before = driver.evidenceSnapshot();
  LinkSnapshot result;
  CHECK(refresh(driver, bus, result).ok());
  constexpr uint8_t expected[] = {1, 1, 16, 0, 4, 5, 6, 25, 1, 16};
  CHECK(bus.frames == first + 10 && bus.calls == call + 2);
  CHECK(bus.sizes[call] == 8 && bus.sizes[call + 1] == 2);
  for (unsigned i = 0; i < 10; ++i) CHECK(bus.trace[first + i] == expected[i]);
  CHECK(result.valid && !result.stale && result.linkUp);
  CHECK(result.rawBmsrFirst == 0x0020 && result.rawBmsrCurrent == 0x0024);
  CHECK(result.rawPhystsFirst == 0x0015 && result.rawPhystsFinal == 0x0015);
  CHECK(result.rawBmsrFinal == 0x0024 && result.rawAner == 3);
  CHECK(result.resolvedModeValid && result.speed == LinkSpeed::MBPS_100 && result.duplex == Duplex::FULL);
  CHECK(result.pauseValid && !result.pauseRx && !result.pauseTx);
  CHECK(result.negotiation == NegotiationState::COMPLETE);
  CHECK(result.timeValid && result.observedAtUs >= bus.now);
  CHECK(result.observedSinceUs == started && result.observedSinceUs <= result.observedAtUs);
  CHECK(result.change == LinkChange::FIRST_VALID && result.observations == 1 && result.changes == 1);
  const auto retained = driver.evidenceSnapshot();
  CHECK(evidence(retained, EvidenceSource::BMSR).samples == evidence(before, EvidenceSource::BMSR).samples + 3);
  CHECK((evidence(retained, EvidenceSource::BMSR).observedZeros & 4U) != 0);
  CHECK(evidence(retained, EvidenceSource::PHYSTS).samples == evidence(before, EvidenceSource::PHYSTS).samples + 2);
  CHECK(evidence(retained, EvidenceSource::ANER).samples == evidence(before, EvidenceSource::ANER).samples + 1);
  CHECK(bus.words[6] == 1); // Exactly one event consumption.
  const auto frames = bus.frames, clocks = bus.clocks;
  for (unsigned i = 0; i < 20; ++i) {
    CHECK(driver.linkSnapshot().observations == 1);
    CHECK(driver.evidenceSnapshot().lastStatus.ok());
  }
  CHECK(bus.frames == frames && bus.clocks == clocks);
}

void transitions() {
  Controller bus; Driver driver; initialize(driver, bus); up(bus);
  LinkSnapshot result;
  CHECK(refresh(driver, bus, result).ok());
  const auto initial = result;
  CHECK(refresh(driver, bus, result).ok());
  CHECK(result.change == LinkChange::REPEATED && result.changes == 1 && result.observations == 2);
  CHECK(result.observedAtUs > initial.observedAtUs);
  bus.down = true;
  const auto batches = driver.transportSnapshot().totalBatches;
  CHECK(refresh(driver, bus, result).ok());
  CHECK(result.valid && !result.stale && !result.linkUp && !result.resolvedModeValid);
  CHECK(result.speed == LinkSpeed::UNKNOWN && result.duplex == Duplex::UNKNOWN && !result.pauseValid);
  CHECK(result.change == LinkChange::CHANGED && result.changes == 2);
  CHECK(driver.transportSnapshot().totalBatches == batches + 2);
  CHECK(driver.transportSnapshot().lastBatch.status.ok());
  up(bus, 0x0001); bus.words[1] = 4;
  CHECK(refresh(driver, bus, result).ok());
  CHECK(result.linkUp && result.negotiation == NegotiationState::IN_PROGRESS && !result.resolvedModeValid);
  up(bus, 0x0013); // 10/half remains transport success, fails a 100/full application policy.
  CHECK(refresh(driver, bus, result).ok());
  CHECK(result.speed == LinkSpeed::MBPS_10 && result.duplex == Duplex::HALF && !result.pauseValid);
  CHECK(!cached100FullUsable(result, result.observedAtUs, 100));
  const auto timestamp = result.observedAtUs;
  bus.failFrame = static_cast<int>(bus.frames);
  CHECK(refresh(driver, bus, result).is(Err::MDIO_TIMEOUT));
  auto stale = driver.linkSnapshot();
  CHECK(stale.valid && stale.stale && stale.change == LinkChange::STALE && stale.observedAtUs == timestamp);
  const auto changes = stale.changes;
  bus.failFrame = static_cast<int>(bus.frames);
  CHECK(refresh(driver, bus, result).is(Err::MDIO_TIMEOUT));
  CHECK(driver.linkSnapshot().changes == changes);
  bus.failFrame = -1;
  CHECK(refresh(driver, bus, result).ok());
  CHECK(result.change == LinkChange::REGAINED && result.changes == changes + 1);
  const auto frames = bus.frames, clocks = bus.clocks;
  driver.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET);
  CHECK(bus.frames == frames && bus.clocks == clocks);
  CHECK(driver.linkSnapshot().stale && driver.linkSnapshot().observedAtUs == result.observedAtUs);
  CHECK(driver.readLinkStatus(result, {1000000, 10}).is(Err::NOT_INITIALIZED));
  CHECK(bus.frames == frames);
}

void coherence() {
  // A permanent transition before either initial current sample is a valid new
  // observation. Every later detectable transition rejects without retry.
  for (bool initialDown : {false, true}) for (int position = 0; position < 10; ++position) {
    Controller bus; Driver driver; initialize(driver, bus); up(bus);
    bus.down = initialDown;
    LinkSnapshot result;
    CHECK(refresh(driver, bus, result).ok());
    const auto old = result;
    const auto frames = bus.frames;
    bus.changeAt = position;
    const auto status = refresh(driver, bus, result);
    if (position <= 1) {
      CHECK(status.ok() && result.linkUp == initialDown);
      CHECK(result.resolvedModeValid == initialDown);
    } else {
      CHECK(status.is(Err::BUSY));
      CHECK(status.detail == static_cast<int32_t>(BusyDetail::OBSERVATION_CHANGED));
      CHECK(result.observedAtUs == old.observedAtUs && result.observations == old.observations);
      CHECK(driver.linkSnapshot().stale);
    }
    CHECK(bus.frames == frames + 10); // Fixed observation, no retry-until-stable.
  }
  for (uint16_t physts : {uint16_t{0x0011}, uint16_t{0x0015}, uint16_t{0x0013}, uint16_t{0x0017}}) {
    Controller bus; Driver driver; initialize(driver, bus); up(bus, physts);
    LinkSnapshot result;
    CHECK(refresh(driver, bus, result).ok() && result.resolvedModeValid);
    CHECK(result.speed == ((physts & 2U) ? LinkSpeed::MBPS_10 : LinkSpeed::MBPS_100));
    CHECK(result.duplex == ((physts & 4U) ? Duplex::FULL : Duplex::HALF));
  }
  // Cross-register disagreement is uncertainty even when both bookends agree.
  Controller bus; Driver driver; initialize(driver, bus); up(bus, 0x0014);
  LinkSnapshot result;
  CHECK(refresh(driver, bus, result).is(Err::BUSY));
  CHECK(!driver.linkSnapshot().valid);
  for (uint16_t modeChange : {uint16_t{2}, uint16_t{4}, uint16_t{6}, uint16_t{16}}) {
    Controller changed; Driver unstable; initialize(unstable, changed); up(changed);
    changed.finalStatusXor = modeChange;
    CHECK(refresh(unstable, changed, result).is(Err::BUSY));
    CHECK(!unstable.linkSnapshot().valid && changed.sampleIndex == 10);
  }
  Controller changed; Driver unstable; initialize(unstable, changed); up(changed);
  changed.finalStatusXor = 16; changed.finalBasicXor = 32;
  CHECK(refresh(unstable, changed, result).is(Err::BUSY)); // AN completion changed coherently between bookends.
}

void pause() {
  // IEEE Table 28B-3 literal 4x4 table. Index bits: 1=Pause, 2=Asym_Pause;
  // expected bits: 1=local RX, 2=local TX. No copied production formula.
  constexpr uint8_t expected[4][4] = {{0,0,0,0}, {0,3,0,3}, {0,0,0,2}, {0,3,1,3}};
  for (unsigned local = 0; local < 4; ++local) for (unsigned partner = 0; partner < 4; ++partner) {
    Controller bus; Driver driver; initialize(driver, bus); up(bus);
    bus.words[4] = static_cast<uint16_t>(0x0101U | (local << 10));
    bus.words[5] = static_cast<uint16_t>(0x0101U | (partner << 10));
    const auto bits = expected[local][partner];
    bus.words[25] = static_cast<uint16_t>(((bits & 1U) ? 0x2000U : 0U) | ((bits & 2U) ? 0x1000U : 0U));
    LinkSnapshot result;
    CHECK(refresh(driver, bus, result).ok());
    CHECK(result.pauseValid && result.pauseRx == ((bits & 1U) != 0) && result.pauseTx == ((bits & 2U) != 0));
    bus.words[25] ^= 0x2000U;
    CHECK(refresh(driver, bus, result).ok());
    CHECK(!result.pauseValid && result.resolvedModeValid);
    CHECK(result.rawPhycr == bus.words[25] && result.rawAnar == bus.words[4] && result.rawAnlpar == bus.words[5]);
  }
  Controller bus; Driver driver; initialize(driver, bus); up(bus);
  bus.words[4] = 0x0500; bus.words[5] = 0x0501; bus.words[25] = 0x3000;
  LinkSnapshot result;
  CHECK(refresh(driver, bus, result).ok() && !result.pauseValid); // Non-IEEE selector.
  struct CommonMode { uint16_t local, partner, status; bool pause; };
  // Matching PHYCR does not qualify pause unless the base pages' highest common
  // technology is the observed full-duplex mode. Literal priority cases.
  constexpr CommonMode modes[] = {
    {0x0141, 0x0141, 0x0015, true}, {0x0141, 0x0141, 0x0017, false},
    {0x0041, 0x0041, 0x0017, true}, {0x0041, 0x0041, 0x0015, false},
    {0x00C1, 0x00C1, 0x0017, false}, {0x0101, 0x0041, 0x0015, false},
    {0x0081, 0x0081, 0x0015, false}
  };
  for (const auto& mode : modes) {
    up(bus, mode.status);
    bus.words[4] = static_cast<uint16_t>(mode.local | 0x0400U);
    bus.words[5] = static_cast<uint16_t>(mode.partner | 0x0400U);
    bus.words[25] = 0x3000;
    CHECK(refresh(driver, bus, result).ok());
    CHECK(result.resolvedModeValid && result.pauseValid == mode.pause);
  }
}

void forced() {
  for (auto speed : {LinkSpeed::MBPS_10, LinkSpeed::MBPS_100}) for (auto duplex : {Duplex::HALF, Duplex::FULL}) {
    Controller bus; Driver driver; initialize(driver, bus);
    LinkSettings profile;
    profile.mode = LinkMode::FORCED; profile.forced = {speed, duplex};
    configure(driver, bus, profile);
    up(bus, 0x0007); // Contradictory PHYSTS is raw only while AN disabled.
    LinkSnapshot result;
    CHECK(refresh(driver, bus, result).ok());
    CHECK(result.linkUp && result.resolvedModeValid && result.speed == speed && result.duplex == duplex);
    CHECK(result.negotiation == NegotiationState::DISABLED && !result.pauseValid);
    CHECK(result.rawPhystsFirst == 7);
  }
  for (bool extension : {false, true}) {
    Controller bus; Driver driver; initialize(driver, bus);
    if (extension) {
      LinkSettings profile;
      profile.mode = LinkMode::FORCED; profile.forced = {LinkSpeed::MBPS_100, Duplex::HALF};
      profile.extendedFullDuplex = true;
      configure(driver, bus, profile);
    } else bus.words[0] = 0x2000; // Trust not verified: CR2 extension is unknown.
    up(bus, 0x0007);
    LinkSnapshot result;
    CHECK(refresh(driver, bus, result).ok());
    CHECK(!result.resolvedModeValid && result.duplex == Duplex::UNKNOWN && !result.pauseValid);
  }
  Controller bus; Driver driver; initialize(driver, bus);
  LinkSettings profile; profile.extendedFullDuplex = true;
  profile.advertisement.technologyMask = LINK_100_FULL;
  configure(driver, bus, profile);
  up(bus); bus.words[6] = 0; // Qualified extended full duplex without AN partner.
  LinkSnapshot result;
  CHECK(refresh(driver, bus, result).ok());
  CHECK(result.resolvedModeValid && result.speed == LinkSpeed::MBPS_100 && result.duplex == Duplex::FULL && !result.pauseValid);
  up(bus, 0x0017); bus.words[6] = 0;
  CHECK(refresh(driver, bus, result).ok());
  CHECK(!result.resolvedModeValid && result.duplex == Duplex::UNKNOWN && !result.pauseValid);
  profile.extendedFullDuplex = false;
  configure(driver, bus, profile);
  up(bus); bus.words[6] = 0;
  CHECK(refresh(driver, bus, result).ok());
  CHECK(!result.resolvedModeValid && result.duplex == Duplex::UNKNOWN && !result.pauseValid);
  bus.words[6] = 0x0011; // Parallel-detection fault does not resolve a mode.
  CHECK(refresh(driver, bus, result).ok() && !result.resolvedModeValid && !result.pauseValid);
}

void failures() {
  constexpr uint8_t sourceAt[] = {1,1,16,0,4,5,6,25,1,16};
  for (unsigned failure = 0; failure < 10; ++failure) {
    Controller bus; Driver driver; initialize(driver, bus); up(bus);
    LinkSnapshot result;
    CHECK(refresh(driver, bus, result).ok());
    const auto old = result;
    const auto before = driver.evidenceSnapshot();
    const auto first = bus.frames;
    bus.failFrame = static_cast<int>(first + failure);
    CHECK(refresh(driver, bus, result).is(Err::MDIO_TIMEOUT));
    CHECK(bus.frames == first + failure + 1);
    CHECK(result.observedAtUs == old.observedAtUs && result.observations == old.observations && !result.stale);
    const auto cached = driver.linkSnapshot();
    CHECK(cached.valid && cached.stale && cached.observedAtUs == old.observedAtUs && cached.linkUp == old.linkUp);
    CHECK(cached.observedSinceUs == old.observedSinceUs);
    CHECK(cached.lastRefreshStatus.is(Err::MDIO_TIMEOUT) && cached.lastRefreshStatus.detail == -19019);
    const auto retained = driver.evidenceSnapshot();
    for (auto source : {EvidenceSource::BMSR, EvidenceSource::PHYSTS, EvidenceSource::ANER}) {
      const uint8_t reg = source == EvidenceSource::BMSR ? 1 : source == EvidenceSource::PHYSTS ? 16 : 6;
      unsigned consumed = 0;
      for (unsigned i = 0; i < failure; ++i) if (sourceAt[i] == reg) ++consumed;
      CHECK(evidence(retained, source).samples == evidence(before, source).samples + consumed);
      if (sourceAt[failure] == reg) CHECK(evidence(retained, source).historyLost);
    }
  }
  // Failed release after complete words preserves consumed evidence but never
  // publishes a successful aggregate or loses the controller failure detail.
  for (unsigned phase = 0; phase < 2; ++phase) {
    Controller bus; Driver driver; initialize(driver, bus); up(bus);
    bus.releaseFailCall = static_cast<int>(bus.calls + phase);
    LinkSnapshot result; result.rawBmcr = 0xBEEF;
    CHECK(!refresh(driver, bus, result).ok());
    CHECK(result.rawBmcr == 0xBEEF && !driver.linkSnapshot().valid);
    CHECK(driver.transportSnapshot().lastBatch.releaseStatus.detail == -19020);
  }
}

void budgets() {
  for (uint32_t allowance = 0; allowance < 10; ++allowance) {
    Controller bus; Driver driver; initialize(driver, bus); up(bus);
    const auto calls = bus.calls;
    LinkSnapshot result; result.rawBmcr = 0xBEEF;
    CHECK(!refresh(driver, bus, result, {1000000, allowance}).ok());
    CHECK(bus.calls == calls && result.rawBmcr == 0xBEEF);
  }
  Controller bus; Driver driver; initialize(driver, bus); up(bus);
  const auto calls = bus.calls;
  LinkSnapshot result;
  // Bounds: two callbacks each acquire/release=8, ten frames=4, clock lag=1.
  CHECK(!refresh(driver, bus, result, {bus.now + 50, 10}).ok());
  CHECK(bus.calls == calls); // Entire ten-read plan is reserved before any I/O.
  CHECK(refresh(driver, bus, result, {bus.now + 58, 10}).ok()); // Strict bound+lag+1 admission.
  CHECK(refresh(driver, bus, result, {1000000, 10}).ok());
  // Every failed admitted refresh withdraws freshness, even when its entire
  // reservation is denied before the first destructive read.
  for (unsigned denial = 0; denial < 3; ++denial) {
    CHECK(refresh(driver, bus, result).ok());
    const auto previous = result;
    const auto before = bus.calls;
    IoBudget insufficient{1000000, 9};
    if (denial == 1) insufficient = {bus.now, 10};
    if (denial == 2) insufficient = {bus.now + 57, 10};
    CHECK(refresh(driver, bus, result, insufficient).is(Err::OPERATION_TIMEOUT));
    const auto cached = driver.linkSnapshot();
    CHECK(bus.calls == before && !result.stale && result.observedAtUs == previous.observedAtUs);
    CHECK(cached.valid && cached.stale && cached.change == LinkChange::STALE);
    CHECK(cached.observedSinceUs == previous.observedSinceUs && cached.observedAtUs == previous.observedAtUs);
    CHECK(cached.observations == previous.observations && cached.lastRefreshStatus.is(Err::OPERATION_TIMEOUT));
  }
  for (unsigned phase = 0; phase < 2; ++phase) {
    Controller late; Driver timed; initialize(timed, late); up(late);
    late.lateCall = static_cast<int>(late.calls + phase);
    CHECK(refresh(timed, late, result).is(Err::OPERATION_TIMEOUT));
    CHECK(!timed.linkSnapshot().valid);
    CHECK(late.sampleIndex == (phase == 0 ? 8U : 10U));
  }
}

void clocks() {
  // Zero is a valid frame timestamp; optional frame stamps may be absent. The
  // aggregate includes the positive clock lag. The owner computes conservative
  // age without a hidden core clock read or unsigned underflow.
  for (bool stamps : {false, true}) {
    Controller bus; Driver driver;
    bus.now = 0; bus.frameTime = 0; bus.stamps = stamps;
    auto config = bus.config();
    CHECK(driver.bind(config).ok());
    OperationToken token;
    CHECK(driver.startInitialize({ResetKind::NONE}, operationBudget(), token).inProgress());
    CHECK(drain(driver, bus, token).ok());
    OperationResult operation; CHECK(driver.takeResult(token, operation).ok());
    up(bus); LinkSnapshot result;
    CHECK(refresh(driver, bus, result).ok());
    CHECK(result.timeValid && result.observedAtUs == 1);
    if (stamps) CHECK(driver.transportSnapshot().lastBatch.frames[1].completedAtUs == 0);
    CHECK(result.observedSinceUs == 0);
    CHECK(!cached100FullUsable(result, result.observedAtUs, 0)); // Lag consumes age allowance.
    CHECK(cached100FullUsable(result, result.observedSinceUs + 100, 100));
    CHECK(!cached100FullUsable(result, result.observedSinceUs + 101, 100));
    result.observedAtUs = 0;
    CHECK(cached100FullUsable(result, 0, 0));
    result.observedAtUs = 2;
    CHECK(!cached100FullUsable(result, 1, 100));
    result.timeValid = false;
    CHECK(!cached100FullUsable(result, 2, 100));
  }
  for (unsigned phase = 0; phase < 2; ++phase) {
    Controller bus; Driver driver; initialize(driver, bus); up(bus);
    LinkSnapshot result; CHECK(refresh(driver, bus, result).ok());
    const auto old = result;
    bus.regressCall = static_cast<int>(bus.calls + phase);
    CHECK(refresh(driver, bus, result).is(Err::INVALID_CONFIG));
    CHECK(driver.transportSnapshot().clockInvalid && driver.linkSnapshot().stale);
    CHECK(driver.linkSnapshot().observedAtUs == old.observedAtUs);
    const auto calls = bus.calls;
    CHECK(!refresh(driver, bus, result).ok() && bus.calls == calls);
  }
  Controller bus; Driver driver; initialize(driver, bus); up(bus);
  bus.now = std::numeric_limits<uint64_t>::max();
  const auto calls = bus.calls;
  LinkSnapshot result;
  CHECK(!refresh(driver, bus, result, {std::numeric_limits<uint64_t>::max(), 10}).ok());
  CHECK(bus.calls == calls && driver.transportSnapshot().clockInvalid);
  // A clock fault discovered by another timed API invalidates the cached link
  // immediately, without requiring the owner to attempt a new link refresh.
  Controller unrelated; Driver other; initialize(other, unrelated); up(unrelated);
  CHECK(refresh(other, unrelated, result).ok());
  const auto previous = result;
  const auto rawCalls = unrelated.calls;
  unrelated.now = 0;
  uint16_t sentinel = 0xBEEF;
  CHECK(other.readClause22(2, sentinel, {1000000, 1}).is(Err::INVALID_CONFIG));
  const auto stale = other.linkSnapshot();
  CHECK(unrelated.calls == rawCalls && sentinel == 0xBEEF);
  CHECK(stale.valid && stale.stale && stale.change == LinkChange::STALE);
  CHECK(stale.observedSinceUs == previous.observedSinceUs && stale.observedAtUs == previous.observedAtUs);
  CHECK(stale.lastRefreshStatus.is(Err::INVALID_CONFIG));
  CHECK(stale.lastRefreshStatus.detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
}

void ownership() {
  Controller bus; Driver driver;
  LinkSnapshot result; result.rawBmcr = 0xBEEF;
  CHECK(driver.readLinkStatus(result, {1000000, 10}).is(Err::NOT_BOUND));
  CHECK(driver.bind(bus.config()).ok());
  CHECK(driver.readLinkStatus(result, {1000000, 10}).is(Err::NOT_INITIALIZED));
  CHECK(bus.calls == 0 && result.rawBmcr == 0xBEEF);
  CHECK(driver.end().ok()); initialize(driver, bus); up(bus);
  Driver other; CHECK(other.bind(bus.config(2)).ok()); bus.competitor = &other;
  CHECK(refresh(driver, bus, result).ok() && bus.exclusions == 10);
  bus.competitor = nullptr;
  OperationToken token;
  LinkSettings profile;
  profile.advertisement.technologyMask = LINK_100_FULL;
  CHECK(driver.startConfigure(profile, operationBudget(), token).inProgress());
  CHECK(refresh(driver, bus, result).ok() && result.resolvedModeValid); // Before a write.
  const auto writes = bus.writes;
  for (unsigned i = 0; i < 20 && bus.writes == writes; ++i)
    CHECK(driver.pollOperation(token).inProgress());
  CHECK(bus.writes > writes);
  CHECK(refresh(driver, bus, result).ok() && result.linkUp && !result.resolvedModeValid && !result.pauseValid);
  CHECK(driver.cancel(token).ok());
  CHECK(refresh(driver, bus, result).ok()); // Result pending admits reads.
  OperationResult operation; CHECK(driver.takeResult(token, operation).ok());
  // Active next-page owner is the only ANER consumer.
  Controller pages; Driver pageDriver; initialize(pageDriver, pages); up(pages);
  ConfigureOptions options;
  options.nextPages.pageCount = 1; options.nextPages.pages[0] = {1, true, false};
  CHECK(pageDriver.startConfigure(profile, options, operationBudget(), token).inProgress());
  const auto calls = pages.calls;
  CHECK(refresh(pageDriver, pages, result).is(Err::BUSY) && pages.calls == calls);
  // Reset WAIT denies reads while the controller lock is released. A hard-reset
  // quiet floor persists after cancellation and result delivery.
  for (auto reset : {ResetKind::PCS, ResetKind::HARD, ResetKind::RESTART}) {
    Controller resetBus; Driver resetDriver; initialize(resetDriver, resetBus); up(resetBus);
    CHECK(refresh(resetDriver, resetBus, result).ok());
    CHECK(resetDriver.startInitialize({reset}, operationBudget(), token).inProgress());
    for (unsigned i = 0; i < 20 && resetDriver.operationSnapshot().phase != OperationPhase::WAIT; ++i)
      CHECK(resetDriver.pollOperation(token).inProgress());
    CHECK(resetDriver.operationSnapshot().phase == OperationPhase::WAIT && !resetBus.locked);
    CHECK(resetDriver.linkSnapshot().stale && resetDriver.linkSnapshot().observedAtUs == result.observedAtUs);
    const auto resetCalls = resetBus.calls;
    CHECK(refresh(resetDriver, resetBus, result).is(Err::BUSY));
    CHECK(resetBus.calls == resetCalls);
    CHECK(resetDriver.cancel(token).ok());
    CHECK(resetDriver.takeResult(token, operation).ok());
    if (reset == ResetKind::HARD) {
      CHECK(resetDriver.operationSnapshot().busQuietValid);
      CHECK(refresh(resetDriver, resetBus, result).is(Err::BUSY));
      CHECK(resetBus.calls == resetCalls);
    }
  }
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"read_plan", readPlan}, {"transitions", transitions}, {"coherence", coherence},
    {"pause", pause}, {"forced", forced}, {"failures", failures},
    {"budgets", budgets}, {"clocks", clocks}, {"ownership", ownership}
  };
  return test::runSuites(argc, argv, suites);
}
