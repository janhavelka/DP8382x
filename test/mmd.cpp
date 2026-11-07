#include "support/FakeController.h"
#include <DP8382x/DP8382x.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
constexpr IoBudget BUDGET{10000, 5};
constexpr uint16_t SENTINEL = 0xA55A;

using test::ReportFault;
struct Controller : test::FakeController {
  Controller() noexcept : FakeController(true) {}
};

void compete(test::FakeController& bus, const IoBudget&) noexcept {
  uint16_t value = SENTINEL;
  const unsigned before = bus.callbacks;
  CHECK(bus.active->end().is(Err::BUSY));
  CHECK(bus.active->readMmd(31, 2, value, BUDGET).is(Err::BUSY));
  CHECK(bus.callbacks == before);
  CHECK(bus.competing->readMmd(31, 2, value, BUDGET).is(Err::BUSY));
  CHECK(bus.competing->writeMmd(31, 0x18, 0, BUDGET).is(Err::BUSY));
  CHECK(bus.competing->modifyMmd(31, 0x18, 2, 2, BUDGET).is(Err::BUSY));
  CHECK(bus.competing->readClause22(2, value, BUDGET).is(Err::BUSY));
  for (uint32_t reg : {0x0DU, 0x0EU}) {
    CHECK(bus.competing->writeClause22(reg, 0, BUDGET).is(Err::INVALID_PARAM));
    CHECK(bus.competing->writeMmd(31, reg, 0, BUDGET).is(Err::INVALID_PARAM));
  }
  CHECK(bus.callbacks == before + 4U && value == SENTINEL);
}

void expectTrace(const Controller& bus, uint16_t address, bool write, uint16_t word, bool rmw = false) {
  CHECK(bus.traceCount == (rmw ? 5U : 4U));
  CHECK(bus.trace[0].kind == FrameKind::WRITE && bus.trace[0].reg == 0x0DU && bus.trace[0].value == 0x001FU);
  CHECK(bus.trace[1].kind == FrameKind::WRITE && bus.trace[1].reg == 0x0EU && bus.trace[1].value == address);
  CHECK(bus.trace[2].kind == FrameKind::WRITE && bus.trace[2].reg == 0x0DU && bus.trace[2].value == 0x401FU);
  CHECK(bus.trace[3].reg == 0x0EU && bus.trace[3].kind == (write ? FrameKind::WRITE : FrameKind::READ));
  if (write) CHECK(bus.trace[3].value == word);
  if (rmw) CHECK(bus.trace[4].kind == FrameKind::WRITE && bus.trace[4].reg == 0x0EU && bus.trace[4].value == word);
  for (unsigned i : {0U, 2U}) CHECK((bus.trace[i].value & 0x3FE0U) == 0U);
}

