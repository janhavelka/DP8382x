#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include <DP8382x/CommandTable.h>

#include <array>
#include <cstdint>
#include <initializer_list>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
constexpr IoBudget BUDGET{10000, 14};
constexpr std::array<uint16_t, 11> SOURCE_ADDRESSES{
    0x0010, 0x0001, 0x0006, 0x000F, 0x0012, 0x0013,
    0x0014, 0x0015, 0x0017, 0x001A, 0x04A1};
constexpr std::array<uint16_t, 11> SOURCE_WORDS{
    0x3FE7, 0x785D, 0x001F, 0x01F0, 0xA55A, 0x5AA5,
    0xABFF, 0x8123, 0x01CF, 0x2111, 0x10FF};

// Independent literals from SNLS647I tables 8-20/25/33/34/36-39/41/44/90/91.
// This model deliberately retains reserved poison bits, clears only sourced
// read effects, and models W0C rather than importing a family write-one recipe.
// It proves host behavior, never hardware latch polarity or physical timing.
enum class ReportFault { NONE, MISSING_VALUE, EXTRA_FRAME, BAD_TIME, ILLEGAL_STATUS, MISSING_ATTEMPT };
struct WireFrame {
  FrameKind kind{FrameKind::READ};
  uint8_t reg{0};
  uint16_t address{0};
  uint16_t word{0};
  unsigned call{0};
};

struct Controller {
  std::array<uint16_t, 0x500> words{};
  std::array<WireFrame, 96> trace{};
  std::array<MdioBatch, 32> requests{};
  std::array<IoBudget, 32> budgets{};
  uint16_t selector{0xFFFF};
  uint16_t address{0xFFFF};
  uint64_t now{100};
  uint64_t frameDuration{2};
  unsigned clocks{0};
  unsigned calls{0};
  unsigned acquired{0};
  unsigned released{0};
  unsigned launched{0};
  int failFrame{-1};
  int expireFrame{-1};
  unsigned releaseFailureCall{0};
  unsigned regressCall{0};
  unsigned faultCall{1};
  uint8_t faultIndex{0};
  ReportFault fault{ReportFault::NONE};
  bool effectPossible{true};
  bool completedTime{true};
  bool locked{false};
  bool pendingWake{true};
  bool reenter{false};
  bool clockReenter{false};
  Driver* driver{nullptr};
  Driver* competitor{nullptr};

