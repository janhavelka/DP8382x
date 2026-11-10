#include "support/Check.h"
#include <DP8382x/DP8382x.h>
#include "../examples/portable/Events.h"
#include "../src/DP8382xEvidence.h"

#include <array>
#include <cstdint>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
constexpr IoBudget BUDGET{100000, 6};
constexpr std::array<uint8_t, 6> PLAN{16, 15, 18, 19, 20, 21};
constexpr std::array<EvidenceSource, 6> SOURCES{
    EvidenceSource::PHYSTS, EvidenceSource::FLDS, EvidenceSource::MISR1,
    EvidenceSource::MISR2, EvidenceSource::FCSCR, EvidenceSource::RECR};

// Independent literal controller, SNLS647I tables 8-33/34/36..39. Reads
// consume silicon history, including the failed read when effectPossible is
// set. This model does not resolve the RECR width or measure physical events.
struct Controller {
  std::array<uint16_t, 32> words{};
  std::array<uint8_t, 256> trace{};
  uint64_t now{100};
  unsigned frames{0}, calls{0}, clocks{0}, locks{0}, releases{0}, exclusions{0};
  int failFrame{-1}, badStamp{-1};
  bool effectPossible{true}, stamps{true}, late{false}, regress{false};
  bool releaseFail{false}, locked{false}, newEvents{false}, clockReentry{false};
  uint16_t selector{0}, address{0}, rxfs{0};
  Driver* owner{nullptr};
  Driver* competitor{nullptr};

  Controller() {
    words[16] = 0x3FE7; words[15] = 0xA5F5;
    words[18] = 0xA55A; words[19] = 0x5AA5;
    words[20] = 0xAB05; words[21] = 0x8123;
  }
  Config config(uint32_t phy = 1) noexcept {
    Config value;
    value.variant = Variant::DP83826I;
    value.interface = MacInterface::RMII;
    value.phyAddress = phy;
    value.transport = {this, execute, clock, {8, 4, 1}};
    return value;
  }
  static uint64_t clock(void* context) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    ++bus.clocks;
    // A competing instance may timestamp its denied admission while this
    // callback owns the controller. The acquiring instance returns unlocked.
    if (bus.competitor == nullptr) CHECK(!bus.locked);
    if (bus.clockReentry && bus.owner) CHECK(bus.owner->clearEvidence().is(Err::BUSY));
    return bus.now;
  }
  uint16_t read(uint8_t reg) noexcept {
    if (reg == 14 && selector == 0x401F) {
      if (address == 0x04A1) {
        const auto value = rxfs;
        rxfs &= 0xFF00;
        return value;
      }
      return address == 0x0467 ? 0x1234 : 0;
    }
    const auto value = words[reg];
    if (reg == 16) words[16] &= 0xF9FF;
    if (reg == 15) words[15] &= 0xFE0F;
    if (reg == 18) { words[18] &= 0x00FF; words[16] &= 0xFF7F; }
    if (reg == 19) words[19] &= 0x00FF;
    if (reg == 20) { words[20] &= 0xFF00; words[16] &= 0xF7FF; }
    if (reg == 21) { words[21] = 0; words[16] &= 0xDFFF; }
    if (reg == 6) words[6] &= 0xFFFD;
    if (reg == 26) words[26] &= 0xFFEF;
    return value;
  }
  static BatchResult execute(void* context, const MdioBatch& batch,
                             const IoBudget& budget) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    ++bus.calls;
    BatchResult result;
    result.releaseStatus = Status::Ok(); result.controllerReusable = true;
    if (bus.locked) {
      result.status = {Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), "Shared owner"};
      return result;
    }
    bus.locked = true; ++bus.locks;
    result.status = Status::Ok();
    CHECK(batch.frameCount <= budget.maxFrames && batch.frameCount <= 8U);
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      auto& receipt = result.frames[i];
      const auto& frame = batch.frames[i];
      const auto ordinal = bus.frames++;
      CHECK(ordinal < bus.trace.size());
      bus.trace[ordinal] = frame.registerAddress;
      receipt.attempted = true;
      const bool failed = static_cast<int>(ordinal) == bus.failFrame;
      uint16_t value = 0;
      if (!failed || bus.effectPossible) {
        if (frame.kind == FrameKind::READ) value = bus.read(frame.registerAddress);
        else if (frame.registerAddress == 13) bus.selector = frame.value;
        else if (frame.registerAddress == 14 && bus.selector == 0x001F) bus.address = frame.value;
      }
      bus.now += 2;
      if (bus.newEvents && frame.registerAddress == 18) {
        bus.words[18] |= 0x4000; // Arrives AFTER its own source was consumed.
        bus.words[19] |= 0x8000; // Arrives BEFORE its source is consumed.
      }
      if (bus.owner) {
        const auto before = bus.calls;
        CounterSnapshot sentinel; sentinel.rawRecr = 0xCAFE;
        CHECK(bus.owner->collectEvents(sentinel, BUDGET).is(Err::BUSY));
        CHECK(sentinel.rawRecr == 0xCAFE);
        CHECK(bus.owner->clearEvidence().is(Err::BUSY));
        CHECK(bus.calls == before);
      }
      if (bus.competitor) {
        uint16_t sentinel = 0xCAFE;
        CHECK(bus.competitor->readMmd(31, 0x0467, sentinel, {100000, 4}).is(Err::BUSY));
        CHECK(sentinel == 0xCAFE);
        ++bus.exclusions;
      }
      if (failed) {
        receipt.effectPossible = bus.effectPossible;
        result.status = {Err::MDIO_TIMEOUT, -20020, "Consumed data unavailable"};
        break;
      }
      receipt.completed = true; receipt.valueValid = frame.kind == FrameKind::READ;
      receipt.value = receipt.valueValid ? value : 0;
      receipt.timeValid = bus.stamps;
      receipt.completedAtUs = bus.stamps ? bus.now : 0;
      if (static_cast<int>(ordinal) == bus.badStamp) receipt.completedAtUs = 1;
    }
    bus.locked = false; ++bus.releases;
    if (bus.late) bus.now = budget.deadlineUs;
    if (bus.regress) bus.now = 0;
    if (bus.releaseFail) {
      result.releaseStatus = {Err::MDIO_ERROR, -20021, "Release failed"};
      result.controllerReusable = false;
    }
    return result;
  }
};
struct Fixture {
  Controller bus;
  Driver driver;
  Fixture() { CHECK(driver.bind(bus.config()).ok()); }
};
const EvidenceSample& sample(const EvidenceSnapshot& snapshot, EvidenceSource source) {
  return snapshot.samples[static_cast<uint8_t>(source)];
}
CounterSnapshot sentinel() {
  CounterSnapshot value; value.rawRecr = 0xCAFE; value.falseCarrierTotal = 123456;
  return value;
}
void unchanged(const CounterSnapshot& value) {
  CHECK(value.rawRecr == 0xCAFE && value.falseCarrierTotal == 123456U && !value.valid);
}

