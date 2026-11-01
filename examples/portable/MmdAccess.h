// Application-owned atomic MMD examples, compiled by test/consumer.
#pragma once

#include <DP8382x/DP8382x.h>

// The owner has already bound the driver and owns the whole instance call.
// A raw strap word is not proof of exact part, mode or a fresh strap resample.
inline DP8382x::Status readStrapWord(DP8382x::DP8382x& phy,
                                   const DP8382x::IoBudget& budget,
                                   uint16_t& strapWord) noexcept {
  uint16_t word = 0;
  const auto status = phy.readMmd(31, 0x0467, word, budget);
  if (!status.ok()) { return status; }
  strapWord = word;
  return status;
}

// Application must establish safe LED0 electrical use and exclude reset/other
// writers. This changes blink to the table8-42 code1 (100 ms) in one batch,
// preserving other documented controls. It does not verify the physical LED.
inline DP8382x::Status setLedBlink100ms(DP8382x::DP8382x& phy,
                                      const DP8382x::IoBudget& budget) noexcept {
  return phy.modifyMmd(31, 0x0018, 0x0600, 0x0200, budget);
}
