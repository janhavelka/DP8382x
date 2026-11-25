/// @file Dp8382xCli.cpp
/// @brief Strict line dispatch and bounded, correlated output.
#include "Dp8382xCli.h"
#include <DP8382x/CommandTable.h>
#include <DP8382x/Version.h>
#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <limits>

namespace DP8382x::Cli {
namespace {
constexpr std::size_t TOKEN_BYTES = 64;
constexpr char PREFIX[] = "@DP8382X ";
constexpr bool separator(char ch) { return ch == ' ' || ch == '\t'; }
enum Route : unsigned { PROBE, SCAN, VALIDATE, APPLY, VERIFY, INVALIDATE, READ, WRITE, MODIFY, BMCR, DUMP };
enum Option : unsigned { COLOR, QUIET, MODE, MODES, PAUSE, PAIR, SPEED, DUPLEX, FLAGS };
enum Lifecycle : unsigned { INITIALIZE, RESET, RESTART, REPLAY, OWNER_START, END };
enum Owner : unsigned { OWNER_POLL, OWNER_CANCEL, OWNER_RESULT, OWNER_ABANDON };
enum Link : unsigned { LINK_READ, LINK_CACHED, LINK_COLLECT };
enum Run : unsigned { WATCH, STRESS, STOP, STATS };
constexpr uint32_t MAX_ROWS = 32;
constexpr uint64_t DIRECT_US = 2000000;
constexpr uint64_t CONFIG_US = 5000000;

bool safeValue(std::string_view value) {
  if (value.empty() || value.size() > TOKEN_BYTES) return false;
  for (const char c : value) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '_' || c == '.' || c == ':' || c == '-') continue;
    return false;
  }
  return true;
}

bool safeString(const char* value) {
  if (!value) return false;
  std::size_t count = 0;
  while (count <= TOKEN_BYTES && value[count] != '\0') ++count;
  return count <= TOKEN_BYTES && safeValue(std::string_view(value, count));
}

template<class UInt>
bool unsignedNumber(std::string_view text, UInt& result) {
  uint32_t base = 10;
  if (text.substr(0, 2) == "0x" || text.substr(0, 2) == "0X") {
    text.remove_prefix(2);
    base = 16;
  }
  if (text.empty()) return false;
  UInt value = 0;
  for (const char c : text) {
    uint32_t digit = base;
    if (c >= '0' && c <= '9') digit = static_cast<uint32_t>(c - '0');
    if (c >= 'A' && c <= 'F') digit = static_cast<uint32_t>(c - 'A') + 10U;
    if (c >= 'a' && c <= 'f') digit = static_cast<uint32_t>(c - 'a') + 10U;
    if (digit >= base || value > (std::numeric_limits<UInt>::max() - digit) / base) return false;
    value = value * base + digit;
  }
  result = value;
  return true;
}

int choice(std::string_view choices, std::string_view value) {
  int index = 0;
  while (!choices.empty()) {
    const auto delimiter = choices.find('|');
    if (choices.substr(0, delimiter) == value) return index;
    if (delimiter == std::string_view::npos) break;
    choices.remove_prefix(delimiter + 1);
    ++index;
  }
  return -1;
}

const char* interfaceName(MacInterface value) {
  return value == MacInterface::RMII ? "RMII" : "MII";
}

const char* variantName(Variant variant) {
  switch (variant) {
    case Variant::DP83826I: return "DP83826I";
    case Variant::DP83826E: return "DP83826E";
    case Variant::DP83826AI: return "DP83826AI";
    case Variant::DP83826AE: return "DP83826AE";
    case Variant::DP83825I: return "DP83825I";
    default: return "unknown";
  }
}

const char* trustName(ConfigurationTrust trust) {
  switch (trust) {
    case ConfigurationTrust::VERIFIED: return "VERIFIED";
    case ConfigurationTrust::DIRTY: return "DIRTY";
    case ConfigurationTrust::UNKNOWN: return "UNKNOWN";
  }
  return "UNKNOWN";
}

unsigned speedMbps(LinkSpeed speed) {
  if (speed == LinkSpeed::MBPS_10) return 10;
  if (speed == LinkSpeed::MBPS_100) return 100;
  return 0;
}

const char* duplexName(Duplex duplex) {
  if (duplex == Duplex::FULL) return "full";
  if (duplex == Duplex::HALF) return "half";
  return "unknown";
}

const char* negotiationName(NegotiationState value) {
  switch (value) {
    case NegotiationState::DISABLED: return "disabled";
    case NegotiationState::IN_PROGRESS: return "in_progress";
    case NegotiationState::COMPLETE: return "complete";
    case NegotiationState::UNKNOWN: return "unknown";
  }
  return "unknown";
}

constexpr std::string_view RESET_KINDS = "none|pcs|hard|restart";

const char* resetName(ResetKind reset) {
  switch (reset) {
    case ResetKind::NONE: return "none";
    case ResetKind::PCS: return "pcs";
    case ResetKind::HARD: return "hard";
    case ResetKind::RESTART: return "restart";
  }
  return "unknown";
}

uint32_t resolvedTechnology(const LinkSnapshot& link) {
  if (link.duplex != Duplex::FULL && link.duplex != Duplex::HALF) return 0;
  if (link.speed == LinkSpeed::MBPS_10) return link.duplex == Duplex::FULL ? LINK_10_FULL : LINK_10_HALF;
  if (link.speed == LinkSpeed::MBPS_100) return link.duplex == Duplex::FULL ? LINK_100_FULL : LINK_100_HALF;
  return 0;
}

const char* observationName(Status status, const LinkSnapshot& link, uint32_t mode, uint32_t policy) {
  if (status.is(Err::MDIO_ERROR) || status.is(Err::MDIO_TIMEOUT)) return "management_failure";
  if (!status.ok()) return "unavailable";
  if (!link.valid) return "unobserved";
  if (link.stale) return "stale";
  if (!link.linkUp) return "down";
  if (link.negotiation == NegotiationState::IN_PROGRESS) return "pending";
  if (mode == 0) return "unresolved";
  if (policy && (mode & policy) == 0) return "wrong_policy";
  return "up";
}

bool pageList(std::string_view text, uint32_t limit, ConfigureOptions& options) {
  if (limit == 0 || limit > MAX_NEXT_PAGES || text.empty() || text.size() % 4 != 0 ||
      text.size() / 4 > MAX_NEXT_PAGES) return false;
  auto& pages = options.nextPages;
  constexpr uint32_t mask = cmd::MASK_ANNPTR_MESSAGE | cmd::MASK_ANNPTR_ACK2 | cmd::MASK_ANNPTR_CODE;
  pages.maxPartnerPages = static_cast<uint8_t>(limit);
  while (!text.empty()) {
    char hex[] = "0x0000";
    std::copy_n(text.begin(), 4, hex + 2);
    uint32_t word = 0;
    if (!unsignedNumber(hex, word) || (word & ~mask)) return false;
    pages.pages[pages.pageCount++] = {static_cast<uint16_t>(word & cmd::MASK_ANNPTR_CODE),
        (word & cmd::MASK_ANNPTR_MESSAGE) != 0, (word & cmd::MASK_ANNPTR_ACK2) != 0};
    text.remove_prefix(4);
  }
  return true;
}

const char* pairName(PairSelection pair) {
  switch (pair) {
    case PairSelection::AUTOMATIC: return "auto";
    case PairSelection::MDI: return "mdi";
    case PairSelection::MDIX: return "mdix";
  }
  return "unknown";
}

unsigned profileFlags(const LinkSettings& profile) {
  return unsigned(profile.crossover.robust) | (unsigned(profile.crossover.swapPair) << 1U) |
      (unsigned(profile.extendedFullDuplex) << 2U);
}

bool validTerminal(Status status) {
  return status.msg && !status.inProgress() && std::strcmp(errorName(status.code), "UNKNOWN") != 0;
}

Status checkedTerminal(Status status) {
  if (!validTerminal(status))
    return Status::Error(Err::INVALID_CONFIG, "Invalid CLI owner result", status.detail);
  return status;
}

Status timeout(TimeoutDetail detail) {
  return Status::Error(Err::OPERATION_TIMEOUT, "CLI work allowance expired", static_cast<int32_t>(detail));
}

Status clockError() {
  return Status::Error(Err::INVALID_CONFIG, "CLI clock regressed",
                       static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
}

bool isClockError(Status status) {
  return status.is(Err::INVALID_CONFIG) && status.detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT);
}

bool safeRegisterWidth(bool mmd, uint32_t value) { return value <= (mmd ? 65535U : 31U); }

bool isWrite(Action action) {
  return action == Action::WRITE_C22 || action == Action::WRITE_MMD ||
      action == Action::MODIFY_MMD || action == Action::MODIFY_BMCR;
}

const char* effect(Action action, Status status, const Receipt& receipt) {
  if (status.ok()) return "applied";
  if (!receipt.batchValid) return "unknown";
  if (receipt.violation != TransportViolation::NONE) return "possible";
  std::size_t target = 0;
  if (action == Action::WRITE_MMD) target = 3;
  if (action == Action::MODIFY_MMD) target = 4;
  if (action == Action::MODIFY_BMCR) target = 1;
  const auto& frame = receipt.batch.frames[target];
  if (frame.completed) return "applied";
  return frame.effectPossible ? "possible" : "none";
}
} // namespace

bool featureNumber(std::string_view token, uint32_t maximum, uint32_t& value) noexcept {
  uint32_t parsed = 0;
  if (!unsignedNumber(token, parsed) || parsed > maximum) return false;
  value = parsed;
  return true;
}

bool featureNumber(std::string_view token, uint64_t& value) noexcept { return unsignedNumber(token, value); }

int featureChoice(std::string_view token, std::string_view choices) noexcept { return choice(choices, token); }

struct Processor::Reply {
  Status status{};
  const char* reason{"none"};
  const char* type{"result"};
  const char* state{"terminal"};
  uint64_t op{0};
  uint64_t deadline{0};
  bool deferred{false};
  // Base record fields fit within the remaining 384 bytes, including CRLF.
  std::array<char, 640> payload{};

  void reject(const char* why) {
    status = Status::Error(Err::INVALID_PARAM, "Invalid CLI input");
    reason = why;
  }

  void checkFeatureFields(const FeatureFields& report) {
    if (report.valid) return;
    if (status.ok()) status = Status::Error(Err::INVALID_CONFIG, "Feature field capacity exceeded");
    reason = "format";
  }