void vectors() {
  Fixture fixture; CounterSnapshot output;
  CHECK(fixture.driver.collectEvents(output, BUDGET).ok());
  CHECK(fixture.bus.frames == 6U && fixture.bus.calls == 1U);
  CHECK(fixture.bus.locks == 1U && fixture.bus.releases == 1U && !fixture.bus.locked);
  for (unsigned i = 0; i < 6U; ++i) CHECK(fixture.bus.trace[i] == PLAN[i]);
  CHECK(output.valid && !output.stale && output.timeValid && output.lastStatus.ok());
  CHECK(output.rawPhysts == 0x3FE7 && output.rawFlds == 0xA5F5);
  CHECK(output.rawMisr1 == 0xA55A && output.rawMisr2 == 0x5AA5);
  CHECK(output.rawFcscr == 0xAB05 && output.rawRecr == 0x8123);
  CHECK(output.misr1Events == 0xA500 && output.misr2Events == 0x5A00 && output.fldCauses == 0x01F0);
  CHECK(output.falseCarrierTotal == 5U && !output.falseCarrierLowerBound && !output.falseCarrierOverflow);
  for (const uint8_t reg : {uint8_t{18}, uint8_t{19}, uint8_t{15}}) {
    const unsigned first = reg == 15 ? 4U : 8U;
    const unsigned last = reg == 15 ? 8U : 15U;
    for (unsigned bit = first; bit <= last; ++bit) {
      Fixture one; one.bus.words.fill(0);
      one.bus.words[reg] = static_cast<uint16_t>(1U << bit);
      CHECK(one.driver.collectEvents(output, BUDGET).ok());
      CHECK(output.misr1Events == (reg == 18 ? (1U << bit) : 0U));
      CHECK(output.misr2Events == (reg == 19 ? (1U << bit) : 0U));
      CHECK(output.fldCauses == (reg == 15 ? (1U << bit) : 0U));
      const auto source = reg == 18 ? EvidenceSource::MISR1 : reg == 19 ? EvidenceSource::MISR2 : EvidenceSource::FLDS;
      CHECK(sample(one.driver.evidenceSnapshot(), source).coalesced);
    }
  }
  for (const uint16_t count : {uint16_t{0}, uint16_t{1}, uint16_t{0x7F}, uint16_t{0x80}, uint16_t{0xFE}, uint16_t{0xFF}}) {
    Fixture one; one.bus.words[20] = static_cast<uint16_t>(0xAB00U | count);
    CHECK(one.driver.collectEvents(output, BUDGET).ok());
    CHECK(output.falseCarrierTotal == count && output.falseCarrierLowerBound == (count == 0xFF));
    CHECK(!output.falseCarrierOverflow);
    CHECK(sample(one.driver.evidenceSnapshot(), EvidenceSource::FCSCR).counterSaturated == (count == 0xFF));
    CHECK(sample(one.driver.evidenceSnapshot(), EvidenceSource::RECR).sampledSum == 0U);
    CHECK(one.driver.collectEvents(output, BUDGET).ok());
    CHECK(output.falseCarrierTotal == count && output.rawFcscr == 0xAB00);
    CHECK(output.falseCarrierLowerBound == (count == 0xFF));
  }
  // Other consumers share the same source-specific retention implementation.
  // Only sourced event bits become eventBits; unresolved RCSR remains raw.
  Fixture sources; EvidenceSnapshot captured;
  sources.bus.words[6] = 0xFFFF; sources.bus.words[26] = 0xFFFF;
  sources.bus.words[23] = 0xFFFF; sources.bus.rxfs = 0xFFFF;
  const uint32_t selected = evidenceBit(EvidenceSource::ANER) | evidenceBit(EvidenceSource::BTSCR) |
      evidenceBit(EvidenceSource::RCSR) | evidenceBit(EvidenceSource::RXFS);
  CHECK(sources.driver.acquireEvidence(selected, captured, {100000, 7}).ok());
  CHECK(sources.bus.frames == 7U && sources.bus.calls == 1U);
  CHECK(sample(captured, EvidenceSource::ANER).eventBits == 0x0002);
  CHECK(sample(captured, EvidenceSource::BTSCR).eventBits == 0x0010);
  CHECK(sample(captured, EvidenceSource::RXFS).eventBits == 0x00E1);
  CHECK(sample(captured, EvidenceSource::RCSR).raw == 0xFFFF);
  CHECK(sample(captured, EvidenceSource::RCSR).eventBits == 0 && !sample(captured, EvidenceSource::RCSR).coalesced);
  for (const uint16_t bit : {uint16_t{0x0001}, uint16_t{0x0020}, uint16_t{0x0040}, uint16_t{0x0080}}) {
    CHECK(sources.driver.clearEvidence().ok()); sources.bus.rxfs = bit;
    CHECK(sources.driver.acquireEvidence(evidenceBit(EvidenceSource::RXFS), captured, {100000, 4}).ok());
    CHECK(sample(captured, EvidenceSource::RXFS).eventBits == bit);
  }
}

