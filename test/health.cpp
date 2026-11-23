#include "support/BoundDriver.h"
#include <DP8382x/DP8382x.h>
#include "../src/DP8382xHealth.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;

// Literal SNLS647I C22/DEVAD 31 model. Failure placement is a physical frame
// ordinal, independent of production health classification and register masks.
struct Controller {
  std::array<uint16_t, 32> words{};
  uint16_t selector{0}, address{0}, vod{0x8400};
  uint64_t now{100}, frameTime{2};
  unsigned calls{0}, frames{0}, clocks{0}, writes{0};
  int faultCall{-1}, failPhase{-1};
  Status injected{Err::MDIO_TIMEOUT, -2147483647, "Injected failure"};
  Status release{Status::Ok()};
  bool noFrames{false}, reusable{true}, late{false}, regress{false}, overflow{false};
  bool shortSuccess{false}, nonterminal{false}, badStamp{false}, wrongVod{false}, terminalOverride{false};
  Controller() { words[0] = 0x3100; words[2] = 0x2000; words[3] = 0xA131; words[4] = 0x0101; }
  Config config(uint32_t phy = 1, uint32_t threshold = 3) noexcept {
    Config result;
    result.variant = Variant::DP83826I; result.interface = MacInterface::RMII;
    result.phyAddress = phy; result.offlineThreshold = threshold;
    result.transport = {this, execute, clock, {8, 4, 1}};
    return result;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<Controller*>(context); ++bus.clocks; return bus.now;
  }
  uint16_t read(uint8_t reg) noexcept {
    if (reg == 0) words[0] = static_cast<uint16_t>(words[0] & 0x7DFFU);
    if (reg == 31) words[31] = 0;
    if (reg == 14 && selector == 0x401F) {
      if (address == 0x030E) return wrongVod ? 0x8400 : vod;
      if (address == 0x0467) return 0x1234;
      if (address == 0x0468) return 0x5678;
      if (address < 32U) return words[address];
    }
    return words[reg];
  }
  void write(uint8_t reg, uint16_t value) noexcept {
    ++writes;
    if (reg == 13) selector = value;
    else if (reg == 14 && selector == 0x001F) address = value;
    else if (reg == 14 && selector == 0x401F) {
      if (address == 0x030E) vod = value;
      else if (address < 32U) words[address] = value;
    } else words[reg] = value;
  }
  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget& budget) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    const bool fault = static_cast<int>(bus.calls++) == bus.faultCall;
    BatchResult result;
    result.status = Status::Ok(); result.releaseStatus = Status::Ok(); result.controllerReusable = true;
    CHECK(batch.frameCount <= budget.maxFrames);
    if (fault && bus.noFrames) result.status = bus.injected;
    else for (uint8_t i = 0; i < batch.frameCount; ++i) {
      if (fault && bus.shortSuccess && i == batch.frameCount - 1U) break;
      const auto& frame = batch.frames[i];
      auto& receipt = result.frames[i];
      receipt.attempted = true; ++bus.frames;
      uint16_t value = frame.value;
      if (frame.readIndex != 0xFF) {
        CHECK(frame.readIndex < i && result.frames[frame.readIndex].valueValid);
        value = static_cast<uint16_t>((result.frames[frame.readIndex].value & frame.preserveMask) | value);
      }
      // Accepted writes can precede a reported terminal error.
      if (frame.kind == FrameKind::WRITE) bus.write(frame.registerAddress, value);
      bus.now += bus.frameTime;
      if (fault && static_cast<int>(i) == bus.failPhase) {
        receipt.effectPossible = true; result.status = bus.injected; break;
      }
      receipt.completed = true;
      if (frame.kind == FrameKind::READ) { receipt.valueValid = true; receipt.value = bus.read(frame.registerAddress); }
    }
    if (fault) {
      result.releaseStatus = bus.release; result.controllerReusable = bus.reusable;
      if (bus.terminalOverride) result.status = bus.injected;
      if (bus.nonterminal) result.status = {Err::IN_PROGRESS, -999, "Invalid terminal status"};
      if (bus.badStamp) { result.frames[0].timeValid = true; result.frames[0].completedAtUs = bus.now + 10; }
      if (bus.late) bus.now = budget.deadlineUs;
      if (bus.regress) bus.now = 0;
      if (bus.overflow) bus.now = std::numeric_limits<uint64_t>::max();
    }
    return result;
  }
  void failNext(int phase = 0) noexcept { faultCall = static_cast<int>(calls); failPhase = phase; }
};
OperationBudget allowance() { return {{1000000, 512}, 10, 8}; }
IoBudget io() { return {1000000, 32}; }
OperationToken start(Driver& driver, ResetKind kind = ResetKind::NONE) {
  OperationToken token; CHECK(driver.startInitialize({kind}, allowance(), token).inProgress()); return token;
}
Status drain(Driver& driver, Controller& bus, OperationToken token) {
  Status status{Err::IN_PROGRESS, 0, "Pending"};
  for (unsigned n = 0; n < 128 && status.inProgress(); ++n) {
    const auto state = driver.operationSnapshot();
    if (state.nextPollValid && bus.now < state.nextPollAtUs) bus.now = state.nextPollAtUs;
    status = driver.pollOperation(token);
  }
  CHECK(!status.inProgress()); return status;
}
OperationResult take(Driver& driver, OperationToken token) {
  OperationResult result; CHECK(driver.takeResult(token, result).ok()); return result;
}
void initialize(Driver& driver, Controller& bus, uint32_t threshold = 3) {
  CHECK(driver.bind(bus.config(1, threshold)).ok());
  const auto token = start(driver); CHECK(drain(driver, bus, token).ok());
  CHECK(take(driver, token).status.ok());
  CHECK(driver.isInitialized() && driver.driverState() == DriverState::READY);
}
Status link(Driver& driver) { LinkSnapshot value; return driver.readLinkStatus(value, io()); }
void sameHealth(const HealthSnapshot& left, const HealthSnapshot& right) {
  CHECK(left.state == right.state && left.initialized == right.initialized);
  CHECK(left.consecutiveFailures == right.consecutiveFailures);
  CHECK(left.totalFailures == right.totalFailures && left.totalSuccess == right.totalSuccess);
  CHECK(left.lastOkUs == right.lastOkUs && left.lastErrorUs == right.lastErrorUs);
  CHECK(left.lastOkTimeValid == right.lastOkTimeValid && left.lastErrorTimeValid == right.lastErrorTimeValid);
  CHECK(left.lastError.code == right.lastError.code && left.lastError.detail == right.lastError.detail);
}

