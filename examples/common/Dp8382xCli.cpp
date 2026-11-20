/// @file Dp8382xCli.cpp
/// @brief Strict line dispatch and bounded, correlated output.
#include "Dp8382xCli.h"
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
} // namespace

struct Processor::Reply {
  Status status{};
  const char* reason{"none"};
  const char* type{"result"};
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
};

const Processor::Command Processor::COMMANDS[] = {
    {"help", "?", 0, 1, 0, &Processor::help, "[topic]", "Command help"},
    {"version", "ver", 0, 0, 0, &Processor::identity, "", "Library and firmware identity"},
    {"session", "", 1, 1, 0, &Processor::identity, "<nonce>", "Correlate a session"},
    {"capabilities", "caps", 0, 0, unsigned(SnapshotKind::CAPABILITIES), &Processor::cached, "", "Cached capabilities"},
    {"local board", "", 0, 0, 0, &Processor::identity, "", "Declared board and backend"},
    {"local color", "", 0, 1, 0, &Processor::configure, "", "Set or show ANSI style"},
    {"local quiet", "", 0, 1, 1, &Processor::configure, "", "Set or show human-output filter"},
    {"local console", "", 0, 0, 0, &Processor::console, "", "Console backpressure counters"},
    {"cached health", "drv|health", 0, 0, unsigned(SnapshotKind::HEALTH), &Processor::cached, "", "Managed MDIO health"},
    {"cached link", "", 0, 0, unsigned(SnapshotKind::LINK), &Processor::cached, "", "Last link and resolution"},
    {"cached settings", "cfg|settings", 0, 0, unsigned(SnapshotKind::SETTINGS), &Processor::cached, "", "Configuration intent and trust"},
    {"cached operation", "", 0, 0, unsigned(SnapshotKind::OPERATION), &Processor::cached, "", "Operation state and restoration debt"},
    {"cached transport", "", 0, 0, unsigned(SnapshotKind::TRANSPORT), &Processor::cached, "", "MDIO batch and frame totals"},
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
}

bool Processor::outputPending() const {
  return _output.queuedBytes() != 0 || _recordSize != 0;
}

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

  const std::string_view name = selected ? selected->name : "unknown";
  char commandName[TOKEN_BYTES + 1]{};
  std::transform(name.begin(), name.end(), commandName, [](char c) { return c == ' ' ? '.' : c; });
  const uint64_t nowUs = _platform.nowUs ? _platform.nowUs(_platform.context) : 0;
  const int length = std::snprintf(_record.data(), _record.size(),
      "%sschema=1 boot=%" PRIu32 " req=%" PRIu64 " type=%s cmd=%s op=0 state=terminal"
      " primary=%s detail=%" PRId32 " cleanup=none cleanup_detail=0"
      " time_valid=%u time_us=%" PRIu64 " deadline_valid=0 deadline_us=0 reason=%s%s\r\n",
      PREFIX, _identity.boot, _request, reply.type, commandName, errorName(reply.status.code),
      reply.status.detail, unsigned(_platform.nowUs != nullptr), nowUs, reply.reason,
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
        PREFIX, _identity.boot, _request);
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
      human(group);
      human("\r\n");
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
    std::snprintf(line, sizeof(line), "  %-17s %-15s %-14s %s\r\n",
                  command.name.data(), usage, alias, command.summary);
    human(line);
  }
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
      const auto delim = option.enumValues.find('|');
      const auto disabled = option.enumValues.substr(0, delim);
      const auto enabled = option.enumValues.substr(delim + 1);
      if (args[0] != enabled && args[0] != disabled) {
        reply.reject("enum");
        return;
      }
      setting = args[0] == enabled;
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
  if (reply.status.inProgress() || std::strcmp(errorName(reply.status.code), "UNKNOWN") == 0) {
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
  }
}
} // namespace DP8382x::Cli