  template<class... Args>
  void fields(const char* format, Args... args) {
    payload[0] = '\0';
    appendFields(format, args...);
  }

  template<class... Args>
  void appendFields(const char* format, Args... args) {
    const std::size_t used = std::strlen(payload.data());
    const int count = std::snprintf(payload.data() + used, payload.size() - used, format, args...);
    if (count < 0 || static_cast<std::size_t>(count) >= payload.size() - used) {
      payload[0] = '\0';
      status = Status::Error(Err::INVALID_CONFIG, "CLI payload capacity exceeded");
      reason = "format";
      type = "result";
    }
  }

  void identityFields(const DeviceIdentity& id) {
    const char* mode = "unknown";
    if (id.mode == OperatingMode::BASIC) mode = "basic";
    if (id.mode == OperatingMode::ENHANCED) mode = "enhanced";
    const char* acceptance = "unassessed";
    if (id.acceptance == IdentityAcceptance::ACCEPTED) acceptance = "accepted";
    if (id.acceptance == IdentityAcceptance::REJECTED) acceptance = "rejected";
    fields(" id1_valid=%u id1=%u id2_valid=%u id2=%u decoded_valid=%u"
           " oui=%" PRIu32 " model=%u revision=%u mode=%s identity_state=%s stale=%u"
           " identity_status=%s identity_detail=%" PRId32,
           unsigned(id.id1Valid), unsigned(id.id1), unsigned(id.id2Valid), unsigned(id.id2),
           unsigned(id.decodedValid), id.oui, unsigned(id.model), unsigned(id.revision),
           mode, acceptance, unsigned(id.stale), errorName(id.admissionStatus.code), id.admissionStatus.detail);
  }

  void linkFields(const LinkSnapshot& link, uint32_t policy, uint32_t round = 0) {
    const bool current = link.valid && !link.stale;
    const bool up = current && link.linkUp;
    const bool modeValid = up && link.resolvedModeValid && link.negotiation != NegotiationState::IN_PROGRESS;
    const uint32_t mode = modeValid ? resolvedTechnology(link) : 0;
    const bool resolved = mode != 0;
    const bool pause = resolved && link.pauseValid;
    const bool advertised = current && (link.rawBmcr & cmd::MASK_BMCR_AN_ENABLE) != 0U &&
        (link.rawAnar & cmd::MASK_ANAR_SELECTOR) == 1U;
    const bool partner = advertised && up && link.negotiation == NegotiationState::COMPLETE &&
        (link.rawAner & (cmd::MASK_ANER_PARTNER_AN | cmd::MASK_ANER_PARALLEL_FAULT)) == cmd::MASK_ANER_PARTNER_AN &&
        (link.rawAnlpar & cmd::MASK_ALNPAR_SELECTOR) == 1U;
    fields(" observation=%s valid=%u stale=%u up=%u resolved=%u speed_mbps=%u duplex=%s negotiation=%s"
           " pause_valid=%u pause_rx=%u pause_tx=%u observed_valid=%u observed_us=%" PRIu64
           " policy_mask=%" PRIu32 " advertised_valid=%u advertised_modes=%u partner_valid=%u partner_modes=%u"
           " bmsr_first=%u bmsr_current=%u anar=%u anlpar=%u aner=%u last_status=%s last_detail=%" PRId32
           " round=%" PRIu32,
           observationName(status, link, mode, policy), unsigned(link.valid), unsigned(link.stale), unsigned(up), unsigned(resolved),
           resolved ? speedMbps(link.speed) : 0, resolved ? duplexName(link.duplex) : "unknown",
           negotiationName(current ? link.negotiation : NegotiationState::UNKNOWN),
           unsigned(pause), unsigned(pause && link.pauseRx), unsigned(pause && link.pauseTx),
           unsigned(link.timeValid), link.timeValid ? link.observedAtUs : 0, policy,
           unsigned(advertised), advertised ? (unsigned(link.rawAnar) >> 5U) & LINK_TECHNOLOGY_MASK : 0U,
           unsigned(partner), partner ? (unsigned(link.rawAnlpar) >> 5U) & LINK_TECHNOLOGY_MASK : 0U,
           unsigned(link.rawBmsrFirst), unsigned(link.rawBmsrCurrent), unsigned(link.rawAnar),
           unsigned(link.rawAnlpar), unsigned(link.rawAner), errorName(link.lastRefreshStatus.code),
           link.lastRefreshStatus.detail, round);
  }

  void eventFields(const CounterSnapshot& counters, uint8_t mask, const std::array<uint16_t, 6>& words) {
    fields(" valid=%u stale=%u event_mask=%u physts=%u flds=%u misr1=%u misr2=%u fcscr=%u recr=%u"
           " false_carrier_total=%" PRIu64 " lower_bound=%u saturated=%u observed_valid=%u observed_us=%" PRIu64
           " last_status=%s last_detail=%" PRId32,
           unsigned(counters.valid), unsigned(counters.stale), unsigned(mask), unsigned(words[0]),
           unsigned(words[1]), unsigned(words[2]), unsigned(words[3]), unsigned(words[4]), unsigned(words[5]),
           counters.falseCarrierTotal, unsigned(counters.falseCarrierLowerBound),
           unsigned(counters.falseCarrierOverflow), unsigned(counters.timeValid),
           counters.timeValid ? counters.observedAtUs : 0, errorName(counters.lastStatus.code), counters.lastStatus.detail);
  }
};

struct Processor::Command {
  std::string_view name;
  std::string_view alias; // Alternatives separated by |.
  std::size_t minArgs;
  std::size_t maxArgs;
  unsigned flags; // Handler's snapshot selector or local option.
  void (Processor::*handler)(const Command&, const Args&, std::size_t, Reply&);
  const char* usage;
  const char* summary;
};

struct Processor::ConfigOption {
  std::string_view group;
  std::string_view name;
  std::string_view enumValues;
  uint32_t min;
  uint32_t max;
  const char* unit;
  bool Processor::*setting;
};

const Processor::ConfigOption Processor::CONFIG[] = {
    {"local", "color", "off|on", 0, 0, "mode", &Processor::_color},
    {"local", "quiet", "", 0, 1, "flag", &Processor::_quiet},
    {"configuration", "mode", "an|forced", 0, 0, "", nullptr},
    {"configuration", "modes", "", 1, LINK_TECHNOLOGY_MASK, "mask", nullptr},
    {"configuration", "pause", "none|symmetric|asymmetric|both", 0, 0, "", nullptr},
    {"configuration", "pair", "auto|mdi|mdix", 0, 0, "", nullptr},
    {"configuration", "speed", "10|100", 0, 0, "Mbit/s", nullptr},
    {"configuration", "duplex", "half|full", 0, 0, "", nullptr},
    {"configuration", "flags", "", 0, 7, "mask", nullptr},
};

