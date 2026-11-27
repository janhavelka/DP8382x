#include "support/BoundDriver.h"
#include "support/FakeController.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {
using namespace DP8382x;
using Driver = ::DP8382x::DP8382x;
constexpr IoBudget BUDGET{10000, 2};

// Literal identity words come from SNLS647I tables 8-21/22. The model is a
// controller response source, not an ACK oracle or physical strap measurement.
enum class IdentityFault { NONE, SHORT, MISSING_ATTEMPT, BAD_TIME, NONTERMINAL, ERROR_AFTER_READS };
struct Controller : test::FakeController {
  std::array<uint16_t, 32> first{};
  std::array<uint16_t, 32> second{};
  IdentityFault identityFault{IdentityFault::NONE};
  bool noFrames{false};
  bool noFrameReleaseFailure{false};
  Status noFrameStatus{Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), "Busy"};
  bool reenterIdentity{false};
  bool competeIdentity{false};
  unsigned rejections{0};

  Controller() {
    first.fill(0xFFFF);
    second.fill(0xFFFF);
    first[1] = 0x2000;
    second[1] = test::basicId2();
    first[2] = 0x2000;
    second[2] = test::enhancedId2();
    duration = 2;
    onFrame = step;
  }
  Config config(uint32_t phy = 1) noexcept {
    auto value = test::FakeController::config(phy);
    value.transport.executeBatch = executeIdentity;
    return value;
  }
  static void step(test::FakeController& base, const IoBudget& budget) noexcept {
    auto& bus = static_cast<Controller&>(base);
    bus.readValue = bus.second[bus.lastRequest.phyAddress];
    if (bus.reenterIdentity && bus.active != nullptr) {
      DeviceIdentity output;
      output.id1 = 0xDEAD;
      const auto status = bus.active->readIdentity(output, budget);
      CHECK(status.is(Err::BUSY));
      CHECK(status.detail == static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
      CHECK(output.id1 == 0xDEAD);
      CHECK(bus.active->end().is(Err::BUSY));
      CHECK(bus.active->bind(bus.config()).is(Err::BUSY));
      ++bus.rejections;
    }
    if (bus.competeIdentity && bus.competing != nullptr) {
      DeviceIdentity output;
      output.id1 = 0xBEEF;
      const auto status = bus.competing->readIdentity(output, budget);
      CHECK(status.is(Err::BUSY));
      CHECK(status.detail == static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
      CHECK(output.id1 == 0xBEEF);
      ++bus.rejections;
    }
  }
  static BatchResult executeIdentity(void* context, const MdioBatch& batch,
                                     const IoBudget& budget) noexcept {
    auto& bus = *static_cast<Controller*>(context);
    if (bus.locked || bus.quarantined) {
      return test::FakeController::execute(context, batch, budget);
    }
    if (bus.noFrames) {
      ++bus.callbacks;
      BatchResult result;
      result.status = bus.noFrameStatus;
      result.releaseStatus = Status::Ok();
      result.controllerReusable = true;
      if (bus.noFrameReleaseFailure) {
        result.releaseStatus = {Err::MDIO_ERROR, -901, "Release failed"};
        result.controllerReusable = false;
      }
      return result;
    }
    bus.readValue = bus.first[batch.phyAddress];
    auto result = test::FakeController::execute(context, batch, budget);
    switch (bus.identityFault) {
      case IdentityFault::NONE: break;
      case IdentityFault::SHORT: result.frames[1] = {}; break;
      case IdentityFault::MISSING_ATTEMPT: result.frames[0].attempted = false; break;
      case IdentityFault::BAD_TIME:
        result.frames[1].timeValid = true;
        result.frames[1].completedAtUs = 1;
        break;
      case IdentityFault::NONTERMINAL:
        result.status = {Err::IN_PROGRESS, -2147483647, "Illegal terminal status"};
        break;
      case IdentityFault::ERROR_AFTER_READS:
        result.status = {Err::MDIO_ERROR, std::numeric_limits<int32_t>::min(), "Late backend error"};
        break;
    }
    return result;
  }
};

DeviceIdentity sentinel() {
  DeviceIdentity result;
  result.declaredVariant = static_cast<Variant>(0xFE);
  result.id1 = 0xDEAD;
  result.id2 = 0xBEEF;
  result.id1Valid = true;
  result.id2Valid = true;
  result.oui = 0x123456;
  result.model = 0x3F;
  result.revision = 0x0F;
  result.mode = OperatingMode::ENHANCED;
  result.decodedValid = true;
  result.acceptance = IdentityAcceptance::REJECTED;
  result.admissionStatus = {Err::DEVICE_MISMATCH, -123, "Sentinel"};
  result.observedAtUs = 0x123456789;
  result.timeValid = true;
  result.stale = false;
  return result;
}
bool sameIdentity(const DeviceIdentity& a, const DeviceIdentity& b) {
  return a.declaredVariant == b.declaredVariant && a.id1 == b.id1 && a.id2 == b.id2 &&
      a.id1Valid == b.id1Valid && a.id2Valid == b.id2Valid && a.oui == b.oui &&
      a.model == b.model && a.revision == b.revision && a.mode == b.mode &&
      a.decodedValid == b.decodedValid && a.acceptance == b.acceptance &&
      a.admissionStatus.code == b.admissionStatus.code &&
      a.admissionStatus.detail == b.admissionStatus.detail &&
      a.admissionStatus.msg == b.admissionStatus.msg && a.observedAtUs == b.observedAtUs &&
      a.timeValid == b.timeValid && a.stale == b.stale;
}
void noCapabilities(const Driver& driver) {
  const auto value = driver.capabilities();
  CHECK(!value.valid);
  CHECK(value.linkModeMask == 0U && value.interfaceMask == 0U);
  CHECK(!value.copper && !value.fiber && !value.gigabit);
}
void acceptedCapabilities(const Driver& driver) {
  const auto value = driver.capabilities();
  CHECK(value.valid && value.declaredVariant == driver.identitySnapshot().declaredVariant);
  CHECK(value.linkModeMask == 0x0FU && value.interfaceMask == 0x03U);
  CHECK(value.copper && !value.fiber && !value.gigabit);
}
void literalTrace(const Controller& bus, uint8_t phy) {
  CHECK(bus.lastRequest.frameCount == 2U);
  CHECK(bus.lastRequest.phyAddress == phy);
  CHECK(bus.traceCount == 2U);
  CHECK(bus.trace[0].kind == FrameKind::READ && bus.trace[0].reg == 0x02U);
  CHECK(bus.trace[1].kind == FrameKind::READ && bus.trace[1].reg == 0x03U);
  CHECK(bus.acquisitions == 1U && bus.releases == 1U && !bus.locked);
  CHECK(bus.launches == 2U);
}
void blockedMutations(Controller& bus, Driver& driver, const Status& expected) {
  const auto calls = bus.callbacks;
  const auto clocks = bus.clocks;
  if (test::aGrade()) {
    const auto status = driver.modifyBasicControl(0x1000, 0x1000, {10000, 2});
    CHECK(status.code == expected.code && status.detail == expected.detail);
    CHECK(bus.callbacks == calls && bus.clocks == clocks);
    return;
  }
  const Status results[] = {
    driver.writeClause22(0x18, 0x12, {10000, 1}),
    driver.writeMmd(31, 0x18, 0x12, {10000, 4}),
    driver.modifyMmd(31, 0x18, 0x12, 0x02, {10000, 5}),
    driver.modifyInterruptEnables(0x12, 1, 1, {10000, 2}),
    driver.modifyWakeControl(1, 1, false, {10000, 14})
  };
  for (const auto& result : results) {
    CHECK(result.code == expected.code && result.detail == expected.detail);
  }
  CHECK(bus.callbacks == calls && bus.clocks == clocks);
}

void testDecode() {
  for (const auto mode : {OperatingMode::BASIC, OperatingMode::ENHANCED}) {
    for (const auto interface : {MacInterface::MII, MacInterface::RMII}) {
      for (const auto requirement : {OperatingMode::UNKNOWN, mode}) {
        Controller bus;
        Driver driver;
        bus.second[1] = mode == OperatingMode::BASIC ? test::basicId2() : test::enhancedId2();
        auto config = bus.config();
        config.interface = interface;
        config.requiredMode = requirement;
        CHECK(driver.bind(config).ok());
        noCapabilities(driver);
        CHECK(bus.callbacks == 0U && bus.clocks == 0U);
        auto output = sentinel();
        CHECK(driver.readIdentity(output, BUDGET).ok());
        CHECK(output.declaredVariant == test::declaredVariant);
        CHECK(output.id1 == 0x2000U && output.id2 == bus.second[1]);
        CHECK(output.id1Valid && output.id2Valid && output.decodedValid);
        CHECK(output.oui == 0x080028U);
        CHECK(output.model == (mode == OperatingMode::BASIC ? 0x11U : 0x13U));
        CHECK(output.revision == (test::aGrade() ? 4U : 1U) && output.mode == mode);
        CHECK(output.acceptance == IdentityAcceptance::ACCEPTED && output.admissionStatus.ok());
        CHECK(!output.stale && output.timeValid && output.observedAtUs == 106U);
        CHECK(sameIdentity(output, driver.identitySnapshot()));
        acceptedCapabilities(driver);
        literalTrace(bus, 1);
        CHECK(bus.lastBudget.deadlineUs == 10000U && bus.lastBudget.maxFrames == 2U);
        CHECK(driver.transportSnapshot().attemptedFrames == 2U);
        CHECK(driver.transportSnapshot().completedFrames == 2U);
        const auto clocks = bus.clocks;
        const auto calls = bus.callbacks;
        for (unsigned i = 0; i < 8U; ++i) {
          acceptedCapabilities(driver);
          CHECK(sameIdentity(output, driver.identitySnapshot()));
        }
        CHECK(bus.callbacks == calls && bus.clocks == clocks);
      }
    }
  }
  // Protocol addresses are five bits, independent of the part's three straps.
  for (uint32_t phy = 0; phy < 32U; ++phy) {
    Controller bus;
    Driver driver;
    bus.first[phy] = 0x2000;
    bus.second[phy] = test::enhancedId2();
    CHECK(driver.bind(bus.config(phy)).ok());
    DeviceIdentity output;
    CHECK(driver.readIdentity(output, BUDGET).ok());
    literalTrace(bus, static_cast<uint8_t>(phy));
  }
}

void testPolicy() {
  // Every raw revision and model code is checked independently of the production
  // accepted-revision table. Each declaration admits exactly one generation.
  for (uint16_t model = 0; model < 64U; ++model) {
    for (uint16_t revision = 0; revision < 16U; ++revision) {
      Controller bus;
      Driver driver;
      bus.second[1] = static_cast<uint16_t>(0xA000U | (uint32_t{model} << 4U) | revision);
      CHECK(driver.bind(bus.config()).ok());
      auto output = sentinel();
      const auto status = driver.readIdentity(output, BUDGET);
      const bool modelAccepted = model == 0x11U || model == 0x13U;
      const bool accepted = modelAccepted && revision == (test::aGrade() ? 4U : 1U);
      CHECK(status.ok() == accepted);
      const auto snapshot = driver.identitySnapshot();
      CHECK(snapshot.oui == 0x080028U && snapshot.model == model && snapshot.revision == revision);
      CHECK(snapshot.id1Valid && snapshot.id2Valid && snapshot.decodedValid && !snapshot.stale);
      CHECK(snapshot.mode == (model == 0x11U ? OperatingMode::BASIC :
          model == 0x13U ? OperatingMode::ENHANCED : OperatingMode::UNKNOWN));
      if (!accepted) {
        CHECK(status.is(Err::DEVICE_MISMATCH));
        CHECK(status.detail == static_cast<int32_t>(modelAccepted ? IdentityDetail::REVISION_NOT_ACCEPTED : IdentityDetail::WRONG_MODEL));
        CHECK(snapshot.acceptance == IdentityAcceptance::REJECTED);
        CHECK(sameIdentity(output, sentinel()));
        noCapabilities(driver);
      }
      literalTrace(bus, 1);
    }
  }
  struct Case { uint16_t id1; uint16_t id2; OperatingMode required; Err code; IdentityDetail detail; };
  const Case cases[] = {
    {0, 0, OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ZERO_ID},
    {0xFFFF, 0xFFFF, OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ONES_ID},
    {0, 0xFFFF, OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ZERO_ID},
    {0xFFFF, 0, OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ZERO_ID},
    {0x2000, 0, OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ZERO_ID},
    {0x2000, 0xFFFF, OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ONES_ID},
    {0, test::basicId2(), OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ZERO_ID},
    {0xFFFF, test::basicId2(), OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ONES_ID},
    {0x2001, test::basicId2(), OperatingMode::UNKNOWN, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_OUI},
    {0x2000, 0xA511, OperatingMode::UNKNOWN, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_OUI},
    {0x2001, 0xA144, OperatingMode::ENHANCED, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_OUI},
    {0x2000, 0xA144, OperatingMode::ENHANCED, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_MODEL},
    {0x2000, static_cast<uint16_t>(test::aGrade() ? 0xA111 : 0xA114), OperatingMode::ENHANCED, Err::DEVICE_MISMATCH, IdentityDetail::REVISION_NOT_ACCEPTED},
    {0x2000, test::basicId2(), OperatingMode::ENHANCED, Err::DEVICE_MISMATCH, IdentityDetail::MODE_MISMATCH},
    {0x2000, test::enhancedId2(), OperatingMode::BASIC, Err::DEVICE_MISMATCH, IdentityDetail::MODE_MISMATCH},
    {0x2000, 0xA140, OperatingMode::UNKNOWN, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_MODEL},
    {0x2000, 0xA150, OperatingMode::UNKNOWN, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_MODEL}
  };
  for (const auto& value : cases) {
    Controller bus;
    Driver driver;
    bus.first[1] = value.id1;
    bus.second[1] = value.id2;
    auto config = bus.config();
    config.requiredMode = value.required;
    CHECK(driver.bind(config).ok());
    auto output = sentinel();
    const auto status = driver.readIdentity(output, BUDGET);
    CHECK(status.code == value.code && status.detail == static_cast<int32_t>(value.detail));
    CHECK(sameIdentity(output, sentinel()));
    const auto snapshot = driver.identitySnapshot();
    CHECK(snapshot.id1Valid && snapshot.id2Valid);
    CHECK(snapshot.id1 == value.id1 && snapshot.id2 == value.id2);
    CHECK(snapshot.declaredVariant == config.variant);
    CHECK(snapshot.acceptance == IdentityAcceptance::REJECTED && !snapshot.stale);
    CHECK(snapshot.admissionStatus.code == status.code && snapshot.admissionStatus.detail == status.detail);
    noCapabilities(driver);
    literalTrace(bus, 1);
    blockedMutations(bus, driver, status);
    CHECK(driver.invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION).ok());
    blockedMutations(bus, driver, status);
  }
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config(7)).ok());
  auto output = sentinel();
  CHECK(driver.readIdentity(output, BUDGET).is(Err::DEVICE_NOT_FOUND));
  CHECK(sameIdentity(output, sentinel()));
  literalTrace(bus, 7); // Never silently scans or selects responding address 1.
}

