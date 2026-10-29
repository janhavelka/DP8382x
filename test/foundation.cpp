#include <DP8382x/CommandTable.h>
#include <DP8382x/Config.h>
#include <DP8382x/Status.h>

#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <type_traits>

namespace {
using namespace DP8382x;
int failures = 0;
unsigned checks = 0;

// Deliberately independent of assert(), so Release/NDEBUG cannot erase checks.
void check(bool condition, const char* expression, int line) {
  ++checks;
  if (!condition) {
    std::fprintf(stderr, "line %d: %s\n", line, expression);
    ++failures;
  }
}
#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

bool sameSettings(const LinkSettings& a, const LinkSettings& b) {
  return a.mode == b.mode &&
      a.advertisement.technologyMask == b.advertisement.technologyMask &&
      a.advertisement.symmetricPause == b.advertisement.symmetricPause &&
      a.advertisement.asymmetricPause == b.advertisement.asymmetricPause &&
      a.advertisement.nextPage == b.advertisement.nextPage &&
      a.forced.speed == b.forced.speed && a.forced.duplex == b.forced.duplex &&
      a.crossover.pairSelection == b.crossover.pairSelection &&
      a.crossover.robust == b.crossover.robust &&
      a.crossover.swapPair == b.crossover.swapPair &&
      a.crossover.swapPolarity == b.crossover.swapPolarity &&
      a.extendedFullDuplex == b.extendedFullDuplex;
}

void expectLink(const LinkSettings& input, Err expected) {
  const LinkSettings before = input;
  const Status result = validateLinkSettings(input);
  CHECK(result.code == expected);
  const int32_t detail = expected == Err::OK ? 0 :
      expected == Err::INVALID_PARAM ? static_cast<int32_t>(ParamDetail::ENUM_VALUE) :
      static_cast<int32_t>(ConfigDetail::LINK_COMBINATION);
  CHECK(result.detail == detail);
  CHECK(result.msg != nullptr);
  CHECK(sameSettings(input, before));
}

void testStatus() {
  constexpr Status success = Status::Ok();
  static_assert(success.ok() && success.is(Err::OK), "constexpr helpers");
  const Status initial;
  CHECK(initial.ok());
  CHECK(initial.detail == 0);
  CHECK(initial.msg != nullptr);
  const char* const names[] = {
    "OK", "IN_PROGRESS", "INVALID_PARAM", "INVALID_CONFIG", "NOT_BOUND",
    "NOT_INITIALIZED", "UNSUPPORTED", "DEVICE_NOT_FOUND", "DEVICE_MISMATCH",
    "MDIO_ERROR", "MDIO_TIMEOUT", "OPERATION_TIMEOUT", "CONFIG_MISMATCH",
    "BUSY", "CANCELLED", "RESULT_NOT_AVAILABLE", "TOKEN_MISMATCH"
  };
  for (unsigned raw = 0; raw < 256U; ++raw) {
    const Err code = static_cast<Err>(raw);
    const Status status{code, 0, "static diagnostic"};
    CHECK(status.ok() == (raw == 0U));
    CHECK(static_cast<bool>(status) == (raw == 0U));
    CHECK(status.inProgress() == (raw == 1U));
    CHECK(status.is(code)); // Equality also works for an invalid representation.
    CHECK(std::strcmp(errorName(code), raw < 17U ? names[raw] : "UNKNOWN") == 0);
  }
  static const char message[] = "backend diagnostic";
  for (int32_t detail : {std::numeric_limits<int32_t>::min(), int32_t{-1},
                         int32_t{0}, std::numeric_limits<int32_t>::max()}) {
    const Status status = Status::Error(Err::MDIO_ERROR, message, detail);
    const Status copy = status;
    CHECK(copy.code == Err::MDIO_ERROR);
    CHECK(copy.detail == detail);
    CHECK(copy.msg == message);
    CHECK(!copy.ok());
  }
}

void testLink() {
  CHECK(!validateLinkSettings(LinkSettings{}).ok());
  // Every possible mask is checked against literal protocol-independent API bits.
  // Every pause/next-page/swap/extended intent remains representable for AN.
  for (unsigned mask = 0; mask < 256U; ++mask) {
    for (unsigned flags = 0; flags < 64U; ++flags) {
      LinkSettings settings;
      settings.advertisement.technologyMask = static_cast<uint8_t>(mask);
      settings.advertisement.symmetricPause = (flags & 1U) != 0U;
      settings.advertisement.asymmetricPause = (flags & 2U) != 0U;
      settings.advertisement.nextPage = (flags & 4U) != 0U;
      settings.crossover.swapPair = (flags & 8U) != 0U;
      settings.crossover.swapPolarity = (flags & 16U) != 0U;
      settings.extendedFullDuplex = (flags & 32U) != 0U;
      expectLink(settings, mask >= 1U && mask <= 15U ? Err::OK : Err::INVALID_CONFIG);
    }
  }
  // Cover all known enum values and forced cross-field constraints independently.
  for (unsigned speed = 0; speed < 3U; ++speed) {
    for (unsigned duplex = 0; duplex < 3U; ++duplex) {
      for (unsigned pair = 0; pair < 3U; ++pair) {
        for (unsigned flags = 0; flags < 256U; ++flags) {
          LinkSettings settings;
          settings.mode = LinkMode::FORCED;
          settings.forced.speed = static_cast<LinkSpeed>(speed);
          settings.forced.duplex = static_cast<Duplex>(duplex);
          settings.crossover.pairSelection = static_cast<PairSelection>(pair);
          settings.crossover.robust = (flags & 1U) != 0U;
          settings.advertisement.technologyMask = (flags & 2U) != 0U ? 8U : 0U;
          settings.advertisement.symmetricPause = (flags & 4U) != 0U;
          settings.advertisement.asymmetricPause = (flags & 8U) != 0U;
          settings.advertisement.nextPage = (flags & 16U) != 0U;
          settings.crossover.swapPair = (flags & 32U) != 0U;
          settings.crossover.swapPolarity = (flags & 64U) != 0U;
          settings.extendedFullDuplex = (flags & 128U) != 0U;
          const bool valid = speed != 0U && duplex != 0U && (flags & 31U) == 1U;
          expectLink(settings, valid ? Err::OK : Err::INVALID_CONFIG);
          // Known inactive forced values and either robust policy are legal in AN.
          settings.mode = LinkMode::AUTONEGOTIATED;
          settings.advertisement.technologyMask = 8U;
          expectLink(settings, Err::OK);
        }
      }
    }
  }
  for (unsigned raw = 0; raw < 256U; ++raw) {
    LinkSettings settings;
    settings.advertisement.technologyMask = 8U;
    settings.mode = static_cast<LinkMode>(raw);
    if (raw > 1U) expectLink(settings, Err::INVALID_PARAM);
    settings.mode = LinkMode::AUTONEGOTIATED;
    settings.forced.speed = static_cast<LinkSpeed>(raw);
    if (raw > 2U) expectLink(settings, Err::INVALID_PARAM);
    settings.forced.speed = LinkSpeed::UNKNOWN;
    settings.forced.duplex = static_cast<Duplex>(raw);
    if (raw > 2U) expectLink(settings, Err::INVALID_PARAM);
    settings.forced.duplex = Duplex::UNKNOWN;
    settings.crossover.pairSelection = static_cast<PairSelection>(raw);
    if (raw > 2U) expectLink(settings, Err::INVALID_PARAM);
    // Enum errors take precedence even when another combination is illegal.
    settings.advertisement.technologyMask = 0U;
    if (raw > 2U) expectLink(settings, Err::INVALID_PARAM);
  }
  for (unsigned raw = 0; raw < 256U; ++raw) {
    CHECK(isValidVariant(static_cast<Variant>(raw)) == (raw == 1U));
  }
  for (uint32_t address = 0; address < 64U; ++address) {
    CHECK(isValidPhyAddress(address) == (address < 32U));
  }
  CHECK(!isValidPhyAddress(256U));
  CHECK(!isValidPhyAddress(std::numeric_limits<uint32_t>::max()));
}

void testMasks() {
  using namespace DP8382x::cmd;
  // Literal mixed command/state observations must not replay W1S bits.
  const uint16_t observedBmcr = 0xB300;
  CHECK((observedBmcr & MASK_BMCR_COMMAND) == 0x8200U);
  CHECK((observedBmcr & MASK_BMCR_PERSISTENT) == 0x3100U);
  CHECK((MASK_BMCR_COMMAND & MASK_BMCR_PERSISTENT) == 0U);
  CHECK((observedBmcr & MASK_BMCR_AN_ENABLE) != 0U);
  CHECK((0xA111U & MASK_PHYIDR2_MODEL) >> 4U == 0x11U);
  CHECK((0xA131U & MASK_PHYIDR2_MODEL) >> 4U == 0x13U);
  CHECK((0xA13FU & MASK_PHYIDR2_REVISION) == 15U);
  const uint16_t advertisement = 0x0D01;
  CHECK((advertisement & MASK_ANAR_TECHNOLOGY) == 0x0100U);
  CHECK((advertisement & MASK_ANAR_PAUSE) == 0x0C00U);
  CHECK((advertisement & MASK_ANAR_SELECTOR) == 1U);
  CHECK((0xC003U & MASK_PHYRCR_COMMAND) == 0xC000U);
  CHECK((MASK_PHYRCR_HARD_RESET | MASK_PHYRCR_CIRCUITRY_RESTART) == 0xC000U);
  CHECK((0x7849U & MASK_BMSR_LINK) == 0U);
  CHECK((0x784DU & MASK_BMSR_LINK) != 0U);
}

void testPassive() {
  static_assert(std::is_trivially_copyable<Status>::value, "Status is a passive value");
  static_assert(std::is_trivially_copyable<Config>::value, "Config copies borrowed handles");
  const Config config;
  CHECK(config.transport.context == nullptr);
  CHECK(config.transport.executeBatch == nullptr);
  CHECK(config.transport.nowUs == nullptr);
  CHECK(config.transport.bounds.frameUs == 0U);
  CHECK(config.transport.bounds.clockLagUs == 0U);
  const BatchResult result;
  CHECK(result.status.is(Err::MDIO_ERROR));
  CHECK(result.releaseStatus.is(Err::MDIO_ERROR));
  CHECK(!result.controllerReusable);
  for (const FrameResult& frame : result.frames) {
    CHECK(!frame.attempted && !frame.completed && !frame.valueValid);
    CHECK(!frame.effectPossible && !frame.timeValid);
  }
  const MdioBatch batch;
  CHECK(batch.frameCount == 0U);
  for (const MdioFrame& frame : batch.frames) {
    CHECK(frame.kind == FrameKind::READ);
    CHECK(frame.readIndex == 0xFFU);
  }
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  if (std::strcmp(argv[1], "status") == 0) testStatus();
  else if (std::strcmp(argv[1], "link") == 0) testLink();
  else if (std::strcmp(argv[1], "masks") == 0) testMasks();
  else if (std::strcmp(argv[1], "passive") == 0) testPassive();
  else if (std::strcmp(argv[1], "--verify-failure-path") == 0) CHECK(false);
  else return 2;
  std::printf("%u checks; %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
