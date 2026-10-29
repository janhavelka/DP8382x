/// @file CommandTable.h
/// @brief Source-proved DP83826I Clause 22 addresses and selected field masks.
/// @details Values come from SNLS647I tables 8-19 through 8-23, 8-31/32 and
/// 8-48. Constants describe the wire map; they do not grant raw access.
#pragma once

#include <cstdint>

/// @brief Portable DP8382x register declarations.
namespace DP8382x {
/// @brief Source-proved wire addresses and masks; no raw-access permission.
namespace cmd {

/// @brief Basic mode control, C22 0x00; command and state bits are mixed.
/// Full-word raw writes must not replay W1S reset/restart commands.
constexpr uint8_t REG_BMCR = 0x00;
/// @brief Basic status, C22 0x01; link and other bits have latch/read effects.
/// Live reads belong to the single event/link collector.
constexpr uint8_t REG_BMSR = 0x01;
/// @brief First read-only identity word, C22 0x02.
constexpr uint8_t REG_PHYIDR1 = 0x02;
/// @brief Second read-only identity word, C22 0x03.
constexpr uint8_t REG_PHYIDR2 = 0x03;
/// @brief Local AN advertisement, C22 0x04; selector and reserved bits need
/// typed composition rather than an unrestricted full-word write.
constexpr uint8_t REG_ANAR = 0x04;
/// @brief Indirect MMD selection register, C22 0x0D; raw access is forbidden.
constexpr uint8_t REG_REGCR = 0x0D;
/// @brief Indirect MMD address/data register, C22 0x0E; raw access is forbidden.
constexpr uint8_t REG_ADDAR = 0x0E;
/// @brief PHY reset/control, C22 0x1F; command bits are W1S, not state.
constexpr uint8_t REG_PHYRCR = 0x1F;

/// @brief BMCR PCS reset W1S command; readback zero is not chip readiness.
constexpr uint16_t MASK_BMCR_RESET = 0x8000;
/// @brief BMCR persistent AN-enable bit.
constexpr uint16_t MASK_BMCR_AN_ENABLE = 0x1000;
/// @brief BMCR AN-restart W1S command.
constexpr uint16_t MASK_BMCR_AN_RESTART = 0x0200;
/// @brief BMCR W1S command bits, distinct from stable writable state.
constexpr uint16_t MASK_BMCR_COMMAND = 0x8200;
/// @brief BMCR owned persistent functional fields; not a raw-write permit.
constexpr uint16_t MASK_BMCR_PERSISTENT = 0x7D80;
/// @brief BMSR latched-low link bit; one read can consume history.
constexpr uint16_t MASK_BMSR_LINK = 0x0004;
/// @brief PHYIDR2 model bits; shift right four after masking.
constexpr uint16_t MASK_PHYIDR2_MODEL = 0x03F0;
/// @brief PHYIDR2 revision nibble; raw value is not acceptance.
constexpr uint16_t MASK_PHYIDR2_REVISION = 0x000F;
/// @brief ANAR 10/100 technology bits; not the LINK_* API bit positions.
constexpr uint16_t MASK_ANAR_TECHNOLOGY = 0x01E0;
/// @brief ANAR symmetric/asymmetric pause bits.
constexpr uint16_t MASK_ANAR_PAUSE = 0x0C00;
/// @brief ANAR selector bits; normal IEEE 802.3 selector is 0x0001.
constexpr uint16_t MASK_ANAR_SELECTOR = 0x001F;
/// @brief PHYRCR hard reset W1S command, distinct from address 0x1F.
constexpr uint16_t MASK_PHYRCR_HARD_RESET = 0x8000;
/// @brief PHYRCR circuitry restart W1S command.
constexpr uint16_t MASK_PHYRCR_CIRCUITRY_RESTART = 0x4000;
/// @brief PHYRCR W1S command mask; preserve neutral zero on unrelated writes.
constexpr uint16_t MASK_PHYRCR_COMMAND = 0xC000;

} } // namespace DP8382x::cmd
