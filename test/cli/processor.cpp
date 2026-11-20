#include "Dp8382xCli.h"

#include <cassert>
#include <limits>
#include <string>
#include <vector>

namespace {
using namespace ::DP8382x;
using namespace ::DP8382x::Cli;

struct Stream {
  Processor cli;
  std::string bytes;
  unsigned writes{0};
  unsigned reads{0};
  std::size_t acceptedPerWrite{Processor::WRITE_BYTES};
  SnapshotKind lastRead{SnapshotKind::CAPABILITIES};
  Status outcome{};
  bool staleLink{false};

  static std::size_t write(void* context, const char* data, std::size_t length) {
    auto& self = *static_cast<Stream*>(context);
    assert(length <= Processor::WRITE_BYTES);
    ++self.writes;
    const auto accepted = std::min(length, self.acceptedPerWrite);
    self.bytes.append(data, accepted);
    return accepted;
  }
  static Status read(void* context, SnapshotKind kind, CachedSnapshot& out) {
    auto& self = *static_cast<Stream*>(context);
    ++self.reads;
    self.lastRead = kind;
    out.health.state = DriverState::READY;
    out.health.lastError = Status::Error(Err::MDIO_ERROR, "", std::numeric_limits<int32_t>::min());
    out.link.valid = out.link.resolvedModeValid = out.link.linkUp = true;
    out.link.stale = self.staleLink;
    out.link.speed = LinkSpeed::MBPS_10;
    out.link.duplex = Duplex::HALF;
    out.settings.trust = ConfigurationTrust::DIRTY;
    out.operation.state = OperationState::RUNNING;
    out.operation.status = {Err::IN_PROGRESS, 0, ""};
    out.operation.token.value = 99;
    return self.outcome;
  }
  static uint64_t now(void*) { return std::numeric_limits<uint64_t>::max(); }