  Controller() {
    for (unsigned i = 0; i < SOURCE_ADDRESSES.size(); ++i) {
      words[SOURCE_ADDRESSES[i]] = SOURCE_WORDS[i];
    }
    words[0x04A0] = 0x1181;
    words[2] = 0x2000;
  }
  Config config() noexcept {
    Config value;
    value.variant = Variant::DP83826I;
    value.interface = MacInterface::RMII;
    value.phyAddress = 1;
    value.transport = {this, execute, clock, {8, 4, 1}};
    return value;
  }
  static uint64_t clock(void* context) noexcept {
    auto& self = *static_cast<Controller*>(context);
    ++self.clocks;
    if (self.clockReenter && self.driver != nullptr) {
      EvidenceSnapshot output;
      output.lastRequested = 0xDEADBEEF;
      CHECK(self.driver->acquireEvidence(1, output, BUDGET).is(Err::BUSY));
      CHECK(self.driver->modifyInterruptEnables(0x12, 1, 1, BUDGET).is(Err::BUSY));
      CHECK(self.driver->modifyWakeControl(0x0100, 0, false, BUDGET).is(Err::BUSY));
      CHECK(self.driver->end().is(Err::BUSY));
      CHECK(output.lastRequested == 0xDEADBEEFU);
    }
    return self.now;
  }
  uint16_t read(uint16_t target) noexcept {
    CHECK(target < words.size());
    if (target >= words.size()) { return 0; }
    const uint16_t value = words[target];
    switch (target) {
      case 0x0001:
        words[target] &= 0xFFEF; // Remote fault read-clear; link fixture remains up.
        words[0x0010] &= 0xFFBF;
        break;
      case 0x0006: words[target] &= 0xFFFD; words[0x0010] &= 0xFEFF; break;
      case 0x000F: words[target] &= 0xFE0F; break;
      case 0x0010: words[target] &= 0xF9FF; break; // Own signal/descrambler RC.
      case 0x0012: words[target] &= 0x00FF; words[0x0010] &= 0xFF7F; break;
      case 0x0013: words[target] &= 0x00FF; break;
      case 0x0014: words[target] &= 0xFF00; words[0x0010] &= 0xF7FF; break;
      case 0x0015: words[target] = 0; words[0x0010] &= 0xDFFF; break;
      case 0x0017: words[target] &= 0xFFF3; break;
      case 0x001A: words[target] &= 0xFFEF; words[0x0010] &= 0xEFFF; break;
      case 0x04A1: words[target] &= 0xFF00; break;
      default: break;
    }
    return value;
  }
  void write(uint8_t reg, uint16_t target, uint16_t value) noexcept {
    if (reg == 0x0D) { selector = value; return; }
    if (reg == 0x0E && selector == 0x001F) { address = value; return; }
    CHECK(target < words.size());
    if (target >= words.size()) { return; }
    if (target == 0x0012 || target == 0x0013) {
      words[target] = static_cast<uint16_t>((words[target] & 0xFF00U) | (value & 0x00FFU));
    } else if (target == 0x04A0) {
      if ((value & 0x0800U) == 0U) { pendingWake = false; }
      words[target] = static_cast<uint16_t>(value & 0xF7FFU);
    } else { words[target] = value; }
  }
  void checkReentry() noexcept {
    if (!reenter || driver == nullptr) { return; }
    EvidenceSnapshot output;
    output.lastRequested = 0xDEADBEEF;
    const unsigned before = calls;
    CHECK(driver->acquireEvidence(1, output, BUDGET).is(Err::BUSY));
    CHECK(driver->modifyInterruptEnables(0x12, 1, 1, BUDGET).is(Err::BUSY));
    CHECK(driver->modifyWakeControl(0x0100, 0, false, BUDGET).is(Err::BUSY));
    CHECK(driver->invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).is(Err::BUSY));
    CHECK(driver->bind(config()).is(Err::BUSY));
    CHECK(driver->end().is(Err::BUSY));
    CHECK(output.lastRequested == 0xDEADBEEFU);
    CHECK(calls == before);
    if (competitor != nullptr) {
      uint16_t word = 0xCAFE;
      CHECK(competitor->readClause22(2, word, BUDGET).is(Err::BUSY));
      CHECK(word == 0xCAFE && calls == before + 1U);
    }
  }
  static BatchResult execute(void* context, const MdioBatch& batch,
                             const IoBudget& budget) noexcept {
    auto& self = *static_cast<Controller*>(context);
    const unsigned call = ++self.calls;
    BatchResult result;
    result.releaseStatus = Status::Ok();
    result.controllerReusable = true;
    if (self.locked) {
      result.status = {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), "Shared controller busy"};
      return result;
    }
    CHECK(batch.frameCount > 0U && batch.frameCount <= 8U);
    CHECK(call <= self.requests.size());
    self.requests[call - 1U] = batch;
    self.budgets[call - 1U] = budget;
    self.locked = true;
    ++self.acquired;
    result.status = Status::Ok();
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      if (self.now >= budget.deadlineUs) {
        result.status = {Err::OPERATION_TIMEOUT, static_cast<int32_t>(TimeoutDetail::DEADLINE), "Deadline"};
        break;
      }
      const auto& frame = batch.frames[i];
      auto& receipt = result.frames[i];
      CHECK(frame.registerAddress < 32U);
      uint16_t value = frame.value;
      if (frame.kind == FrameKind::READ) {
        CHECK(frame.value == 0U && frame.preserveMask == 0U && frame.readIndex == 0xFFU);
      } else if (frame.readIndex == 0xFFU) {
        CHECK(frame.preserveMask == 0U);
      } else {
        CHECK(frame.readIndex < i);
        CHECK(batch.frames[frame.readIndex].kind == FrameKind::READ);
        CHECK(result.frames[frame.readIndex].completed && result.frames[frame.readIndex].valueValid);
        CHECK((frame.value & frame.preserveMask) == 0U);
        value = static_cast<uint16_t>((result.frames[frame.readIndex].value & frame.preserveMask) | frame.value);
      }
      const uint16_t target = frame.registerAddress == 0x0E && self.selector == 0x401F ? self.address : frame.registerAddress;
      const unsigned ordinal = self.launched++;
      CHECK(ordinal < self.trace.size());
      self.trace[ordinal] = {frame.kind, frame.registerAddress, target, value, call};
      receipt.attempted = true;
      const bool failed = static_cast<int>(ordinal) == self.failFrame;
      if (!failed || self.effectPossible) {
        if (frame.kind == FrameKind::READ) { value = self.read(target); }
        else { self.write(frame.registerAddress, target, value); }
      }
      self.now += self.frameDuration;
      if (failed) {
        receipt.effectPossible = self.effectPossible;
        result.status = {Err::MDIO_TIMEOUT, -37, "Injected frame failure"};
      } else {
        receipt.completed = true;
        receipt.timeValid = self.completedTime;
        receipt.completedAtUs = self.completedTime ? self.now : 0U;
        if (frame.kind == FrameKind::READ) { receipt.valueValid = true; receipt.value = value; }
      }
      self.checkReentry();
      if (static_cast<int>(ordinal) == self.expireFrame) { self.now = budget.deadlineUs; }
      if (failed) { break; }
    }
    if (call == self.faultCall) {
      switch (self.fault) {
        case ReportFault::MISSING_VALUE: result.frames[self.faultIndex].valueValid = false; break;
        case ReportFault::MISSING_ATTEMPT:
          result.frames[self.faultIndex].attempted = false;
          result.frames[self.faultIndex].effectPossible = false;
          break;
        case ReportFault::EXTRA_FRAME: result.frames[batch.frameCount].attempted = true; break;
        case ReportFault::BAD_TIME: result.frames[self.faultIndex].completedAtUs = 99; result.frames[self.faultIndex].timeValid = true; break;
        case ReportFault::ILLEGAL_STATUS: result.status = {Err::IN_PROGRESS, -19, "Nonterminal report"}; break;
        case ReportFault::NONE: break;
      }
    }
    self.locked = false;
    ++self.released;
    if (call == self.releaseFailureCall) {
      result.releaseStatus = {Err::MDIO_ERROR, -901, "Release failed"};
      result.controllerReusable = false;
    }
    if (call == self.regressCall) { self.now = 1; }
    return result;
  }
};

struct BoundDriver {
  Controller bus;
  Driver driver;
  BoundDriver() { CHECK(driver.bind(bus.config()).ok()); bus.driver = &driver; }
};

const EvidenceSample& sample(const EvidenceSnapshot& evidence, EvidenceSource source) {
  return evidence.samples[static_cast<uint8_t>(source)];
}
EvidenceSnapshot sentinel() {
  EvidenceSnapshot value;
  value.lastRequested = 0xDEADBEEF;
  value.samples[0].raw = 0xCAFE;
  return value;
}
void checkSentinel(const EvidenceSnapshot& value) {
  CHECK(value.lastRequested == 0xDEADBEEFU && value.samples[0].raw == 0xCAFE);
}