const Processor::Command Processor::COMMANDS[] = {
    {"help", "?", 0, 1, 0, &Processor::help, "[topic]", "Command help"},
    {"version", "ver", 0, 0, 0, &Processor::identity, "", "Library and firmware identity"},
    {"session", "", 1, 1, 0, &Processor::identity, "<nonce>", "Correlate a session"},
    {"capabilities", "caps", 0, 0, unsigned(SnapshotKind::CAPABILITIES), &Processor::cached, "", "Cached capabilities"},
    {"local board", "", 0, 0, 0, &Processor::identity, "", "Declared board and backend"},
    {"local color", "", 0, 1, COLOR, &Processor::configure, "", "Set or show ANSI style"},
    {"local quiet", "", 0, 1, QUIET, &Processor::configure, "", "Set or show human-output filter"},
    {"local console", "", 0, 0, 0, &Processor::console, "", "Console backpressure counters"},
    {"application status", "", 0, 0, unsigned(ApplicationCommand::STATUS), &Processor::application, "", "Cached application evidence"},
    {"application start", "", 0, 0, unsigned(ApplicationCommand::START), &Processor::application, "", "Start with verified settings"},
    {"application stop", "", 0, 0, unsigned(ApplicationCommand::STOP), &Processor::application, "", "Stop and enter maintenance"},
    {"application admit", "", 0, 0, unsigned(ApplicationCommand::ADMIT), &Processor::application, "", "Admit a raw frame session"},
    {"application release", "", 0, 0, unsigned(ApplicationCommand::RELEASE), &Processor::application, "", "Release a raw frame session"},
    {"application send", "", 0, 0, unsigned(ApplicationCommand::SEND), &Processor::application, "", "Queue the opt-in test frame"},
    {"cached health", "drv|health", 0, 0, unsigned(SnapshotKind::HEALTH), &Processor::cached, "", "Managed MDIO health"},
    {"cached link", "", 0, 0, unsigned(SnapshotKind::LINK), &Processor::cached, "", "Last link and resolution"},
    {"cached settings", "cfg|settings", 0, 0, unsigned(SnapshotKind::SETTINGS), &Processor::cached, "", "Configuration intent and trust"},
    {"cached operation", "", 0, 0, unsigned(SnapshotKind::OPERATION), &Processor::cached, "", "Operation state and restoration debt"},
    {"cached transport", "", 0, 0, unsigned(SnapshotKind::TRANSPORT), &Processor::cached, "", "MDIO batch and frame totals"},
    {"cached events", "", 0, 0, unsigned(SnapshotKind::EVENTS), &Processor::cached, "", "Stored events and chip counters"},
    {"connection identity", "", 0, 0, unsigned(SnapshotKind::IDENTITY), &Processor::cached, "", "Last accepted or rejected identity"},
    {"connection probe", "", 0, 0, PROBE, &Processor::route, "", "Acquire bound identity"},
    {"connection discover", "", 0, 2, SCAN, &Processor::route, "[first last]", "Read raw IDs, never select address"},
    {"connection initialize", "init|begin", 0, 1, INITIALIZE, &Processor::lifecycle, "[none|pcs|hard|restart]", "Initialize and replay owned settings"},
    {"connection end", "", 0, 0, END, &Processor::lifecycle, "", "Release through the application owner"},
    {"configuration desired", "", 0, 0, unsigned(SnapshotKind::SETTINGS), &Processor::cached, "", "Desired profile"},
    {"configuration observed", "", 0, 0, unsigned(SnapshotKind::SETTINGS), &Processor::cached, "", "Observed profile and trust"},
    {"configuration validate", "", 5, 7, VALIDATE, &Processor::route, "<profile>", "Check without changing hardware"},
    {"configuration apply", "", 5, 7, APPLY, &Processor::route, "<profile>", "Apply under maintenance admission"},
    {"configuration verify", "", 0, 0, VERIFY, &Processor::route, "", "Read back owned configuration"},
    {"configuration invalidate", "", 0, 0, INVALIDATE, &Processor::route, "", "Mark externally changed without MDIO"},
    {"registers read", "", 2, 3, READ, &Processor::route, "c22 reg | mmd devad reg", "Read safe register"},
    {"registers write", "", 3, 4, WRITE, &Processor::route, "c22 reg value | mmd devad reg value", "Maintenance LEDCR write"},
    {"registers modify", "", 4, 4, MODIFY, &Processor::route, "<devad> <reg> <mask> <value>", "Atomic vendor LEDCR RMW"},
    {"registers bmcr", "", 2, 2, BMCR, &Processor::route, "<mask> <value>", "Stopped AN/speed/duplex maintenance"},
    {"registers dump", "", 3, 4, DUMP, &Processor::route, "c22 first last | mmd devad first last", "Up to 32 policy-checked rows"},
    {"link read", "", 0, 1, LINK_READ, &Processor::link, "[allowed-modes]", "Acquire coherent live link"},
    {"link partner", "", 0, 1, LINK_READ, &Processor::link, "[allowed-modes]", "Acquire local and partner advertisement"},
    {"link cached", "", 0, 1, LINK_CACHED, &Processor::link, "[allowed-modes]", "Copy last observation and apply policy mask"},
    {"link collect", "", 0, 0, LINK_COLLECT, &Processor::link, "", "Collect six event and counter sources"},
    {"link watch", "", 2, 3, WATCH, &Processor::run, "count duration-ms [allowed-modes]", "Sample cached owner link"},
    {"recovery reset", "", 1, 1, RESET, &Processor::lifecycle, "<pcs|hard>", "Reset and verify owned replay"},
    {"recovery restart", "", 0, 0, RESTART, &Processor::lifecycle, "", "Restart PHY circuitry and verify replay"},
    {"recovery reinitialize", "", 0, 1, REPLAY, &Processor::lifecycle, "[none|pcs|hard|restart]", "Explicit recovery with a new allowance"},
    {"owner start", "", 1, 2, OWNER_START, &Processor::lifecycle, "initialize [reset-kind] | verify", "Admit a cooperative operation"},
    {"owner poll", "", 1, 1, OWNER_POLL, &Processor::owner, "<token>", "Advance and report one operation step"},
    {"owner status", "", 0, 0, unsigned(SnapshotKind::OPERATION), &Processor::cached, "", "Copy current owner slot"},
    {"owner result", "", 0, 1, OWNER_RESULT, &Processor::owner, "[token]", "Copy latest delivered result receipt"},
    {"owner cancel", "", 1, 1, OWNER_CANCEL, &Processor::owner, "<token>", "Freeze the active token and preserve effects"},
    {"owner abandon", "", 0, 0, OWNER_ABANDON, &Processor::owner, "", "Requires application disposition"},
    {"test stress", "", 2, 2, STRESS, &Processor::run, "count duration-ms", "Count safe PHYID1 reads"},
    {"test stop", "", 0, 0, STOP, &Processor::run, "", "Stop a watch or stress run"},
    {"test stats", "", 0, 0, STATS, &Processor::run, "", "Copy latest finished watch or stress counts"},
    {"diagnostics selftest", "", 0, 0, 0, &Processor::selftest, "", "Read identity and reset status without stimulus"},
#define FEATURE(id, name, minimum, maximum, mode, usage, summary) \
    {name, "", minimum, maximum, unsigned(Feature::id), &Processor::feature, usage, summary},
#include "Dp8382xCliDiagnosticCommands.inc"
#include "Dp8382xCliControlCommands.inc"
#include "Dp8382xCliPinCommands.inc"
#undef FEATURE
};

bool Processor::setup(const Platform& platform, const Identity& identity) {
  if (_platform.write || !platform.write || !platform.readCached ||
      !safeString(identity.firmware) || !safeString(identity.tree) ||
      !safeString(identity.sdk) || !safeString(identity.target) ||
      !safeString(identity.backend) || !safeString(identity.clock) || !safeString(identity.console) ||
      !isValidPhyAddress(identity.phyAddress) ||
      (identity.variant == Variant::DP83825I &&
       (identity.interface != MacInterface::RMII ||
        (identity.rmiiClock != RmiiClock::LEADER_25MHZ && identity.rmiiClock != RmiiClock::FOLLOWER_50MHZ))) ||
      (identity.variant != Variant::UNKNOWN && !isValidVariant(identity.variant)) ||
      (identity.interface != MacInterface::RMII && identity.interface != MacInterface::MII)) return false;
  const std::string_view tree(identity.tree);
  if ((tree != "clean" && tree != "dirty" && tree != "unknown") ||
      (std::string_view(identity.firmware) == "unknown" && tree != "unknown")) return false;

  _platform = platform;
  _identity = identity;
  human("> ");
  return true;
}

std::size_t Processor::feed(std::string_view bytes) {
  if (!_platform.write) return 0;
  std::size_t accepted = 0;
  while (accepted < std::min(bytes.size(), FEED_BYTES) && _recordSize == 0 && _control.req == 0) {
    const char ch = bytes[accepted++];
    if (_afterCr) {
      _afterCr = false;
      if (ch == '\n') continue;
    }
    if (ch == '\r' || ch == '\n') {
      _afterCr = ch == '\r';
      finishLine();
      _line.fill('\0'); // A completed line can include wake credentials.
      _lineSize = 0;
      _discardReason = nullptr;
    } else if (_discardReason) {
      continue;
    } else if (ch == '\b' || ch == '\x7f') {
      if (_lineSize != 0) --_lineSize;
    } else if (ch != '\t' && (ch < ' ' || ch > '~')) {
      _discardReason = "byte";
    } else if (_lineSize == _line.size()) {
      _discardReason = "line_length";
    } else {
      _line[_lineSize++] = ch;
    }
  }
  return accepted;
}

void Processor::discardInput() {
  if (!_platform.write) return;
  // A lost byte breaks CR/LF adjacency. The next delimiter ends the bad line.
  _afterCr = false;
  if (!_discardReason) _discardReason = "input_loss";
}

void Processor::tick() {
  if (!_platform.write) return;
  _output.drain(_platform.context, _platform.write, WRITE_BYTES);
  flushRecord();
  if (_control.req != 0) { serviceControl(); return; }
  if (workPending()) serviceJob();
}

bool Processor::outputPending() const {
  return _output.queuedBytes() != 0 || _recordSize != 0;
}
bool Processor::workPending() const { return _job.step != Work::IDLE; }

void Processor::human(std::string_view bytes) {
  if (!_quiet) _output.pushHuman(bytes);
}

void Processor::flushRecord() {
  if (_recordSize != 0 && _output.pushRecord({_record.data(), _recordSize})) {
    _recordSize = 0;
    human("> ");
  }
}

void Processor::finishLine() {
  ++_request;
  Reply reply;
  Args args{};
  std::size_t count = 0;
  const Command* selected = nullptr;
  std::size_t nameWords = 0;
  if (_discardReason) {
    reply.reject(_discardReason);
  } else {
    std::size_t pos = 0;
    while (pos < _lineSize) {
      while (pos < _lineSize && separator(_line[pos])) ++pos;
      if (pos == _lineSize) break;
      const auto start = pos;
      while (pos < _lineSize && !separator(_line[pos])) ++pos;
      if (count == args.size() || pos - start > TOKEN_BYTES) {
        reply.reject(count == args.size() ? "token_count" : "token_length");
        break;
      }
      args[count++] = {_line.data() + start, pos - start};
    }
    if (reply.status.ok()) {
      for (const auto& command : COMMANDS) {
        const auto space = command.name.find(' ');
        if (args[0] == command.name.substr(0, space) &&
            (space == std::string_view::npos || args[1] == command.name.substr(space + 1))) {
          selected = &command;
          nameWords = space == std::string_view::npos ? 1 : 2;
          break;
        }
        auto aliases = command.alias;
        while (!aliases.empty()) {
          const auto bar = aliases.find('|');
          if (args[0] == aliases.substr(0, bar)) {
            selected = &command;
            nameWords = 1;
            break;
          }
          if (bar == std::string_view::npos) break;
          aliases.remove_prefix(bar + 1);
        }
        if (selected) break;
      }
      if (!selected) reply.reject(count == 0 ? "empty" : "command");
      else {
        count -= nameWords;
        if (count < selected->minArgs || count > selected->maxArgs) reply.reject("arity");
        else {
          std::move(args.begin() + static_cast<std::ptrdiff_t>(nameWords), args.end(), args.begin());
          (this->*selected->handler)(*selected, args, count, reply);
        }
      }
    }
  }

  if (!reply.deferred) record(selected ? selected->name : "unknown", _request, reply);
}

bool Processor::sampleClock(uint64_t& now) {
  if (!_platform.nowUs || _clockFault) return false;
  const auto sampled = _platform.nowUs(_platform.context);
  if (sampled < _lastUs) {
    _clockFault = true;
    return false;
  }
  _lastUs = now = sampled;
  return true;
}

void Processor::record(std::string_view name, uint64_t req, const Reply& reply) {
  char commandName[TOKEN_BYTES + 1]{};
  std::transform(name.begin(), name.end(), commandName, [](char c) { return c == ' ' ? '.' : c; });
  if (isClockError(reply.status) && std::strcmp(reply.reason, "none") == 0) _clockFault = true;
  uint64_t nowUs = 0;
  const bool timeValid = sampleClock(nowUs);
  const int length = std::snprintf(_record.data(), _record.size(),
      "%sschema=1 boot=%" PRIu32 " req=%" PRIu64 " type=%s cmd=%s op=%" PRIu64 " state=%s"
      " primary=%s detail=%" PRId32 " cleanup=none cleanup_detail=0"
      " time_valid=%u time_us=%" PRIu64 " deadline_valid=%u deadline_us=%" PRIu64 " reason=%s%s\r\n",
      PREFIX, _identity.boot, req, reply.type, commandName, reply.op, reply.state, errorName(reply.status.code),
      reply.status.detail, unsigned(timeValid), nowUs,
      unsigned(reply.deadline != 0), reply.deadline, reply.reason,
      reply.payload.data());
  // All fields have static size limits. Keep the acceptance invariant if a
  // future handler or formatter violates those limits.
  if (length > 0 && static_cast<std::size_t>(length) < _record.size()) {
    _recordSize = static_cast<std::size_t>(length);
  } else {
    const int fallback = std::snprintf(_record.data(), _record.size(),
        "%sschema=1 boot=%" PRIu32 " req=%" PRIu64 " type=result cmd=unknown op=0 state=terminal"
        " primary=INVALID_CONFIG detail=0 cleanup=none cleanup_detail=0"
        " time_valid=0 time_us=0 deadline_valid=0 deadline_us=0 reason=format\r\n",
        PREFIX, _identity.boot, req);
    _recordSize = fallback > 0 ? static_cast<std::size_t>(fallback) : 0;
  }
  flushRecord();
}