void failures() {
  for (unsigned phase = 0; phase < 6U; ++phase) {
    for (const bool possible : {false, true}) {
      Fixture fixture;
      fixture.bus.failFrame = static_cast<int>(phase); fixture.bus.effectPossible = possible;
      auto output = sentinel();
      const auto status = fixture.driver.collectEvents(output, BUDGET);
      CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail == -20020); unchanged(output);
      CHECK(fixture.bus.frames == phase + 1U && fixture.bus.calls == 1U && !fixture.bus.locked);
      const auto evidence = fixture.driver.evidenceSnapshot();
      for (unsigned i = 0; i < 6U; ++i) {
        const auto& retained = sample(evidence, SOURCES[i]);
        CHECK(retained.valid == (i < phase));
        CHECK(retained.stale == (i >= phase));
        CHECK(retained.historyLost == (i == phase && possible));
        if (i < phase) CHECK(retained.timeValid && retained.observedAtUs == 102U + i * 2U);
      }
      CHECK(!fixture.driver.counterSnapshot().valid && fixture.driver.counterSnapshot().stale);
      fixture.bus.failFrame = -1;
      CHECK(fixture.driver.collectEvents(output, BUDGET).ok());
      CHECK(sample(fixture.driver.evidenceSnapshot(), SOURCES[phase]).historyLost == possible);
      if (phase == 4 && possible) CHECK(output.falseCarrierLowerBound && output.falseCarrierTotal == 0U);
      if (phase == 5) CHECK(output.falseCarrierTotal == 5U); // FCSCR survived the failed batch.
    }
  }
  Fixture previous; CounterSnapshot old;
  CHECK(previous.driver.collectEvents(old, BUDGET).ok());
  previous.bus.words[18] = 0x4000; previous.bus.words[19] = 0x8000;
  previous.bus.failFrame = static_cast<int>(previous.bus.frames + 3U);
  auto output = sentinel();
  CHECK(previous.driver.collectEvents(output, BUDGET).is(Err::MDIO_TIMEOUT)); unchanged(output);
  const auto cached = previous.driver.counterSnapshot();
  CHECK(cached.valid && cached.stale && cached.rawMisr1 == old.rawMisr1 && cached.observedAtUs == old.observedAtUs);
  CHECK(sample(previous.driver.evidenceSnapshot(), EvidenceSource::MISR1).raw == 0x4000);
  CHECK(sample(previous.driver.evidenceSnapshot(), EvidenceSource::MISR2).historyLost);
  Fixture release; release.bus.releaseFail = true;
  CHECK(release.driver.collectEvents(output, BUDGET).is(Err::MDIO_ERROR)); unchanged(output);
  CHECK(sample(release.driver.evidenceSnapshot(), EvidenceSource::RECR).valid);
  CHECK(release.driver.transportSnapshot().lastBatch.releaseStatus.detail == -20021);
  Fixture primary; primary.bus.releaseFail = true; primary.bus.failFrame = 3;
  CHECK(primary.driver.collectEvents(output, BUDGET).is(Err::MDIO_TIMEOUT)); unchanged(output);
  CHECK(primary.driver.transportSnapshot().lastBatch.releaseStatus.detail == -20021);
  for (unsigned allowed = 0; allowed < 6U; ++allowed) {
    Fixture shortBudget;
    CHECK(shortBudget.driver.collectEvents(output, {100000, allowed}).is(Err::OPERATION_TIMEOUT));
    unchanged(output); CHECK(shortBudget.bus.frames == 0U && shortBudget.bus.calls == 0U);
  }
}