void compositions() {
  // Command neutral values are per-register. BMCR pin-induced power is not intent.
  CHECK(cmd::composeBmcr(0xFFFF, 0x1000, 0, false) == 0x6580U);
  CHECK(cmd::composeBmcr(0xFFFF, 0x1000, 0, true) == 0x6D80U);
  CHECK(cmd::composeBmcr(0x8200, 0x1000, 0x1000, false) == 0x1000U);
  CHECK(cmd::MASK_PHYRCR_HARD_RESET == 0x8000U);
  CHECK(cmd::MASK_PHYRCR_CIRCUITRY_RESTART == 0x4000U);
  CHECK(cmd::composeInterruptEnables(0xA55A, 0x0024, 0x0004) == 0x005EU);
  CHECK(cmd::composeWakeControl(0x1181, 0x0600, 0x0400, false) == 0x1D81U);
  CHECK(cmd::composeWakeControl(0x1181, 0x0600, 0x0400, true) == 0x1581U);
  CHECK(cmd::composeWakeControl(0x1181, 0, 0, false) == 0x1981U);
  CHECK(cmd::composeWakeControl(0x1981, 0, 0, true) == 0x1181U);
  CHECK(cmd::composeWakeControl(0xFFFF, 0x1000, 0, false) == 0xEFFFU);
  CHECK(cmd::composeWakeControl(0xFFFF, 0x1000, 0, true) == 0xE7FFU);
  CHECK(evidenceBit(static_cast<EvidenceSource>(255)) == 0U);
  CHECK(evidenceBit(EvidenceSource::COUNT) == 0U);
  CHECK(ALL_EVIDENCE == 0x07FFU);

  // Re-derived from rendered SNLS647I table 8-49 p74, table 8-85 pp87-88,
  // and table 8-89 pp90-91. These literals do not use the ledger as an oracle.
  CHECK(cmd::REG_MLEDCR == 0x0025U);
  CHECK(cmd::MASK_MLEDCR_SOURCE == 0x0078U); // Four bits, not the older 3-bit mask.
  CHECK(cmd::MASK_MLEDCR_POLARITY_SWAP == 0x0200U);
  CHECK(cmd::MASK_MLEDCR_ROUTE_ENABLE == 0x0001U);
  CHECK((cmd::MASK_MLEDCR_SOURCE | cmd::MASK_MLEDCR_POLARITY_SWAP |
         cmd::MASK_MLEDCR_ROUTE_ENABLE) == 0x0279U);
  constexpr uint16_t mledRouted[] = {
    0x0001, 0x0009, 0x0011, 0x0019, 0x0021, 0x0029, 0x0031, 0x0039,
    0x0041, 0x0049, 0x0051, 0x0059, 0x0061, 0x0069, 0x0071
  };
  for (unsigned code = 0; code < 15U; ++code) {
    CHECK((((code << 3U) & cmd::MASK_MLEDCR_SOURCE) |
           cmd::MASK_MLEDCR_ROUTE_ENABLE) == mledRouted[code]);
  }
  // 0xF is reserved; these are encoding tests, not a feature-admission API.
  CHECK(((0x000FU << 3U) & cmd::MASK_MLEDCR_SOURCE) == 0x0078U);

  CHECK(cmd::REG_LEDCFG == 0x0460U);
  CHECK(cmd::MASK_LEDCFG_LED1_SOURCE == 0x00F0U);
  CHECK(cmd::MASK_LEDCFG_LED2_SOURCE == 0x0F00U);
  CHECK(cmd::MASK_LEDCFG_LED3_SOURCE == 0xF000U);
  CHECK(((0x5665U & ~uint32_t{cmd::MASK_LEDCFG_LED1_SOURCE}) |
         ((0x000CU << 4U) & cmd::MASK_LEDCFG_LED1_SOURCE)) == 0x56C5U);
  CHECK(((0x5665U & ~uint32_t{cmd::MASK_LEDCFG_LED3_SOURCE}) |
         ((0x0002U << 12U) & cmd::MASK_LEDCFG_LED3_SOURCE)) == 0x2665U);
  CHECK(((0xFFFFU & ~uint32_t{cmd::MASK_LEDCFG_LED1_SOURCE}) | 0x00A0U) == 0xFFAFU);
  CHECK(cmd::MASK_CR2_EXTENDED_DUPLEX == 0x0020U);
  CHECK((0xFFFFU & ~uint32_t{cmd::MASK_CR2_EXTENDED_DUPLEX}) == 0xFFDFU);

  CHECK(cmd::REG_LEDCFG2 == 0x0469U);
  CHECK(cmd::MASK_LEDCFG2_LED1_POLARITY == 0x0004U);
  CHECK(cmd::MASK_LEDCFG2_LED1_DRIVE_VALUE == 0x0002U);
  CHECK(cmd::MASK_LEDCFG2_LED1_DRIVE_ENABLE == 0x0001U);
  CHECK(cmd::MASK_LEDCFG2_LED2_POLARITY == 0x0040U);
  CHECK(cmd::MASK_LEDCFG2_LED2_DRIVE_VALUE == 0x0020U);
  CHECK(cmd::MASK_LEDCFG2_LED2_DRIVE_ENABLE == 0x0010U);
  const uint32_t ledcfg2Owned = cmd::MASK_LEDCFG2_LED1_POLARITY |
      cmd::MASK_LEDCFG2_LED1_DRIVE_VALUE | cmd::MASK_LEDCFG2_LED1_DRIVE_ENABLE |
      cmd::MASK_LEDCFG2_LED2_POLARITY | cmd::MASK_LEDCFG2_LED2_DRIVE_VALUE |
      cmd::MASK_LEDCFG2_LED2_DRIVE_ENABLE;
  CHECK(ledcfg2Owned == 0x0077U && (ledcfg2Owned & 0xFF88U) == 0U);
  // Bit10 is reserved RH/W with unknown reset, not an LED3 polarity control.
  CHECK(((0x0488U & ~ledcfg2Owned) | 0x0041U) == 0x04C9U);
  CHECK(((0xFFFFU & ~uint32_t{0x0007}) | 0x0001U) == 0xFFF9U);
  CHECK(((0xFFFFU & ~uint32_t{0x0070}) | 0x0050U) == 0xFFDFU);
}

