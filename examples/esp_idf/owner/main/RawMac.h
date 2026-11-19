#pragma once
#include "FramePort.h"
#include "PhyOwner.h"
#include "esp_eth_driver.h"
#include <cstdlib>

namespace owner_example {
// P4's EMAC RX task transfers this allocation to stack_input. FramePort only
// borrows it for a bounded copy. An intentional drop still consumes the input.
inline esp_err_t receiveRaw(esp_eth_handle_t, uint8_t* buffer, uint32_t length, void* context) noexcept {
  if (context) static_cast<FramePort*>(context)->receive(buffer, length);
  std::free(buffer);
  return context ? ESP_OK : ESP_ERR_INVALID_ARG;
}

struct TxCounts {
  uint64_t denied{}, queuedToDma{}, backpressure{}, linkDown{}, failed{};
  bool attempted{};
  esp_err_t firstError{}, lastError{};
};

// Called by the single owner after a queue pop, inside the native admission
// gate. A false return rejects the frame before any SDK call. On an attempt
// the pinned P4 driver borrows bytes until return and lastError is exact.
inline bool transmitAdmitted(PhyOwner& owner, FramePort& frames, uint64_t nowUs, esp_eth_handle_t driver,
                             RawFrame& frame, TxCounts& counts) noexcept {
  if (!owner.allowFrame(nowUs, frame.generation).ok()) {
    ++counts.denied;
    if (owner.snapshot().phase != OwnerPhase::ACTIVE) frames.setSession(0);
    return false;
  }
  counts.attempted = true;
  counts.lastError = esp_eth_transmit(driver, frame.bytes, frame.length);
  if (counts.firstError == ESP_OK) counts.firstError = counts.lastError;
  switch (counts.lastError) {
    case ESP_OK: ++counts.queuedToDma; break;
    case ESP_ERR_NO_MEM: ++counts.backpressure; break;
    case ESP_ERR_INVALID_STATE:
      ++counts.linkDown;
      owner.missingEvidence(nowUs);
      frames.setSession(0);
      break;
    default: ++counts.failed; break;
  }
  return true;
}
} // namespace owner_example
