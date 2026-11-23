#include "Dp8382xConsoleService.h"
#include "../support/Check.h"

#include <array>
#include <atomic>
#include <cstdarg>
#include <string>
#include <thread>

namespace {
using namespace DP8382x;
using namespace DP8382x::Cli;

struct Sink {
  std::string bytes;
  std::size_t limit{NativeConsole::DRAIN_BYTES};
  unsigned calls{0};

  static std::size_t write(void* opaque, const char* bytes, std::size_t size) {
    auto& sink = *static_cast<Sink*>(opaque);
    CHECK(size != 0 && size <= NativeConsole::DRAIN_BYTES);
    ++sink.calls;
    const auto accepted = std::min(size, sink.limit);
    sink.bytes.append(bytes, accepted);
    return accepted;
  }

  std::size_t drain(NativeConsole& console) {
    const auto before = calls;
    const auto accepted = console.drain(this, write);
    CHECK(calls <= before + 1U);
    return accepted;
  }
};

struct Fixture {
  NativeConsole console;
  Processor processor;
  Sink sink;
  uint64_t nowUs{0};
  unsigned starts{0}, polls{0}, takes{0}, unexpected{0};
  bool finishAllowed{false}, resultPending{false};

  Fixture() {
    Identity id;
    id.boot = 47;
    CHECK(processor.setup({this, write, cache, clock, execute}, id));
  }

  void offer(std::string_view input) { CHECK(console.offerInput(input) == input.size()); }

  void service(unsigned steps, bool drain = true) {
    for (unsigned i = 0; i < steps; ++i) {
      nowUs += 100;
      const auto before = starts + polls + takes;
      console.service(processor);
      CHECK(starts + polls + takes <= before + 1U);
      if (drain) (void)sink.drain(console);
    }
  }

  static std::size_t write(void* context, const char* bytes, std::size_t count) {
    return static_cast<Fixture*>(context)->console.write(bytes, count);
  }

  static uint64_t clock(void* context) { return static_cast<Fixture*>(context)->nowUs; }

  static Status cache(void*, SnapshotKind, CachedSnapshot&) { return Status::Ok(); }