void acquisition() {
  BoundDriver fixture;
  auto output = sentinel();
  CHECK(fixture.driver.acquireEvidence(0x07FF, output, BUDGET).ok());
  CHECK(fixture.bus.calls == 2U && fixture.bus.launched == 14U);
  CHECK(fixture.bus.acquired == 2U && fixture.bus.released == 2U && !fixture.bus.locked);
  CHECK(fixture.bus.requests[0].frameCount == 8U && fixture.bus.requests[1].frameCount == 6U);
  CHECK(fixture.bus.budgets[0].maxFrames == 14U && fixture.bus.budgets[1].maxFrames == 6U);
  CHECK(fixture.bus.budgets[0].deadlineUs == 10000U && fixture.bus.budgets[1].deadlineUs == 10000U);
  for (unsigned i = 0; i < 10U; ++i) {
    CHECK(fixture.bus.trace[i].kind == FrameKind::READ);
    CHECK(fixture.bus.trace[i].address == SOURCE_ADDRESSES[i]);
  }
  CHECK(fixture.bus.trace[10].reg == 0x0DU && fixture.bus.trace[10].word == 0x001FU);
  CHECK(fixture.bus.trace[11].reg == 0x0EU && fixture.bus.trace[11].word == 0x04A1U);
  CHECK(fixture.bus.trace[12].reg == 0x0DU && fixture.bus.trace[12].word == 0x401FU);
  CHECK(fixture.bus.trace[13].kind == FrameKind::READ && fixture.bus.trace[13].address == 0x04A1U);
  for (unsigned i = 0; i < SOURCE_WORDS.size(); ++i) {
    const auto& evidence = output.samples[i];
    CHECK(evidence.valid && evidence.timeValid && !evidence.stale && !evidence.historyLost);
    CHECK(evidence.raw == SOURCE_WORDS[i] && evidence.observedOnes == SOURCE_WORDS[i]);
    CHECK(evidence.observedZeros == static_cast<uint16_t>(~SOURCE_WORDS[i]));
    CHECK(evidence.samples == 1U);
  }
  CHECK(output.samples[0].observedAtUs == 102U);
  CHECK(output.samples[10].observedAtUs == 128U);
  CHECK(output.lastRequested == 0x07FFU && output.lastStatus.ok() && !output.configurationDirty);
  const auto transport = fixture.driver.transportSnapshot();
  CHECK(transport.totalBatches == 2U && transport.attemptedFrames == 14U && transport.completedFrames == 14U);
  const unsigned clocks = fixture.bus.clocks;
  const unsigned calls = fixture.bus.calls;
  CHECK(fixture.driver.evidenceSnapshot().samples[0].raw == 0x3FE7U);
  CHECK(fixture.driver.transportSnapshot().totalBatches == 2U);
  CHECK(fixture.bus.clocks == clocks && fixture.bus.calls == calls);
  // Physical bits were consumed, but snapshots never consume again.
  CHECK((fixture.bus.words[0x000F] & 0x01F0U) == 0U);
  CHECK((fixture.bus.words[0x0010] & 0x39C0U) == 0U);
  CHECK(fixture.driver.acquireEvidence(evidenceBit(EvidenceSource::FLDS), output, BUDGET).ok());
  const auto& fld = sample(output, EvidenceSource::FLDS);
  CHECK(fld.raw == 0U && fld.observedOnes == 0x01F0U && fld.samples == 2U);
  CHECK(fixture.driver.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
  CHECK(fixture.driver.evidenceSnapshot().samples[3].valid);
  CHECK(fixture.driver.evidenceSnapshot().samples[3].stale);
  CHECK(fixture.driver.evidenceSnapshot().configurationDirty);
  CHECK(fixture.bus.clocks == clocks + 2U && fixture.bus.calls == calls + 1U);
  CHECK(fixture.driver.end().ok());
  CHECK(!fixture.driver.evidenceSnapshot().samples[0].valid);
  // Latched-low observations survive a later high sample as their own evidence.
  BoundDriver latch;
  latch.bus.words[1] = 0x7849;
  CHECK(latch.driver.acquireEvidence(2, output, BUDGET).ok());
  latch.bus.words[1] = 0x784D;
  CHECK(latch.driver.acquireEvidence(2, output, BUDGET).ok());
  CHECK(sample(output, EvidenceSource::BMSR).raw == 0x784DU);
  CHECK((sample(output, EvidenceSource::BMSR).observedZeros & 4U) != 0U);
  CHECK((sample(output, EvidenceSource::BMSR).observedOnes & 4U) != 0U);
  CHECK(latch.driver.writeClause22(0x18, 0x0012, BUDGET).ok());
  CHECK(sample(latch.driver.evidenceSnapshot(), EvidenceSource::BMSR).stale);
  CHECK(sample(latch.driver.evidenceSnapshot(), EvidenceSource::BMSR).valid);
  CHECK(latch.driver.evidenceSnapshot().configurationDirty);
}

void failures() {
  for (int failed = 0; failed < 14; ++failed) {
    BoundDriver fixture;
    fixture.bus.failFrame = failed;
    auto output = sentinel();
    const auto status = fixture.driver.acquireEvidence(0x07FF, output, BUDGET);
    CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail == -37);
    checkSentinel(output);
    CHECK(fixture.bus.launched == static_cast<unsigned>(failed + 1));
    CHECK(fixture.bus.calls == (failed < 8 ? 1U : 2U));
    CHECK(fixture.bus.acquired == fixture.bus.released && !fixture.bus.locked);
    const auto evidence = fixture.driver.evidenceSnapshot();
    for (int i = 0; i < 11; ++i) {
      const int ordinal = i == 10 ? 13 : i;
      const auto& retained = evidence.samples[static_cast<unsigned>(i)];
      CHECK(retained.valid == (ordinal < failed));
      CHECK(retained.historyLost == (ordinal == failed));
      CHECK(retained.samples == (ordinal < failed ? 1U : 0U));
      if (ordinal < failed) { CHECK(retained.raw == SOURCE_WORDS[static_cast<unsigned>(i)]); }
    }
    const auto transport = fixture.driver.transportSnapshot();
    CHECK(transport.attemptedFrames == static_cast<unsigned>(failed + 1));
    CHECK(transport.completedFrames == static_cast<unsigned>(failed));
    CHECK(!evidence.configurationDirty); // Selector traffic is not configuration mutation.
  }
  // A controller-proved no-effect failure is stale, but loses no hardware history.
  BoundDriver noEffect;
  noEffect.bus.failFrame = 0;
  noEffect.bus.effectPossible = false;
  auto output = sentinel();
  CHECK(noEffect.driver.acquireEvidence(evidenceBit(EvidenceSource::FLDS), output, BUDGET).is(Err::MDIO_TIMEOUT));
  CHECK(!sample(noEffect.driver.evidenceSnapshot(), EvidenceSource::FLDS).historyLost);
  CHECK(noEffect.bus.words[0x0F] == 0x01F0U);
  // Lost history survives later successful collection rather than becoming zero events.
  BoundDriver lost;
  lost.bus.failFrame = 0;
  CHECK(lost.driver.acquireEvidence(evidenceBit(EvidenceSource::FLDS), output, BUDGET).is(Err::MDIO_TIMEOUT));
  lost.bus.failFrame = -1;
  CHECK(lost.driver.acquireEvidence(evidenceBit(EvidenceSource::FLDS), output, BUDGET).ok());
  CHECK(sample(output, EvidenceSource::FLDS).raw == 0U);
  CHECK(sample(output, EvidenceSource::FLDS).valid && sample(output, EvidenceSource::FLDS).historyLost);
}