void lifecycle() {
  Controller bus; Driver driver;
  CHECK(driver.driverState() == DriverState::UNINIT);
  CHECK(!driver.healthSnapshot().initialized);
  CHECK(std::strcmp(driverStateName(DriverState::UNINIT), "UNINIT") == 0);
  CHECK(std::strcmp(driverStateName(DriverState::READY), "READY") == 0);
  CHECK(std::strcmp(driverStateName(DriverState::DEGRADED), "DEGRADED") == 0);
  CHECK(std::strcmp(driverStateName(DriverState::OFFLINE), "OFFLINE") == 0);
  CHECK(std::strcmp(driverStateName(static_cast<DriverState>(255)), "UNKNOWN") == 0);
  CHECK(driver.bind(bus.config()).ok());
  const auto token = start(driver);
  CHECK(bus.calls == 0 && bus.clocks == 0 && driver.driverState() == DriverState::UNINIT);
  CHECK(driver.pollOperation(token, 0).inProgress());
  CHECK(bus.calls == 0 && driver.healthSnapshot().totalSuccess == 0);
  CHECK(driver.pollOperation(token).inProgress());
  CHECK(driver.healthSnapshot().totalSuccess == 1 && driver.driverState() == DriverState::UNINIT);
  CHECK(drain(driver, bus, token).ok());
  CHECK(driver.healthSnapshot().totalSuccess == 7 && driver.healthSnapshot().totalFailures == 0);
  CHECK(bus.calls == 7 && bus.frames == 20);
  CHECK(driver.transportSnapshot().totalBatches == 7 && driver.transportSnapshot().attemptedFrames == 20);
  CHECK(driver.driverState() == DriverState::READY);
  const auto pending = driver.operationSnapshot();
  const auto calls = bus.calls, clocks = bus.clocks;
  CHECK(driver.end().is(Err::BUSY) && driver.bind(bus.config()).is(Err::BUSY));
  for (unsigned n = 0; n < 3; ++n) {
    CHECK(driver.driverState() == DriverState::READY); CHECK(driver.healthSnapshot().initialized);
  }
  driver.resetStatistics();
  CHECK(bus.calls == calls && bus.clocks == clocks);
  CHECK(driver.healthSnapshot().totalSuccess == 0 && driver.driverState() == DriverState::READY);
  CHECK(driver.operationSnapshot().token.value == pending.token.value);
  CHECK(take(driver, token).status.ok());
  CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
  CHECK(driver.driverState() == DriverState::UNINIT && !driver.healthSnapshot().initialized);
  CHECK(bus.calls == calls && bus.clocks == clocks);
  CHECK(driver.end().ok());
  CHECK(driver.healthSnapshot().totalSuccess == 0 && !driver.healthSnapshot().lastOkTimeValid);
  CHECK(driver.bind(bus.config()).ok());
  CHECK(driver.healthSnapshot().lastError.ok());
  CHECK(bus.calls == calls && bus.clocks == clocks);
}