  static Status execute(void* context, const Request& request, Receipt& receipt) {
    auto& fixture = *static_cast<Fixture*>(context);
    switch (request.action) {
      case Action::START_INITIALIZE:
        ++fixture.starts;
        receipt.token = {41};
        return Status::Error(Err::IN_PROGRESS, "Fixture admitted");
      case Action::POLL:
        CHECK(request.token.value == 41);
        ++fixture.polls;
        if (!fixture.finishAllowed) return Status::Error(Err::IN_PROGRESS, "Fixture running");
        fixture.resultPending = true;
        return Status::Ok();
      case Action::TAKE:
        CHECK(request.token.value == 41 && fixture.resultPending);
        ++fixture.takes;
        fixture.resultPending = false;
        receipt.result = Status::Ok();
        receipt.persistentRestored = true;
        return Status::Ok();
      default:
        ++fixture.unexpected;
        return Status::Error(Err::INVALID_PARAM, "Unexpected fixture request");
    }
  }
};

int formatted(NativeConsole& console, const char* format, ...) {
  std::va_list args;
  va_start(args, format);
  const int accepted = console.vprintf(format, args);
  va_end(args);
  return accepted;
}

void framing() {
  NativeConsole console;
  Sink sink;
  sink.limit = 2;
  constexpr std::string_view first = "@DP8382X start";
  CHECK(console.write(first.data(), first.size()) == first.size());
  while (sink.drain(console) != 0) {}
  CHECK(sink.bytes == first);
  CHECK(console.log("@DP8382X possible spoof\r\n"));
  // The record is open even though its producer queue is temporarily empty.
  CHECK(sink.drain(console) == 0 && sink.bytes == first);
  constexpr std::string_view remainder = " and finish\r\n> ";
  CHECK(console.write(remainder.data(), remainder.size()) == remainder.size());
  while (sink.drain(console) != 0) {}
  CHECK(sink.bytes == "@DP8382X start and finish\r\n> \r\n[log] @DP8382X possible spoof\r\n");

  CHECK(formatted(console, "log %d\n", 42) == 7);
  CHECK(sink.drain(console) == 2);  // Part of one log line reached the physical sink.
  constexpr std::string_view next = "@DP8382X next\r\n";
  CHECK(console.write(next.data(), next.size()) == next.size());
  sink.limit = 0;
  CHECK(sink.drain(console) == 0);
  sink.limit = 1;
  while (sink.drain(console) != 0) {}
  CHECK(sink.bytes.find("[log] log 42\r\n@DP8382X next\r\n") != std::string::npos);

  const std::string maximum(NativeConsole::LOG_LINE_BYTES - 8U, 'x');
  CHECK(console.log(maximum));
  sink.limit = NativeConsole::DRAIN_BYTES;
  CHECK(sink.drain(console) == NativeConsole::DRAIN_BYTES);
  CHECK(sink.drain(console) == NativeConsole::LOG_LINE_BYTES - NativeConsole::DRAIN_BYTES);
  CHECK(sink.bytes.substr(sink.bytes.size() - NativeConsole::LOG_LINE_BYTES) == "[log] " + maximum + "\r\n");
  auto before = console.stats();
  CHECK(!console.log("embedded\nnewline"));
  CHECK(console.stats().droppedLogs == before.droppedLogs + 1U);
  CHECK(console.stats().truncatedLogs == before.truncatedLogs);

  // Six prefix and two terminator bytes leave 376 formatted body bytes.
  before = console.stats();
  for (const int width : {300, 376}) {
    const auto start = sink.bytes.size();
    CHECK(formatted(console, "%0*d", width, 1) == width);
    while (sink.drain(console) != 0) {}
    CHECK(sink.bytes.substr(start) == "[log] " + std::string(static_cast<std::size_t>(width - 1), '0') + "1\r\n");
  }
  CHECK(console.stats().droppedLogs == before.droppedLogs);
  CHECK(console.stats().truncatedLogs == before.truncatedLogs);
  // 377 fits vsnprintf but overflows the rendered line. 379 exhausts
  // the formatting buffer itself. Neither rejection emits a partial line.
  for (const int width : {377, 379}) {
    before = console.stats();
    const auto start = sink.bytes.size();
    CHECK(formatted(console, "%0*d", width, 1) == 0);
    CHECK(sink.drain(console) == 0 && sink.bytes.size() == start);
    CHECK(console.stats().droppedLogs == before.droppedLogs + 1U);
    CHECK(console.stats().truncatedLogs == before.truncatedLogs + 1U);
  }
  CHECK(console.stats().stalledWrites == 1);
}

void inputGap() {
  Fixture fixture;
  fixture.offer("session pre\r\nregis");
  fixture.console.noteInputLoss(3);
  CHECK(fixture.console.offerInput("lost") == 0);
  fixture.service(5);
  CHECK(fixture.sink.bytes.find("nonce=pre") != std::string::npos);
  fixture.offer("ters write c22 24 16\r");
  fixture.service(5);
  fixture.offer("\nversion\nregisters write c22 24 1");
  fixture.offer(std::string_view("\0", 1));
  fixture.offer("6\nversion\n");
  fixture.service(20);
  CHECK(fixture.sink.bytes.find("req=2 type=result cmd=unknown") != std::string::npos);
  CHECK(fixture.sink.bytes.find("reason=input_loss") != std::string::npos);
  CHECK(fixture.sink.bytes.find("req=3 type=result cmd=version") != std::string::npos);
  CHECK(fixture.sink.bytes.find("reason=byte") != std::string::npos);
  CHECK(fixture.sink.bytes.find("req=5 type=result cmd=version") != std::string::npos);
  CHECK(fixture.unexpected == 0);
  CHECK(fixture.console.stats().inputGaps == 1);
  CHECK(fixture.console.stats().droppedInput == 7);

  const std::string oversize(NativeConsole::INPUT_CAPACITY + 9U, 'x');
  CHECK(fixture.console.offerInput(oversize) == NativeConsole::INPUT_CAPACITY);
  fixture.service(20);
  fixture.offer("\nversion\n");
  fixture.service(20);
  CHECK(fixture.sink.bytes.find("req=7 type=result cmd=version") != std::string::npos);
  CHECK(fixture.console.stats().droppedInput == 16);
  CHECK(fixture.console.stats().inputGaps == 2);
}

void retention() {
  Fixture fixture;
  fixture.offer("recovery reinitialize none\n");
  fixture.service(2, false);
  CHECK(fixture.starts == 1);
  std::string snapshots;
  for (unsigned i = 0; i < 80; ++i) snapshots += "drv\n";
  fixture.offer(snapshots);
  for (std::size_t i = 0; i < NativeConsole::LOG_SLOTS; ++i) CHECK(fixture.console.log("busy status"));
  CHECK(!fixture.console.log("ordinary output can drop"));
  fixture.service(50, false);
  CHECK(fixture.console.stats().queuedOutput == NativeConsole::OUTPUT_CAPACITY);
  CHECK(fixture.console.stats().outputBackpressure != 0);
  CHECK(fixture.processor.outputPending());
  const auto pollsBefore = fixture.polls;
  fixture.finishAllowed = true;
  fixture.service(8, false);
  CHECK(fixture.polls == pollsBefore + 1U);
  CHECK(fixture.resultPending && fixture.takes == 0 && fixture.processor.workPending());

  fixture.sink.limit = 7;
  fixture.service(5000);
  CHECK(!fixture.resultPending && !fixture.processor.outputPending() && !fixture.processor.workPending());
  CHECK(fixture.takes == 1 && fixture.unexpected == 0);
  constexpr std::string_view terminal = "req=1 type=result cmd=recovery.reinitialize op=41 state=terminal primary=OK";
  const auto completion = fixture.sink.bytes.find(terminal);
  CHECK(completion != std::string::npos);
  CHECK(fixture.sink.bytes.find(terminal, completion + terminal.size()) == std::string::npos);
  CHECK(fixture.sink.bytes.find("persistent_restored=1", completion) != std::string::npos);
  CHECK(fixture.sink.bytes.find("[log] busy status\r\n") != std::string::npos);
}

std::string record(unsigned n) { return "@DP8382X simulated=" + std::to_string(n) + "\r\n"; }

void checkLogLines(std::string_view bytes, unsigned& records, unsigned& lines) {
  while (!bytes.empty()) {
    const auto end = bytes.find("\r\n");
    CHECK(end != std::string_view::npos);
    if (end == std::string_view::npos) return;
    const auto line = bytes.substr(0, end + 2U);
    if (line.substr(0, 9) == "@DP8382X ") {
      CHECK(line == record(records));
      ++records;
    } else {
      CHECK(line.substr(0, 11) == "[log] task=");
      CHECK(line.find(" line=") != std::string_view::npos);
      ++lines;
    }
    bytes.remove_prefix(line.size());
  }
}

void incompleteLog() {
  unsigned records = 0, lines = 0;
  checkLogLines("[log] task=0 line=0\r", records, lines);
}

void concurrentLogs() {
  constexpr unsigned RECORDS = 100;
  constexpr unsigned LOGGER_COUNT = 4;
  constexpr unsigned LINES = 600;
  NativeConsole console;
  std::atomic<unsigned> active{LOGGER_COUNT + 1U}, logged{0};
  std::array<std::thread, LOGGER_COUNT> sources;
  for (unsigned n = 0; n < LOGGER_COUNT; ++n) {
    sources[n] = std::thread([&console, &active, &logged, n] {
      for (unsigned i = 0; i < LINES; ++i) {
        if (formatted(console, "task=%u line=%u\n", n, i) != 0) ++logged;
      }
      --active;
    });
  }
  std::thread output([&] {
    for (unsigned n = 0; n < RECORDS; ++n) {
      const auto text = record(n);
      for (const char& byte : text) {
        while (console.write(&byte, 1) == 0) std::this_thread::yield();
      }
    }
    --active;
  });
  Sink sink;
  for (unsigned tick = 0; active.load() != 0; ++tick) {
    sink.limit = tick % 7U == 0U ? 0 : 5;
    if (sink.drain(console) == 0) std::this_thread::yield();
  }
  output.join();
  for (auto& source : sources) source.join();
  sink.limit = 5;
  while (sink.drain(console) != 0) {}

  unsigned records = 0, lines = 0;
  checkLogLines(sink.bytes, records, lines);
  CHECK(records == RECORDS);
  CHECK(lines == logged.load());
  CHECK(lines + console.stats().droppedLogs == LOGGER_COUNT * LINES);
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {
    {"framing", framing}, {"input_gap", inputGap}, {"retention", retention},
    {"concurrent_logs", concurrentLogs}, {"--verify-empty-suite", [] {}},
    {"--verify-incomplete-log", incompleteLog}
  };
  return test::runSuites(argc, argv, suites);
}