void reports() {
  // A corrupt attempt flag cannot turn a completed destructive effect into
  // proof of no consumption. Reject its value but retain possible history loss.
  {
    BoundDriver fixture;
    fixture.bus.fault = ReportFault::MISSING_ATTEMPT;
    auto output = sentinel();
    CHECK(fixture.driver.acquireEvidence(evidenceBit(EvidenceSource::FLDS), output, BUDGET).is(Err::MDIO_ERROR));
    checkSentinel(output);
    CHECK((fixture.bus.words[0x000F] & 0x01F0U) == 0U);
    const auto retained = sample(fixture.driver.evidenceSnapshot(), EvidenceSource::FLDS);
    CHECK(!retained.valid && retained.historyLost && retained.stale);
    CHECK(retained.samples == 0U);
    const auto report = fixture.driver.transportSnapshot();
    CHECK(report.violation == TransportViolation::FRAME_REPORT && report.stopped);
    CHECK(!report.lastBatch.frames[0].attempted && report.lastBatch.frames[0].completed);
    CHECK(!report.lastBatch.frames[0].effectPossible);
  }
  // The same report corruption after a physical target write must invalidate
  // prior configuration evidence even though no attempted prefix is trusted.
  {
    BoundDriver fixture;
    fixture.bus.fault = ReportFault::MISSING_ATTEMPT;
    CHECK(fixture.driver.writeClause22(0x18, 0x0012, BUDGET).is(Err::MDIO_ERROR));
    CHECK(fixture.bus.words[0x0018] == 0x0012U);
    CHECK(fixture.driver.evidenceSnapshot().configurationDirty);
    const auto report = fixture.driver.transportSnapshot();
    CHECK(report.violation == TransportViolation::FRAME_REPORT && report.stopped);
    CHECK(!report.lastBatch.frames[0].attempted && report.lastBatch.frames[0].completed);
    CHECK(!report.lastBatch.frames[0].valueValid && !report.lastBatch.frames[0].effectPossible);
    CHECK(fixture.bus.calls == 1U && fixture.bus.launched == 1U);
    CHECK(fixture.bus.acquired == fixture.bus.released && !fixture.bus.locked);
  }
  for (unsigned release : {1U, 2U}) {
    BoundDriver fixture;
    fixture.bus.releaseFailureCall = release;
    auto output = sentinel();
    const auto status = fixture.driver.acquireEvidence(0x07FF, output, BUDGET);
    CHECK(status.is(Err::MDIO_ERROR) && status.detail == -901);
    checkSentinel(output);
    CHECK(fixture.bus.calls == release);
    const auto evidence = fixture.driver.evidenceSnapshot();
    CHECK(evidence.samples[0].valid && !evidence.samples[0].historyLost);
    CHECK(evidence.samples[10].valid == (release == 2U));
    CHECK(!fixture.driver.transportSnapshot().controllerReusable);
  }
  {
    BoundDriver fixture;
    fixture.bus.fault = ReportFault::MISSING_VALUE;
    fixture.bus.faultIndex = 1;
    auto output = sentinel();
    CHECK(fixture.driver.acquireEvidence(0x0019, output, BUDGET).is(Err::MDIO_ERROR));
    checkSentinel(output);
    const auto evidence = fixture.driver.evidenceSnapshot();
    CHECK(evidence.samples[0].valid);
    CHECK(!evidence.samples[3].valid && evidence.samples[3].historyLost);
    CHECK(!evidence.samples[4].valid && evidence.samples[4].historyLost);
    CHECK(fixture.driver.transportSnapshot().stopped);
  }
  for (ReportFault fault : {ReportFault::EXTRA_FRAME, ReportFault::BAD_TIME, ReportFault::ILLEGAL_STATUS}) {
    BoundDriver fixture;
    fixture.bus.fault = fault;
    auto output = sentinel();
    CHECK(fixture.driver.acquireEvidence(1, output, BUDGET).is(Err::MDIO_ERROR));
    checkSentinel(output);
    const auto evidence = fixture.driver.evidenceSnapshot();
    CHECK(evidence.samples[0].valid && evidence.samples[0].raw == 0x3FE7U);
    CHECK(!evidence.samples[0].historyLost);
    CHECK(evidence.samples[0].timeValid == (fault != ReportFault::BAD_TIME));
    CHECK(fixture.driver.transportSnapshot().stopped);
  }
  for (int expired : {0, 7, 13}) {
    BoundDriver fixture;
    fixture.bus.expireFrame = expired;
    auto output = sentinel();
    CHECK(fixture.driver.acquireEvidence(0x07FF, output, BUDGET).is(Err::OPERATION_TIMEOUT));
    checkSentinel(output);
    CHECK(fixture.bus.launched == static_cast<unsigned>(expired + 1));
    CHECK(fixture.driver.evidenceSnapshot().samples[0].valid);
    CHECK(!fixture.driver.evidenceSnapshot().samples[0].historyLost);
    CHECK(fixture.driver.transportSnapshot().late);
  }
  {
    BoundDriver fixture;
    fixture.bus.regressCall = 1;
    auto output = sentinel();
    CHECK(fixture.driver.acquireEvidence(1, output, BUDGET).is(Err::INVALID_CONFIG));
    CHECK(fixture.driver.evidenceSnapshot().samples[0].valid);
    CHECK(!fixture.driver.evidenceSnapshot().samples[0].timeValid);
    CHECK(fixture.driver.transportSnapshot().clockInvalid);
    CHECK(fixture.driver.acquireEvidence(1, output, BUDGET).is(Err::INVALID_CONFIG));
    CHECK(fixture.bus.calls == 1U);
  }
  {
    BoundDriver fixture;
    fixture.bus.completedTime = false;
    auto output = sentinel();
    CHECK(fixture.driver.acquireEvidence(1, output, BUDGET).ok());
    CHECK(output.samples[0].timeValid && output.samples[0].observedAtUs == 103U);
  }
  {
    BoundDriver fixture;
    fixture.bus.now = 0;
    fixture.bus.frameDuration = 0;
    auto output = sentinel();
    CHECK(fixture.driver.acquireEvidence(1, output, BUDGET).ok());
    CHECK(output.samples[0].timeValid && output.samples[0].observedAtUs == 0U);
  }
  {
    BoundDriver fixture;
    fixture.bus.failFrame = 0;
    fixture.bus.expireFrame = 0;
    fixture.bus.releaseFailureCall = 1;
    auto output = sentinel();
    const auto status = fixture.driver.acquireEvidence(1, output, BUDGET);
    CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail == -37);
    CHECK(fixture.driver.transportSnapshot().late);
    CHECK(fixture.driver.transportSnapshot().lastBatch.releaseStatus.detail == -901);
    CHECK(fixture.driver.evidenceSnapshot().samples[0].historyLost);
  }
}