void transitions() {
  for (uint32_t threshold : {1U, 2U, 3U, 5U}) {
    Controller bus; Driver driver; initialize(driver, bus, threshold);
    driver.resetStatistics();
    for (uint32_t failure = 1; failure <= threshold + 1U; ++failure) {
      bus.failNext(); CHECK(link(driver).is(Err::MDIO_TIMEOUT));
      const auto health = driver.healthSnapshot();
      CHECK(health.totalFailures == failure && health.consecutiveFailures == failure);
      CHECK(health.totalSuccess == 0 && health.initialized);
      CHECK(health.state == (failure >= threshold ? DriverState::OFFLINE : DriverState::DEGRADED));
      CHECK(health.lastError.is(Err::MDIO_TIMEOUT) && health.lastError.detail == -2147483647);
      CHECK(health.lastErrorTimeValid && health.lastErrorUs == bus.now);
    }
    const auto failure = driver.healthSnapshot();
    const auto calls = bus.calls;
    CHECK(link(driver).ok()); // OFFLINE cannot close admission; cable remains down.
    CHECK(bus.calls == calls + 2 && driver.driverState() == DriverState::READY);
    const auto success = driver.healthSnapshot();
    CHECK(success.totalSuccess == 2 && success.consecutiveFailures == 0);
    CHECK(success.lastError.detail == failure.lastError.detail && success.lastErrorUs == failure.lastErrorUs);
    CHECK(!driver.linkSnapshot().linkUp && driver.linkSnapshot().valid);
    CHECK(success.lastOkTimeValid);
  }
  Controller maximum; Driver maxDriver; initialize(maxDriver, maximum, 0xFFFFFFFFU);
  maximum.failNext(); CHECK(link(maxDriver).is(Err::MDIO_TIMEOUT));
  CHECK(maxDriver.driverState() == DriverState::DEGRADED);
  Controller policy; Driver policyDriver; initialize(policyDriver, policy);
  policy.words[0] = 0; policy.words[1] = 4; policy.words[16] = 7;
  CHECK(link(policyDriver).ok());
  CHECK(policyDriver.linkSnapshot().resolvedModeValid);
  CHECK(policyDriver.linkSnapshot().speed == LinkSpeed::MBPS_10 && policyDriver.linkSnapshot().duplex == Duplex::HALF);
  CHECK(policyDriver.driverState() == DriverState::READY && policyDriver.healthSnapshot().totalFailures == 0);
  policy.words[0] = 0x1000; policy.words[16] = 1;
  CHECK(link(policyDriver).ok());
  CHECK(policyDriver.linkSnapshot().negotiation == NegotiationState::IN_PROGRESS);
  CHECK(!policyDriver.linkSnapshot().resolvedModeValid);
  CHECK(policyDriver.healthSnapshot().totalFailures == 0);
  Controller bus; Driver driver;
  CHECK(driver.bind(bus.config(1, 0)).is(Err::INVALID_CONFIG));
  CHECK(bus.calls == 0 && bus.clocks == 0);
}

