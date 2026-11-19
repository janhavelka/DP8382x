#include "support/Check.h"
#include "../examples/esp_idf/owner/main/FramePort.h"
#include <array>
#include <cstring>
#include <thread>

namespace {
using namespace owner_example;

void queue() {
  static_assert(sizeof(FramePort) <= 13 * 1024, "Fixed application queue memory budget");
  FramePort port;
  std::array<uint8_t, 1518> packet{};
  RawFrame rx, tx;
  CHECK(!port.receive(packet.data(), 14));
  CHECK(!port.queueTx(0, packet.data(), 14));
  CHECK(port.setSession(5));
  CHECK(!port.quiesce());
  CHECK(!port.receive(nullptr, 14));
  CHECK(!port.receive(packet.data(), 13));
  CHECK(!port.receive(packet.data(), 1519));
  CHECK(!port.queueTx(5, nullptr, 14));
  CHECK(!port.queueTx(5, packet.data(), 13));
  CHECK(!port.queueTx(5, packet.data(), 1519));
  CHECK(!port.queueTx(6, packet.data(), 14));
  for (uint8_t i = 1; i <= 4; ++i) {
    packet.fill(i);
    CHECK(port.receive(packet.data(), packet.size()));
    CHECK(port.queueTx(5, packet.data(), packet.size()));
  }
  CHECK(!port.receive(packet.data(), 14));
  CHECK(!port.queueTx(5, packet.data(), 14));
  packet.fill(0); // Neither RX nor TX borrows the caller's storage after enqueue.
  for (uint8_t i = 1; i <= 4; ++i) {
    CHECK(port.popRx(5, rx));
    CHECK(port.popTx(5, tx));
    CHECK(rx.length == 1518 && rx.generation == 5 && rx.bytes[0] == i && rx.bytes[1517] == i);
    CHECK(tx.length == 1518 && tx.generation == 5 && std::memcmp(rx.bytes, tx.bytes, tx.length) == 0);
  }
  CHECK(!port.popRx(5, rx) && !port.popTx(5, tx));
  const auto counts = port.counts();
  CHECK(counts.rxAccepted == 4 && counts.rxDelivered == 4 && counts.rxDropped == 5);
  CHECK(counts.txQueued == 4 && counts.txDropped == 6);
}

void session() {
  FramePort port;
  std::array<uint8_t, 14> packet{};
  RawFrame frame;
  constexpr uint64_t first = 3;
  constexpr uint64_t second = (uint64_t{1} << 32) + 3;
  CHECK(port.setSession(first));
  CHECK(port.receive(packet.data(), packet.size()));
  CHECK(port.queueTx(first, packet.data(), packet.size()));
  CHECK(port.setSession(0));
  CHECK(!port.receive(packet.data(), packet.size())); // Late callback while closed.
  CHECK(!port.queueTx(first, packet.data(), packet.size()));
  CHECK(port.setSession(second));
  packet[0] = 0x92;
  CHECK(port.receive(packet.data(), packet.size()));
  CHECK(port.queueTx(second, packet.data(), packet.size()));
  CHECK(!port.popRx(first, frame) && !port.popTx(first, frame)); // Old caller cannot consume current FIFO.
  frame.length = 54;
  CHECK(!port.popRx(second, frame) && frame.length == 54); // One stale slot per call.
  CHECK(!port.popTx(second, frame) && frame.length == 54);
  CHECK(port.popRx(second, frame) && frame.bytes[0] == 0x92 && frame.generation == second);
  CHECK(port.popTx(second, frame) && frame.bytes[0] == 0x92 && frame.generation == second);
  CHECK(port.queueTx(second, packet.data(), packet.size()));
  CHECK(port.receive(packet.data(), packet.size()));
  CHECK(port.setSession(0));
  CHECK(port.quiesce() && port.quiesce());
  CHECK(!port.popRx(0, frame) && !port.popTx(0, frame));
  CHECK(port.counts().rxDropped == 3 && port.counts().txDropped == 3);
}

void load() {
  FramePort port;
  std::array<uint8_t, 14> packet{};
  RawFrame frame;
  CHECK(port.setSession(1));
  for (unsigned turn = 0; turn < 40; ++turn) {
    for (unsigned arrival = 0; arrival < 4; ++arrival) port.receive(packet.data(), packet.size());
    CHECK(port.popRx(1, frame)); // One service slot even with continuous ingress.
  }
  CHECK(port.setSession(0));
  CHECK(port.quiesce());
  CHECK(port.counts().rxDelivered == 40 && port.counts().rxAccepted == 43 && port.counts().rxDropped == 120);

  // A real second execution context overlaps ingress with closure, draining
  // and reopening. Every attempt is accounted for with any scheduling order.
  FramePort concurrent;
  std::thread receiver([&] {
    for (unsigned i = 0; i < 4096; ++i) concurrent.receive(packet.data(), packet.size());
  });
  for (uint64_t generation = 1; generation <= 1024; ++generation) {
    if (concurrent.setSession(generation) && concurrent.popRx(generation, frame))
      CHECK(frame.generation == generation && frame.length == 14);
    CHECK(concurrent.setSession(0));
    (void)concurrent.quiesce(); // Contention yields to the next bounded turn.
  }
  receiver.join();
  CHECK(concurrent.quiesce());
  const auto counts = concurrent.counts();
  CHECK(counts.rxAccepted >= counts.rxDelivered);
  CHECK(counts.rxDropped + counts.rxDelivered == 4096);
}
}

int main(int argc, char** argv) {
  const test::Suite suites[] = {{"queue", queue}, {"session", session}, {"load", load}};
  return test::runSuites(argc, argv, suites);
}
