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
  bool discoveryReceipt{true};
  bool cancelled{false};
  uint64_t timeUs{1234};
  std::vector<Request> requests;

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
  static uint64_t liveClock(void* ptr) { return static_cast<Stream*>(ptr)->timeUs; }
  static Status live(void* ptr, const Request& request, Receipt& receipt) {
    auto& s = *static_cast<Stream*>(ptr);
    s.requests.push_back(request);
    if (request.action == Action::DISCOVER && s.discoveryReceipt) {
      receipt.batchValid = true;
      receipt.batch.frames[0].attempted = receipt.batch.frames[1].attempted = true;
      receipt.identity.id1 = 0x2000;
      receipt.identity.id2 = 0xA131;
      receipt.identity.id1Valid = receipt.identity.id2Valid = true;
    } else if (request.action == Action::WRITE_C22) {
      receipt.batchValid = true;
      receipt.batch.frames[0].attempted = receipt.batch.frames[0].completed = true;
      receipt.batch.releaseStatus = {static_cast<Err>(255), -71, "Malformed release"};
      receipt.violation = TransportViolation::RELEASE_REPORT;
      receipt.releaseStatus = Status::Error(Err::MDIO_ERROR, "Invalid release report", -71);
      return receipt.releaseStatus;
    } else if (request.action == Action::START_CONFIGURE) {
      receipt.token.value = 81;
      return {Err::IN_PROGRESS, 0, ""};
    } else if (request.action == Action::CANCEL) {
      s.cancelled = true;
    } else if (request.action == Action::TAKE) {
      receipt.mutationCompleted = true;
      if (s.cancelled) {
        receipt.result = Status::Error(Err::CANCELLED, "Clock invalid");
        receipt.cleanupOwed = true;
      }
    } else if (request.action == Action::RECONCILE) {
      receipt.sdkError = 0x103;
    }
    return Status::Ok();
  }

  void attachLive() {
    Identity i;
    i.boot = 44;
    i.variant = Variant::DP83826I;
    assert(cli.setup({this, write, read, liveClock, live}, i));
  }

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
  const auto profileHelp = s.bytes.find("  an <modes");
  assert(s.bytes.find("\r\nconfiguration\r\n") < profileHelp);
  assert(profileHelp < s.bytes.find("\r\nregisters\r\n"));
  has(s.bytes, "Up to 32 policy-checked rows\r\n");
  has(s.bytes, "LEDCR 0x18 or 31:0x18 allows bits 0x0692.");
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
  s.outcome = {Err::OK, 5, nullptr};
  s.send("cached transport\n");
  const auto r = s.records();
  assert(s.reads == 8 && r.size() == 8);
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
  has(r[7], "primary=INVALID_CONFIG detail=5");
  has(r[7], "time_valid=1 time_us=18446744073709551615");
  has(r[7], "reason=callback");
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