void accounting() {
  // Every physical phase of a four-frame managed MMD read can fail. Successful
  // selector writes do not turn the failed transaction into health successes.
  for (int phase = 0; phase < 4; ++phase) {
    auto [bus, driver] = test::BoundDriver<Controller>{};
    const auto token = start(driver);
    CHECK(driver.pollOperation(token).inProgress()); // Two identity reads.
    bus.failNext(phase);
    CHECK(driver.pollOperation(token).is(Err::MDIO_TIMEOUT));
    const auto health = driver.healthSnapshot();
    CHECK(health.totalSuccess == 1 && health.totalFailures == 1 && health.consecutiveFailures == 1);
    CHECK(health.state == DriverState::UNINIT);
    CHECK(driver.transportSnapshot().totalBatches == 2);
    CHECK(driver.transportSnapshot().attemptedFrames == 3U + static_cast<unsigned>(phase));
    CHECK(driver.transportSnapshot().completedFrames == 2U + static_cast<unsigned>(phase));
    CHECK(take(driver, token).status.detail == -2147483647);
  }
  // Both link callbacks count independently; all ten failure positions retain
  // exactly the successful earlier callback, never its successful frame prefix.
  for (int phase = 0; phase < 10; ++phase) {
    Controller bus; Driver driver; initialize(driver, bus); driver.resetStatistics();
    bus.faultCall = static_cast<int>(bus.calls) + (phase >= 8 ? 1 : 0);
    bus.failPhase = phase >= 8 ? phase - 8 : phase;
    CHECK(link(driver).is(Err::MDIO_TIMEOUT));
    CHECK(driver.healthSnapshot().totalSuccess == (phase >= 8 ? 1U : 0U));
    CHECK(driver.healthSnapshot().totalFailures == 1 && driver.healthSnapshot().consecutiveFailures == 1);
    CHECK(driver.transportSnapshot().attemptedFrames == 21U + static_cast<unsigned>(phase));
    CHECK(driver.transportSnapshot().completedFrames == 20U + static_cast<unsigned>(phase));
  }
  auto [wrongPart, rejected] = test::BoundDriver<Controller>{}; wrongPart.words[3] = 0xA141;
  const auto wrongToken = start(rejected);
  CHECK(drain(rejected, wrongPart, wrongToken).is(Err::DEVICE_MISMATCH));
  CHECK(rejected.healthSnapshot().totalSuccess == 1 && rejected.healthSnapshot().totalFailures == 0);
  CHECK(rejected.driverState() == DriverState::UNINIT);
  CHECK(take(rejected, wrongToken).status.is(Err::DEVICE_MISMATCH));

  auto [semantic, mismatch] = test::BoundDriver<Controller>{}; semantic.wrongVod = true;
  const auto token = start(mismatch);
  CHECK(drain(mismatch, semantic, token).is(Err::CONFIG_MISMATCH));
  CHECK(mismatch.healthSnapshot().totalSuccess == 7 && mismatch.healthSnapshot().totalFailures == 0);
  CHECK(mismatch.driverState() == DriverState::UNINIT && take(mismatch, token).status.is(Err::CONFIG_MISMATCH));
}

void configuration() {
  Controller bus; Driver driver; initialize(driver, bus); driver.resetStatistics();
  LinkSettings settings; settings.advertisement.technologyMask = LINK_100_FULL;
  OperationToken token;
  CHECK(driver.startConfigure(settings, allowance(), token).inProgress());
  CHECK(driver.healthSnapshot().totalSuccess == 0);
  CHECK(drain(driver, bus, token).ok());
  CHECK(take(driver, token).status.ok());
  // Six preflight reads, six two-frame updates, six verification reads.
  CHECK(driver.healthSnapshot().totalSuccess == 18 && driver.healthSnapshot().totalFailures == 0);
  CHECK(driver.transportSnapshot().attemptedFrames == 44);
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::VERIFIED);
  settings.advertisement.technologyMask = LINK_10_HALF;
  CHECK(driver.startConfigure(settings, allowance(), token).inProgress());
  for (unsigned n = 0; n < 6; ++n) CHECK(driver.pollOperation(token).inProgress());
  bus.failNext(1); // CR1 target write accepted, then failure.
  CHECK(driver.pollOperation(token).is(Err::MDIO_TIMEOUT));
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
  const auto failed = take(driver, token);
  CHECK(failed.mutationPossible && failed.cleanupOwed);
  CHECK(driver.healthSnapshot().consecutiveFailures == 1);
  CHECK(link(driver).ok()); // Read transport can recover health, never profile trust.
  CHECK(driver.driverState() == DriverState::READY);
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
  CHECK(driver.operationSnapshot().cleanupOwed);
  CHECK(driver.healthSnapshot().lastError.detail == -2147483647);
  const auto before = driver.healthSnapshot();
  uint16_t word = 0;
  CHECK(driver.readClause22(4, word, io()).ok());
  sameHealth(driver.healthSnapshot(), before);
  CHECK(driver.settingsSnapshot().trust == ConfigurationTrust::DIRTY);
  driver.resetStatistics();
  CHECK(driver.operationSnapshot().cleanupOwed && driver.settingsSnapshot().desiredValid);
  CHECK(driver.settingsSnapshot().desired.advertisement.technologyMask == LINK_10_HALF);
  CHECK(failed.status.detail == -2147483647);
}