// SNLS638C tables 6-9 and 7-5/6. Enumerate the entire model/revision
// space so the Linux I/S labels and the two 826 generations cannot alias 825I.
void testP25Identity() {
  CHECK(test::declaredVariant == Variant::DP83825I);
  uint8_t maximum = 0xEE;
  CHECK(strapAddressMax(test::declaredVariant, maximum) && maximum == 3);
  for (unsigned model = 0; model < 64; ++model) {
    for (unsigned revision = 0; revision < 16; ++revision) {
      Controller bus;
      Driver driver;
      bus.second[1] = static_cast<uint16_t>(0xA000U | (model << 4U) | revision);
      CHECK(driver.bind(bus.config()).ok() && bus.callbacks == 0 && bus.clocks == 0);
      auto output = sentinel();
      const auto status = driver.readIdentity(output, BUDGET);
      const auto cached = driver.identitySnapshot();
      const auto calls = bus.callbacks, clocks = bus.clocks;
      const bool expected = model == 0x14 && revision == 0;
      CHECK(cached.id1Valid && cached.id2Valid && cached.id1 == 0x2000 && cached.id2 == bus.second[1]);
      CHECK(cached.model == model && cached.revision == revision && cached.oui == 0x080028);
      if (expected) {
        CHECK(status.ok() && output.mode == OperatingMode::UNKNOWN && output.acceptance == IdentityAcceptance::ACCEPTED);
        const auto caps = driver.capabilities();
        CHECK(caps.valid && caps.linkModeMask == 15 && caps.interfaceMask == 2 && caps.copper && !caps.fiber);
      } else {
        CHECK(status.is(Err::DEVICE_MISMATCH) && sameIdentity(output, sentinel()));
        CHECK(cached.acceptance == IdentityAcceptance::REJECTED && !driver.capabilities().valid);
        uint16_t unchanged = 0xC0DE;
        CHECK(!driver.readMmd(31, 0x0467, unchanged, {10000, 4}).ok() && unchanged == 0xC0DE);
        CHECK(!driver.modifyBasicControl(0x1000, 0x1000, {10000, 2}).ok());
      }
      literalTrace(bus, 1);
      CHECK(bus.callbacks == calls && bus.clocks == clocks);
    }
  }
  for (auto words : {std::array<uint16_t, 2>{0, 0}, {0xFFFF, 0xFFFF}, {0x2001, 0xA140}}) {
    Controller bus;
    Driver driver;
    bus.first[1] = words[0]; bus.second[1] = words[1];
    CHECK(driver.bind(bus.config()).ok());
    auto output = sentinel();
    CHECK(!driver.readIdentity(output, BUDGET).ok() && sameIdentity(output, sentinel()));
    CHECK(driver.identitySnapshot().id1 == words[0] && driver.identitySnapshot().id2 == words[1]);
    literalTrace(bus, 1);
  }
  for (int cut = 0; cut < 2; ++cut) {
    Controller bus;
    Driver driver;
    bus.failAt = cut;
    CHECK(driver.bind(bus.config()).ok());
    auto output = sentinel();
    CHECK(driver.readIdentity(output, BUDGET).is(Err::MDIO_TIMEOUT) && sameIdentity(output, sentinel()));
    const auto saved = driver.identitySnapshot();
    CHECK(saved.id1Valid == (cut == 1) && !saved.id2Valid && saved.stale && !driver.capabilities().valid);
  }
  for (auto invalid : {0, 1, 2, 3, 4, 5, 6, 7}) {
    Controller bus;
    Driver driver;
    auto config = bus.config();
    if (invalid == 0) config.rmiiClock = RmiiClock::UNSPECIFIED;
    if (invalid == 1) config.rmiiClock = static_cast<RmiiClock>(0xFE);
    if (invalid == 2) config.interface = MacInterface::MII;
    if (invalid == 3) config.interface = MacInterface::RGMII;
    if (invalid == 4) config.requiredMode = OperatingMode::ENHANCED;
    if (invalid == 5) config.medium = Medium::FIBER;
    if (invalid == 6) config.medium = static_cast<Medium>(255);
    if (invalid == 7) config.mdcHz = 20000001; // Section 5.7 T4, above 20 MHz.
    CHECK(driver.bind(bus.config()).ok()); // Invalid rebind must preserve the prior binding.
    CHECK(!driver.bind(config).ok() && bus.callbacks == 0 && bus.clocks == 0);
    bus.second[1] = 0xA140;
    DeviceIdentity observed;
    CHECK(driver.readIdentity(observed, BUDGET).ok() && observed.declaredVariant == Variant::DP83825I);
  }
}