void testTraces() {
  for (uint32_t phy : {0U, 31U}) {
    Controller bus; Driver driver; CHECK(driver.bind(bus.config(phy)).ok());
    bus.selector[phy] = 0xFFFF; bus.address[phy] = 0xFFFF; bus.led[phy] = 0xFFFF;
    uint16_t word = SENTINEL;
    CHECK(driver.readMmd(31, 2, word, BUDGET).ok());
    CHECK(word == 0x2000U); expectTrace(bus, 2, false, 0);
    CHECK(bus.lastBudget.deadlineUs == 10000U && bus.lastBudget.maxFrames == 5U);
    CHECK(driver.writeMmd(31, 0x18, 0x0692, BUDGET).ok());
    expectTrace(bus, 0x18, true, 0x0692);
    CHECK(bus.led[phy] == 0x0692U);
    bus.led[phy] = 0xFFFF;
    CHECK(driver.modifyMmd(31, 0x18, 0x0600, 0x0200, BUDGET).ok());
    expectTrace(bus, 0x18, false, 0x0292, true);
    CHECK(bus.led[phy] == 0x0292U);
    CHECK(bus.lastRequest.frames[4].readIndex == 3U && bus.lastRequest.frames[4].preserveMask == 0x0092U);
    CHECK(bus.lastRequest.frames[4].value == 0x0200U);
    CHECK(bus.lastBudget.deadlineUs == 10000U && bus.lastBudget.maxFrames == 5U);
    const auto snapshot = driver.transportSnapshot();
    CHECK(snapshot.totalBatches == 3U && snapshot.attemptedFrames == 13U && snapshot.completedFrames == 13U);
    CHECK(bus.acquisitions == 3U && bus.releases == 3U && !bus.locked);
    CHECK(driver.end().ok());
  }
  for (const auto tuple : {std::array<uint16_t, 2>{3, 0x0014}, {7, 0x003C}, {7, 0x003D}}) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
    uint16_t word = SENTINEL;
    CHECK(driver.readMmd(tuple[0], tuple[1], word, BUDGET).ok());
    CHECK(bus.traceCount == 4U);
    CHECK(bus.trace[0].reg == 0x0DU && bus.trace[0].value == tuple[0]);
    CHECK(bus.trace[1].reg == 0x0EU && bus.trace[1].value == tuple[1]);
    CHECK(bus.trace[2].reg == 0x0DU && bus.trace[2].value == (0x4000U | tuple[0]));
    CHECK(bus.trace[3].reg == 0x0EU && bus.trace[3].kind == FrameKind::READ);
    CHECK(driver.end().ok());
  }
  // Every legal stable LEDCR mask/value pair, using original table8-42 literals.
  for (uint32_t mask = 1; mask <= 0x0692U; ++mask) {
    if ((mask & ~uint32_t{0x0692}) != 0U) continue;
    for (uint32_t value = 0; value <= mask; ++value) {
      if ((value & ~mask) != 0U) continue;
      Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
      bus.led[1] = 0xFFFF;
      CHECK(driver.modifyMmd(31, 0x18, mask, value, BUDGET).ok());
      CHECK(bus.led[1] == ((0x0692U & ~mask) | value));
      CHECK(bus.launches == 5U && bus.callbacks == 1U);
      CHECK(driver.end().ok());
    }
  }
}

