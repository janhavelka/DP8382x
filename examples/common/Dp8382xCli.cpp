/// @file Dp8382xCli.cpp
/// @brief Strict line dispatch and bounded, correlated output.
#include "Dp8382xCli.h"
#include <DP8382x/CommandTable.h>
#include <DP8382x/Version.h>
#include <algorithm>
#include <cinttypes>
#include <cstdarg>
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

bool unsignedNumber(std::string_view text, uint32_t& result) {
  uint32_t base = 10;
  if (text.substr(0, 2) == "0x" || text.substr(0, 2) == "0X") {
    text.remove_prefix(2);
    base = 16;
  }
  if (text.empty()) return false;
  uint32_t value = 0;
  for (const char c : text) {
    uint32_t digit = base;
    if (c >= '0' && c <= '9') digit = static_cast<uint32_t>(c - '0');
    if (c >= 'A' && c <= 'F') digit = static_cast<uint32_t>(c - 'A') + 10U;
    if (c >= 'a' && c <= 'f') digit = static_cast<uint32_t>(c - 'a') + 10U;
    if (digit >= base || value > (std::numeric_limits<uint32_t>::max() - digit) / base) return false;
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

  void fields(const char* format, ...) {
    va_list args;
    va_start(args, format);
    const int count = std::vsnprintf(payload.data(), payload.size(), format, args);
    va_end(args);
    if (count < 0 || static_cast<std::size_t>(count) >= payload.size()) {
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
    {"cached health", "drv|health", 0, 0, unsigned(SnapshotKind::HEALTH), &Processor::cached, "", "Managed MDIO health"},
    {"cached link", "", 0, 0, unsigned(SnapshotKind::LINK), &Processor::cached, "", "Last link and resolution"},
    {"cached settings", "cfg|settings", 0, 0, unsigned(SnapshotKind::SETTINGS), &Processor::cached, "", "Configuration intent and trust"},
    {"cached operation", "", 0, 0, unsigned(SnapshotKind::OPERATION), &Processor::cached, "", "Operation state and restoration debt"},
    {"cached transport", "", 0, 0, unsigned(SnapshotKind::TRANSPORT), &Processor::cached, "", "MDIO batch and frame totals"},
    {"connection identity", "", 0, 0, unsigned(SnapshotKind::IDENTITY), &Processor::cached, "", "Last accepted or rejected identity"},
    {"connection probe", "", 0, 0, PROBE, &Processor::route, "", "Acquire bound identity"},
    {"connection discover", "", 0, 2, SCAN, &Processor::route, "[first last]", "Read raw IDs, never select address"},
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
};

bool Processor::setup(const Platform& platform, const Identity& identity) {
  if (_platform.write || !platform.write || !platform.readCached ||
      !safeString(identity.firmware) || !safeString(identity.tree) ||
      !safeString(identity.sdk) || !safeString(identity.target) ||
      !safeString(identity.backend) || !safeString(identity.clock) ||
      !isValidPhyAddress(identity.phyAddress) ||
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
  while (accepted < std::min(bytes.size(), FEED_BYTES) && _recordSize == 0) {
    const char ch = bytes[accepted++];
    if (_afterCr) {
      _afterCr = false;
      if (ch == '\n') continue;
    }
    if (ch == '\r' || ch == '\n') {
      _afterCr = ch == '\r';
      finishLine();
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
  std::string_view previousGroup;
  for (const auto& command : COMMANDS) {
    const auto space = command.name.find(' ');
    const auto group = command.handler == &Processor::configure ? CONFIG[command.flags].group :
        space == std::string_view::npos ? "General" : command.name.substr(0, space);
    if (!topic.empty() && topic != command.name.substr(0, space)) continue;

    if (group != previousGroup) {
      if (previousGroup == "configuration") profileHelp();
      char heading[32]{};
      std::snprintf(heading, sizeof(heading), "%.*s\r\n", int(group.size()), group.data());
      human(heading);
      previousGroup = group;
    }
    char usage[96]{};
    if (command.handler == &Processor::configure) {
      const auto& option = CONFIG[command.flags];
      if (!option.enumValues.empty()) std::snprintf(usage, sizeof(usage), "[%s]", option.enumValues.data());
      else std::snprintf(usage, sizeof(usage), "[%" PRIu32 "..%" PRIu32 " %s]", option.min, option.max, option.unit);
    } else {
      std::snprintf(usage, sizeof(usage), "%s", command.usage);
    }

    char line[224]{};
    const char* alias = command.alias.empty() ? "" : command.alias.data();
    std::snprintf(line, sizeof(line), "  %-24s %-38s %-12s %s\r\n",
                  command.name.data(), usage, alias, command.summary);
    human(line);
  }
  if (previousGroup == "configuration") profileHelp();
  if (previousGroup == "registers") {
    human("  C22 offsets 0..31. MMD takes an explicit DEVAD and 16-bit offset.\r\n"
          "  Dumps skip protected registers. Writes require maintenance.\r\n"
          "  LEDCR 0x18 or 31:0x18 allows bits 0x0692. BMCR allows mask 0x3100.\r\n");
  }
}

void Processor::profileHelp() {
  char lines[600]{};
  std::snprintf(lines, sizeof(lines),
      "  an <modes %u..%u> <%s> <%s> <flags %u..%u>\r\n"
      "     Modes: 1 10/half, 2 10/full, 4 100/half, 8 100/full. Combine bits.\r\n"
      "     Optional: wait, or <partner-limit 1..16> <packed four-digit hex pages>\r\n"
      "  forced <%s> <%s> <%s> <flags>\r\n"
      "  Flags: 1 robust, 2 coupled pair/polarity mirror, 4 extended full duplex.\r\n"
      "  Forced requires robust. Extended duplex excludes forced 10 and native half duplex.\r\n",
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
               " clock=%s variant=%s interface=%s address=%" PRIu32,
               int(nonce.size()), nonce.data(), VERSION_STRING, _identity.firmware, _identity.tree,
               _identity.sdk, _identity.target, _identity.backend, _identity.clock,
               _identity.variant == Variant::DP83826I ? "DP83826I" : "unknown",
               interfaceName(_identity.interface), _identity.phyAddress);
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
  reply.fields(" queued=%zu dropped_human=%" PRIu64 " stalled_writes=%" PRIu64,
               _output.queuedBytes(), uint64_t(_output.droppedHuman()),
               uint64_t(_output.stalledWrites()));
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
      break;
  }
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
    reply.status = validateConfiguration(request.settings, request.options);
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
    if (count == 0 && _identity.variant == Variant::DP83826I) end = 7;
    else if (count != 2 || !unsignedNumber(args[0], request.address) || !unsignedNumber(args[1], end) ||
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
  }

  if (!_platform.execute || !_platform.nowUs) {
    reply.status = Status::Error(Err::INVALID_CONFIG, "No live owner route");
    reply.reason = "backend";
    return;
  }
  if (workPending()) {
    reply.status = Status::Error(Err::BUSY, "CLI work owns service slot",
                                 static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
    return;
  }
  uint64_t now = 0;
  if (!sampleClock(now)) {
    reply.status = clockError();
    return;
  }
  if (now > std::numeric_limits<uint64_t>::max() - durationUs) {
    reply.status = timeout(TimeoutDetail::BOUND_ADMISSION);
    return;
  }
  request.budget.io = {now + durationUs, frames};
  request.budget.pollIntervalUs = 1000;
  request.budget.maxStalledPolls = 4096;
  _job = {};
  _job.step = step;
  _job.request = request;
  _job.requestId = _request;
  _job.command = command.name;
  _job.end = end;
  reply.deferred = true;
}

void Processor::serviceJob() {
  auto& request = _job.request;
  Receipt receipt{};
  const auto call = [&](Action action) {
    request.action = action;
    return _platform.execute(_platform.context, request, receipt);
  };
  if (_job.step == Work::POLL) {
    uint64_t now = 0;
    if (!sampleClock(now)) {
      // Freeze the token without advancing its hardware work on a broken clock.
      if (call(Action::CANCEL).is(Err::BUSY)) return;
      _job.stop = clockError();
    } else {
      const auto status = call(Action::POLL);
      if (status.inProgress()) return; // Output backpressure cannot withhold polls.
      if (isClockError(status)) _clockFault = true;
    }
    _job.step = Work::TAKE;
    return;
  }
  if (_recordSize != 0) return;
  if (_job.step == Work::DISCOVERY || _job.step == Work::DUMP) {
    sequenceStep();
    return;
  }

  Reply reply;
  reply.op = request.token.value;
  reply.deadline = request.budget.io.deadlineUs;
  if (_job.step == Work::TAKE || _job.step == Work::RECONCILE) {
    if (_job.step == Work::TAKE) {
      const auto delivered = checkedTerminal(call(Action::TAKE));
      if (delivered.is(Err::BUSY) || delivered.is(Err::RESULT_NOT_AVAILABLE)) {
        _job.step = Work::POLL;
        return;
      }
      receipt.result = delivered.ok() ? checkedTerminal(receipt.result) : delivered;
      _job.result = receipt;
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
  } else {
    uint64_t now = 0;
    if (!sampleClock(now)) {
      reply.status = clockError();
    } else if (now >= reply.deadline) {
      reply.status = timeout(TimeoutDetail::DEADLINE);
    } else {
      reply.status = call(request.action);
      const bool start = request.action == Action::START_CONFIGURE || request.action == Action::START_VERIFY;
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
      } else if (isWrite(request.action)) {
        const auto release = receipt.batchValid ? errorName(receipt.releaseStatus.code) : "none";
        reply.fields(" effect=%s batch_valid=%u batch_violation=%u release=%s release_detail=%" PRId32,
                     effect(request.action, reply.status, receipt), unsigned(receipt.batchValid),
                     unsigned(receipt.violation), release,
                     receipt.batchValid ? receipt.releaseStatus.detail : 0);
      } else if (reply.status.ok() && request.action != Action::INVALIDATE) {
        reply.fields(" address=%" PRIu32 " devad=%" PRIu32 " value=%u",
                     request.address, request.devad, unsigned(receipt.value));
      }
    }
  }
  record(_job.command, _job.requestId, reply);
  _job = {};
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
