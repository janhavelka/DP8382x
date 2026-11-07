// Explicit destructive acquisition; compiled and linked by test/consumer.
#pragma once
#include <DP8382x/DP8382x.h>

// Application admits history consumption and serializes the bound driver.
// Check Status before using output; partial history stays in evidenceSnapshot.
inline DP8382x::Status collectBasicHistory(DP8382x::DP8382x& phy,
                                          const DP8382x::IoBudget& budget,
                                          DP8382x::EvidenceSnapshot& output) noexcept {
  const uint32_t sources = DP8382x::evidenceBit(DP8382x::EvidenceSource::PHYSTS) |
                           DP8382x::evidenceBit(DP8382x::EvidenceSource::BMSR);
  return phy.acquireEvidence(sources, output, budget);
}