void Processor::help(const Command&, const Args& args, std::size_t count, Reply& reply) {
  const std::string_view topic = count == 0 ? "" : args[0];
  bool found = false;
  for (const auto& command : COMMANDS) {
    if (topic == command.name.substr(0, command.name.find(' '))) found = true;
  }
  if (!topic.empty() && !found) {
    reply.reject("topic");
    return;
  }

  human(_color ? "\r\n\x1b[36m=== DP8382x CLI Help ===\x1b[0m\r\n" :
                 "\r\n=== DP8382x CLI Help ===\r\n");
  if (topic.empty()) human("Use help <group> for arguments.\r\n");
  const auto groupHelp = [&](std::string_view group) {
    if (group == "configuration") profileHelp();
    if (group == "registers") {
      human("  C22 offsets 0..31. MMD takes an explicit DEVAD and 16-bit offset.\r\n"
            "  Dumps skip protected registers. Writes require maintenance.\r\n"
            "  BMCR allows mask 0x3100.\r\n");
      if (_identity.variant == Variant::DP83825I) human("  DP83825I raw LED programming is unavailable.\r\n");
      else human(isA(_identity.variant) ? "  A LEDCR 0x18 is read-only through raw access. Use controls pins.\r\n" :
            "  I/E LEDCR 0x18 or 31:0x18 allows bits 0x0692.\r\n");
    }
    if (group == "link" && !topic.empty()) human("  Allowed modes: mask 1..15 for 10/half, 10/full, 100/half, 100/full.\r\n"
                                               "  Watch copies owner evidence. Count 1..32, duration count..5000 ms.\r\n");
    if (group == "test" && !topic.empty()) human("  Stress count 1..32, duration count..5000 ms, one safe read per round.\r\n");
  };
  std::string_view previousGroup;
  for (const auto& command : COMMANDS) {
    const auto space = command.name.find(' ');
    const auto group = command.handler == &Processor::configure ? CONFIG[command.flags].group :
        space == std::string_view::npos ? "General" : command.name.substr(0, space);
    if (!topic.empty() && topic != command.name.substr(0, space)) continue;

    if (group != previousGroup) {
      groupHelp(previousGroup);
      char heading[32]{};
      std::snprintf(heading, sizeof(heading), "%.*s\r\n", int(group.size()), group.data());
      human(heading);
      previousGroup = group;
    }
    const char* alias = command.alias.empty() ? "" : command.alias.data();
    char line[224]{};
    if (topic.empty()) {
      std::snprintf(line, sizeof(line), "  %s%s%s%s\r\n", command.name.data(),
                    *alias ? " (" : "", alias, *alias ? ")" : "");
      human(line);
      continue;
    }
    char usage[96]{};
    if (command.handler == &Processor::configure) {
      const auto& option = CONFIG[command.flags];
      if (!option.enumValues.empty()) std::snprintf(usage, sizeof(usage), "[%s]", option.enumValues.data());
      else std::snprintf(usage, sizeof(usage), "[%" PRIu32 "..%" PRIu32 " %s]", option.min, option.max, option.unit);
    } else {
      std::snprintf(usage, sizeof(usage), "%s", command.usage);
    }

    std::snprintf(line, sizeof(line), "  %-24s %-38s %-12s %s\r\n",
                  command.name.data(), usage, alias, command.summary);
    human(line);
  }
  groupHelp(previousGroup);
}

void Processor::profileHelp() {
  char lines[600]{};
  std::snprintf(lines, sizeof(lines),
      "  an <modes %u..%u> <%s> <%s> <flags %u..%u>\r\n"
      "     Modes: 1 10/half, 2 10/full, 4 100/half, 8 100/full. Combine bits.\r\n"
      "     Optional: wait, or <partner-limit 1..16> <packed four-digit hex pages>\r\n"
      "  forced <%s> <%s> <%s> <flags>\r\n"
      "  Flags: 1 robust, 2 coupled pair/polarity mirror, 4 extended full duplex.\r\n"
      "  DP83826I/E/AI/AE forced mode requires robust. Extended duplex excludes forced 10 and native half duplex.\r\n",
      unsigned(CONFIG[MODES].min), unsigned(CONFIG[MODES].max), CONFIG[PAUSE].enumValues.data(),
      CONFIG[PAIR].enumValues.data(), unsigned(CONFIG[FLAGS].min), unsigned(CONFIG[FLAGS].max),
      CONFIG[SPEED].enumValues.data(), CONFIG[DUPLEX].enumValues.data(), CONFIG[PAIR].enumValues.data());
  human(lines);
}

void Processor::identity(const Command& command, const Args& args, std::size_t, Reply& reply) {
  const bool session = command.name == "session";
  if (session && !safeValue(args[0])) {
    reply.reject("nonce");
    return;
  }
  if (session) reply.type = "session";
  const auto nonce = session ? args[0] : std::string_view("none");
  reply.fields(" nonce=%.*s library=%s firmware=%s tree=%s sdk=%s target=%s backend=%s"
               " clock=%s variant=%s interface=%s address=%" PRIu32 " console=%s baud=%" PRIu32,
               int(nonce.size()), nonce.data(), VERSION_STRING, _identity.firmware, _identity.tree,
               _identity.sdk, _identity.target, _identity.backend, _identity.clock,
               variantName(_identity.variant),
               interfaceName(_identity.interface), _identity.phyAddress, _identity.console, _identity.baud);
  if (_identity.variant == Variant::DP83825I)
    reply.appendFields(" rmii_role=%s", _identity.rmiiClock == RmiiClock::LEADER_25MHZ ? "leader" : "follower");
}

void Processor::configure(const Command& command, const Args& args, std::size_t count, Reply& reply) {
  const auto& option = CONFIG[command.flags];
  bool& setting = this->*option.setting;
  if (count != 0) {
    if (!option.enumValues.empty()) {
      const int selected = choice(option.enumValues, args[0]);
      if (selected < 0) { reply.reject("enum"); return; }
      setting = selected != 0;
    } else {
      uint32_t value = 0;
      if (!unsignedNumber(args[0], value) || value < option.min || value > option.max) {
        reply.reject("number");
        return;
      }
      setting = value != 0;
    }
  }
  reply.fields(" %.*s=%u", int(option.name.size()), option.name.data(), unsigned(setting));
}

void Processor::console(const Command&, const Args&, std::size_t, Reply& reply) {
  FeatureFields fields;
  if (_platform.consoleStats) _platform.consoleStats(_platform.context, fields);
  reply.fields(" queued=%zu dropped_human=%" PRIu64 " stalled_writes=%" PRIu64 "%s",
               _output.queuedBytes(), uint64_t(_output.droppedHuman()),
               uint64_t(_output.stalledWrites()), fields.text.data());
  reply.checkFeatureFields(fields);
}

void Processor::application(const Command& command, const Args&, std::size_t, Reply& reply) {
  if (!_platform.execute) {
    reply.status = Status::Error(Err::UNSUPPORTED, "No application command route");
    return;
  }
  Request request;
  request.action = Action::APPLICATION;
  request.application = static_cast<ApplicationCommand>(command.flags);
  if (request.application != ApplicationCommand::STATUS) {
    schedule(command.name, request, Work::CALL, CONFIG_US, reply);
    return;
  }
  Receipt receipt{};
  reply.status = checkedTerminal(_platform.execute(_platform.context, request, receipt));
  reply.fields("%s", receipt.featureFields.text.data());
  reply.checkFeatureFields(receipt.featureFields);
}