void rawAndLocal() {
  Controller bus; Driver driver; initialize(driver, bus);
  bus.failNext(); CHECK(link(driver).is(Err::MDIO_TIMEOUT));
  const auto health = driver.healthSnapshot();
  uint16_t word = 0;
  CHECK(driver.readClause22(2, word, io()).ok());
  CHECK(driver.readMmd(31, 0x0467, word, io()).ok());
  DeviceIdentity identity; CHECK(driver.readIdentity(identity, io()).ok());
  EvidenceSnapshot evidence;
  CHECK(driver.acquireEvidence(evidenceBit(EvidenceSource::MISR1), evidence, io()).ok());
  CounterSnapshot counters; CHECK(driver.collectEvents(counters, io()).ok());
  sameHealth(driver.healthSnapshot(), health);
  const auto calls = bus.calls;
  CHECK(driver.readClause22(0x8000, word, io()).is(Err::INVALID_PARAM));
  LinkSnapshot sentinel;
  CHECK(driver.readLinkStatus(sentinel, {bus.now, 10}).is(Err::OPERATION_TIMEOUT));
  CHECK(driver.readLinkStatus(sentinel, {1000000, 0}).is(Err::OPERATION_TIMEOUT));
  CHECK(driver.pollOperation({0}).is(Err::TOKEN_MISMATCH));
  CHECK(bus.calls == calls); sameHealth(driver.healthSnapshot(), health);
  bus.failNext(); CHECK(driver.readMmd(31, 0x0467, word, io()).is(Err::MDIO_TIMEOUT));
  sameHealth(driver.healthSnapshot(), health);
  const auto transport = driver.transportSnapshot();
  const auto history = driver.evidenceSnapshot();
  const auto lastOk = health.lastOkUs, lastError = health.lastErrorUs;
  const auto clocks = bus.clocks;
  driver.resetStatistics();
  CHECK(driver.healthSnapshot().totalSuccess == 0 && driver.healthSnapshot().totalFailures == 0);
  CHECK(driver.healthSnapshot().consecutiveFailures == 1 && driver.driverState() == DriverState::DEGRADED);
  CHECK(driver.healthSnapshot().lastOkUs == lastOk && driver.healthSnapshot().lastErrorUs == lastError);
  CHECK(driver.transportSnapshot().totalBatches == transport.totalBatches);
  CHECK(driver.evidenceSnapshot().lastRequested == history.lastRequested);
  CHECK(bus.clocks == clocks);
  CHECK(driver.modifyInterruptEnables(0x12, 1, 1, io()).ok());
  const auto afterMaintenance = driver.healthSnapshot();
  CHECK(afterMaintenance.totalSuccess == 0 && afterMaintenance.totalFailures == 0 && afterMaintenance.consecutiveFailures == 1);
  auto [interleaved, operation] = test::BoundDriver<Controller>{};
  const auto token = start(operation);
  const auto beforeRaw = operation.healthSnapshot();
  CHECK(operation.readClause22(2, word, io()).ok());
  interleaved.failNext();
  CHECK(operation.readMmd(31, 0x0467, word, io()).is(Err::MDIO_TIMEOUT));
  sameHealth(operation.healthSnapshot(), beforeRaw);
  CHECK(operation.pollOperation(token).inProgress());
  CHECK(operation.healthSnapshot().totalSuccess == 1 && operation.healthSnapshot().totalFailures == 0);
  CHECK(operation.cancel(token).ok()); CHECK(take(operation, token).status.is(Err::CANCELLED));
}