void testFailures() {
  for (int phase = 0; phase < 2; ++phase) {
    auto [bus, driver] = test::BoundDriver<Controller>{};
    bus.failAt = phase;
    auto output = sentinel();
    const auto status = driver.readIdentity(output, BUDGET);
    CHECK(status.is(Err::MDIO_TIMEOUT) && status.detail == -700 - phase);
    CHECK(sameIdentity(output, sentinel()));
    const auto snapshot = driver.identitySnapshot();
    CHECK(snapshot.id1Valid == (phase == 1) && !snapshot.id2Valid);
    CHECK(!snapshot.decodedValid && snapshot.acceptance == IdentityAcceptance::UNASSESSED);
    CHECK(snapshot.stale && !snapshot.timeValid);
    CHECK(snapshot.admissionStatus.code == status.code && snapshot.admissionStatus.detail == status.detail);
    noCapabilities(driver);
    CHECK(bus.callbacks == 1U && bus.launches == static_cast<unsigned>(phase + 1));
    CHECK(bus.acquisitions == 1U && bus.releases == 1U && !bus.locked);
    CHECK(driver.transportSnapshot().attemptedFrames == static_cast<uint32_t>(phase + 1));
    CHECK(driver.transportSnapshot().completedFrames == static_cast<uint32_t>(phase));
    blockedMutations(bus, driver, status);
  }
  for (const auto fault : {IdentityFault::SHORT, IdentityFault::MISSING_ATTEMPT,
       IdentityFault::BAD_TIME, IdentityFault::NONTERMINAL, IdentityFault::ERROR_AFTER_READS}) {
    auto [bus, driver] = test::BoundDriver<Controller>{};
    bus.identityFault = fault;
    auto output = sentinel();
    const auto status = driver.readIdentity(output, BUDGET);
    CHECK(status.is(Err::MDIO_ERROR));
    CHECK(sameIdentity(output, sentinel()));
    const auto snapshot = driver.identitySnapshot();
    CHECK(snapshot.acceptance == IdentityAcceptance::UNASSESSED && snapshot.stale);
    CHECK(snapshot.id1Valid == (fault != IdentityFault::MISSING_ATTEMPT));
    CHECK(snapshot.id2Valid == (fault != IdentityFault::MISSING_ATTEMPT && fault != IdentityFault::SHORT));
    CHECK(snapshot.decodedValid == snapshot.id2Valid);
    if (fault == IdentityFault::BAD_TIME) CHECK(!snapshot.timeValid);
    if (fault == IdentityFault::NONTERMINAL) CHECK(status.detail == -2147483647);
    if (fault == IdentityFault::ERROR_AFTER_READS) CHECK(status.detail == std::numeric_limits<int32_t>::min());
    noCapabilities(driver);
    literalTrace(bus, 1);
  }
  for (unsigned fault = 0; fault < 3U; ++fault) {
    auto [bus, driver] = test::BoundDriver<Controller>{};
    if (fault == 0U) bus.releaseFail = true;
    if (fault == 1U) bus.duration = 100;
    if (fault == 2U) bus.regress = true;
    auto output = sentinel();
    const auto status = driver.readIdentity(output, {150, 2});
    CHECK(status.code == (fault == 0U ? Err::MDIO_ERROR : fault == 1U ? Err::OPERATION_TIMEOUT : Err::INVALID_CONFIG));
    CHECK(sameIdentity(output, sentinel()));
    const auto snapshot = driver.identitySnapshot();
    CHECK(snapshot.id1Valid);
    CHECK(snapshot.id2Valid == (fault != 1U));
    CHECK(snapshot.acceptance == IdentityAcceptance::UNASSESSED && snapshot.stale);
    if (fault == 2U) CHECK(!snapshot.timeValid);
    noCapabilities(driver);
  }
  // A complete late response retains both words but cannot publish acceptance.
  auto [bus, driver] = test::BoundDriver<Controller>{};
  bus.expireAfter = 1;
  auto output = sentinel();
  CHECK(driver.readIdentity(output, {150, 2}).is(Err::OPERATION_TIMEOUT));
  CHECK(sameIdentity(output, sentinel()));
  CHECK(driver.identitySnapshot().id1Valid && driver.identitySnapshot().id2Valid);
  noCapabilities(driver);
  // Two frames need fixed(9)+frames(10)+clock lag(2) strictly before deadline.
  for (const uint64_t deadline : {uint64_t{121}, uint64_t{122}}) {
    auto [edge, edgeDriver] = test::BoundDriver<Controller>{};
    auto edgeOutput = sentinel();
    const auto status = edgeDriver.readIdentity(edgeOutput, {deadline, 2});
    if (deadline == 121U) {
      CHECK(status.is(Err::OPERATION_TIMEOUT));
      CHECK(status.detail == static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION));
      CHECK(sameIdentity(edgeOutput, sentinel()));
      CHECK(edge.callbacks == 0U && edge.launches == 0U);
    } else {
      CHECK(status.ok());
      CHECK(edge.callbacks == 1U && edge.launches == 2U);
    }
  }
}