void testAdmission() {
  Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
  auto rejected = [&](uint32_t devad, uint32_t address, uint32_t mask, uint32_t value,
                      unsigned operation, Err expected) {
    const auto calls = bus.callbacks; const auto clocks = bus.clocks;
    uint16_t output = SENTINEL;
    const auto status = operation == 0U ? driver.readMmd(devad, address, output, BUDGET) :
        operation == 1U ? driver.writeMmd(devad, address, value, BUDGET) :
                          driver.modifyMmd(devad, address, mask, value, BUDGET);
    CHECK(status.is(expected)); CHECK(output == SENTINEL);
    if (expected == Err::INVALID_CONFIG) {
      CHECK(status.detail == static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
    } else {
      const ParamDetail detail = devad > 31U || address > 0xFFFFU ? ParamDetail::ADDRESS_RANGE :
          (operation != 0U && value > 0xFFFFU) || (operation == 2U && mask > 0xFFFFU)
              ? ParamDetail::VALUE_WIDTH : ParamDetail::ACCESS_POLICY;
      CHECK(status.detail == static_cast<int32_t>(detail));
    }
    CHECK(bus.callbacks == calls && bus.clocks == clocks);
  };
  for (uint32_t devad = 0; devad <= 32U; ++devad) {
    if (devad == 31U) continue;
    for (unsigned operation = 0; operation < 3U; ++operation)
      rejected(devad, 0x18, 2, 2, operation,
               Err::INVALID_PARAM);
  }
  for (uint32_t value : {0x10000U, 0xFFFFFFFFU}) {
    rejected(31, value, 2, 2, 0, Err::INVALID_PARAM);
    rejected(31, value, 2, 2, 1, Err::INVALID_PARAM);
    rejected(31, 0x18, 2, value, 1, Err::INVALID_PARAM);
    rejected(31, 0x18, value, 2, 2, Err::INVALID_PARAM);
    rejected(31, 0x18, 2, value, 2, Err::INVALID_PARAM);
    rejected(value, 0x18, 2, 2, 0, Err::INVALID_PARAM);
  }
  constexpr uint32_t allowed = 0x9B420FBDU; // Literal nonconsuming C22 alias map.
  for (uint32_t address = 0; address < 32U; ++address) {
    if ((allowed & (uint32_t{1} << address)) != 0U) {
      uint16_t value = SENTINEL;
      CHECK(driver.readMmd(31, address, value, BUDGET).ok());
      CHECK(bus.lastRequest.frameCount == 4U);
    } else rejected(31, address, 2, 2, 0, Err::INVALID_PARAM);
    if (address != 0x18U) {
      rejected(31, address, 2, 2, 1, Err::INVALID_PARAM);
      rejected(31, address, 2, 2, 2, Err::INVALID_PARAM);
    }
  }
  for (uint32_t address : {0x20U, 0x21U, 0x400U, 0xFFFFU})
    rejected(31, address, 2, 2, 1, Err::INVALID_PARAM);
  // Original detailed tables: raw nonconsuming extended words, not decoded
  // feature results or permission to change their companion controls.
  for (uint32_t address : {0x0025U, 0x0027U, 0x002AU, 0x0117U, 0x0131U, 0x0170U,
                          0x0173U, 0x0175U, 0x0176U, 0x0177U, 0x0178U, 0x0180U,
                          0x0185U, 0x018AU, 0x0218U, 0x0302U, 0x0303U, 0x0304U,
                          0x0305U, 0x0306U, 0x0308U, 0x030BU, 0x030CU, 0x030EU,
                          0x040DU, 0x0456U, 0x0460U, 0x0461U, 0x0467U, 0x0468U,
                          0x0469U, 0x04A0U, 0x04A2U, 0x04A3U, 0x04A4U, 0x04A5U,
                          0x04A6U, 0x04A7U}) {
    uint16_t value = SENTINEL;
    CHECK(driver.readMmd(31, address, value, BUDGET).ok());
    CHECK(value == 0x2000U);
    expectTrace(bus, static_cast<uint16_t>(address), false, 0);
    rejected(31, address, 2, 2, 1, Err::INVALID_PARAM);
    rejected(31, address, 2, 2, 2, Err::INVALID_PARAM);
  }
  for (uint32_t address : {0x04D1U, 0x04D3U, 0x04DFU, 0x033EU, 0x033FU, 0x031BU,
                          0x0466U, 0x04CFU, 0x0416U, 0x04F5U, 0x04E0U, 0x031FU, 0x04F4U})
    for (unsigned operation = 0; operation < 3U; ++operation)
      rejected(31, address, 2, 2, operation, Err::INVALID_PARAM);
  for (uint32_t address : {0x20U, 0x21U, 0x172U, 0x174U, 0x17AU, 0x190U, 0x225U,
                          0x30AU, 0x400U, 0x4A1U, 0x8000U, 0xFFFFU})
    rejected(31, address, 2, 2, 0, Err::INVALID_PARAM);
  // All-reserved detailed entries and exceptional recipe addresses are not
  // generic readable/writable registers, irrespective of a plausible name.
  for (uint32_t address : {0x0171U, 0x0181U, 0x0182U, 0x0183U, 0x0184U, 0x0186U,
                          0x0187U, 0x0188U, 0x0189U, 0x0404U, 0x0122U, 0x0123U,
                          0x0130U, 0x0428U})
    for (unsigned operation = 0; operation < 3U; ++operation)
      rejected(31, address, 2, 2, operation, Err::INVALID_PARAM);
  for (uint32_t mask : {0U, 1U, 0xF96DU, 0xFFFFU})
    rejected(31, 0x18, mask, 0, 2, Err::INVALID_PARAM);
  rejected(31, 0x18, 2, 0x10, 2, Err::INVALID_PARAM);
  rejected(31, 0x18, 2, 1, 1, Err::INVALID_PARAM);
  CHECK(driver.end().ok());

  for (bool rmw : {false, true}) {
    const uint32_t frames = rmw ? 5U : 4U;
    for (uint32_t allowance = 0; allowance <= frames; ++allowance) {
      Controller bounded; Driver target; CHECK(target.bind(bounded.config()).ok());
      uint16_t word = SENTINEL;
      const IoBudget budget{1000, allowance};
      const auto status = rmw ? target.modifyMmd(31, 0x18, 2, 2, budget) : target.readMmd(31, 2, word, budget);
      CHECK(status.is(allowance == frames ? Err::OK : Err::OPERATION_TIMEOUT));
      CHECK(bounded.callbacks == (allowance == frames ? 1U : 0U));
      CHECK(bounded.launches == (allowance == frames ? frames : 0U));
      CHECK(target.end().ok());
    }
    // raw100 + lag1 + acquire3 + n*frame4 + release2 + overhead3.
    const uint64_t equality = 109U + uint64_t{frames} * 4U;
    for (uint64_t deadline : {equality, equality + 1U}) {
      Controller bounded; Driver target; CHECK(target.bind(bounded.config()).ok());
      uint16_t word = SENTINEL;
      const IoBudget budget{deadline, frames};
      const auto status = rmw ? target.modifyMmd(31, 0x18, 2, 2, budget) : target.readMmd(31, 2, word, budget);
      CHECK(status.is(deadline == equality ? Err::OPERATION_TIMEOUT : Err::OK));
      CHECK(bounded.callbacks == (deadline == equality ? 0U : 1U));
      CHECK(target.end().ok());
    }
  }
}

void testFailures() {
  for (unsigned operation = 0; operation < 3U; ++operation) {
    const unsigned frames = operation == 2U ? 5U : 4U;
    for (unsigned phase = 0; phase < frames; ++phase) {
      for (bool accepted : {false, true}) {
        Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
        bus.selector[1] = 0x401F; bus.address[1] = 0x0460; bus.led[1] = 0;
        bus.failAt = static_cast<int>(phase); bus.acceptFailedWrite = accepted;
        uint16_t value = SENTINEL;
        const auto status = operation == 0U ? driver.readMmd(31, 0x18, value, BUDGET) :
            operation == 1U ? driver.writeMmd(31, 0x18, 2, BUDGET) : driver.modifyMmd(31, 0x18, 2, 2, BUDGET);
        CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail == -700 - static_cast<int32_t>(phase));
        CHECK(value == SENTINEL && bus.callbacks == 1U && bus.launches == phase + 1U);
        CHECK(!bus.locked && bus.releases == 1U);
        const auto receipt = driver.transportSnapshot();
        CHECK(receipt.attemptedFrames == phase + 1U && receipt.completedFrames == phase);
        CHECK(receipt.lastBatch.frames[phase].effectPossible);
        const bool targetWritten = operation != 0U && phase == frames - 1U && accepted;
        CHECK(bus.led[1] == (targetWritten ? 2U : 0U));
        // A new explicit call always addresses fully, regardless of selector effects.
        bus.failAt = -1; bus.acceptFailedWrite = false;
        CHECK(driver.readMmd(31, 0x18, value, BUDGET).ok());
        expectTrace(bus, 0x18, false, 0);
        CHECK(value == (targetWritten ? 2U : 0U));
        CHECK(receipt.lastBatch.status.is(Err::MDIO_TIMEOUT)); // Copied prior uncertainty remains historical.
        CHECK(bus.callbacks == 2U && bus.acquisitions == 2U && bus.releases == 2U);
        CHECK(driver.transportSnapshot().attemptedFrames == phase + 5U);
        CHECK(driver.end().ok());
      }
    }
    for (unsigned phase = 0; phase < frames; ++phase) {
      Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
      bus.expireAfter = static_cast<int>(phase);
      uint16_t value = SENTINEL;
      const auto status = operation == 0U ? driver.readMmd(31, 0x18, value, BUDGET) :
          operation == 1U ? driver.writeMmd(31, 0x18, 2, BUDGET) : driver.modifyMmd(31, 0x18, 2, 2, BUDGET);
      CHECK(status.is(Err::OPERATION_TIMEOUT));
      CHECK(value == SENTINEL && bus.launches == phase + 1U && bus.callbacks == 1U);
      CHECK(driver.transportSnapshot().attemptedFrames == phase + 1U);
      CHECK(driver.transportSnapshot().completedFrames == phase + 1U);
      CHECK(bus.led[1] == (operation != 0U && phase == frames - 1U ? 2U : 0U));
      CHECK(!bus.locked && bus.releases == 1U);
      CHECK(driver.transportSnapshot().late);
      CHECK(driver.end().ok());
    }
  }
}

