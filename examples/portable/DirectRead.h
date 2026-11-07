// Application-owned direct read example, compiled by test/consumer.
#pragma once

#include <DP8382x/DP8382x.h>
#include <DP8382x/CommandTable.h>

// Owner must supply a truthful bounded backend and the original deadline.
// The context remains alive through bind/read/end. This does not accept identity.
inline DP8382x::Status readIdentityWord(const DP8382x::Config& config,
                                      const DP8382x::IoBudget& budget,
                                      uint16_t& identityWord) noexcept {
  DP8382x::DP8382x phy;
  auto status = phy.bind(config);
  if (!status.ok()) { return status; }
  uint16_t word = 0;
  status = phy.readClause22(DP8382x::cmd::REG_PHYIDR1, word, budget);
  const auto detach = phy.end(); // Local only; backend faults still belong to owner.
  if (!status.ok()) { return status; }
  if (!detach.ok()) { return detach; }
  identityWord = word; // Never use the output before testing Status.
  return status;
}
