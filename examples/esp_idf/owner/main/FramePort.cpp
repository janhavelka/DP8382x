#include "FramePort.h"
#include <cstring>

namespace owner_example {

static_assert(std::atomic<uint32_t>::is_always_lock_free, "RX accounting requires lock-free 32-bit atomics");

bool FramePort::Queue::push(uint64_t generation, const uint8_t* bytes, size_t length) noexcept {
  if (!bytes || length < FRAME_HEADER_BYTES || length > FRAME_CAPACITY || count == FRAME_QUEUE_SLOTS)
    return false;
  auto& slot = slots[(head + count) % FRAME_QUEUE_SLOTS];
  slot.generation = generation;
  slot.length = static_cast<uint16_t>(length);
  std::memcpy(slot.bytes, bytes, length);
  ++count;
  return true;
}

bool FramePort::Queue::pop(uint64_t generation, RawFrame& output) noexcept {
  // Callers check count and consume at most this one slot.
  const auto& slot = slots[head];
  head = (head + 1) % FRAME_QUEUE_SLOTS;
  --count;
  if (slot.generation != generation) return false;
  output.generation = slot.generation;
  output.length = slot.length;
  std::memcpy(output.bytes, slot.bytes, slot.length);
  return true;
}

bool FramePort::setSession(uint64_t generation) noexcept {
  if (generation == _activeGeneration) return true;
  _rxOpen.store(false);
  _activeGeneration = 0;
  if (generation == 0) return true;
  if (_rxBusy.test_and_set(std::memory_order_acquire)) return false;
  _rxGeneration = generation;
  _activeGeneration = generation;
  _rxOpen.store(true);
  _rxBusy.clear(std::memory_order_release);
  return true;
}

bool FramePort::receive(const uint8_t* bytes, size_t length) noexcept {
  bool queued = false;
  if (!_rxBusy.test_and_set(std::memory_order_acquire)) {
    // Closing never waits for an in-flight copy. A subsequent open acquires
    // this same flag, so a copy admitted before close keeps the old tag.
    if (_rxOpen.load()) queued = _rx.push(_rxGeneration, bytes, length);
    _rxBusy.clear(std::memory_order_release);
  }
  (queued ? _rxAccepted : _rxDropped).fetch_add(1, std::memory_order_relaxed);
  return queued;
}

bool FramePort::queueTx(uint64_t generation, const uint8_t* bytes, size_t length) noexcept {
  if (!generation || generation != _activeGeneration || !_tx.push(generation, bytes, length)) {
    ++_txDropped;
    return false;
  }
  ++_txQueued;
  return true;
}

bool FramePort::popRx(uint64_t generation, RawFrame& output) noexcept {
  if (generation != _activeGeneration || _rxBusy.test_and_set(std::memory_order_acquire)) return false;
  bool delivered = false;
  if (_rx.count) {
    delivered = _rx.pop(generation, output);
    if (delivered) ++_rxDelivered;
    else _rxDropped.fetch_add(1, std::memory_order_relaxed);
  }
  _rxBusy.clear(std::memory_order_release);
  return delivered;
}

bool FramePort::popTx(uint64_t generation, RawFrame& output) noexcept {
  if (generation != _activeGeneration || !_tx.count) return false;
  if (_tx.pop(generation, output)) return true;
  ++_txDropped;
  return false;
}

bool FramePort::quiesce() noexcept {
  if (_activeGeneration || _rxBusy.test_and_set(std::memory_order_acquire)) return false;
  _rxDropped.fetch_add(static_cast<uint32_t>(_rx.count), std::memory_order_relaxed);
  _rx.count = 0;
  _rxBusy.clear(std::memory_order_release);
  _txDropped += static_cast<uint32_t>(_tx.count);
  _tx.count = 0;
  return true;
}

FrameCounts FramePort::counts() const noexcept {
  return {_rxAccepted.load(std::memory_order_relaxed), _rxDropped.load(std::memory_order_relaxed),
          _rxDelivered, _txQueued, _txDropped};
}

} // namespace owner_example