void testReports() {
  for (ReportFault fault : {ReportFault::HOLE, ReportFault::CONTINUED_AFTER_FAILURE,
                           ReportFault::MISSING_READ_VALUE, ReportFault::UNUSED_ATTEMPT,
                           ReportFault::WRITE_READ_VALUE, ReportFault::NONTERMINAL_STATUS,
                           ReportFault::UNKNOWN_STATUS, ReportFault::NULL_MESSAGE,
                           ReportFault::REGRESSING_TIMESTAMP, ReportFault::EARLY_TIMESTAMP,
                           ReportFault::TIMESTAMP_WITHOUT_VALIDITY, ReportFault::SHORT_OK}) {
    Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
    bus.reportFault = fault;
    uint16_t value = SENTINEL;
    const auto status = driver.readMmd(31, 2, value, BUDGET);
    CHECK(status.is(Err::MDIO_ERROR)); CHECK(value == SENTINEL);
    const auto receipt = driver.transportSnapshot();
    CHECK(receipt.totalBatches == 1U);
    CHECK(receipt.attemptedFrames <= 4U && receipt.completedFrames <= 4U);
    if (fault == ReportFault::HOLE) CHECK(receipt.attemptedFrames == 1U && receipt.completedFrames == 1U);
    if (fault == ReportFault::CONTINUED_AFTER_FAILURE) CHECK(receipt.attemptedFrames == 2U && receipt.completedFrames == 1U);
    if (fault == ReportFault::MISSING_READ_VALUE) CHECK(receipt.attemptedFrames == 4U && receipt.completedFrames == 3U);
    if (fault == ReportFault::UNUSED_ATTEMPT || fault == ReportFault::NONTERMINAL_STATUS ||
        fault == ReportFault::UNKNOWN_STATUS || fault == ReportFault::NULL_MESSAGE ||
        fault == ReportFault::REGRESSING_TIMESTAMP || fault == ReportFault::EARLY_TIMESTAMP ||
        fault == ReportFault::TIMESTAMP_WITHOUT_VALIDITY)
      CHECK(receipt.attemptedFrames == 4U && receipt.completedFrames == 4U);
    if (fault == ReportFault::WRITE_READ_VALUE) CHECK(receipt.attemptedFrames == 1U && receipt.completedFrames == 0U);
    if (fault == ReportFault::REGRESSING_TIMESTAMP) {
      CHECK(receipt.violation == TransportViolation::FRAME_REPORT);
      CHECK(receipt.lastBatch.frames[0].completedAtUs > receipt.lastBatch.frames[1].completedAtUs);
    }
    if (fault == ReportFault::SHORT_OK) CHECK(receipt.attemptedFrames == 3U && receipt.completedFrames == 3U);
    CHECK(driver.readMmd(31, 2, value, BUDGET).is(Err::BUSY));
    CHECK(bus.callbacks == 1U && !bus.locked);
    CHECK(driver.end().ok());
  }
  // A backend that reports an unusable read then a dependent target write has
  // broken sequencing. Do not count the later frame as a trusted prefix, but
  // retain its original possible/completed hardware effects for owner recovery.
  Controller bus; Driver driver; CHECK(driver.bind(bus.config()).ok());
  bus.reportFault = ReportFault::MISSING_READ_VALUE;
  CHECK(driver.modifyMmd(31, 0x18, 2, 2, BUDGET).is(Err::MDIO_ERROR));
  const auto receipt = driver.transportSnapshot();
  CHECK(receipt.violation == TransportViolation::FRAME_REPORT);
  CHECK(receipt.attemptedFrames == 4U && receipt.completedFrames == 3U);
  CHECK(receipt.lastBatch.frames[3].attempted && !receipt.lastBatch.frames[3].valueValid);
  CHECK(receipt.lastBatch.frames[4].attempted && receipt.lastBatch.frames[4].completed);
  CHECK(bus.led[1] == 2U && bus.launches == 5U);
  uint16_t output = SENTINEL;
  CHECK(driver.readMmd(31, 0x18, output, BUDGET).is(Err::BUSY));
  CHECK(output == SENTINEL && bus.callbacks == 1U);
  CHECK(driver.end().ok());
}