void reports() {
  struct Case { Err primary; int32_t detail; Err release; bool reusable; bool count; int32_t error; };
  const Case cases[] = {
    {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), Err::OK, true, false, 0},
    {Err::BUSY, static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED), Err::OK, false, false, 0},
    {Err::OPERATION_TIMEOUT, static_cast<int32_t>(TimeoutDetail::DEADLINE), Err::OK, true, false, 0},
    {Err::MDIO_TIMEOUT, -12345, Err::OK, true, true, -12345},
    {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), Err::MDIO_ERROR, false, true, -37},
    {Err::MDIO_TIMEOUT, -12345, Err::MDIO_ERROR, false, true, -12345},
    {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), Err::OK, false, true, 0},
    {Err::BUSY, static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED), Err::OK, true, true, 0}
  };
  for (const auto& entry : cases) {
    Controller bus; Driver driver; initialize(driver, bus); driver.resetStatistics();
    bus.failNext(); bus.noFrames = true;
    bus.injected = {entry.primary, entry.detail, "Injected admission"};
    bus.release = {entry.release, entry.release == Err::OK ? 0 : -37, "Injected release"};
    bus.reusable = entry.reusable;
    const auto status = link(driver);
    CHECK(status.code == entry.primary);
    const auto health = driver.healthSnapshot();
    CHECK(health.totalSuccess == 0 && health.totalFailures == (entry.count ? 1U : 0U));
    CHECK(driver.transportSnapshot().totalBatches == 8 && driver.transportSnapshot().attemptedFrames == 20);
    if (entry.count) { CHECK(health.lastError.detail == entry.error); CHECK(health.lastError.is(entry.primary == Err::MDIO_TIMEOUT ? Err::MDIO_TIMEOUT : Err::MDIO_ERROR)); }
  }
  for (bool allCompleted : {false, true}) {
    Controller bus; Driver driver; initialize(driver, bus); driver.resetStatistics();
    bus.failNext(allCompleted ? -1 : 3); bus.terminalOverride = allCompleted;
    bus.injected = {Err::OPERATION_TIMEOUT, static_cast<int32_t>(TimeoutDetail::DEADLINE), "Transfer budget"};
    CHECK(link(driver).is(Err::OPERATION_TIMEOUT));
    CHECK(driver.healthSnapshot().totalFailures == 1 && driver.healthSnapshot().totalSuccess == 0);
    CHECK(driver.healthSnapshot().lastError.is(Err::MDIO_ERROR));
    CHECK(driver.healthSnapshot().lastError.detail == static_cast<int32_t>(TimeoutDetail::DEADLINE));
    CHECK(driver.transportSnapshot().attemptedFrames == (allCompleted ? 28U : 24U));
    CHECK(driver.transportSnapshot().completedFrames == (allCompleted ? 28U : 23U));
  }

  for (bool invalidRelease : {false, true}) {
    Controller bus; Driver driver; initialize(driver, bus); driver.resetStatistics();
    bus.faultCall = static_cast<int>(bus.calls); bus.reusable = false;
    if (invalidRelease) bus.release = {Err::IN_PROGRESS, -37, "Invalid release"};
    CHECK(link(driver).is(Err::MDIO_ERROR));
    CHECK(driver.healthSnapshot().totalFailures == 1 && driver.healthSnapshot().totalSuccess == 0);
    CHECK(driver.healthSnapshot().lastError.is(Err::MDIO_ERROR));
    CHECK(driver.healthSnapshot().lastError.detail == (invalidRelease ? -37 : 0));
  }

  for (int fault = 0; fault < 4; ++fault) {
    Controller bus; Driver driver; initialize(driver, bus); driver.resetStatistics();
    bus.faultCall = static_cast<int>(bus.calls);
    bus.shortSuccess = fault == 0; bus.nonterminal = fault == 1; bus.badStamp = fault == 2;
    if (fault == 3) { bus.release = {Err::MDIO_ERROR, -37, "Release"}; bus.reusable = false; }
    CHECK(link(driver).is(Err::MDIO_ERROR));
    CHECK(driver.healthSnapshot().totalFailures == 1 && driver.healthSnapshot().totalSuccess == 0);
    CHECK(driver.transportSnapshot().completedFrames == (fault == 0 ? 27U : 28U));
    CHECK(driver.healthSnapshot().lastError.detail == (fault == 1 ? -999 : fault == 3 ? -37 : 0));
  }
}