  void attach(bool clock = true) {
    Identity identity;
    identity.boot = 4294967295U;
    assert(cli.setup({this, write, read, clock ? now : nullptr}, identity));
    assert(!cli.setup({this, write, read, nullptr}, identity));
    assert(reads == 0 && writes == 0);
  }
  void drain() {
    for (unsigned i = 0; i < 10000 && cli.outputPending(); ++i) cli.tick();
    assert(!cli.outputPending());
  }
  void send(std::string_view line) {
    while (!line.empty()) {
      const auto count = cli.feed(line);
      assert(count != 0 && count <= Processor::FEED_BYTES);
      line.remove_prefix(count);
    }
    drain();
  }
  std::vector<std::string_view> records() const {
    std::vector<std::string_view> found;
    std::string_view output(bytes);
    while (!output.empty()) {
      const auto end = output.find("\r\n");
      const auto line = output.substr(0, end);
      if (line.substr(0, 9) == "@DP8382X ") {
        assert(end != std::string_view::npos);
        assert(end + 2 <= ConsoleBuffer::MAX_RECORD_BYTES);
        assert(line.find('\x1b') == std::string_view::npos);
        found.push_back(line);
      }
      if (end == std::string_view::npos) break;
      output.remove_prefix(end + 2);
    }
    return found;
  }
};

void has(std::string_view record, std::string_view fragment) {
  assert(record.find(fragment) != std::string_view::npos);
}

void grammar() {
  Stream s;
  const Platform platform{&s, Stream::write, Stream::read, nullptr};
  Identity identity;
  identity.firmware = "bad identity\n";
  assert(!s.cli.setup(platform, identity));
  assert(s.cli.feed("ver\n") == 0);
  s.attach();
  const std::string input = "help\tlocal\r\n?\nveX\x7f" "r\rver\r\n"
      "local color on\n?\nlocal color OFF\nlocal color off\n"
      "local quiet 0x1\n?\nlocal quiet +0\nlocal quiet -0\n"
      "local quiet 4294967296\nlocal quiet 0x100000000\nlocal quiet 01junk\n"
      "local quiet 0\nver extra\nhelp absent\n";
  for (const char ch : input) s.send(std::string_view(&ch, 1));
  const auto r = s.records();
  assert(r.size() == 18 && s.reads == 0);
  has(r[0], "cmd=help");
  has(r[2], "cmd=version");
  has(r[3], "req=4 type=result cmd=version");
  has(r[6], "reason=enum");
  has(r[8], "quiet=1");
  has(r[9], "cmd=help op=0 state=terminal primary=OK");
  for (std::size_t i = 10; i <= 14; ++i) has(r[i], "reason=number");
  has(r[15], "quiet=0");
  has(r[16], "reason=arity");
  has(r[17], "reason=topic");
  has(s.bytes, "\x1b[36m=== DP8382x CLI Help ===\x1b[0m");
  has(s.bytes, "cached health");
}

void loss() {
  Stream s;
  s.attach(false);
  s.send("local quiet ");
  s.cli.discardInput();
  s.send("\b\b1\r\n");
  s.send("local quiet\n");
  constexpr char malformed[] = "drv\0\b\n\xc3\xa9\n\x1a\n";
  s.send(std::string_view(malformed, sizeof(malformed) - 1));
  s.send(std::string(193, 'x') + "\bdrv\n");
  s.send(std::string(65, 'x') + "\n");
  s.send("a a a a a a a a a a a\n\n");
  s.send("ver\r");
  s.cli.discardInput();
  s.send("\nver\n");
  const auto r = s.records();
  assert(r.size() == 12 && s.reads == 0);
  has(r[0], "reason=input_loss");
  has(r[1], "quiet=0");
  for (std::size_t i = 2; i <= 4; ++i) has(r[i], "reason=byte");
  has(r[5], "reason=line_length");
  has(r[6], "reason=token_length");
  has(r[7], "reason=token_count");
  has(r[8], "reason=empty");
  has(r[10], "reason=input_loss");
  has(r[11], "req=12 type=result cmd=version");
  has(r[11], "time_valid=0 time_us=0");
}

void cached() {
  Stream s;
  s.attach();
  s.send("drv\n");
  assert(s.lastRead == SnapshotKind::HEALTH);
  s.send("cached link\n");
  s.staleLink = true;
  s.send("cached link\n");
  s.send("cfg\n");
  assert(s.lastRead == SnapshotKind::SETTINGS);
  s.send("cached operation\n");
  s.outcome = Status::Error(Err::BUSY, "", -47);
  s.send("caps\n");
  s.outcome = {Err::IN_PROGRESS, -48, ""};
  s.send("cached transport\n");
  const auto r = s.records();
  assert(s.reads == 7 && r.size() == 7);
  has(r[0], "health=READY");
  has(r[0], "last_detail=-2147483648");
  has(r[1], "up=1 resolved=1 speed_mbps=10 duplex=half");
  has(r[2], "valid=1 stale=1 up=0 resolved=0 speed_mbps=0 duplex=unknown");
  has(r[3], "trust=DIRTY");
  has(r[4], "cmd=cached.operation op=0 state=terminal primary=OK");
  has(r[4], "slot=running token=99 slot_status=IN_PROGRESS");
  has(r[5], "primary=BUSY detail=-47");
  has(r[6], "primary=INVALID_CONFIG detail=-48");
  has(r[6], "reason=callback");
}

void retention() {
  Stream s;
  s.attach();
  s.acceptedPerWrite = 0;
  unsigned admitted = 0;
  for (unsigned i = 0; i < 100; ++i) {
    if (s.cli.feed("drv\n") == 0) break;
    ++admitted;
  }
  assert(admitted > 1 && admitted < 100 && s.reads == admitted);
  const std::string session = "session " + std::string(64, 'Z') + "\n";
  assert(s.cli.feed(session) == 0);
  s.cli.tick();
  assert(s.writes == 1 && s.bytes.empty() && s.cli.outputPending());
  s.acceptedPerWrite = 3;
  s.drain();
  s.send(session);
  s.send("local console\n");
  const auto r = s.records();
  assert(r.size() == admitted + 2 && s.reads == admitted);
  for (unsigned i = 0; i < admitted; ++i) {
    has(r[i], "req=" + std::to_string(i + 1) + " type=result cmd=cached.health");
  }
  has(r[admitted], "boot=4294967295 req=" + std::to_string(admitted + 1) + " type=session");
  has(r[admitted], "nonce=" + std::string(64, 'Z') + " library=");
  has(r[admitted], "time_valid=1 time_us=18446744073709551615");
  has(r[admitted + 1], "stalled_writes=1");
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string_view suite(argv[1]);
  if (suite == "grammar") grammar();
  else if (suite == "loss") loss();
  else if (suite == "cached") cached();
  else if (suite == "retention") retention();
  else return 2;
}