void Processor::cached(const Command& command, const Args&, std::size_t, Reply& reply) {
  const auto kind = static_cast<SnapshotKind>(command.flags);
  CachedSnapshot copy{};
  reply.status = _platform.readCached(_platform.context, kind, copy);
  if (!validTerminal(reply.status)) {
    reply.status = Status::Error(Err::INVALID_CONFIG, "Invalid cached callback result", reply.status.detail);
    reply.reason = "callback";
  }
  if (!reply.status.ok()) return;

  switch (kind) {
    case SnapshotKind::CAPABILITIES: {
      const auto& s = copy.capabilities;
      reply.fields(" valid=%u modes=%u interfaces=%u copper=%u fiber=%u gigabit=%u",
                   unsigned(s.valid), s.valid ? unsigned(s.linkModeMask) : 0U,
                   s.valid ? unsigned(s.interfaceMask) : 0U,
                   unsigned(s.valid && s.copper), unsigned(s.valid && s.fiber), unsigned(s.valid && s.gigabit));
      break;
    }
    case SnapshotKind::HEALTH: {
      const auto& s = copy.health;
      reply.fields(" health=%s initialized=%u successes=%" PRIu32 " failures=%" PRIu32
                   " consecutive=%" PRIu32 " last_error=%s last_detail=%" PRId32
                   " last_ok_valid=%u last_ok_us=%" PRIu64 " last_error_valid=%u last_error_us=%" PRIu64,
                   driverStateName(s.state), unsigned(s.initialized), s.totalSuccess, s.totalFailures,
                   s.consecutiveFailures, errorName(s.lastError.code), s.lastError.detail,
                   unsigned(s.lastOkTimeValid), s.lastOkTimeValid ? s.lastOkUs : 0,
                   unsigned(s.lastErrorTimeValid), s.lastErrorTimeValid ? s.lastErrorUs : 0);
      break;
    }
    case SnapshotKind::LINK: {
      const auto& s = copy.link;
      const bool current = s.valid && !s.stale;
      const bool resolved = current && s.linkUp && s.resolvedModeValid;
      const bool pause = resolved && s.pauseValid;
      reply.fields(" valid=%u stale=%u up=%u resolved=%u speed_mbps=%u duplex=%s negotiation=%s"
                   " pause_valid=%u pause_rx=%u pause_tx=%u observed_valid=%u observed_us=%" PRIu64,
                   unsigned(s.valid), unsigned(s.stale), unsigned(current && s.linkUp), unsigned(resolved),
                   resolved ? speedMbps(s.speed) : 0, resolved ? duplexName(s.duplex) : "unknown",
                   negotiationName(current ? s.negotiation : NegotiationState::UNKNOWN),
                   unsigned(pause), unsigned(pause && s.pauseRx), unsigned(pause && s.pauseTx),
                   unsigned(s.timeValid), s.timeValid ? s.observedAtUs : 0);
      break;
    }
    case SnapshotKind::SETTINGS: {
      const auto& s = copy.settings;
      const auto mode = [](const LinkSettings& settings, bool valid) {
        return !valid ? "unknown" : settings.mode == LinkMode::AUTONEGOTIATED ? "an" : "forced";
      };
      if (command.name != "cached settings") {
        const bool desired = command.name == "configuration desired";
        const bool valid = desired ? s.desiredValid : s.observedValid;
        const auto profile = valid ? (desired ? s.desired : s.observed) : LinkSettings{};
        reply.fields(" valid=%u stale=%u trust=%s mode=%s modes=%u speed_mbps=%u duplex=%s"
                     " pause_sym=%u pause_asym=%u pair=%s flags=%u",
                     unsigned(valid), unsigned(!desired && s.observedStale), trustName(s.trust),
                     mode(profile, valid), unsigned(profile.advertisement.technologyMask),
                     speedMbps(profile.forced.speed), duplexName(profile.forced.duplex),
                     unsigned(profile.advertisement.symmetricPause), unsigned(profile.advertisement.asymmetricPause),
                     valid ? pairName(profile.crossover.pairSelection) : "unknown",
                     valid ? profileFlags(profile) : 0U);
        break;
      }
      reply.fields(" trust=%s desired_valid=%u desired_mode=%s desired_modes=%u"
                   " desired_speed_mbps=%u desired_duplex=%s observed_valid=%u observed_stale=%u"
                   " observed_mode=%s observed_modes=%u observed_speed_mbps=%u observed_duplex=%s",
                   trustName(s.trust), unsigned(s.desiredValid), mode(s.desired, s.desiredValid),
                   s.desiredValid ? unsigned(s.desired.advertisement.technologyMask) : 0,
                   s.desiredValid ? speedMbps(s.desired.forced.speed) : 0,
                   s.desiredValid ? duplexName(s.desired.forced.duplex) : "unknown",
                   unsigned(s.observedValid), unsigned(s.observedStale), mode(s.observed, s.observedValid),
                   s.observedValid ? unsigned(s.observed.advertisement.technologyMask) : 0,
                   s.observedValid ? speedMbps(s.observed.forced.speed) : 0,
                   s.observedValid ? duplexName(s.observed.forced.duplex) : "unknown");
      break;
    }
    case SnapshotKind::OPERATION: {
      const auto& s = copy.operation;
      const char* state = s.state == OperationState::IDLE ? "idle" :
          s.state == OperationState::RUNNING ? "running" : "result_pending";
      reply.fields(" slot=%s token=%" PRIu64 " slot_status=%s slot_detail=%" PRId32
                   " cleanup_owed=%u temporary=%u slot_deadline_us=%" PRIu64,
                   state, s.token.value, errorName(s.status.code), s.status.detail,
                   unsigned(s.cleanupOwed), unsigned(s.temporaryMayBeActive), s.remaining.deadlineUs);
      break;
    }
    case SnapshotKind::TRANSPORT: {
      const auto& s = copy.transport;
      reply.fields(" batches=%" PRIu32 " attempted=%" PRIu32 " completed=%" PRIu32
                   " stopped=%u clock_invalid=%u controller_reusable=%u",
                   s.totalBatches, s.attemptedFrames, s.completedFrames, unsigned(s.stopped),
                   unsigned(s.clockInvalid), unsigned(s.controllerReusable));
      break;
    }
    case SnapshotKind::IDENTITY:
      reply.identityFields(copy.identity);
      if (_identity.variant == Variant::DP83825I) {
        reply.appendFields(" sor1_valid=%u sor1=%u rcsr_valid=%u rcsr=%u straps_stale=%u",
                     unsigned(copy.straps.sor1Valid), unsigned(copy.straps.sor1), unsigned(copy.straps.rcsrValid),
                     unsigned(copy.straps.rcsr), unsigned(copy.straps.stale));
      }
      break;
    case SnapshotKind::EVENTS: {
      reply.eventFields(copy.counters, copy.eventValidMask, copy.eventWords);
      break;
    }
  }
}

void Processor::lifecycle(const Command& command, const Args& args, std::size_t count, Reply& reply) {
  Request request;
  request.action = Action::START_INITIALIZE;
  request.budget.io.maxFrames = 512;
  std::string_view reset = "none";
  switch (command.flags) {
    case END:
      request.action = Action::END;
      request.budget.io.maxFrames = 0;
      break;
    case RESTART: reset = "restart"; break;
    case RESET:
      if (args[0] != "pcs" && args[0] != "hard") { reply.reject("enum"); return; }
      reset = args[0];
      break;
    case OWNER_START:
      if (args[0] == "verify" && count == 1) request.action = Action::START_VERIFY;
      else if (args[0] == "initialize") { if (count == 2) reset = args[1]; }
      else { reply.reject("enum"); return; }
      break;
    default:
      if (count != 0) reset = args[0];
      break;
  }
  const int selected = choice(RESET_KINDS, reset);
  if (selected < 0) { reply.reject("enum"); return; }
  request.reset = static_cast<ResetKind>(selected);
  if (schedule(command.name, request, Work::CALL, CONFIG_US, reply))
    _job.initialize = request.action == Action::START_INITIALIZE;
}

void Processor::owner(const Command& command, const Args& args, std::size_t count, Reply& reply) {
  if (command.flags == OWNER_ABANDON) {
    Request request;
    request.action = Action::ABANDON;
    schedule(command.name, request, Work::CALL, 0, reply);
    return;
  }
  uint64_t token = 0;
  if (count != 0 && (!unsignedNumber(args[0], token) || token == 0)) { reply.reject("number"); return; }
  if (command.flags == OWNER_RESULT) {
    if (_lastResult.token.value == 0)
      reply.status = Status::Error(Err::RESULT_NOT_AVAILABLE, "No CLI result was delivered");
    else if (token && token != _lastResult.token.value)
      reply.status = Status::Error(Err::TOKEN_MISMATCH, "Different retained CLI result");
    else {
      reply.op = _lastResult.token.value;
      reply.fields(" origin_req=%" PRIu64 " outcome=%s outcome_detail=%" PRId32
                   " core_status=%s core_detail=%" PRId32 " sdk_error=%" PRId32
                   " mutation_completed=%u mutation_possible=%u cleanup_owed=%u temporary=%u",
                   _lastResult.requestId, errorName(_lastResult.outcome.code), _lastResult.outcome.detail,
                   errorName(_lastResult.coreStatus.code), _lastResult.coreStatus.detail, _lastResult.sdkError,
                   unsigned(_lastResult.mutationCompleted), unsigned(_lastResult.mutationPossible),
                   unsigned(_lastResult.cleanupOwed), unsigned(_lastResult.temporary));
    }
    return;
  }
  const bool canCancel = command.flags == OWNER_CANCEL && _job.step == Work::TAKE;
  if (_job.step != Work::POLL && !canCancel) {
    reply.status = Status::Error(Err::RESULT_NOT_AVAILABLE, "No CLI operation waiting for a poll or cancel");
  } else if (token != _job.request.token.value) {
    reply.status = Status::Error(Err::TOKEN_MISMATCH, "Different active CLI token");
  } else {
    _control = {command.name, command.flags == OWNER_POLL ? Action::POLL : Action::CANCEL, _request};
    reply.deferred = true;
  }
}

void Processor::link(const Command& command, const Args& args, std::size_t count, Reply& reply) {
  Request request;
  if (count != 0 && (!unsignedNumber(args[0], request.mask) ||
      request.mask == 0 || request.mask > LINK_TECHNOLOGY_MASK)) { reply.reject("range"); return; }
  if (command.flags == LINK_CACHED) {
    CachedSnapshot copy;
    reply.status = checkedTerminal(_platform.readCached(_platform.context, SnapshotKind::LINK, copy));
    if (reply.status.ok()) reply.linkFields(copy.link, request.mask);
    return;
  }
  const bool collect = command.flags == LINK_COLLECT;
  request.action = collect ? Action::COLLECT_EVENTS : Action::OBSERVE_LINK;
  request.budget.io.maxFrames = collect ? 6U : 10U;
  schedule(command.name, request, Work::CALL, DIRECT_US, reply);
}

void Processor::run(const Command& command, const Args& args, std::size_t count, Reply& reply) {
  const bool running = _job.step == Work::WATCH || _job.step == Work::STRESS;
  if (command.flags == STOP) {
    if (!running) reply.status = Status::Error(Err::RESULT_NOT_AVAILABLE, "No active watch or stress run");
    else if (_job.stop.ok() && _job.rows < _job.end)
      _job.stop = Status::Error(Err::CANCELLED, "Run stopped by application");
    return;
  }
  if (command.flags == STATS) {
    reply.fields(" running=%u origin_req=%" PRIu64 " rounds=%" PRIu32 " attempted=%" PRIu32
                 " attempted_known=%u managed=%u outcome=%s outcome_detail=%" PRId32,
                 unsigned(running), _lastRunReq, _lastRounds, _lastAttempts, unsigned(_lastAttemptsKnown),
                 0U, errorName(_lastRunStatus.code), _lastRunStatus.detail);
    return;
  }
  const bool watch = command.flags == WATCH;
  uint32_t rounds = 0, milliseconds = 0;
  Request request;
  if (!unsignedNumber(args[0], rounds) || !unsignedNumber(args[1], milliseconds) ||
      rounds == 0 || rounds > MAX_ROWS || milliseconds < rounds || milliseconds > 5000U ||
      (count == 3 && (!unsignedNumber(args[2], request.mask) ||
       request.mask == 0 || request.mask > LINK_TECHNOLOGY_MASK))) { reply.reject("range"); return; }
  request.action = Action::READ_C22;
  request.address = cmd::REG_PHYIDR1;
  request.budget.io.maxFrames = watch ? 0U : rounds;
  const uint64_t duration = uint64_t{milliseconds} * 1000U;
  if (schedule(command.name, request, watch ? Work::WATCH : Work::STRESS, duration, reply)) {
    _job.end = rounds;
    _job.intervalUs = duration / rounds;
  }
}

