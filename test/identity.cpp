#include "support/FakeController.h"

#include <array>
#include <cstdint>
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
    second[1] = 0xA111;
    first[2] = 0x2000;
    second[2] = 0xA131;
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
  CHECK(value.valid);
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
        bus.second[1] = mode == OperatingMode::BASIC ? 0xA111 : 0xA131;
        auto config = bus.config();
        config.interface = interface;
        config.requiredMode = requirement;
        CHECK(driver.bind(config).ok());
        noCapabilities(driver);
        CHECK(bus.callbacks == 0U && bus.clocks == 0U);
        auto output = sentinel();
        CHECK(driver.readIdentity(output, BUDGET).ok());
        CHECK(output.declaredVariant == Variant::DP83826I);
        CHECK(output.id1 == 0x2000U && output.id2 == bus.second[1]);
        CHECK(output.id1Valid && output.id2Valid && output.decodedValid);
        CHECK(output.oui == 0x080028U);
        CHECK(output.model == (mode == OperatingMode::BASIC ? 0x11U : 0x13U));
        CHECK(output.revision == 1U && output.mode == mode);
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
    bus.second[phy] = 0xA131;
    CHECK(driver.bind(bus.config(phy)).ok());
    DeviceIdentity output;
    CHECK(driver.readIdentity(output, BUDGET).ok());
    literalTrace(bus, static_cast<uint8_t>(phy));
  }
}

void testPolicy() {
  // Every raw revision and model code is checked independently of the production
  // accepted-revision table. A-generation revision 4 is deliberately rejected.
  for (uint16_t model = 0; model < 64U; ++model) {
    for (uint16_t revision = 0; revision < 16U; ++revision) {
      Controller bus;
      Driver driver;
      bus.second[1] = static_cast<uint16_t>(0xA000U | (model << 4U) | revision);
      CHECK(driver.bind(bus.config()).ok());
      auto output = sentinel();
      const auto status = driver.readIdentity(output, BUDGET);
      const bool modelAccepted = model == 0x11U || model == 0x13U;
      const bool accepted = modelAccepted && revision == 1U;
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
    {0, 0xA111, OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ZERO_ID},
    {0xFFFF, 0xA111, OperatingMode::UNKNOWN, Err::DEVICE_NOT_FOUND, IdentityDetail::ONES_ID},
    {0x2001, 0xA111, OperatingMode::UNKNOWN, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_OUI},
    {0x2000, 0xA511, OperatingMode::UNKNOWN, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_OUI},
    {0x2001, 0xA144, OperatingMode::ENHANCED, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_OUI},
    {0x2000, 0xA144, OperatingMode::ENHANCED, Err::DEVICE_MISMATCH, IdentityDetail::WRONG_MODEL},
    {0x2000, 0xA114, OperatingMode::ENHANCED, Err::DEVICE_MISMATCH, IdentityDetail::REVISION_NOT_ACCEPTED},
    {0x2000, 0xA111, OperatingMode::ENHANCED, Err::DEVICE_MISMATCH, IdentityDetail::MODE_MISMATCH},
    {0x2000, 0xA131, OperatingMode::BASIC, Err::DEVICE_MISMATCH, IdentityDetail::MODE_MISMATCH},
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

void testFailures() {
  for (int phase = 0; phase < 2; ++phase) {
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
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
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
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
    Controller bus;
    Driver driver;
    CHECK(driver.bind(bus.config()).ok());
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
  Controller bus;
  Driver driver;
  CHECK(driver.bind(bus.config()).ok());
  bus.expireAfter = 1;
  auto output = sentinel();
  CHECK(driver.readIdentity(output, {150, 2}).is(Err::OPERATION_TIMEOUT));
  CHECK(sameIdentity(output, sentinel()));
  CHECK(driver.identitySnapshot().id1Valid && driver.identitySnapshot().id2Valid);
  noCapabilities(driver);
  // Two frames need fixed(9)+frames(10)+clock lag(2) strictly before deadline.
  for (const uint64_t deadline : {uint64_t{121}, uint64_t{122}}) {
    Controller edge;
    Driver edgeDriver;
    CHECK(edgeDriver.bind(edge.config()).ok());
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
  for (const auto variant : {Variant::UNKNOWN, static_cast<Variant>(2), static_cast<Variant>(255)}) {
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
  bus.second[1] = 0xA131;
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
  bus.second[1] = 0xA111;
  CHECK(driver.readIdentity(output, BUDGET).ok());
  CHECK(driver.writeClause22(0x18, 0x12, {10000, 1}).ok());
  bus.second[1] = 0xA140;
  CHECK(driver.readIdentity(output, BUDGET).is(Err::DEVICE_MISMATCH));
  CHECK(driver.bind(bus.config(1)).ok());
  CHECK(driver.writeClause22(0x18, 0x12, {10000, 1}).ok()); // Fresh owner attestation is explicit.

  // Re-reading identical words does not make unrelated collected samples stale.
  Controller changed;
  Driver observed;
  CHECK(observed.bind(changed.config()).ok());
  CHECK(observed.readIdentity(output, BUDGET).ok());
  EvidenceSnapshot evidence;
  CHECK(observed.acquireEvidence(evidenceBit(EvidenceSource::BMSR), evidence, {10000, 1}).ok());
  CHECK(!observed.evidenceSnapshot().samples[1].stale);
  CHECK(observed.readIdentity(output, BUDGET).ok());
  CHECK(!observed.evidenceSnapshot().samples[1].stale);
  CHECK(!observed.evidenceSnapshot().configurationDirty);
  changed.second[1] = 0xA131;
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
  CHECK(second.bind(bus.config(2)).ok());
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
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"decode", testDecode}, {"policy", testPolicy}, {"failures", testFailures},
    {"lifecycle", testLifecycle}, {"ownership", testOwnership}
  };
  return test::runSuites(argc, argv, suites);
}