void interrupts() {
  for (uint32_t address : {0x12U, 0x13U}) {
    BoundDriver fixture;
    fixture.bus.words[address] = 0xA55A;
    CHECK(fixture.driver.modifyInterruptEnables(address, 0x0024, 0x0004, {10000, 2}).ok());
    CHECK(fixture.bus.calls == 1U && fixture.bus.launched == 2U);
    CHECK(fixture.bus.requests[0].frames[1].readIndex == 0U);
    CHECK(fixture.bus.requests[0].frames[1].preserveMask == 0x00DBU);
    CHECK(fixture.bus.trace[1].word == 0x005EU);
    CHECK(fixture.bus.words[address] == 0x005EU);
    const auto evidence = fixture.driver.evidenceSnapshot();
    const auto& retained = sample(evidence, address == 0x12U ? EvidenceSource::MISR1 : EvidenceSource::MISR2);
    CHECK(retained.valid && retained.raw == 0xA55AU && retained.samples == 1U);
    CHECK(retained.observedOnes == 0xA55AU && !retained.historyLost);
    CHECK(evidence.configurationDirty);
  }
  for (int failed : {0, 1}) {
    BoundDriver fixture;
    fixture.bus.failFrame = failed;
    CHECK(fixture.driver.modifyInterruptEnables(0x12, 0x0024, 0x0004, BUDGET).is(Err::MDIO_TIMEOUT));
    const auto evidence = fixture.driver.evidenceSnapshot();
    const auto& retained = sample(evidence, EvidenceSource::MISR1);
    CHECK(retained.valid == (failed == 1));
    CHECK(retained.historyLost == (failed == 0));
    CHECK(evidence.configurationDirty == (failed == 1));
    CHECK(fixture.bus.launched == static_cast<unsigned>(failed + 1));
  }
}

