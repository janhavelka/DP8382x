#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace owner_example {

// The example accepts an Ethernet header through one full VLAN-tagged frame,
// excluding FCS. The hardware may support frame sizes outside this policy.
inline constexpr size_t FRAME_HEADER_BYTES = 14;
inline constexpr size_t FRAME_CAPACITY = 1518;
inline constexpr size_t FRAME_QUEUE_SLOTS = 4;

struct RawFrame {
  uint64_t generation{};
  uint16_t length{};
  uint8_t bytes[FRAME_CAPACITY]{};
};

// Counts are modulo 2^32. Drops include rejected arrivals/requests and already
// queued slots discarded on withdrawal. RX can change between counter loads.
// The application records SDK TX results.
struct FrameCounts {
  uint32_t rxAccepted{};
  uint32_t rxDropped{};
  uint32_t rxDelivered{};
  uint32_t txQueued{};
  uint32_t txDropped{};
};

// Static application storage. Only receive() runs on the P4 MAC RX task.
// All other methods belong to the single application task. No call waits for
// the other task or keeps a pointer into a caller's memory.
class FramePort {
public:
  // Supply the current PhyOwner session generation, or zero to close ingress
  // immediately. Released generations are never reused. Opening a new session
  // can return false on contention and leaves ingress closed until a later turn.
  bool setSession(uint64_t generation) noexcept;

  // Copy one borrowed frame. The stack_input caller owns and frees its original
  // buffer on *every* return. The generation identifies application handoff at
  // callback admission; the SDK supplies no generation for on-wire arrival.
  bool receive(const uint8_t* bytes, size_t length) noexcept;
  bool queueTx(uint64_t generation, const uint8_t* bytes, size_t length) noexcept;

  // Each takes at most one slot. A stale front slot is dropped. A false return
  // leaves the destination unchanged. TX still needs a current owner admission
  // immediately before esp_eth_transmit borrows the destination buffer.
  bool popRx(uint64_t generation, RawFrame& output) noexcept;
  bool popTx(uint64_t generation, RawFrame& output) noexcept;

  // With ingress closed, discard both queues in constant work. False means
  // open or contended. Success proves application queue quiescence only.
  bool quiesce() noexcept;
  FrameCounts counts() const noexcept;

private:
  struct Queue {
    RawFrame slots[FRAME_QUEUE_SLOTS]{};
    size_t head{};
    size_t count{};
    bool push(uint64_t generation, const uint8_t* bytes, size_t length) noexcept;
    bool pop(uint64_t generation, RawFrame& output) noexcept;
  };

  Queue _rx;
  Queue _tx;
  std::atomic_flag _rxBusy = ATOMIC_FLAG_INIT;
  std::atomic<bool> _rxOpen{false};
  uint64_t _rxGeneration{};     // Serialized by _rxBusy.
  uint64_t _activeGeneration{}; // Application task only.
  std::atomic<uint32_t> _rxAccepted{0};
  std::atomic<uint32_t> _rxDropped{0};
  uint32_t _rxDelivered{};
  uint32_t _txQueued{};
  uint32_t _txDropped{};
};

} // namespace owner_example