void testOwnership() {
  for (unsigned operation = 0; operation < 3U; ++operation) {
    Controller bus; Driver first, second;
    CHECK(first.bind(bus.config(0)).ok()); CHECK(second.bind(bus.config(31)).ok());
    bus.active = &first; bus.competing = &second; bus.onFrame = compete;
    uint16_t word = SENTINEL;
    const unsigned frames = operation == 2U ? 5U : 4U;
    const auto status = operation == 0U ? first.readMmd(31, 2, word, BUDGET) :
        operation == 1U ? first.writeMmd(31, 0x18, 2, BUDGET) : first.modifyMmd(31, 0x18, 2, 2, BUDGET);
    CHECK(status.ok());
    CHECK(bus.acquisitions == 1U && bus.releases == 1U && bus.launches == frames);
    CHECK(bus.callbacks == 1U + frames * 4U);
    CHECK(second.transportSnapshot().attemptedFrames == 0U);
    CHECK(first.transportSnapshot().attemptedFrames == frames);
    bus.onFrame = nullptr;
    CHECK(second.writeMmd(31, 0x18, 0x10, BUDGET).ok());
    expectTrace(bus, 0x18, true, 0x10);
    CHECK(bus.trace[0].phy == 31U && bus.led[31] == 0x10U);
    CHECK(bus.led[0] == (operation == 0U ? 0U : 2U));
    CHECK(first.end().ok()); CHECK(second.end().ok());
  }
  for (bool primaryFailure : {false, true}) {
    Controller bus; Driver first, second;
    CHECK(first.bind(bus.config(0)).ok()); CHECK(second.bind(bus.config(31)).ok());
    bus.releaseFail = true;
    if (primaryFailure) bus.failAt = 2;
    uint16_t word = SENTINEL;
    const auto status = first.readMmd(31, 2, word, BUDGET);
    CHECK(status.is(primaryFailure ? Err::MDIO_TIMEOUT : Err::MDIO_ERROR));
    CHECK(status.detail == (primaryFailure ? -702 : -901));
    CHECK(word == SENTINEL && bus.quarantined && bus.locked);
    CHECK(first.transportSnapshot().lastBatch.releaseStatus.detail == -901);
    const unsigned launches = bus.launches;
    CHECK(second.readMmd(31, 2, word, BUDGET).is(Err::BUSY));
    CHECK(bus.launches == launches);
    CHECK(first.end().ok()); CHECK(first.bind(bus.config(0)).ok());
    CHECK(first.readMmd(31, 2, word, BUDGET).is(Err::BUSY));
    CHECK(bus.launches == launches);
    bus.locked = false; bus.quarantined = false; bus.releaseFail = false; bus.failAt = -1;
    CHECK(second.readMmd(31, 2, word, BUDGET).ok());
    expectTrace(bus, 2, false, 0);
    CHECK(first.end().ok()); CHECK(second.end().ok());
  }
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"traces", testTraces},
    {"admission", testAdmission},
    {"failures", testFailures},
    {"reports", testReports},
    {"ownership", testOwnership},
  };
  return test::runSuites(argc, argv, suites);
}