void wake() {
  for (bool acknowledge : {false, true}) {
    BoundDriver fixture;
    CHECK(fixture.driver.modifyWakeControl(0x0600, 0x0400, acknowledge, BUDGET).ok());
    CHECK(fixture.bus.calls == 3U && fixture.bus.launched == 14U);
    CHECK(fixture.bus.requests[0].frameCount == 4U && fixture.bus.requests[1].frameCount == 5U && fixture.bus.requests[2].frameCount == 5U);
    CHECK(fixture.bus.budgets[0].maxFrames == 14U && fixture.bus.budgets[1].maxFrames == 10U && fixture.bus.budgets[2].maxFrames == 5U);
    CHECK(fixture.bus.trace[3].address == 0x04A0U && fixture.bus.trace[3].kind == FrameKind::READ);
    CHECK(fixture.bus.trace[4].address == 0x0013U && fixture.bus.trace[4].kind == FrameKind::READ);
    CHECK(fixture.bus.trace[8].address == 0x04A1U && fixture.bus.trace[8].kind == FrameKind::READ);
    CHECK(fixture.bus.trace[12].address == 0x04A0U && fixture.bus.trace[12].kind == FrameKind::READ);
    CHECK(fixture.bus.trace[13].address == 0x04A0U && fixture.bus.trace[13].kind == FrameKind::WRITE);
    CHECK(fixture.bus.trace[12].call == fixture.bus.trace[13].call);
    CHECK(fixture.bus.trace[13].word == (acknowledge ? 0x1581U : 0x1D81U));
    CHECK(fixture.bus.pendingWake == !acknowledge);
    CHECK(fixture.bus.words[0x04A0] == 0x1581U);
    const auto evidence = fixture.driver.evidenceSnapshot();
    CHECK(sample(evidence, EvidenceSource::MISR2).raw == 0x5AA5U);
    CHECK(sample(evidence, EvidenceSource::RXFS).raw == 0x10FFU);
    CHECK(evidence.configurationDirty);
  }
  {
    BoundDriver fixture;
    CHECK(fixture.driver.modifyWakeControl(0x0100, 0x0100, false, BUDGET).ok());
    CHECK(fixture.bus.calls == 1U && fixture.bus.launched == 4U);
    CHECK(fixture.bus.pendingWake && fixture.bus.words[0x04A1] == 0x10FFU);
    CHECK(!fixture.driver.evidenceSnapshot().configurationDirty);
    CHECK(!sample(fixture.driver.evidenceSnapshot(), EvidenceSource::RXFS).valid);
  }
  {
    BoundDriver fixture;
    CHECK(fixture.driver.modifyWakeControl(0, 0, true, BUDGET).ok());
    CHECK(fixture.bus.trace[13].word == 0x1181U && !fixture.bus.pendingWake);
  }
  {
    BoundDriver fixture;
    fixture.bus.words[0x04A0] = 0xFFFF;
    CHECK(fixture.driver.modifyWakeControl(0x1000, 0, false, BUDGET).ok());
    CHECK(fixture.bus.trace[13].word == 0xEFFFU); // Reserved poison and unknown bit6 preserved.
    CHECK(fixture.bus.pendingWake);
  }
  for (int failed = 0; failed < 14; ++failed) {
    BoundDriver fixture;
    fixture.bus.failFrame = failed;
    CHECK(fixture.driver.modifyWakeControl(0x0600, 0x0400, false, BUDGET).is(Err::MDIO_TIMEOUT));
    CHECK(fixture.bus.launched == static_cast<unsigned>(failed + 1));
    const auto evidence = fixture.driver.evidenceSnapshot();
    CHECK(sample(evidence, EvidenceSource::MISR2).valid == (failed > 4));
    CHECK(sample(evidence, EvidenceSource::MISR2).historyLost == (failed == 4));
    CHECK(sample(evidence, EvidenceSource::RXFS).valid == (failed > 8));
    CHECK(sample(evidence, EvidenceSource::RXFS).historyLost == (failed == 8));
    CHECK(evidence.configurationDirty == (failed == 13));
    CHECK(fixture.bus.pendingWake); // Failed neutral write still never acknowledges.
    CHECK(fixture.bus.acquired == fixture.bus.released && !fixture.bus.locked);
  }
  {
    BoundDriver fixture;
    fixture.bus.failFrame = 13;
    CHECK(fixture.driver.modifyWakeControl(0, 0, true, BUDGET).is(Err::MDIO_TIMEOUT));
    CHECK(!fixture.bus.pendingWake); // A failed acknowledgment can still have applied.
    CHECK(sample(fixture.driver.evidenceSnapshot(), EvidenceSource::RXFS).raw == 0x10FFU);
    CHECK(sample(fixture.driver.evidenceSnapshot(), EvidenceSource::MISR2).raw == 0x5AA5U);
    CHECK(fixture.driver.evidenceSnapshot().configurationDirty);
  }
  {
    BoundDriver fixture;
    fixture.bus.releaseFailureCall = 2;
    CHECK(fixture.driver.modifyWakeControl(0, 0, true, BUDGET).is(Err::MDIO_ERROR));
    CHECK(fixture.bus.launched == 9U && fixture.bus.calls == 2U && fixture.bus.pendingWake);
    CHECK(sample(fixture.driver.evidenceSnapshot(), EvidenceSource::RXFS).valid);
    CHECK(!fixture.driver.evidenceSnapshot().configurationDirty);
  }
}