void Processor::feature(const Command& command, const Args& args, std::size_t count, Reply& reply) {
  if (!_platform.execute) {
    reply.status = Status::Error(Err::INVALID_CONFIG, "No typed owner route");
    reply.reason = "backend";
    return;
  }
  Request request;
  request.action = Action::FEATURE;
  request.feature = static_cast<Feature>(command.flags);
  auto& copied = request.featureArgs;
  std::size_t offset = 0;
  copied.count = static_cast<uint8_t>(count);
  for (std::size_t i = 0; i < count; ++i) {
    copied.lengths[i] = static_cast<uint8_t>(args[i].size());
    std::copy(args[i].begin(), args[i].end(), copied.bytes.begin() + static_cast<std::ptrdiff_t>(offset));
    offset += args[i].size();
  }
  const auto mode = featureMode(request.feature);
  if (mode == FeatureMode::CACHED) {
    Receipt receipt;
    reply.status = checkedTerminal(_platform.execute(_platform.context, request, receipt));
    reply.fields("%s", receipt.featureFields.text.data());
    reply.checkFeatureFields(receipt.featureFields);
    return;
  }
  request.budget.io.maxFrames = mode == FeatureMode::READ ? 64 : 512;
  schedule(command.name, request, Work::CALL, mode == FeatureMode::READ ? DIRECT_US : CONFIG_US, reply);
}

void Processor::selftest(const Command& command, const Args&, std::size_t, Reply& reply) {
  Request request;
  request.action = Action::PROBE;
  request.budget.io.maxFrames = 3;
  if (schedule(command.name, request, Work::SELFTEST, DIRECT_US, reply)) _job.end = 2;
}

bool Processor::parseProfile(const Args& args, std::size_t count, Request& request) {
  auto& profile = request.settings;
  const int mode = choice(CONFIG[MODE].enumValues, args[0]);
  if (mode == 0) {
    uint32_t modes = 0;
    if (!unsignedNumber(args[1], modes) || modes < CONFIG[MODES].min || modes > CONFIG[MODES].max)
      return false;
    const int pauseMask = choice(CONFIG[PAUSE].enumValues, args[2]);
    if (pauseMask < 0) return false;
    profile.advertisement = {static_cast<uint8_t>(modes), (pauseMask & 1) != 0, (pauseMask & 2) != 0};
    if (count == 6) {
      if (args[5] != "wait") return false;
      request.options.waitForCompletion = true;
    }
    if (count == 7) {
      uint32_t limit = 0;
      if (!unsignedNumber(args[5], limit) || !pageList(args[6], limit, request.options)) return false;
    }
  } else if (mode == 1 && count == 5) {
    profile.mode = LinkMode::FORCED;
    const int speed = choice(CONFIG[SPEED].enumValues, args[1]);
    const int duplex = choice(CONFIG[DUPLEX].enumValues, args[2]);
    if (speed < 0 || duplex < 0) return false;
    profile.forced.speed = speed == 0 ? LinkSpeed::MBPS_10 : LinkSpeed::MBPS_100;
    profile.forced.duplex = duplex == 0 ? Duplex::HALF : Duplex::FULL;
  } else return false;

  const int pair = choice(CONFIG[PAIR].enumValues, args[3]);
  if (pair < 0) return false;
  profile.crossover.pairSelection = static_cast<PairSelection>(pair);
  uint32_t flags = 0;
  if (!unsignedNumber(args[4], flags) || flags > CONFIG[FLAGS].max) return false;
  profile.crossover.robust = (flags & 1U) != 0;
  profile.crossover.swapPair = profile.crossover.swapPolarity = (flags & 2U) != 0;
  profile.extendedFullDuplex = (flags & 4U) != 0;
  return true;
}

void Processor::route(const Command& command, const Args& args, std::size_t count, Reply& reply) {
  Request request{};
  Work step = Work::CALL;
  uint32_t end = 0;
  uint32_t frames = 0;
  uint64_t durationUs = DIRECT_US;
  const auto route = static_cast<Route>(command.flags);
  if (route == APPLY || route == VALIDATE) {
    if (!parseProfile(args, count, request)) { reply.reject("profile"); return; }
    reply.status = validateConfiguration(_identity.variant, request.settings, request.options);
    if (!reply.status.ok() || route == VALIDATE) return;
    request.action = Action::START_CONFIGURE;
    frames = 512;
    durationUs = CONFIG_US;
  } else if (route == VERIFY) {
    request.action = Action::START_VERIFY;
    frames = 512;
    durationUs = CONFIG_US;
  } else if (route == INVALIDATE) {
    request.action = Action::INVALIDATE;
  } else if (route == PROBE) {
    request.action = Action::PROBE;
    frames = 2;
  } else if (route == SCAN) {
    request.action = Action::DISCOVER;
    step = Work::DISCOVERY;
    if (count == 0) {
      uint8_t maximum = 0;
      if (!strapAddressMax(_identity.variant, maximum)) { reply.reject("range"); return; }
      end = maximum;
    } else if (count != 2 || !unsignedNumber(args[0], request.address) || !unsignedNumber(args[1], end) ||
             end > 31 || request.address > end) { reply.reject("range"); return; }
    frames = (end - request.address + 1U) * 2U;
  } else if (route == BMCR) {
    request.action = Action::MODIFY_BMCR;
    if (!unsignedNumber(args[0], request.mask) || !unsignedNumber(args[1], request.value)) {
      reply.reject("number"); return;
    }
    constexpr uint32_t owned = cmd::MASK_BMCR_AN_ENABLE | cmd::MASK_BMCR_SPEED | cmd::MASK_BMCR_DUPLEX;
    if (!request.mask || (request.mask & ~owned) || (request.value & ~request.mask)) {
      reply.reject("access"); return;
    }
    request.address = cmd::REG_BMCR;
    frames = 2;
  } else if (route == MODIFY) {
    request.action = Action::MODIFY_MMD;
    if (!unsignedNumber(args[0], request.devad) || !unsignedNumber(args[1], request.address) ||
        !unsignedNumber(args[2], request.mask) || !unsignedNumber(args[3], request.value)) {
      reply.reject("number"); return;
    }
    if (request.devad != cmd::DEVAD_VENDOR || request.address != cmd::REG_LEDCR ||
        !request.mask || (request.mask & ~uint32_t{cmd::MASK_LEDCR_DIRECT}) ||
        (request.value & ~request.mask)) { reply.reject("access"); return; }
    frames = 5;
  } else {
    const bool mmd = args[0] == "mmd";
    if (!mmd && args[0] != "c22") { reply.reject("space"); return; }
    const std::size_t offset = mmd ? 2 : 1;
    if (count != offset + (route == READ ? 1U : 2U)) { reply.reject("arity"); return; }
    if (mmd && (!unsignedNumber(args[1], request.devad) || request.devad > 31)) {
      reply.reject("range"); return;
    }
    if (!unsignedNumber(args[offset], request.address) || !safeRegisterWidth(mmd, request.address)) {
      reply.reject("range"); return;
    }
    frames = mmd ? 4U : 1U;
    request.action = mmd ? Action::READ_MMD : Action::READ_C22;
    if (route == DUMP) {
      if (!unsignedNumber(args[offset + 1], end) || !safeRegisterWidth(mmd, end) ||
          end < request.address || end - request.address >= MAX_ROWS) { reply.reject("range"); return; }
      frames *= end - request.address + 1U;
      step = Work::DUMP;
    } else if (route == WRITE) {
      if (!unsignedNumber(args[offset + 1], request.value) || request.value > 65535) {
        reply.reject("range"); return;
      }
      if (request.address != cmd::REG_LEDCR || (mmd && request.devad != cmd::DEVAD_VENDOR) ||
          (request.value & ~uint32_t{cmd::MASK_LEDCR_DIRECT})) { reply.reject("access"); return; }
      request.action = mmd ? Action::WRITE_MMD : Action::WRITE_C22;
    }
    const uint32_t last = route == DUMP ? end : request.address;
    if ((route == READ || route == DUMP) && mmd && request.devad == cmd::DEVAD_VENDOR &&
        request.address <= cmd::REG_RXFSOP3 && last >= cmd::REG_RXFSOP1) {
      reply.reject("access"); // Credentials never enter raw CLI output.
      return;
    }
  }

  request.budget.io.maxFrames = frames;
  if (schedule(command.name, request, step, durationUs, reply)) _job.end = end;
}