void testLifecycle() {
  Controller bus;
  Driver driver;
  auto output = sentinel();
  CHECK(driver.readIdentity(output, BUDGET).is(Err::NOT_BOUND));
  CHECK(sameIdentity(output, sentinel()));
  noCapabilities(driver);
  CHECK(bus.callbacks == 0U && bus.clocks == 0U);
  CHECK(driver.bind(bus.config()).ok());
  CHECK(driver.readIdentity(output, BUDGET).ok());
  const auto accepted = driver.identitySnapshot();
  const auto calls = bus.callbacks;
  for (const auto budget : {IoBudget{10000, 0}, IoBudget{10000, 1}, IoBudget{100, 2}, IoBudget{125, 2}}) {
    auto unchanged = sentinel();
    CHECK(driver.readIdentity(unchanged, budget).is(Err::OPERATION_TIMEOUT));
    CHECK(sameIdentity(unchanged, sentinel()));
    CHECK(sameIdentity(driver.identitySnapshot(), accepted));
    acceptedCapabilities(driver);
    CHECK(bus.callbacks == calls);
  }
  for (const auto variant : {Variant::UNKNOWN, static_cast<Variant>(10), static_cast<Variant>(255)}) {
    auto config = bus.config();
    config.variant = variant;
    const auto clocks = bus.clocks;
    const auto status = driver.bind(config);
    CHECK(status.is(Err::INVALID_CONFIG));
    CHECK(status.detail == static_cast<int32_t>(ConfigDetail::ENUM_VALUE));
    CHECK(sameIdentity(driver.identitySnapshot(), accepted));
    CHECK(bus.callbacks == calls && bus.clocks == clocks);
  }
  for (const auto interface : {MacInterface::UNKNOWN, MacInterface::RGMII, static_cast<MacInterface>(255)}) {
    auto config = bus.config();
    config.interface = interface;
    const auto clocks = bus.clocks;
    const auto status = driver.bind(config);
    CHECK(status.code == (interface == MacInterface::RGMII ? Err::UNSUPPORTED : Err::INVALID_CONFIG));
    CHECK(status.detail == (interface == MacInterface::RGMII ? static_cast<int32_t>(UnsupportedDetail::INTERFACE) :
        interface == MacInterface::UNKNOWN ? static_cast<int32_t>(ConfigDetail::INTERFACE_REQUIRED) :
        static_cast<int32_t>(ConfigDetail::ENUM_VALUE)));
    CHECK(sameIdentity(driver.identitySnapshot(), accepted));
    CHECK(bus.callbacks == calls && bus.clocks == clocks);
  }
  for (const auto badAddress : {uint32_t{32}, uint32_t{0x8000}, std::numeric_limits<uint32_t>::max()}) {
    const auto status = driver.bind(bus.config(badAddress));
    CHECK(status.is(Err::INVALID_CONFIG));
    CHECK(status.detail == static_cast<int32_t>(ConfigDetail::ADDRESS_RANGE));
    CHECK(sameIdentity(driver.identitySnapshot(), accepted));
    CHECK(bus.callbacks == calls);
  }
  auto config = bus.config();
  config.requiredMode = static_cast<OperatingMode>(255);
  CHECK(driver.bind(config).is(Err::INVALID_CONFIG));
  CHECK(sameIdentity(driver.identitySnapshot(), accepted));
  bus.noFrames = true;
  for (const auto status : {Status{Err::BUSY, static_cast<int32_t>(BusyDetail::OWNER_ADMISSION), "Busy"},
        Status{Err::OPERATION_TIMEOUT, static_cast<int32_t>(TimeoutDetail::DEADLINE), "Expired"}}) {
    bus.noFrameStatus = status;
    CHECK(driver.readIdentity(output, BUDGET).code == status.code);
    CHECK(sameIdentity(driver.identitySnapshot(), accepted));
    acceptedCapabilities(driver);
  }
  bus.noFrameStatus = {Err::MDIO_ERROR, -123456789, "No returned data"};
  const auto noData = driver.readIdentity(output, BUDGET);
  CHECK(noData.detail == -123456789);
  CHECK(!driver.identitySnapshot().id1Valid && !driver.identitySnapshot().id2Valid);
  noCapabilities(driver);
  blockedMutations(bus, driver, noData);
  bus.noFrames = false;
  CHECK(driver.readIdentity(output, BUDGET).ok());
  CHECK(driver.writeClause22(0x18, 0x12, {10000, 1}).ok());
  CHECK(driver.writeClause22(0x18, 0x12, {10000, 1}).ok()); // Stale success still permits maintenance.
  CHECK(driver.readIdentity(output, BUDGET).ok());
  for (const auto reason : {InvalidationReason::EXTERNAL_RESET, InvalidationReason::EXTERNAL_POWER_ON,
       InvalidationReason::EXTERNAL_MUTATION, InvalidationReason::RAW_MUTATION, InvalidationReason::AMBIGUOUS_MUTATION}) {
    const auto before = driver.identitySnapshot();
    const auto beforeCalls = bus.callbacks;
    const auto beforeClocks = bus.clocks;
    CHECK(driver.invalidateDeviceState(reason).ok());
    const auto after = driver.identitySnapshot();
    CHECK(after.id1 == before.id1 && after.id2 == before.id2 && after.id1Valid && after.id2Valid);
    CHECK(after.stale);
    noCapabilities(driver);
    CHECK(bus.callbacks == beforeCalls && bus.clocks == beforeClocks);
    CHECK(driver.readIdentity(output, BUDGET).ok());
  }
  uint16_t raw = 0;
  CHECK(driver.readClause22(2, raw, {10000, 1}).ok());
  acceptedCapabilities(driver); // A raw ID word does not replace the accepted pair.
  CHECK(driver.writeClause22(0x18, 0x12, {10000, 1}).ok());
  CHECK(driver.identitySnapshot().stale);
  noCapabilities(driver);
  CHECK(driver.readIdentity(output, BUDGET).ok());
  bus.second[1] = test::enhancedId2();
  CHECK(driver.readIdentity(output, BUDGET).ok());
  CHECK(output.mode == OperatingMode::ENHANCED && output.model == 0x13U);
  bus.failAt = 1;
  CHECK(driver.readIdentity(output, BUDGET).is(Err::MDIO_TIMEOUT));
  CHECK(driver.identitySnapshot().id1Valid && !driver.identitySnapshot().id2Valid);
  noCapabilities(driver); // Never combine the previous id2 with this attempt's id1.
  bus.failAt = -1;
  CHECK(driver.bind(bus.config(2)).ok());
  CHECK(!driver.identitySnapshot().id1Valid && !driver.identitySnapshot().id2Valid);
  noCapabilities(driver);
  CHECK(driver.readIdentity(output, BUDGET).ok());
  CHECK(driver.end().ok());
  CHECK(!driver.identitySnapshot().id1Valid && !driver.identitySnapshot().id2Valid);
  noCapabilities(driver);
  CHECK(driver.bind(bus.config(1)).ok());
  bus.second[1] = 0xA140;
  const auto rejected = driver.readIdentity(output, BUDGET);
  CHECK(rejected.is(Err::DEVICE_MISMATCH));
  blockedMutations(bus, driver, rejected);
  bus.second[1] = test::basicId2();
  CHECK(driver.readIdentity(output, BUDGET).ok());
  CHECK(driver.writeClause22(0x18, 0x12, {10000, 1}).ok());
  bus.second[1] = 0xA140;
  CHECK(driver.readIdentity(output, BUDGET).is(Err::DEVICE_MISMATCH));
  CHECK(driver.bind(bus.config(1)).ok());
  CHECK(driver.writeClause22(0x18, 0x12, {10000, 1}).ok()); // Fresh owner attestation is explicit.

  // Re-reading identical words does not make unrelated collected samples stale.
  auto [changed, observed] = test::BoundDriver<Controller>{};
  CHECK(observed.readIdentity(output, BUDGET).ok());
  EvidenceSnapshot evidence;
  CHECK(observed.acquireEvidence(evidenceBit(EvidenceSource::BMSR), evidence, {10000, 1}).ok());
  CHECK(!observed.evidenceSnapshot().samples[1].stale);
  CHECK(observed.readIdentity(output, BUDGET).ok());
  CHECK(!observed.evidenceSnapshot().samples[1].stale);
  CHECK(!observed.evidenceSnapshot().configurationDirty);
  changed.second[1] = test::enhancedId2();
  CHECK(observed.readIdentity(output, BUDGET).ok());
  CHECK(observed.evidenceSnapshot().samples[1].stale);
  CHECK(observed.evidenceSnapshot().configurationDirty);
  acceptedCapabilities(observed);

  // No-frame admission preserves prior identity only with sound release evidence.
  changed.noFrames = true;
  changed.noFrameReleaseFailure = true;
  CHECK(observed.readIdentity(output, BUDGET).is(Err::BUSY));
  noCapabilities(observed);
  CHECK(observed.identitySnapshot().stale);
  CHECK(observed.transportSnapshot().lastBatch.releaseStatus.detail == -901);
}