void routes() {
  Stream scan;
  scan.attachLive();
  scan.send("connection discover 0 2\nconnection discover 0 0\nsession Scan44\n");
  // Each tick can call the owner once, even when the next tick uses the same clock.
  while (scan.cli.workPending()) {
    const auto before = scan.requests.size();
    scan.cli.tick();
    assert(scan.requests.size() <= before + 1);
  }
  scan.drain();
  assert(scan.requests.size() == 3);
  for (uint32_t i = 0; i < 3; ++i) {
    assert(scan.requests[i].action == Action::DISCOVER);
    assert(scan.requests[i].address == i);
    assert(scan.requests[i].budget.io.deadlineUs == 2001234U);
    assert(scan.requests[i].budget.io.maxFrames == 6U - 2U * i);
  }
  const auto rows = scan.records();
  assert(rows.size() == 6);
  has(rows[0], "req=2 type=result cmd=connection.discover op=0 state=terminal primary=BUSY");
  has(rows[1], "req=3 type=session cmd=session");
  has(rows[1], "nonce=Scan44");
  has(rows[2], "req=1 type=sample");
  has(rows[5], "rows=3 errors=0 skipped=0 attempted=6 complete=1");

  Stream regression;
  regression.attachLive();
  assert(regression.cli.feed("connection discover 0 2\n") == 24);
  ++regression.timeUs;
  regression.cli.tick();
  regression.timeUs -= 1;
  regression.cli.tick();
  assert(regression.requests.size() == 1 && !regression.cli.workPending());
  regression.drain();
  has(regression.records()[1], "primary=INVALID_CONFIG");
  has(regression.records()[1], "time_valid=0 time_us=0");
  has(regression.records()[1], "rows=1 errors=0 skipped=0 attempted=2 complete=0");
  regression.timeUs += 2;
  regression.send("connection discover 0 2\n");
  assert(regression.requests.size() == 1);
  has(regression.records().back(), "primary=INVALID_CONFIG detail=5");

  Stream expiry;
  expiry.attachLive();
  assert(expiry.cli.feed("connection discover 0 2\n") == 24);
  expiry.timeUs = 2001233;
  expiry.cli.tick();
  expiry.timeUs = 2001234;
  expiry.cli.tick();
  expiry.drain();
  assert(expiry.requests.size() == 1);
  has(expiry.records()[1], "primary=OPERATION_TIMEOUT");
  has(expiry.records()[1], "rows=1 errors=0 skipped=0 attempted=2 complete=0");

  Stream incomplete;
  incomplete.discoveryReceipt = false;
  incomplete.attachLive();
  incomplete.send("connection discover 0 2\n");
  assert(incomplete.requests.size() == 1);
  has(incomplete.records()[0], "primary=INVALID_CONFIG detail=1");
  has(incomplete.records()[1], "attempted=0 complete=0 attempted_known=0 end_status=INVALID_CONFIG");

  Stream write;
  write.attachLive();
  write.send("registers write c22 24 16\n");
  assert(write.requests.size() == 1);
  has(write.records()[0], "primary=MDIO_ERROR detail=-71");
  has(write.records()[0], "effect=possible batch_valid=1 batch_violation=4 release=MDIO_ERROR release_detail=-71");

  Stream configure;
  configure.attachLive();
  const std::string_view command = "configuration apply an 15 both auto 1 2 20040003\n";
  assert(configure.cli.feed(command) == command.size());
  configure.acceptedPerWrite = 0;
  configure.cli.tick(); // Owner admits the job, queues the admission record.
  assert(configure.requests.size() == 1);
  const auto& pages = configure.requests.front().options.nextPages;
  assert(pages.maxPartnerPages == 2 && pages.pageCount == 2);
  assert(pages.pages[0].message && !pages.pages[0].acknowledge2 && pages.pages[0].data == 4);
  assert(!pages.pages[1].message && !pages.pages[1].acknowledge2 && pages.pages[1].data == 3);
  for (unsigned i = 0; i < 100 && configure.cli.feed("ver\n") != 0; ++i) {}
  assert(configure.cli.feed("session waiting\n") == 0);
  configure.cli.tick(); // An admitted core poll continues even if output is stalled.
  assert(configure.requests.size() == 2 && configure.requests[1].action == Action::POLL);
  configure.cli.tick();
  assert(configure.requests.size() == 2 && configure.cli.workPending());
  configure.acceptedPerWrite = 2;
  for (unsigned i = 0; i < 5000 && configure.cli.workPending(); ++i) configure.cli.tick();
  assert(!configure.cli.workPending());
  configure.drain();
  const auto results = configure.records();
  has(results.front(), "req=1 type=result cmd=configuration.apply op=81 state=admitted primary=IN_PROGRESS");
  has(results.back(), "req=1 type=result cmd=configuration.apply op=81 state=terminal primary=INVALID_CONFIG");
  has(results.back(), "core_status=OK core_detail=0 sdk_error=259 mutation_completed=1");
  assert(configure.requests.size() == 4);

  Stream cancel;
  cancel.attachLive();
  assert(cancel.cli.feed(command) == command.size());
  cancel.cli.tick(); // Admit at 1234 us. Session observes a later time than the owner.
  cancel.timeUs = 2000;
  assert(cancel.cli.feed("session upstream\n") == 17);
  cancel.timeUs = 1999;
  cancel.cli.tick();
  cancel.cli.tick();
  cancel.drain();
  assert(cancel.requests.size() == 3);
  assert(cancel.requests[1].action == Action::CANCEL && cancel.requests[2].action == Action::TAKE);
  has(cancel.records().back(), "req=1 type=result cmd=configuration.apply op=81 state=terminal");
  has(cancel.records().back(), "primary=INVALID_CONFIG detail=5");
  has(cancel.records().back(), "time_valid=0 time_us=0");
  has(cancel.records().back(), "core_status=CANCELLED core_detail=0 sdk_error=0 mutation_completed=1");
  has(cancel.records().back(), "cleanup_owed=1");
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string_view suite(argv[1]);
  if (suite == "grammar") grammar();
  else if (suite == "loss") loss();
  else if (suite == "cached") cached();
  else if (suite == "retention") retention();
  else if (suite == "routes") routes();
  else return 2;
}
