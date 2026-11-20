#include "ConsoleBuffer.h"
#include "../support/Check.h"

#include <algorithm>
#include <string>

namespace {

using DP8382x::Cli::ConsoleBuffer;
constexpr std::string_view RECORD = "@DP8382X schema=1\r\n";

struct Sink {
  std::string bytes;
  std::size_t limit{ConsoleBuffer::CAPACITY};
  std::size_t offered{0};
  unsigned calls{0};

  static std::size_t write(void* context, const char* data, std::size_t size) {
    auto& sink = *static_cast<Sink*>(context);
    ++sink.calls;
    sink.offered = size;
    const auto accepted = std::min(size, sink.limit);
    sink.bytes.append(data, accepted);
    return accepted;
  }
};

void drainAll(ConsoleBuffer& queue, Sink& sink) {
  while (queue.queuedBytes() > 0) {
    if (queue.drain(&sink, Sink::write, ConsoleBuffer::CAPACITY) == 0) {
      CHECK(false);
      break;
    }
  }
}

void lineBoundaries() {
  ConsoleBuffer queue;
  Sink sink;
  CHECK(queue.pushHuman("> "));
  drainAll(queue, sink);
  CHECK(queue.pushRecord(RECORD)); // Even an already-written prompt opens a line.
  CHECK(queue.pushRecord(RECORD));
  CHECK(queue.pushHuman("help\r\n"));
  CHECK(queue.pushRecord(RECORD));
  CHECK(queue.pushHuman("\x1b[0m"));
  CHECK(queue.pushRecord(RECORD)); // The prefix must also be the first raw byte.
  drainAll(queue, sink);
  std::string expected = "> \r\n";
  expected.append(RECORD).append(RECORD).append("help\r\n").append(RECORD);
  expected.append("\x1b[0m\r\n").append(RECORD);
  CHECK(sink.bytes == expected);
}

void atomicRecords() {
  ConsoleBuffer queue;
  Sink sink;
  const std::string fill(ConsoleBuffer::CAPACITY - RECORD.size(), 'a');
  CHECK(queue.pushHuman(fill));
  CHECK(!queue.pushRecord(RECORD)); // Record fits only without its required separator.
  CHECK(queue.queuedBytes() == fill.size());
  CHECK(queue.drain(&sink, Sink::write, 1) == 1);
  CHECK(!queue.pushRecord(RECORD));
  CHECK(queue.drain(&sink, Sink::write, 1) == 1);
  CHECK(queue.pushRecord(RECORD)); // Separator and record use all remaining room.
  CHECK(queue.queuedBytes() == ConsoleBuffer::CAPACITY);
  CHECK(!queue.pushHuman("dropped"));
  CHECK(queue.droppedHuman() == 1);
  CHECK(!queue.pushRecord(RECORD));
  CHECK(queue.queuedBytes() == ConsoleBuffer::CAPACITY);
  drainAll(queue, sink);
  auto expected = fill + "\r\n";
  expected.append(RECORD);
  CHECK(sink.bytes == expected);

  CHECK(!queue.pushRecord("missing terminator"));
  CHECK(!queue.pushRecord("extra\nline\r\n"));
  CHECK(!queue.pushRecord("\r\n"));
  CHECK(!queue.pushRecord("\x1b[0mrecord\r\n"));
  CHECK(!queue.pushRecord(std::string_view("embedded\0nul\r\n", 14)));
  CHECK(queue.queuedBytes() == 0);
  const auto maximum = std::string(ConsoleBuffer::MAX_RECORD_BYTES - 2, 'a') + "\r\n";
  CHECK(queue.pushRecord(maximum));
  CHECK(!queue.pushRecord("a" + maximum));
  CHECK(queue.queuedBytes() == ConsoleBuffer::MAX_RECORD_BYTES);
}

void writerBackpressure() {
  ConsoleBuffer queue;
  Sink sink;
  CHECK(queue.pushHuman("> "));
  CHECK(queue.pushRecord(RECORD));
  const auto initial = queue.queuedBytes();
  CHECK(queue.drain(&sink, nullptr, initial) == 0);
  CHECK(queue.drain(&sink, Sink::write, 0) == 0);
  CHECK(sink.calls == 0);
  sink.limit = 0;
  CHECK(queue.drain(&sink, Sink::write, 5) == 0);
  CHECK(sink.offered == 5 && sink.calls == 1);
  CHECK(queue.stalledWrites() == 1 && queue.queuedBytes() == initial);
  sink.limit = 1;
  CHECK(queue.drain(&sink, Sink::write, 3) == 1);
  CHECK(sink.offered == 3 && sink.calls == 2);
  CHECK(queue.queuedBytes() == initial - 1);
  CHECK(queue.pushHuman("next\r\n"));
  sink.limit = 3;
  drainAll(queue, sink);
  std::string expected = "> \r\n";
  expected.append(RECORD).append("next\r\n");
  CHECK(sink.bytes == expected);
  CHECK(queue.stalledWrites() == 1);
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {{"line_boundaries", lineBoundaries},
                                {"atomic_records", atomicRecords},
                                {"writer_backpressure", writerBackpressure}};
  return test::runSuites(argc, argv, suites);
}