void testOwnership() {
  Controller bus;
  Driver first;
  Driver second;
  CHECK(first.bind(bus.config(1)).ok());
  auto otherConfig = bus.config(2);
  otherConfig.variant = test::declaredVariant == Variant::DP83826I ? Variant::DP83826E : Variant::DP83826I;
  CHECK(second.bind(otherConfig).ok());
  DeviceIdentity firstOutput;
  DeviceIdentity secondOutput;
  CHECK(second.readIdentity(secondOutput, BUDGET).ok());
  const auto secondAccepted = second.identitySnapshot();
  bus.active = &first;
  bus.competing = &second;
  bus.reenterIdentity = true;
  bus.competeIdentity = true;
  CHECK(first.readIdentity(firstOutput, BUDGET).ok());
  CHECK(firstOutput.mode == OperatingMode::BASIC);
  CHECK(bus.rejections == 4U);
  CHECK(sameIdentity(second.identitySnapshot(), secondAccepted));
  acceptedCapabilities(first);
  acceptedCapabilities(second);
  CHECK(bus.acquisitions == 2U && bus.releases == 2U && bus.launches == 4U);
  CHECK(first.transportSnapshot().attemptedFrames == 2U);
  CHECK(second.transportSnapshot().attemptedFrames == 2U);
  CHECK(first.invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
  noCapabilities(first);
  acceptedCapabilities(second);
  CHECK(sameIdentity(second.identitySnapshot(), secondAccepted));
  bus.reenterIdentity = false;
  bus.competeIdentity = false;
  CHECK(first.bind(bus.config(2)).ok());
  CHECK(first.readIdentity(firstOutput, BUDGET).ok());
  CHECK(firstOutput.mode == OperatingMode::ENHANCED);
  CHECK(firstOutput.id1 == secondOutput.id1 && firstOutput.id2 == secondOutput.id2);
  CHECK(firstOutput.declaredVariant == test::declaredVariant);
  CHECK(second.identitySnapshot().declaredVariant == otherConfig.variant);
  CHECK(secondOutput.declaredVariant != firstOutput.declaredVariant);
}

void testScan() {
  Controller bus;
  Driver driver;
  uint16_t id1 = 0xDEAD, id2 = 0xBEEF;
  bool invoked = true;
  CHECK(driver.readIdentityAt(0, id1, id2, BUDGET, &invoked).is(Err::NOT_BOUND));
  CHECK(!invoked && id1 == 0xDEAD && id2 == 0xBEEF);
  CHECK(driver.bind(bus.config()).ok());
  DeviceIdentity identity;
  CHECK(driver.readIdentity(identity, BUDGET).ok());
  const auto accepted = driver.identitySnapshot();
  const auto settings = driver.settingsSnapshot();
  const auto health = driver.healthSnapshot();
  const auto before = driver.transportSnapshot();
  const auto clocks = bus.clocks;
  for (const auto address : {uint32_t{32}, uint32_t{0x8000}, UINT32_MAX}) {
    invoked = true;
    CHECK(driver.readIdentityAt(address, id1, id2, BUDGET, &invoked).is(Err::INVALID_PARAM));
    CHECK(!invoked && id1 == 0xDEAD && id2 == 0xBEEF);
    CHECK(bus.callbacks == before.totalBatches && bus.clocks == clocks);
  }
  // Discovery returns raw words, even values that identity acceptance rejects.
  bus.first[0] = 0; bus.second[0] = 0xFFFF;
  bus.first[31] = 0x1234; bus.second[31] = 0x5678;
  for (const uint32_t address : {0U, 31U}) {
    invoked = false;
    CHECK(driver.readIdentityAt(address, id1, id2, BUDGET, &invoked).ok());
    CHECK(invoked && id1 == bus.first[address] && id2 == bus.second[address]);
    CHECK(bus.lastRequest.phyAddress == address && bus.lastRequest.frameCount == 2U);
    CHECK(bus.trace[0].reg == 2U && bus.trace[1].reg == 3U);
    CHECK(sameIdentity(driver.identitySnapshot(), accepted));
    acceptedCapabilities(driver);
    CHECK(driver.settingsSnapshot().trust == settings.trust);
  }
  const auto scanned = driver.transportSnapshot();
  CHECK(scanned.totalBatches == before.totalBatches + 2U);
  CHECK(scanned.attemptedFrames == before.attemptedFrames + 4U);
  CHECK(scanned.completedFrames == before.completedFrames + 4U);
  CHECK(scanned.lastBatch.frames[0].value == 0x1234 && scanned.lastBatch.frames[1].value == 0x5678);
  CHECK(driver.healthSnapshot().totalSuccess == health.totalSuccess);
  CHECK(driver.healthSnapshot().totalFailures == health.totalFailures);
  CHECK(driver.readIdentity(identity, BUDGET).ok());
  CHECK(bus.lastRequest.phyAddress == 1U && identity.id2 == test::basicId2());
  invoked = true;
  CHECK(driver.readIdentityAt(31, id1, id2, {10000, 1}, &invoked).is(Err::OPERATION_TIMEOUT));
  CHECK(!invoked && driver.transportSnapshot().totalBatches == scanned.totalBatches + 1U);
  CHECK(id1 == 0x1234 && id2 == 0x5678);

  for (int phase = 0; phase < 2; ++phase) {
    auto [failed, observed] = test::BoundDriver<Controller>{};
    failed.failAt = phase;
    invoked = false; id1 = 0xDEAD; id2 = 0xBEEF;
    CHECK(observed.readIdentityAt(31, id1, id2, BUDGET, &invoked).is(Err::MDIO_TIMEOUT));
    CHECK(invoked && id1 == 0xDEAD && id2 == 0xBEEF);
    const auto receipt = observed.transportSnapshot();
    CHECK(receipt.totalBatches == 1U && receipt.attemptedFrames == static_cast<uint32_t>(phase + 1));
    CHECK(receipt.completedFrames == static_cast<uint32_t>(phase));
    CHECK(receipt.lastBatch.frames[static_cast<size_t>(phase)].effectPossible);
    CHECK(observed.healthSnapshot().totalFailures == 0U);
    CHECK(!failed.locked && failed.releases == 1U);
  }
  for (unsigned fault = 0; fault < 3U; ++fault) {
    auto [failed, observed] = test::BoundDriver<Controller>{};
    if (fault == 0U) failed.expireAfter = 1;
    if (fault == 1U) failed.regress = true;
    if (fault == 2U) failed.identityFault = IdentityFault::SHORT;
    invoked = false; id1 = 0xDEAD; id2 = 0xBEEF;
    const auto status = observed.readIdentityAt(0, id1, id2, {150, 2}, &invoked);
    CHECK(status.code == (fault == 0U ? Err::OPERATION_TIMEOUT : fault == 1U ? Err::INVALID_CONFIG : Err::MDIO_ERROR));
    CHECK(invoked && id1 == 0xDEAD && id2 == 0xBEEF);
    const auto receipt = observed.transportSnapshot();
    CHECK(receipt.totalBatches == 1U && receipt.attemptedFrames == (fault == 2U ? 1U : 2U));
    CHECK(receipt.late == (fault == 0U) && receipt.clockInvalid == (fault == 1U));
    CHECK((receipt.violation != TransportViolation::NONE) == (fault == 2U));
    CHECK(observed.healthSnapshot().totalSuccess == 0U && observed.healthSnapshot().totalFailures == 0U);
    if (fault != 0U) {
      invoked = true;
      CHECK(!observed.readIdentityAt(0, id1, id2, BUDGET, &invoked).ok());
      CHECK(!invoked && receipt.totalBatches == observed.transportSnapshot().totalBatches);
    }
  }

  // Raw scans cannot bypass either an active operation or its retained result.
  auto [owned, active] = test::BoundDriver<Controller>{};
  OperationToken token;
  CHECK(active.startInitialize({ResetKind::PCS}, {{10000, 64}, 10, 16}, token).inProgress());
  invoked = true;
  CHECK(active.readIdentityAt(31, id1, id2, BUDGET, &invoked).is(Err::BUSY));
  CHECK(!invoked && owned.callbacks == 0U);
  // Reach the real reset write with the existing controller, then cancel. The
  // resulting cleanup obligation remains after its terminal result is taken.
  for (unsigned polls = 0; active.operationSnapshot().phase != OperationPhase::COMMAND; ++polls) {
    CHECK(polls < 16U);
    CHECK(active.pollOperation(token).inProgress());
  }
  CHECK(active.pollOperation(token).inProgress());
  CHECK(active.cancel(token).ok());
  const auto calls = owned.callbacks;
  invoked = true;
  CHECK(active.readIdentityAt(31, id1, id2, BUDGET, &invoked).detail == static_cast<int32_t>(BusyDetail::RESULT_PENDING));
  CHECK(!invoked && owned.callbacks == calls);
  OperationResult result;
  CHECK(active.takeResult(token, result).ok() && result.cleanupOwed);
  invoked = true;
  CHECK(active.readIdentityAt(31, id1, id2, BUDGET, &invoked).detail == static_cast<int32_t>(BusyDetail::CLEANUP_REQUIRED));
  CHECK(!invoked && owned.callbacks == calls);
}
MacInterface p22Interface = MacInterface::UNKNOWN;

// While this map admits only identity reads, cover both the general access
// policy and the typed paths that can bypass the raw register allowlist.
void checkP22IdentityFence(Controller& bus, Driver& driver) {
  const auto calls = bus.callbacks, clocks = bus.clocks;
  uint16_t word = 0xCAFE;
  bool invoked = true;
  CHECK(!driver.readClause22(0, word, {10000, 1}, &invoked).ok() && !invoked);
  CHECK(!driver.readMmd(31, 0x0467, word, {10000, 4}, &invoked).ok() && !invoked);
  CHECK(word == 0xCAFE);
  CHECK(!driver.modifyBasicControl(0x1000, 0x1000, {10000, 2}, &invoked).ok() && !invoked);
  EvidenceSnapshot evidence;
  evidence.lastRequested = 0xCAFE;
  CHECK(!driver.acquireEvidence(evidenceBit(EvidenceSource::MISR1), evidence, {10000, 1}, &invoked).ok() && !invoked);
  CHECK(evidence.lastRequested == 0xCAFE);
  WakeSnapshot wake;
  wake.timeValid = true;
  CHECK(!driver.readWakeStatus(wake, {10000, 64}).ok() && wake.timeValid);
  LinkSnapshot link;
  link.valid = true;
  CHECK(!driver.readLinkStatus(link, {10000, 64}).ok() && link.valid);
  OperationToken token{123};
  LinkSettings settings;
  settings.advertisement.technologyMask = 0x0F;
  const auto configure = driver.startConfigure(settings, {{10000, 64}, 10, 16}, token);
  CHECK(!configure.ok() && !configure.inProgress() && token.value == 123);
  for (auto reset : {ResetKind::NONE, ResetKind::PCS, ResetKind::HARD, ResetKind::RESTART, ResetKind::EEE_PCS}) {
    const auto status = driver.startInitialize({reset}, {{10000, 64}, 10, 16}, token);
    CHECK(!status.ok() && !status.inProgress() && token.value == 123);
  }
  CHECK(bus.callbacks == calls && bus.clocks == clocks);
}

// SNLS505H tables 8-3/4. Each CTest row declares its own suffix and
// interface. Both are board inputs when all twelve rows observe the same ID.
void testP22Identity() {
  const auto variant = test::declaredVariant;
  uint8_t maximum = 0;
  CHECK(isValidVariant(variant) && strapAddressMax(variant, maximum) && maximum == 31);
  for (uint8_t phy : {uint8_t{0}, uint8_t{31}}) {
    Controller bus;
    Driver driver;
    auto config = bus.config(phy);
    config.interface = p22Interface;
    config.mdcHz = 25000000;
    CHECK(driver.bind(config).ok() && bus.callbacks == 0 && bus.clocks == 0);
    checkP22IdentityFence(bus, driver);
    bus.first[phy] = 0x2000;
    bus.second[phy] = 0xA240;
    auto output = sentinel();
    CHECK(driver.readIdentity(output, BUDGET).ok());
    literalTrace(bus, phy);
    CHECK(output.id1Valid && output.id2Valid && output.decodedValid && !output.stale);
    CHECK(output.id1 == 0x2000 && output.id2 == 0xA240 && output.oui == 0x080028);
    CHECK(output.model == 0x24 && output.revision == 0 && output.mode == OperatingMode::UNKNOWN);
    CHECK(output.acceptance == IdentityAcceptance::ACCEPTED && output.declaredVariant == variant);
    const auto calls = bus.callbacks, clocks = bus.clocks;
    CHECK(sameIdentity(driver.identitySnapshot(), output));
    const auto caps = driver.capabilities();
    CHECK(caps.valid && caps.declaredVariant == variant && caps.interfaceMask == 7);
    CHECK(caps.linkModeMask == 0x0F && caps.copper && !caps.gigabit);
    CHECK(caps.fiber == (variant == Variant::DP83822IF || variant == Variant::DP83822HF));
    CHECK(bus.callbacks == calls && bus.clocks == clocks);
    checkP22IdentityFence(bus, driver);
  }

  Controller bus;
  Driver driver;
  auto config = bus.config();
  config.interface = p22Interface;
  auto rejectConfig = [&](Config invalid) {
    CHECK(!driver.bind(invalid).ok() && bus.callbacks == 0 && bus.clocks == 0);
  };
  auto invalid = config;
  invalid.phyAddress = 32;
  rejectConfig(invalid);
  invalid = config;
  for (auto mode : {OperatingMode::BASIC, OperatingMode::ENHANCED, static_cast<OperatingMode>(0xFE)}) {
    invalid.requiredMode = mode;
    rejectConfig(invalid);
  }
  invalid = config;
  invalid.medium = Medium::FIBER;
  const auto fiber = driver.bind(invalid);
  if (variant == Variant::DP83822I || variant == Variant::DP83822H) {
    CHECK(fiber.is(Err::UNSUPPORTED) && fiber.detail == static_cast<int32_t>(UnsupportedDetail::MEDIUM));
  } else if (p22Interface == MacInterface::RGMII) {
    CHECK(fiber.is(Err::INVALID_CONFIG) && fiber.detail == static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
  } else {
    CHECK(fiber.is(Err::UNSUPPORTED) && fiber.detail == static_cast<int32_t>(UnsupportedDetail::FEATURE));
  }
  invalid.medium = static_cast<Medium>(0xFE);
  rejectConfig(invalid);
  invalid = config;
  invalid.interface = static_cast<MacInterface>(0xFE);
  rejectConfig(invalid);
  invalid = config;
  invalid.mdcHz = 25000001;
  rejectConfig(invalid);
  CHECK(driver.bind(config).ok());

  auto rejectId = [&](uint16_t first, uint16_t second, Err code, IdentityDetail detail) {
    bus.first[1] = first;
    bus.second[1] = second;
    auto output = sentinel();
    const auto calls = bus.callbacks, launches = bus.launches;
    const auto status = driver.readIdentity(output, BUDGET);
    CHECK(status.is(code) && status.detail == static_cast<int32_t>(detail));
    CHECK(bus.callbacks == calls + 1 && bus.launches == launches + 2);
    CHECK(bus.lastRequest.frameCount == 2 && bus.lastRequest.phyAddress == 1);
    CHECK(bus.lastRequest.frames[0].kind == FrameKind::READ && bus.lastRequest.frames[0].registerAddress == 2);
    CHECK(bus.lastRequest.frames[1].kind == FrameKind::READ && bus.lastRequest.frames[1].registerAddress == 3);
    CHECK(sameIdentity(output, sentinel()));
    const auto saved = driver.identitySnapshot();
    CHECK(saved.acceptance == IdentityAcceptance::REJECTED && saved.declaredVariant == variant);
    CHECK(saved.id1Valid && saved.id1 == first && saved.id2Valid && saved.id2 == second);
    noCapabilities(driver);
    checkP22IdentityFence(bus, driver);
  };
  rejectId(0, 0xA240, Err::DEVICE_NOT_FOUND, IdentityDetail::ZERO_ID);
  rejectId(0x2000, 0, Err::DEVICE_NOT_FOUND, IdentityDetail::ZERO_ID);
  rejectId(0xFFFF, 0xA240, Err::DEVICE_NOT_FOUND, IdentityDetail::ONES_ID);
  rejectId(0x2000, 0xFFFF, Err::DEVICE_NOT_FOUND, IdentityDetail::ONES_ID);
  rejectId(0x2001, 0xA240, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_OUI);
  rejectId(0x2000, 0xA640, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_OUI);
  rejectId(0x2000, 0xA111, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_MODEL);
  rejectId(0x2000, 0xA140, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_MODEL);
  for (uint16_t second = 0xA241; second <= 0xA24F; ++second) {
    rejectId(0x2000, second, Err::DEVICE_MISMATCH, IdentityDetail::REVISION_NOT_ACCEPTED);
  }

  bus.second[1] = 0xA240;
  bus.failAt = 1;
  auto output = sentinel();
  CHECK(driver.readIdentity(output, BUDGET).is(Err::MDIO_TIMEOUT) && sameIdentity(output, sentinel()));
  const auto partial = driver.identitySnapshot();
  CHECK(partial.id1Valid && partial.id1 == 0x2000 && !partial.id2Valid && partial.stale);
  CHECK(partial.acceptance == IdentityAcceptance::UNASSESSED && !partial.decodedValid);
  checkP22IdentityFence(bus, driver);
  bus.failAt = -1;
  CHECK(driver.readIdentity(output, BUDGET).ok() && output.declaredVariant == variant);
  checkP22IdentityFence(bus, driver);
}

void testP22Straps() {
  StrapEvidence sample;
  sample.sor1Valid = true;
  sample.sor2Valid = true;
  sample.stale = false;
  Dp83822Straps decoded{};
  const auto decode = [&](uint16_t sor1, uint16_t sor2) {
    sample.sor1 = sor1;
    sample.sor2 = sor2;
    CHECK(decodeDp83822Straps(sample, decoded));
  };

  // SNLS505H tables 8-84/85 give latch mode numbers; table 7-10 supplies pin functions.
  decode(0x0FC3, 0x0000);
  CHECK(decoded.phyAddress == 1 && decoded.medium == Medium::COPPER);
  CHECK(decoded.interface == MacInterface::MII && decoded.rmiiClock == RmiiClock::UNSPECIFIED);
  CHECK(decoded.autonegotiation && decoded.technologyMask == 0x0F && decoded.autoMdix);
  CHECK(!decoded.signalDetect && !decoded.eeeEnabled && !decoded.fastLinkDrop);
  CHECK(!decoded.led0TestMode && decoded.crsMode == 4);

  struct AddressCase { uint16_t sor1, sor2; uint8_t address; bool eee, fld; };
  const AddressCase addresses[] = {
    {0, 0, 0, false, false}, {0x0C00, 0, 1, false, false},
    {0x2000, 0, 2, false, false}, {0x3000, 0, 2, false, false},
    {0x4000, 0, 0, true, false}, {0x8000, 0, 4, true, false},
    {0xC000, 0, 4, false, false}, {0, 1, 0, false, true},
    {0, 2, 8, false, true}, {0, 3, 8, false, false},
    {0, 8, 16, false, false}, {0, 12, 16, false, false},
    {0xFC00, 0x000F, 31, false, false}
  };
  for (const auto& row : addresses) {
    decode(row.sor1, row.sor2);
    CHECK(decoded.phyAddress == row.address);
    CHECK(decoded.eeeEnabled == row.eee && decoded.fastLinkDrop == row.fld);
  }

  // Table 7-11: AN_EN and AN_1 use different pins from the two-bit LED_0 mode.
  struct LinkCase { uint16_t sor1, sor2; bool an; uint8_t mask; };
  const LinkCase copper[] = {
    {0x1000, 4, false, 1}, {0x1003, 4, false, 2},
    {0, 4, false, 4}, {3, 4, false, 8},
    {0x2000, 12, true, 1}, {0x2003, 12, true, 3},
    {0x3000, 12, true, 5}, {0x3003, 12, true, 15},
    {0x2003, 8, false, 2}, {0x3000, 8, false, 4}
  };
  for (const auto& row : copper) {
    decode(row.sor1, row.sor2);
    CHECK(decoded.medium == Medium::COPPER && decoded.autonegotiation == row.an);
    CHECK(decoded.technologyMask == row.mask && !decoded.led0TestMode);
  }
  // COL modes 2/3 select FX. AN_EN and AN_1 are don't-care; RX_ER now selects SD.
  decode(0x0400, 0);
  CHECK(decoded.medium == Medium::FIBER && decoded.phyAddress == 0);
  CHECK(!decoded.autonegotiation && decoded.technologyMask == 4);
  CHECK(!decoded.signalDetect && !decoded.autoMdix);
  decode(0x1B03, 4);
  CHECK(decoded.medium == Medium::FIBER && decoded.phyAddress == 1);
  CHECK(!decoded.autonegotiation && decoded.technologyMask == 8);
  CHECK(decoded.signalDetect && !decoded.autoMdix);

  // All eight rows of the RGMII_EN/RMII_EN/XI_50 truth table (7-13).
  struct InterfaceCase { uint16_t sor1; MacInterface interface; RmiiClock clock; bool autoMdix; };
  const InterfaceCase interfaces[] = {
    {0, MacInterface::MII, RmiiClock::UNSPECIFIED, false},
    {0x0010, MacInterface::UNKNOWN, RmiiClock::UNSPECIFIED, false},
    {0x0320, MacInterface::RMII, RmiiClock::LEADER_25MHZ, true},
    {0x0330, MacInterface::RMII, RmiiClock::FOLLOWER_50MHZ, true},
    {0x0100, MacInterface::RGMII, RmiiClock::UNSPECIFIED, false},
    {0x0110, MacInterface::UNKNOWN, RmiiClock::UNSPECIFIED, false},
    {0x0220, MacInterface::RGMII, RmiiClock::UNSPECIFIED, true},
    {0x0230, MacInterface::UNKNOWN, RmiiClock::UNSPECIFIED, true}
  };
  for (const auto& row : interfaces) {
    decode(row.sor1, 0);
    CHECK(decoded.interface == row.interface && decoded.rmiiClock == row.clock);
    CHECK(decoded.autoMdix == row.autoMdix && !decoded.signalDetect);
  }

  // Reserved high SOR2 and SOR1[3:2] do not describe a LED_1 strap.
  struct LedCase { uint16_t sor1; uint8_t crsMode; bool testMode; uint8_t mask; };
  const LedCase leds[] = {
    {0x000C, 1, false, 5}, {0x004D, 2, true, 0},
    {0x008E, 3, true, 0}, {0x00CF, 4, false, 15}
  };
  for (const auto& row : leds) {
    decode(row.sor1, 0xFFF0);
    CHECK(decoded.led0TestMode == row.testMode && decoded.crsMode == row.crsMode);
    CHECK(decoded.technologyMask == row.mask && decoded.phyAddress == 0);
  }

  // A partial or historical capture cannot replace the caller's previous decoded result.
  for (const bool stale : {false, true}) {
    for (const bool firstValid : {false, true}) {
      for (const bool secondValid : {false, true}) {
        if (!stale && firstValid && secondValid) { continue; }
        auto incomplete = sample;
        incomplete.stale = stale;
        incomplete.sor1Valid = firstValid;
        incomplete.sor2Valid = secondValid;
        incomplete.sor1 = 0xAB21;
        incomplete.sor2 = 0x000A;
        CHECK(!decodeDp83822Straps(incomplete, decoded));
        CHECK(decoded.phyAddress == 0 && decoded.medium == Medium::COPPER);
        CHECK(decoded.interface == MacInterface::MII && decoded.rmiiClock == RmiiClock::UNSPECIFIED);
        CHECK(decoded.technologyMask == 15 && decoded.autonegotiation && !decoded.autoMdix);
        CHECK(!decoded.signalDetect && !decoded.eeeEnabled && !decoded.fastLinkDrop);
        CHECK(!decoded.led0TestMode && decoded.crsMode == 4);
      }
    }
  }
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"p22_straps", testP22Straps}, {"p25", testP25Identity}, {"decode", testDecode},
    {"policy", testPolicy}, {"failures", testFailures},
    {"lifecycle", testLifecycle}, {"ownership", testOwnership}, {"scan", testScan}
  };
  if (argc == 4 && std::strcmp(argv[1], "p22_identity") == 0) {
    if (std::strcmp(argv[2], "I") == 0) test::declaredVariant = Variant::DP83822I;
    else if (std::strcmp(argv[2], "H") == 0) test::declaredVariant = Variant::DP83822H;
    else if (std::strcmp(argv[2], "IF") == 0) test::declaredVariant = Variant::DP83822IF;
    else if (std::strcmp(argv[2], "HF") == 0) test::declaredVariant = Variant::DP83822HF;
    else return 2;
    if (std::strcmp(argv[3], "MII") == 0) p22Interface = MacInterface::MII;
    else if (std::strcmp(argv[3], "RMII") == 0) p22Interface = MacInterface::RMII;
    else if (std::strcmp(argv[3], "RGMII") == 0) p22Interface = MacInterface::RGMII;
    else return 2;
    const test::Suite p22[] = {{"p22_identity", testP22Identity}};
    return test::runSuites(2, argv, p22);
  }
  return test::runVariantSuites(argc, argv, suites);
}