void timing() {
  // A zero clock is valid even though no optional per-frame timestamp is supplied.
  Controller zero; Driver driver; zero.now = 0; zero.frameTime = 0;
  initialize(driver, zero); CHECK(driver.healthSnapshot().lastOkTimeValid);
  CHECK(driver.healthSnapshot().lastOkUs == 0);
  zero.failNext(); CHECK(link(driver).is(Err::MDIO_TIMEOUT));
  CHECK(driver.healthSnapshot().lastErrorTimeValid && driver.healthSnapshot().lastErrorUs == 0);
  for (bool failure : {false, true}) for (bool overflow : {false, true}) {
    Controller bus; Driver subject; initialize(subject, bus);
    bus.failNext(); CHECK(link(subject).is(Err::MDIO_TIMEOUT));
    const auto old = subject.healthSnapshot();
    bus.faultCall = static_cast<int>(bus.calls); bus.failPhase = failure ? 0 : -1;
    bus.regress = !overflow; bus.overflow = overflow;
    const auto status = link(subject);
    CHECK(status.is(failure ? Err::MDIO_TIMEOUT : Err::INVALID_CONFIG));
    const auto health = subject.healthSnapshot();
    CHECK(subject.transportSnapshot().clockInvalid);
    CHECK(health.totalFailures == old.totalFailures + (failure ? 1U : 0U));
    CHECK(health.totalSuccess == old.totalSuccess + (failure ? 0U : 1U));
    if (failure) {
      CHECK(!health.lastErrorTimeValid && health.lastErrorUs == 0);
      CHECK(health.lastOkTimeValid && health.lastOkUs == old.lastOkUs);
    } else {
      CHECK(!health.lastOkTimeValid && health.lastOkUs == 0);
      CHECK(health.lastErrorTimeValid && health.lastErrorUs == old.lastErrorUs);
    }
  }
  Controller late; Driver lateDriver; initialize(lateDriver, late); lateDriver.resetStatistics();
  late.faultCall = static_cast<int>(late.calls); late.late = true;
  CHECK(link(lateDriver).is(Err::OPERATION_TIMEOUT));
  CHECK(lateDriver.healthSnapshot().totalSuccess == 1 && lateDriver.healthSnapshot().totalFailures == 0);
  CHECK(lateDriver.transportSnapshot().late);
  Controller excluded; Driver excludedDriver; initialize(excludedDriver, excluded); excludedDriver.resetStatistics();
  excluded.failNext(); excluded.noFrames = true; excluded.regress = true;
  excluded.injected = {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), "Busy"};
  CHECK(link(excludedDriver).is(Err::BUSY));
  CHECK(excludedDriver.healthSnapshot().totalSuccess == 0 && excludedDriver.healthSnapshot().totalFailures == 0);
  CHECK(excludedDriver.transportSnapshot().clockInvalid);
}

void restoration() {
  for (bool restoreFails : {false, true}) {
    Controller bus; Driver driver; initialize(driver, bus);
    const auto token = start(driver);
    for (unsigned n = 0; n < 5; ++n) CHECK(driver.pollOperation(token).inProgress());
    bus.failNext(3); // Mandatory VOD target accepted before reported error.
    CHECK(driver.pollOperation(token).is(Err::MDIO_TIMEOUT));
    CHECK(driver.operationSnapshot().cleanupOwed);
    const auto failed = take(driver, token);
    CHECK(failed.status.is(Err::MDIO_TIMEOUT) && failed.mutationPossible);
    const auto failure = driver.healthSnapshot();
    CHECK(failure.consecutiveFailures == 1);
    const auto restorationToken = start(driver);
    CHECK(restorationToken.value != token.value);
    if (restoreFails) bus.failNext();
    const auto status = drain(driver, bus, restorationToken);
    CHECK(status.is(restoreFails ? Err::MDIO_TIMEOUT : Err::OK));
    const auto restored = take(driver, restorationToken);
    CHECK(restored.token.value != failed.token.value);
    CHECK(failed.status.is(Err::MDIO_TIMEOUT) && failed.status.detail == -2147483647);
    CHECK(driver.healthSnapshot().lastError.detail == -2147483647);
    if (restoreFails) {
      CHECK(driver.healthSnapshot().consecutiveFailures == 2 && driver.operationSnapshot().cleanupOwed);
    } else {
      CHECK(driver.healthSnapshot().consecutiveFailures == 0 && !driver.operationSnapshot().cleanupOwed);
      CHECK(driver.driverState() == DriverState::READY);
    }
  }
  for (ResetKind kind : {ResetKind::NONE, ResetKind::PCS, ResetKind::HARD, ResetKind::RESTART}) {
    Controller bus; Driver driver; initialize(driver, bus);
    const auto token = start(driver, kind);
    const auto before = driver.healthSnapshot();
    CHECK(driver.cancel(token).ok()); sameHealth(driver.healthSnapshot(), before);
    CHECK(take(driver, token).status.is(Err::CANCELLED));
    const auto resetToken = start(driver, kind);
    CHECK(drain(driver, bus, resetToken).ok());
    CHECK(take(driver, resetToken).status.ok());
    CHECK(driver.driverState() == DriverState::READY && driver.healthSnapshot().totalFailures == 0);
  }
}