void admission() {
  BoundDriver fixture;
  auto output = sentinel();
  for (uint32_t mask : {0U, 0x0800U, 0x80000000U, 0xFFFFFFFFU}) {
    CHECK(fixture.driver.acquireEvidence(mask, output, BUDGET).is(Err::INVALID_PARAM));
  }
  CHECK(fixture.driver.acquireEvidence(0x07FF, output, {10000, 13}).is(Err::OPERATION_TIMEOUT));
  for (uint32_t address : {0U, 0x10U, 0x1FU, 0x10012U}) {
    CHECK(fixture.driver.modifyInterruptEnables(address, 1, 1, BUDGET).is(Err::INVALID_PARAM));
  }
  for (uint32_t mask : {0U, 0x0100U, 0xFFFFU, 0x10000U}) {
    CHECK(fixture.driver.modifyInterruptEnables(0x12, mask, 0, BUDGET).is(Err::INVALID_PARAM));
  }
  CHECK(fixture.driver.modifyInterruptEnables(0x12, 1, 2, BUDGET).is(Err::INVALID_PARAM));
  CHECK(fixture.driver.modifyInterruptEnables(0x12, 1, 0x10000, BUDGET).is(Err::INVALID_PARAM));
  for (uint32_t mask : {0U, 0x0002U, 0x0010U, 0x0040U, 0x0200U, 0x0400U, 0x0800U, 0x2000U, 0x8000U, 0x10000U}) {
    CHECK(fixture.driver.modifyWakeControl(mask, 0, false, BUDGET).is(Err::INVALID_PARAM));
  }
  CHECK(fixture.driver.modifyWakeControl(0x0100, 0x0080, false, BUDGET).is(Err::INVALID_PARAM));
  CHECK(fixture.driver.modifyWakeControl(0x0100, 0x10000, true, BUDGET).is(Err::INVALID_PARAM));
  CHECK(fixture.driver.modifyWakeControl(0x0100, 0, false, {10000, 13}).is(Err::OPERATION_TIMEOUT));
  CHECK(fixture.driver.invalidateDeviceState(static_cast<InvalidationReason>(255)).is(Err::INVALID_PARAM));
  CHECK(fixture.bus.clocks == 0U && fixture.bus.calls == 0U);
  checkSentinel(output);
  for (uint16_t address : SOURCE_ADDRESSES) {
    uint16_t value = 0xCAFE;
    if (address < 32U) { CHECK(fixture.driver.readClause22(address, value, BUDGET).is(Err::INVALID_PARAM)); }
    CHECK(fixture.driver.readMmd(31, address, value, BUDGET).is(Err::INVALID_PARAM));
    CHECK(fixture.driver.modifyMmd(31, address, 1, 1, BUDGET).is(Err::INVALID_PARAM));
    CHECK(value == 0xCAFE);
  }
  for (uint32_t address : {0x0DU, 0x0EU, 0x1EU, 0x0122U, 0x0123U, 0x0130U, 0x0428U}) {
    uint16_t value = 0xCAFE;
    CHECK(fixture.driver.readMmd(31, address, value, BUDGET).is(Err::INVALID_PARAM));
    CHECK(value == 0xCAFE);
  }
  // Named field masks do not widen maintenance access to later LED/GPIO owners.
  for (uint32_t address : {0x0025U, 0x0303U, 0x0304U, 0x0305U, 0x0306U, 0x0460U, 0x0469U}) {
    CHECK(fixture.driver.writeMmd(31, address, 0, BUDGET).is(Err::INVALID_PARAM));
    CHECK(fixture.driver.modifyMmd(31, address, 0x0400, 0, BUDGET).is(Err::INVALID_PARAM));
    CHECK(fixture.driver.modifyMmd(31, address, 0x0077, 0x0041, BUDGET).is(Err::INVALID_PARAM));
  }
  CHECK(fixture.bus.clocks == 0U && fixture.bus.calls == 0U);
  Driver unbound;
  CHECK(unbound.acquireEvidence(1, output, BUDGET).is(Err::NOT_BOUND));
  CHECK(unbound.modifyInterruptEnables(0x12, 1, 1, BUDGET).is(Err::NOT_BOUND));
  CHECK(unbound.modifyWakeControl(0x0100, 0, false, BUDGET).is(Err::NOT_BOUND));
  CHECK(fixture.driver.acquireEvidence(1, output, {100, 14}).is(Err::OPERATION_TIMEOUT));
  CHECK(fixture.bus.calls == 0U);
}

void counters() {
  BoundDriver fixture;
  auto output = sentinel();
  fixture.bus.words[0x14] = 0xAB03;
  fixture.bus.words[0x15] = 0x8000;
  CHECK(fixture.driver.acquireEvidence(0x00C0, output, BUDGET).ok());
  fixture.bus.words[0x14] = 0xCD02;
  fixture.bus.words[0x15] = 0x1234;
  CHECK(fixture.driver.acquireEvidence(0x00C0, output, BUDGET).ok());
  CHECK(sample(output, EvidenceSource::FCSCR).raw == 0xCD02U);
  CHECK(sample(output, EvidenceSource::FCSCR).sampledSum == 5U);
  CHECK(sample(output, EvidenceSource::RECR).raw == 0x1234U);
  CHECK(sample(output, EvidenceSource::RECR).sampledSum == 0x9234U);
  CHECK(!sample(output, EvidenceSource::FCSCR).sumSaturated);
  CHECK(!sample(output, EvidenceSource::RECR).sumSaturated);
  CHECK(sample(output, EvidenceSource::FCSCR).samples == 2U);
  CHECK(sample(output, EvidenceSource::RECR).samples == 2U);
  fixture.bus.words[0x14] = 0x00FF;
  CHECK(fixture.driver.acquireEvidence(0x0040, output, BUDGET).ok());
  CHECK(sample(output, EvidenceSource::FCSCR).sampledSum == 260U);
  // This is retained arithmetic, not an assertion of RECR width or qualified totals.
}

void ownership() {
  for (unsigned operation : {0U, 1U, 2U}) {
    BoundDriver fixture;
    Driver competitor;
    CHECK(competitor.bind(fixture.bus.config()).ok());
    fixture.bus.competitor = &competitor;
    fixture.bus.reenter = true;
    auto output = sentinel();
    if (operation == 0U) { CHECK(fixture.driver.acquireEvidence(0x0019, output, BUDGET).ok()); }
    else if (operation == 1U) { CHECK(fixture.driver.modifyInterruptEnables(0x12, 0x24, 4, BUDGET).ok()); }
    else { CHECK(fixture.driver.modifyWakeControl(0x0600, 0x0400, false, BUDGET).ok()); }
    CHECK(!fixture.bus.locked && fixture.bus.acquired == fixture.bus.released);
    CHECK(fixture.bus.acquired == (operation == 2U ? 3U : 1U));
    CHECK(competitor.transportSnapshot().completedFrames == 0U);
  }
  BoundDriver clock;
  clock.bus.clockReenter = true;
  auto output = sentinel();
  CHECK(clock.driver.acquireEvidence(0x0019, output, BUDGET).ok());
  CHECK(clock.bus.calls == 1U && clock.bus.clocks == 2U);
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"compositions", compositions}, {"acquisition", acquisition},
    {"failures", failures}, {"reports", reports}, {"interrupts", interrupts},
    {"wake", wake}, {"admission", admission}, {"counters", counters},
    {"ownership", ownership}
  };
  return test::runSuites(argc, argv, suites);
}