bool Processor::schedule(std::string_view command, const Request& request, Work step,
                         uint64_t durationUs, Reply& reply) {
  const bool local = request.action == Action::ABANDON || request.action == Action::END;
  if ((!_platform.execute && step != Work::WATCH) || (!_platform.nowUs && !local)) {
    reply.status = Status::Error(Err::INVALID_CONFIG, "No live owner route");
    reply.reason = "backend";
    return false;
  }
  if (workPending()) {
    reply.status = Status::Error(Err::BUSY, "CLI work owns service slot",
                                 static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
    return false;
  }
  if (step == Work::WATCH || step == Work::STRESS || step == Work::SELFTEST) {
    CachedSnapshot snapshot;
    reply.status = checkedTerminal(_platform.readCached(_platform.context, SnapshotKind::OPERATION, snapshot));
    if (!reply.status.ok()) return false;
    const auto& operation = snapshot.operation;
    if (operation.state != OperationState::IDLE || operation.cleanupOwed || operation.temporaryMayBeActive) {
      const auto detail = operation.state == OperationState::RUNNING ? BusyDetail::OPERATION_ACTIVE :
          operation.state == OperationState::RESULT_PENDING ? BusyDetail::RESULT_PENDING : BusyDetail::CLEANUP_REQUIRED;
      reply.status = Status::Error(Err::BUSY, "Owner work takes precedence over a run", static_cast<int32_t>(detail));
      return false;
    }
  }
  uint64_t deadline = 0;
  uint64_t now = 0;
  if (durationUs != 0) {
    if (!sampleClock(now)) {
      if (!local) { reply.status = clockError(); return false; }
    } else if (now > std::numeric_limits<uint64_t>::max() - durationUs) {
      if (!local) { reply.status = timeout(TimeoutDetail::BOUND_ADMISSION); return false; }
    } else {
      deadline = now + durationUs;
    }
  }
  _job = {};
  _job.step = step;
  _job.request = request;
  _job.request.budget.io.deadlineUs = deadline;
  _job.request.budget.maxStalledPolls = 4096;
  _job.requestId = _request;
  _job.command = command;
  _job.nextUs = now;
  reply.deferred = true;
  return true;
}

void Processor::serviceControl() {
  if (_recordSize != 0) return;
  auto request = _job.request;
  request.action = _control.action;
  Reply reply;
  reply.op = request.token.value;
  if (request.action == Action::POLL) reply.deadline = request.budget.io.deadlineUs;
  if (request.action == Action::POLL) {
    const auto status = pollJob();
    reply.fields(" origin_req=%" PRIu64 " poll_status=%s poll_detail=%" PRId32,
                 _job.requestId, errorName(status.code), status.detail);
  } else {
    Receipt receipt{};
    reply.status = checkedTerminal(_platform.execute(_platform.context, request, receipt));
    if (reply.status.ok()) _job.step = Work::TAKE;
    reply.fields(" origin_req=%" PRIu64, _job.requestId);
  }
  record(_control.command, _control.req, reply);
  _control = {};
}

Status Processor::pollJob() {
  auto request = _job.request;
  if (_job.stop.ok()) {
    uint64_t now = 0;
    if (!sampleClock(now)) _job.stop = clockError();
    else if (now >= request.budget.io.deadlineUs) _job.stop = timeout(TimeoutDetail::DEADLINE);
  }
  // A poll might be refused before the core sees time. Freeze the token without
  // bus access when the CLI's bound expires, even if output remains stalled.
  request.action = _job.stop.ok() ? Action::POLL : Action::CANCEL;
  Receipt receipt{};
  auto status = _platform.execute(_platform.context, request, receipt);
  if (request.action == Action::CANCEL) {
    status = checkedTerminal(status);
    if (status.is(Err::BUSY)) return status;
    if (status.ok()) status = _job.stop;
  } else if (status.inProgress()) {
    return status;
  } else {
    status = checkedTerminal(status);
    if (isClockError(status)) _clockFault = true;
    if (status.is(Err::BUSY)) {
      CachedSnapshot copy;
      const auto copied = _platform.readCached(_platform.context, SnapshotKind::OPERATION, copy);
      if (!copied.ok() || copy.operation.state != OperationState::RESULT_PENDING ||
          copy.operation.token.value != request.token.value) return status;
    }
  }
  _job.step = Work::TAKE;
  return status;
}

void Processor::serviceJob() {
  auto& request = _job.request;
  Receipt receipt{};
  const auto call = [&](Action action) {
    request.action = action;
    return _platform.execute(_platform.context, request, receipt);
  };
  if (_job.step == Work::POLL) {
    pollJob();
    return;
  }
  if (_recordSize != 0) return;
  if (_job.step == Work::DISCOVERY || _job.step == Work::DUMP) {
    sequenceStep();
    return;
  }
  if (_job.step == Work::WATCH || _job.step == Work::STRESS) {
    runStep();
    return;
  }
  if (_job.step == Work::SELFTEST) {
    selftestStep();
    return;
  }

  Reply reply;
  reply.op = request.token.value;
  reply.deadline = request.budget.io.deadlineUs;
  if (_job.step == Work::TAKE || _job.step == Work::RECONCILE) {
    if (_job.step == Work::TAKE) {
      const auto delivered = checkedTerminal(call(Action::TAKE));
      if (delivered.is(Err::BUSY)) return;
      if (delivered.is(Err::RESULT_NOT_AVAILABLE)) {
        _job.step = Work::POLL;
        return;
      }
      receipt.result = delivered.ok() ? checkedTerminal(receipt.result) : delivered;
      _job.result = receipt;
      _job.result.token = delivered.ok() ? request.token : OperationToken{};
      if (_job.stop.ok() && receipt.result.ok() && _job.command == "configuration apply") {
        _job.step = Work::RECONCILE;
        return;
      }
      if (!_job.stop.ok() && (receipt.result.ok() || receipt.result.is(Err::CANCELLED)))
        reply.status = _job.stop;
    } else {
      uint64_t now = 0;
      reply.status = sampleClock(now) ? checkedTerminal(call(Action::RECONCILE)) : clockError();
      if (reply.status.ok() && receipt.sdkError != 0)
        reply.status = Status::Error(Err::INVALID_CONFIG, "SDK reconciliation failed");
    }
    const auto& result = _job.result;
    if (reply.status.ok()) reply.status = result.result;
    if (_job.initialize) {
      const auto& straps = result.straps;
      reply.fields(" core_status=%s core_detail=%" PRId32 " requested_reset=%s effective_reset=%s"
                   " reset_required=%u reset_accepted=%u reset_complete=%u persistent_restored=%u"
                   " mutation_completed=%u mutation_possible=%u cleanup_owed=%u temporary=%u"
                   " clock_invalid=%u link_valid=0 sor1_valid=%u sor1=%u sor2_valid=%u sor2=%u straps_stale=%u",
                   errorName(result.result.code), result.result.detail, resetName(request.reset),
                   result.token.value ? resetName(result.reset) : "unknown", unsigned(result.commandRequired),
                   unsigned(result.commandAccepted), unsigned(result.protocolComplete), unsigned(result.persistentRestored),
                   unsigned(result.mutationCompleted), unsigned(result.mutationPossible), unsigned(result.cleanupOwed),
                   unsigned(result.temporary), unsigned(result.clockInvalid || _clockFault),
                   unsigned(straps.sor1Valid), unsigned(straps.sor1), unsigned(straps.sor2Valid), unsigned(straps.sor2),
                   unsigned(straps.stale));
      if (_identity.variant == Variant::DP83825I)
        reply.appendFields(" rcsr_valid=%u rcsr=%u", unsigned(straps.rcsrValid), unsigned(straps.rcsr));
    } else if (request.feature != Feature::NONE) {
      reply.fields(" core_status=%s core_detail=%" PRId32 " mutation_completed=%u mutation_possible=%u"
                   " cleanup_owed=%u temporary=%u persistent_restored=%u effective_reset=%s"
                   " reset_accepted=%u reset_complete=%u%s",
                   errorName(result.result.code), result.result.detail, unsigned(result.mutationCompleted),
                   unsigned(result.mutationPossible), unsigned(result.cleanupOwed), unsigned(result.temporary),
                   unsigned(result.persistentRestored), resetName(result.reset), unsigned(result.commandAccepted),
                   unsigned(result.protocolComplete), result.featureFields.text.data());
      reply.checkFeatureFields(result.featureFields);
    } else {
      const auto& pages = result.nextPages;
      char received[4 * MAX_NEXT_PAGES + 1]{};
      const auto pageCount = std::min(pages.receivedPages, MAX_NEXT_PAGES);
      for (std::size_t i = 0; i < pageCount; ++i) {
        char* const slot = received + 4 * i;
        if (pages.pages[i].valid)
          std::snprintf(slot, 5, "%04X", unsigned(pages.pages[i].raw));
        else std::memcpy(slot, "----", 4);
      }
      reply.fields(" core_status=%s core_detail=%" PRId32 " sdk_error=%" PRId32
                   " mutation_completed=%u mutation_possible=%u cleanup_owed=%u temporary=%u"
                   " an_complete=%u np_requested=%u np_complete=%u np_owed=%u np_refused=%u"
                   " np_base_event=%u np_base_valid=%u np_base=%u np_missing=%u"
                   " np_tx=%u np_rx=%u np_duplicates=%" PRIu32 " np_words=%s",
                   errorName(result.result.code), result.result.detail, receipt.sdkError,
                   unsigned(result.mutationCompleted), unsigned(result.mutationPossible),
                   unsigned(result.cleanupOwed), unsigned(result.temporary),
                   unsigned(result.negotiationComplete), unsigned(pages.requested), unsigned(pages.complete),
                   unsigned(pages.npMayRemainSet), unsigned(pages.partnerRefused), unsigned(pages.baseReceived),
                   unsigned(pages.basePageValid), unsigned(pages.basePage), unsigned(pages.pagePayloadMissing),
                   unsigned(pages.transmittedPages), unsigned(pageCount), pages.duplicatePages,
                   pageCount == 0 ? "none" : received);
    }
    if (result.token.value != 0) {
      _lastResult = {_job.requestId, result.token, reply.status, result.result, receipt.sdkError,
                     result.mutationCompleted, result.mutationPossible, result.cleanupOwed, result.temporary};
    }
  } else {
    uint64_t now = 0;
    const bool local = request.action == Action::ABANDON || request.action == Action::END;
    if (request.action == Action::END && reply.deadline != 0 && !sampleClock(now)) {
      request.budget.io.deadlineUs = reply.deadline = 0;
    }
    if (!local && !sampleClock(now)) {
      reply.status = clockError();
    } else if (!local && now >= reply.deadline) {
      reply.status = timeout(TimeoutDetail::DEADLINE);
    } else {
      reply.status = call(request.action);
      const bool featureCall = request.action == Action::FEATURE;
      if (featureCall) request.featureArgs = {}; // Only admission consumes the arguments.
      const auto mode = featureMode(request.feature);
      const bool start = request.action == Action::START_CONFIGURE || request.action == Action::START_VERIFY ||
          request.action == Action::START_INITIALIZE ||
          (featureCall && (mode == FeatureMode::START || mode == FeatureMode::RESET || mode == FeatureMode::OBSERVE));
      if (start && reply.status.inProgress() && receipt.token.value != 0) {
        request.token = receipt.token;
        _job.step = Work::POLL;
        reply.op = request.token.value;
        reply.state = "admitted";
        record(_job.command, _job.requestId, reply);
        return;
      }
      reply.status = checkedTerminal(reply.status);
      if (request.action == Action::PROBE) {
        reply.identityFields(receipt.identity);
      } else if (request.action == Action::OBSERVE_LINK) {
        if (!reply.status.ok()) receipt.link.lastRefreshStatus = reply.status;
        reply.linkFields(receipt.link, request.mask);
      } else if (request.action == Action::COLLECT_EVENTS) {
        if (!reply.status.ok()) receipt.counters.lastStatus = reply.status;
        reply.eventFields(receipt.counters, receipt.eventValidMask, receipt.eventWords);
      } else if (featureCall || request.action == Action::APPLICATION) {
        reply.fields("%s", receipt.featureFields.text.data());
        reply.checkFeatureFields(receipt.featureFields);
      } else if (isWrite(request.action)) {
        const auto release = receipt.batchValid ? errorName(receipt.releaseStatus.code) : "none";
        reply.fields(" effect=%s batch_valid=%u batch_violation=%u release=%s release_detail=%" PRId32,
                     effect(request.action, reply.status, receipt), unsigned(receipt.batchValid),
                     unsigned(receipt.violation), release,
                     receipt.batchValid ? receipt.releaseStatus.detail : 0);
      } else if (reply.status.ok() && (request.action == Action::READ_C22 || request.action == Action::READ_MMD)) {
        reply.fields(" address=%" PRIu32 " devad=%" PRIu32 " value=%u",
                     request.address, request.devad, unsigned(receipt.value));
      }
    }
  }
  record(_job.command, _job.requestId, reply);
  _job = {};
}

void Processor::finishRun(Status status) {
  if (status.ok() && _clockFault) status = clockError();
  Reply reply;
  reply.status = status;
  reply.deadline = _job.request.budget.io.deadlineUs;
  reply.fields(" rounds=%" PRIu32 " requested=%" PRIu32 " attempted=%" PRIu32 " attempted_known=%u managed=0",
               _job.rows, _job.end, _job.attempted, unsigned(_job.attemptedKnown));
  if (_job.step != Work::SELFTEST) {
    _lastRunReq = _job.requestId;
    _lastRounds = _job.rows;
    _lastAttempts = _job.attempted;
    _lastAttemptsKnown = _job.attemptedKnown;
    _lastRunStatus = status;
  }
  record(_job.command, _job.requestId, reply);
  _job = {};
}

void Processor::selftestStep() {
  if (!_job.stop.ok() || _job.rows == _job.end) { finishRun(_job.stop); return; }
  uint64_t now = 0;
  const auto deadline = _job.request.budget.io.deadlineUs;
  if (!sampleClock(now)) { finishRun(clockError()); return; }
  if (now >= deadline) { finishRun(timeout(TimeoutDetail::DEADLINE)); return; }
  auto& request = _job.request;
  Receipt receipt;
  Reply reply;
  reply.deadline = deadline;
  reply.type = "sample";
  reply.state = "progress";
  reply.status = checkedTerminal(_platform.execute(_platform.context, request, receipt));
  const bool identity = request.action == Action::PROBE;
  uint32_t attempted = 0;
  if (receipt.batchValid && receipt.violation == TransportViolation::NONE) {
    for (const auto& frame : receipt.batch.frames) if (frame.attempted) ++attempted;
  }
  const auto validFrame = [](const auto& frame) {
    return frame.attempted && frame.completed && frame.valueValid;
  };
  const bool complete = receipt.batchValid && attempted == (identity ? 2U : 1U) &&
      validFrame(receipt.batch.frames[0]) &&
      (!identity || (validFrame(receipt.batch.frames[1]) && receipt.identity.id1Valid && receipt.identity.id2Valid));
  if (receipt.violation != TransportViolation::NONE || (reply.status.ok() && !complete)) {
    _job.attemptedKnown = false;
    if (reply.status.ok()) reply.status = Status::Error(Err::INVALID_CONFIG, "No audited selftest batch",
                                                       static_cast<int32_t>(ConfigDetail::TRANSPORT));
  }
  _job.attempted += attempted;
  request.budget.io.maxFrames -= std::min(request.budget.io.maxFrames, attempted);
  if (!identity && reply.status.ok() && (receipt.value & cmd::MASK_PHYRCR_COMMAND) != 0) {
    reply.status = Status::Error(Err::BUSY, "PHY reset or restart in progress");
  }
  reply.fields(" check=%s id1_valid=%u id1=%u id2_valid=%u id2=%u value=%u attempted=%" PRIu32,
               identity ? "identity" : "reset_status", unsigned(receipt.identity.id1Valid), unsigned(receipt.identity.id1),
               unsigned(receipt.identity.id2Valid), unsigned(receipt.identity.id2), unsigned(receipt.value), attempted);
  if (reply.status.ok()) ++_job.rows;
  else _job.stop = reply.status;
  request.action = Action::READ_C22;
  request.address = cmd::REG_PHYRCR;
  record(_job.command, _job.requestId, reply);
}

void Processor::runStep() {
  if (!_job.stop.ok() || _job.rows == _job.end) { finishRun(_job.stop); return; }
  uint64_t now = 0;
  const auto deadline = _job.request.budget.io.deadlineUs;
  if (!sampleClock(now)) { finishRun(clockError()); return; }
  if (now >= deadline) { finishRun(timeout(TimeoutDetail::DEADLINE)); return; }
  if (now < _job.nextUs) return;

  Reply reply;
  reply.type = "sample";
  reply.state = "progress";
  reply.deadline = deadline;
  const auto round = _job.rows + 1;
  if (_job.step == Work::WATCH) {
    CachedSnapshot snapshot;
    reply.status = checkedTerminal(_platform.readCached(_platform.context, SnapshotKind::LINK, snapshot));
    if (!reply.status.ok()) snapshot.link.lastRefreshStatus = reply.status;
    reply.linkFields(snapshot.link, _job.request.mask, round);
  } else {
    Receipt receipt;
    reply.status = checkedTerminal(_platform.execute(_platform.context, _job.request, receipt));
    uint32_t attempted = 0;
    const auto& frame = receipt.batch.frames[0];
    const bool complete = receipt.batchValid && frame.attempted && frame.completed && frame.valueValid;
    if (receipt.violation != TransportViolation::NONE || (reply.status.ok() && !complete)) {
      _job.attemptedKnown = false;
      if (reply.status.ok()) reply.status = Status::Error(Err::INVALID_CONFIG, "Missing stress batch receipt",
                                                         static_cast<int32_t>(ConfigDetail::TRANSPORT));
    } else if (receipt.batchValid && frame.attempted) {
      attempted = 1;
      ++_job.attempted;
      --_job.request.budget.io.maxFrames;
    }
    reply.fields(" round=%" PRIu32 " address=%" PRIu32 " valid=%u value=%u attempted=%" PRIu32,
                 round, _job.request.address, unsigned(reply.status.ok()), unsigned(receipt.value), attempted);
  }
  if (reply.status.ok()) ++_job.rows;
  else _job.stop = reply.status;
  _job.nextUs = now + std::min(_job.intervalUs, deadline - now);
  record(_job.command, _job.requestId, reply);
}

void Processor::sequenceStep() {
  auto& job = _job;
  auto& request = job.request;
  const bool scan = job.step == Work::DISCOVERY;
  Reply reply;
  reply.deadline = request.budget.io.deadlineUs;
  Status stop = job.stop;
  if (stop.ok() && _clockFault) stop = clockError();
  if (stop.ok() && request.address <= job.end) {
    uint64_t now = 0;
    if (!sampleClock(now)) stop = clockError();
    else if (now >= reply.deadline) stop = timeout(TimeoutDetail::DEADLINE);
  }
  if (!stop.ok() || request.address > job.end) {
    reply.status = job.firstError.ok() ? stop : job.firstError;
    reply.fields(" rows=%" PRIu32 " errors=%" PRIu32 " skipped=%" PRIu32
                 " attempted=%" PRIu32 " complete=%u attempted_known=%u end_status=%s end_detail=%" PRId32,
                 job.rows, job.errors, job.skipped, job.attempted, unsigned(stop.ok()), unsigned(job.attemptedKnown),
                 errorName(stop.code), stop.detail);
    record(job.command, job.requestId, reply);
    job = {};
    return;
  }

  Receipt receipt{};
  reply.status = checkedTerminal(_platform.execute(_platform.context, request, receipt));
  const bool skipped = !scan && reply.status.is(Err::INVALID_PARAM) &&
      reply.status.detail == static_cast<int32_t>(ParamDetail::ACCESS_POLICY);
  uint32_t attempted = 0;
  if (receipt.violation != TransportViolation::NONE || (reply.status.ok() && !receipt.batchValid)) {
    if (reply.status.ok()) {
      reply.status = Status::Error(Err::INVALID_CONFIG, "Range owner did not supply an audited batch",
                                   static_cast<int32_t>(ConfigDetail::TRANSPORT));
    }
    job.attemptedKnown = false;
    job.stop = reply.status;
  } else if (receipt.batchValid) {
    for (const auto& frame : receipt.batch.frames) if (frame.attempted) ++attempted;
  }
  job.attempted += attempted;
  auto& remainingFrames = request.budget.io.maxFrames;
  remainingFrames = attempted > remainingFrames ? 0 : remainingFrames - attempted;
  ++job.rows;
  if (skipped) ++job.skipped;
  else if (!reply.status.ok()) {
    ++job.errors;
    if (job.firstError.ok()) job.firstError = reply.status;
  }
  reply.type = "sample";
  reply.state = "progress";
  if (scan) {
    const auto& id = receipt.identity;
    const bool candidate = reply.status.ok() && id.id1Valid && id.id2Valid &&
        id.id1 != 0 && id.id1 != 65535 && id.id2 != 0 && id.id2 != 65535;
    reply.fields(" address=%" PRIu32 " id1_valid=%u id1=%u id2_valid=%u id2=%u"
                 " candidate=%u attempted=%" PRIu32 " batch_valid=%u batch_violation=%u",
                 request.address, unsigned(id.id1Valid), unsigned(id.id1),
                 unsigned(id.id2Valid), unsigned(id.id2), unsigned(candidate),
                 attempted, unsigned(receipt.batchValid), unsigned(receipt.violation));
  } else {
    reply.fields(" address=%" PRIu32 " devad=%" PRIu32 " valid=%u value=%u skipped=%u"
                 " attempted=%" PRIu32 " batch_valid=%u batch_violation=%u",
                 request.address, request.devad, unsigned(reply.status.ok()), unsigned(receipt.value),
                 unsigned(skipped), attempted, unsigned(receipt.batchValid), unsigned(receipt.violation));
  }
  record(job.command, job.requestId, reply);
  ++request.address;
  if (reply.status.is(Err::BUSY) || reply.status.is(Err::OPERATION_TIMEOUT) ||
      reply.status.is(Err::NOT_BOUND) || isClockError(reply.status)) {
    // Global denial ends the range. The sample keeps the attempted address and prefix.
    job.stop = reply.status;
  }
}
} // namespace DP8382x::Cli