void cancellation() {
  for (ResetKind kind : {ResetKind::NONE, ResetKind::PCS, ResetKind::HARD, ResetKind::RESTART}) {
    // Cancel after every admitted batch boundary, including accepted reset/VOD
    // and verification. No clock/frame callback may arise from cancellation.
    for (unsigned boundary = 1; boundary <= 12; ++boundary) {
      Controller bus; Driver driver; initialize(driver, bus);
      const auto token = start(driver, kind);
      Status status{Err::IN_PROGRESS, 0, "Pending"};
      for (unsigned step = 0; step < boundary && status.inProgress(); ++step) {
        const auto operation = driver.operationSnapshot();
        if (operation.nextPollValid && bus.now < operation.nextPollAtUs) bus.now = operation.nextPollAtUs;
        status = driver.pollOperation(token);
      }
      if (!status.inProgress()) { CHECK(take(driver, token).status.ok()); continue; }
      const auto health = driver.healthSnapshot();
      const auto calls = bus.calls, clocks = bus.clocks;
      CHECK(driver.cancel(token).ok());
      CHECK(bus.calls == calls && bus.clocks == clocks);
      sameHealth(driver.healthSnapshot(), health);
      const auto result = take(driver, token);
      CHECK(result.status.is(Err::CANCELLED));
      CHECK(result.cleanupOwed == driver.operationSnapshot().cleanupOwed);
      if (result.commandAccepted && (kind == ResetKind::PCS || kind == ResetKind::HARD))
        CHECK(driver.driverState() == DriverState::UNINIT);
    }
  }
}

void saturation() {
  static_assert(sizeof(HealthSnapshot{}.totalSuccess) == 4);
  static_assert(sizeof(HealthSnapshot{}.totalFailures) == 4);
  static_assert(sizeof(HealthSnapshot{}.consecutiveFailures) == 4);
  static_assert(sizeof(HealthSnapshot{}.lastErrorUs) == 8);
  for (uint32_t initial : {0U, 0xFFFFFFFEU, 0xFFFFFFFFU}) {
    HealthSnapshot health;
    health.totalSuccess = initial; health.totalFailures = initial; health.consecutiveFailures = initial;
    const uint32_t expected = initial == 0U ? 1U : 0xFFFFFFFFU;
    detail::retainHealth(health, {Err::MDIO_ERROR, -2147483647, "Failure"}, 0, true);
    CHECK(health.totalFailures == expected && health.consecutiveFailures == expected);
    CHECK(health.totalSuccess == initial && health.lastErrorTimeValid && health.lastErrorUs == 0);
    detail::retainHealth(health, Status::Ok(), 0xFFFFFFFFFFFFFFFFULL, true);
    CHECK(health.totalSuccess == expected && health.consecutiveFailures == 0);
    CHECK(health.totalFailures == expected && health.lastError.detail == -2147483647);
    CHECK(health.lastOkTimeValid && health.lastOkUs == 0xFFFFFFFFFFFFFFFFULL);
    detail::retainHealth(health, {Err::MDIO_TIMEOUT, -37, "Failure"}, 123, false);
    CHECK(!health.lastErrorTimeValid && health.lastErrorUs == 0 && health.lastError.detail == -37);
    CHECK(health.lastOkTimeValid && health.lastOkUs == 0xFFFFFFFFFFFFFFFFULL);
    detail::retainHealth(health, Status::Ok(), 123, false);
    CHECK(!health.lastOkTimeValid && health.lastOkUs == 0);
  }
}

void isolation() {
  Controller firstBus, secondBus; Driver first, second;
  initialize(first, firstBus); initialize(second, secondBus);
  const auto secondHealth = second.healthSnapshot();
  firstBus.failNext(); CHECK(link(first).is(Err::MDIO_TIMEOUT));
  sameHealth(second.healthSnapshot(), secondHealth);
  const auto calls = firstBus.calls, clocks = firstBus.clocks;
  CHECK(first.invalidateDeviceState(InvalidationReason::EXTERNAL_POWER_ON).ok());
  CHECK(first.driverState() == DriverState::UNINIT && first.healthSnapshot().consecutiveFailures == 1);
  first.resetStatistics();
  CHECK(first.healthSnapshot().consecutiveFailures == 1 && first.healthSnapshot().lastError.detail == -2147483647);
  CHECK(firstBus.calls == calls && firstBus.clocks == clocks);
  CHECK(first.end().ok()); CHECK(first.bind(firstBus.config()).ok());
  CHECK(first.healthSnapshot().consecutiveFailures == 0 && first.healthSnapshot().lastError.ok());
  sameHealth(second.healthSnapshot(), secondHealth);
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"configuration", configuration}, {"lifecycle", lifecycle}, {"transitions", transitions}, {"accounting", accounting},
    {"raw_local", rawAndLocal}, {"reports", reports}, {"timing", timing},
    {"restoration", restoration}, {"isolation", isolation}, {"saturation", saturation}, {"cancellation", cancellation}
  };
  return test::runSuites(argc, argv, suites);
}