void history() {
  // Directly exercise the same private arithmetic used by the collector. No
  // driver state injection, fake counter width or billions of acquisitions.
  EvidenceSample boundary;
  boundary.samples = 0xFFFFFFFEU;
  detail::retainSampleCount(boundary);
  CHECK(boundary.samples == 0xFFFFFFFFU && !boundary.samplesSaturated);
  detail::retainSampleCount(boundary);
  CHECK(boundary.samples == 0xFFFFFFFFU && boundary.samplesSaturated);
  boundary.sampledSum = UINT64_C(0xFFFFFFFFFFFFFFFE);
  detail::retainFalseCarriers(boundary, 1);
  CHECK(boundary.sampledSum == UINT64_C(0xFFFFFFFFFFFFFFFF) && !boundary.sumSaturated);
  detail::retainFalseCarriers(boundary, 0);
  CHECK(!boundary.sumSaturated && !boundary.counterLowerBound);
  detail::retainFalseCarriers(boundary, 1);
  CHECK(boundary.sampledSum == UINT64_C(0xFFFFFFFFFFFFFFFF) && boundary.sumSaturated);
  CHECK(boundary.counterLowerBound && !boundary.counterSaturated);
  detail::retainFalseCarriers(boundary, 0);
  CHECK(boundary.counterLowerBound && boundary.sumSaturated);
  Fixture fixture; fixture.bus.words.fill(0); fixture.bus.words[18] = 0x0101;
  fixture.bus.newEvents = true; CounterSnapshot output;
  CHECK(fixture.driver.collectEvents(output, BUDGET).ok());
  CHECK(output.rawMisr1 == 0x0101 && output.rawMisr2 == 0x8000);
  CHECK(output.misr1Events == 0x0100);
  fixture.bus.newEvents = false;
  CHECK(fixture.driver.collectEvents(output, BUDGET).ok());
  CHECK(output.rawMisr1 == 0x4001 && output.misr1Events == 0x4000 && output.misr2Events == 0);
  const auto evidence = fixture.driver.evidenceSnapshot();
  CHECK(sample(evidence, EvidenceSource::MISR1).samples == 2U);
  CHECK(sample(evidence, EvidenceSource::MISR1).eventBits == 0x4100);
  CHECK(!sample(evidence, EvidenceSource::MISR1).samplesSaturated);
  const auto frames = fixture.bus.frames, clocks = fixture.bus.clocks;
  const auto transport = fixture.driver.transportSnapshot();
  const auto link = fixture.driver.linkSnapshot();
  const auto settings = fixture.driver.settingsSnapshot();
  char first[256]{}, second[256]{};
  CHECK(formatCachedEvents(fixture.driver.counterSnapshot(), first, sizeof(first)));
  CHECK(formatCachedEvents(fixture.driver.counterSnapshot(), second, sizeof(second)));
  CHECK(std::strcmp(first, second) == 0);
  char shortBuffer[2]{};
  CHECK(!formatCachedEvents(output, shortBuffer, sizeof(shortBuffer)));
  CHECK(!formatCachedEvents(output, nullptr, 0));
  fixture.bus.words[18] |= 0x2000;
  CHECK(fixture.driver.clearEvidence().ok());
  CHECK(fixture.bus.frames == frames && fixture.bus.clocks == clocks && fixture.bus.words[18] == 0x2001);
  CHECK(!fixture.driver.counterSnapshot().valid && fixture.driver.counterSnapshot().stale);
  CHECK(!sample(fixture.driver.evidenceSnapshot(), EvidenceSource::MISR1).valid);
  CHECK(fixture.driver.transportSnapshot().completedFrames == transport.completedFrames);
  CHECK(fixture.driver.linkSnapshot().change == link.change);
  CHECK(fixture.driver.settingsSnapshot().trust == settings.trust);
  CHECK(fixture.driver.collectEvents(output, BUDGET).ok() && output.misr1Events == 0x2000);
  EvidenceSnapshot raw;
  CHECK(fixture.driver.acquireEvidence(evidenceBit(EvidenceSource::FCSCR), raw, {100000, 1}).ok());
  CHECK(fixture.driver.counterSnapshot().valid && fixture.driver.counterSnapshot().stale);
  CHECK(fixture.driver.counterSnapshot().rawMisr1 == output.rawMisr1);
  CHECK(fixture.driver.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
  CHECK(fixture.driver.counterSnapshot().stale);
  CHECK(sample(fixture.driver.evidenceSnapshot(), EvidenceSource::FCSCR).discontinuous);
  CHECK(fixture.driver.collectEvents(output, BUDGET).ok() && output.falseCarrierLowerBound);
  CHECK(fixture.driver.evidenceSnapshot().configurationDirty);
  CHECK(fixture.driver.clearEvidence().ok());
  CHECK(fixture.driver.evidenceSnapshot().configurationDirty);
  CHECK(!sample(fixture.driver.evidenceSnapshot(), EvidenceSource::FCSCR).discontinuous);
  CHECK(fixture.driver.end().ok() && !fixture.driver.counterSnapshot().valid);
  CHECK(fixture.driver.bind(fixture.bus.config()).ok());
  CHECK(!sample(fixture.driver.evidenceSnapshot(), EvidenceSource::FCSCR).discontinuous);
}

void ownership() {
  Driver unbound; auto untouched = sentinel();
  CHECK(unbound.clearEvidence().is(Err::NOT_BOUND));
  CHECK(unbound.collectEvents(untouched, BUDGET).is(Err::NOT_BOUND)); unchanged(untouched);
  Fixture fixture; Driver competitor;
  CHECK(competitor.bind(fixture.bus.config(2)).ok());
  fixture.bus.owner = &fixture.driver; fixture.bus.competitor = &competitor;
  fixture.bus.clockReentry = true; CounterSnapshot output;
  CHECK(sampleEvents(fixture.driver, BUDGET, output).ok());
  CHECK(fixture.bus.frames == 6U && fixture.bus.exclusions == 6U && fixture.bus.locks == 1U);
  fixture.bus.owner = nullptr; fixture.bus.competitor = nullptr; fixture.bus.clockReentry = false;
  uint16_t straps = 0;
  CHECK(competitor.readMmd(31, 0x0467, straps, {100000, 4}).ok() && straps == 0x1234);
  CHECK(fixture.bus.frames == 10U && fixture.bus.locks == 2U && fixture.bus.releases == 2U);
  OperationToken token;
  CHECK(fixture.driver.startInitialize({ResetKind::NONE}, {{100000, 64}, 1000, 20}, token).inProgress());
  const auto frames = fixture.bus.frames, clocks = fixture.bus.clocks;
  CHECK(fixture.driver.clearEvidence().is(Err::BUSY));
  CHECK(fixture.bus.frames == frames && fixture.bus.clocks == clocks);
  CHECK(fixture.driver.cancel(token).ok());
  const auto operation = fixture.driver.operationSnapshot();
  CHECK(fixture.driver.clearEvidence().ok());
  CHECK(fixture.driver.operationSnapshot().state == operation.state);
  OperationResult result;
  CHECK(fixture.driver.takeResult(token, result).ok() && result.status.is(Err::CANCELLED));
}

void clocks() {
  for (const bool stamps : {false, true}) {
    Fixture fixture; fixture.bus.stamps = stamps; CounterSnapshot output;
    CHECK(fixture.driver.collectEvents(output, BUDGET).ok());
    const auto evidence = fixture.driver.evidenceSnapshot();
    for (unsigned i = 0; i < 6U; ++i) {
      CHECK(sample(evidence, SOURCES[i]).timeValid);
      if (stamps) CHECK(sample(evidence, SOURCES[i]).observedAtUs == 102U + i * 2U);
      else CHECK(sample(evidence, SOURCES[i]).observedAtUs >= 112U);
    }
    CHECK(output.timeValid && output.observedAtUs >= 112U);
  }
  Fixture late; late.bus.late = true; auto output = sentinel();
  CHECK(late.driver.collectEvents(output, BUDGET).is(Err::OPERATION_TIMEOUT)); unchanged(output);
  CHECK(late.bus.frames == 6U && sample(late.driver.evidenceSnapshot(), EvidenceSource::RECR).valid);
  Fixture regressed; regressed.bus.regress = true;
  CHECK(!regressed.driver.collectEvents(output, BUDGET).ok()); unchanged(output);
  for (const auto source : SOURCES) {
    CHECK(sample(regressed.driver.evidenceSnapshot(), source).valid);
    CHECK(!sample(regressed.driver.evidenceSnapshot(), source).timeValid);
  }
  Fixture partial; partial.bus.badStamp = 3;
  CHECK(partial.driver.collectEvents(output, BUDGET).is(Err::MDIO_ERROR)); unchanged(output);
  const auto evidence = partial.driver.evidenceSnapshot();
  CHECK(sample(evidence, EvidenceSource::MISR1).valid && sample(evidence, EvidenceSource::MISR1).timeValid);
  CHECK(sample(evidence, EvidenceSource::MISR2).valid && !sample(evidence, EvidenceSource::MISR2).timeValid);
  Fixture unrelated; CounterSnapshot old;
  CHECK(unrelated.driver.collectEvents(old, BUDGET).ok());
  unrelated.bus.regress = true;
  uint16_t word = 0xCAFE;
  CHECK(!unrelated.driver.readClause22(2, word, {100000, 1}).ok());
  CHECK(word == 0xCAFE && unrelated.driver.counterSnapshot().valid && unrelated.driver.counterSnapshot().stale);
  CHECK(unrelated.driver.counterSnapshot().observedAtUs == old.observedAtUs);
  // Past local acquisition time remains meaningful as history; its stale flag
  // prevents using it as a current observation after another API breaks time.
  CHECK(unrelated.driver.counterSnapshot().timeValid == old.timeValid);
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"vectors", vectors}, {"failures", failures}, {"history", history},
    {"ownership", ownership}, {"clocks", clocks}
  };
  return test::runSuites(argc, argv, suites);
}
