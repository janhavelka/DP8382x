/// @file CommandTable.h
/// @brief DP83826I named register and field-mask catalog with source/access citations.
/// @details The authoritative ledger owns legal encodings, reset knowledge and
/// procedure gates. Constants describe the map; they do not grant raw access.
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
/// @brief Vendor MMD selector; SNLS647I 8.3.11.1, table 8-31.
constexpr uint16_t DEVAD_VENDOR = 0x001F;
/// @brief Complete vendor address-selection word; reserved REGCR bits zero.
constexpr uint16_t REGCR_VENDOR_ADDRESS = 0x001F;
/// @brief Complete no-postincrement vendor data control; REGCR reserved bits zero.
constexpr uint16_t REGCR_VENDOR_DATA = 0x401F;
/// @brief PHY reset/control, C22 0x1F; command bits are W1S, not state.
constexpr uint8_t REG_PHYRCR = 0x1F;
/// @brief LED0 control, C22 0x18; SNLS647I table 8-42.
constexpr uint8_t REG_LEDCR = 0x18;
/// @brief LEDCR blink, polarity, drive-enable and level; other bits stay zero.
constexpr uint16_t MASK_LEDCR_DIRECT = 0x0692;

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
/// @brief ANAR.pause symmetric-pause field; RH/W, SNLS647I table 8-23.
constexpr uint16_t MASK_ANAR_SYMMETRIC_PAUSE = 0x0400;
/// @brief ANAR selector bits; normal IEEE 802.3 selector is 0x0001.
constexpr uint16_t MASK_ANAR_SELECTOR = 0x001F;
/// @brief PHYRCR hard reset W1S command, distinct from address 0x1F.
constexpr uint16_t MASK_PHYRCR_HARD_RESET = 0x8000;
/// @brief PHYRCR circuitry restart W1S command.
constexpr uint16_t MASK_PHYRCR_CIRCUITRY_RESTART = 0x4000;
/// @brief PHYRCR W1S command mask; preserve neutral zero on unrelated writes.
constexpr uint16_t MASK_PHYRCR_COMMAND = 0xC000;

/// @brief Source-implied EEE tuple, subject to physical Q1 qualification.
struct EeeRegister {
  uint8_t devad; ///< MMD selector; never the vendor alias map.
  uint16_t address; ///< Low wire offset inferred from the documented family notation.
  bool safeRead; ///< False requires a later typed collector consumer.
};
/// @brief D2 source-implied tuples; SNLS647I 8.3.11.1 plus SNLS638C tables 7-99..103.
constexpr EeeRegister EEE_REGISTERS[] = {
  {3, 0x0001, false}, {3, 0x0014, true}, {3, 0x0016, false},
  {7, 0x003C, true}, {7, 0x003D, true}
};

// Complete DP83826I numeric names. The authoritative ledger owns legal encodings,
// applicability, unknown reset bits and per-field source/access prose. These
// constants never grant a raw access or recipe permission.
/// @brief TDR recipe-only offset; SNLA428 table 2-1, no raw access permission.
constexpr uint16_t REG_TDR_0411 = 0x0411;
/// @brief BMCR.loopback; access RW; 8.5.1.1, table 8-19; table 8-17 inventory.
constexpr uint16_t MASK_BMCR_LOOPBACK = 0x4000;
/// @brief BMCR.speed; access RHW; 8.5.1.1, table 8-19; table 8-17 inventory.
constexpr uint16_t MASK_BMCR_SPEED = 0x2000;
/// @brief BMCR.power_down; access RW; 8.5.1.1, table 8-19; table 8-17 inventory.
constexpr uint16_t MASK_BMCR_POWER_DOWN = 0x0800;
/// @brief BMCR.isolate; access RHW; 8.5.1.1, table 8-19; table 8-17 inventory.
constexpr uint16_t MASK_BMCR_ISOLATE = 0x0400;
/// @brief BMCR.duplex; access RHW; 8.5.1.1, table 8-19; table 8-17 inventory.
constexpr uint16_t MASK_BMCR_DUPLEX = 0x0100;
/// @brief BMCR.collision_test; access RW; 8.5.1.1, table 8-19; table 8-17 inventory.
constexpr uint16_t MASK_BMCR_COLLISION_TEST = 0x0080;
/// @brief BMSR.t4; access R; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_T4 = 0x8000;
/// @brief BMSR.full100; access R; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_FULL100 = 0x4000;
/// @brief BMSR.half100; access R; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_HALF100 = 0x2000;
/// @brief BMSR.full10; access R; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_FULL10 = 0x1000;
/// @brief BMSR.half10; access R; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_HALF10 = 0x0800;
/// @brief BMSR.preamble; access R; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_PREAMBLE = 0x0040;
/// @brief BMSR.an_complete; access RH; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_AN_COMPLETE = 0x0020;
/// @brief BMSR.remote_fault; access RC; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_REMOTE_FAULT = 0x0010;
/// @brief BMSR.an_ability; access R; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_AN_ABILITY = 0x0008;
/// @brief BMSR.jabber; access RH; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_JABBER = 0x0002;
/// @brief BMSR.extended; access R; 8.5.1.2, table 8-20; table 8-17 inventory.
constexpr uint16_t MASK_BMSR_EXTENDED = 0x0001;
/// @brief PHYIDR1.oui_high; access R; 8.5.1.3, table 8-21; table 8-17 inventory.
constexpr uint16_t MASK_PHYIDR1_OUI_HIGH = 0xFFFF;
/// @brief PHYIDR2.oui_low; access R; 8.5.1.4, table 8-22; table 8-17 inventory.
constexpr uint16_t MASK_PHYIDR2_OUI_LOW = 0xFC00;
/// @brief ANAR.next_page; access RW; 8.5.1.5, table 8-23; table 8-17 inventory.
constexpr uint16_t MASK_ANAR_NEXT_PAGE = 0x8000;
/// @brief ANAR.remote_fault; access RW; 8.5.1.5, table 8-23; table 8-17 inventory.
constexpr uint16_t MASK_ANAR_REMOTE_FAULT = 0x2000;
/// @brief ANAR.asym_pause; access RW; 8.5.1.5, table 8-23; table 8-17 inventory.
constexpr uint16_t MASK_ANAR_ASYM_PAUSE = 0x0800;
/// @brief ANAR.t4; access R; 8.5.1.5, table 8-23; table 8-17 inventory.
constexpr uint16_t MASK_ANAR_T4 = 0x0200;
/// @brief ANAR.full100; access RHW; 8.5.1.5, table 8-23; table 8-17 inventory.
constexpr uint16_t MASK_ANAR_FULL100 = 0x0100;
/// @brief ANAR.half100; access RHW; 8.5.1.5, table 8-23; table 8-17 inventory.
constexpr uint16_t MASK_ANAR_HALF100 = 0x0080;
/// @brief ANAR.full10; access RHW; 8.5.1.5, table 8-23; table 8-17 inventory.
constexpr uint16_t MASK_ANAR_FULL10 = 0x0040;
/// @brief ANAR.half10; access RHW; 8.5.1.5, table 8-23; table 8-17 inventory.
constexpr uint16_t MASK_ANAR_HALF10 = 0x0020;
/// @brief ALNPAR C22 address; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t REG_ALNPAR = 0x0005;
/// @brief ALNPAR.next_page; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_NEXT_PAGE = 0x8000;
/// @brief ALNPAR.ack; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_ACK = 0x4000;
/// @brief ALNPAR.remote_fault; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_REMOTE_FAULT = 0x2000;
/// @brief ALNPAR.asym_pause; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_ASYM_PAUSE = 0x0800;
/// @brief ALNPAR.pause; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_PAUSE = 0x0400;
/// @brief ALNPAR.t4; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_T4 = 0x0200;
/// @brief ALNPAR.full100; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_FULL100 = 0x0100;
/// @brief ALNPAR.half100; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_HALF100 = 0x0080;
/// @brief ALNPAR.full10; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_FULL10 = 0x0040;
/// @brief ALNPAR.half10; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_HALF10 = 0x0020;
/// @brief ALNPAR.selector; access R; 8.5.1.6, table 8-24; table 8-17 inventory.
constexpr uint16_t MASK_ALNPAR_SELECTOR = 0x001F;
/// @brief ANER C22 address; 8.5.1.7, table 8-25; table 8-17 inventory.
constexpr uint16_t REG_ANER = 0x0006;
/// @brief ANER.parallel_fault; access RH; 8.5.1.7, table 8-25; table 8-17 inventory.
constexpr uint16_t MASK_ANER_PARALLEL_FAULT = 0x0010;
/// @brief ANER.partner_next; access R; 8.5.1.7, table 8-25; table 8-17 inventory.
constexpr uint16_t MASK_ANER_PARTNER_NEXT = 0x0008;
/// @brief ANER.local_next; access R; 8.5.1.7, table 8-25; table 8-17 inventory.
constexpr uint16_t MASK_ANER_LOCAL_NEXT = 0x0004;
/// @brief ANER.page; access RH; 8.5.1.7, table 8-25; table 8-17 inventory.
constexpr uint16_t MASK_ANER_PAGE = 0x0002;
/// @brief ANER.partner_an; access R; 8.5.1.7, table 8-25; table 8-17 inventory.
constexpr uint16_t MASK_ANER_PARTNER_AN = 0x0001;
/// @brief ANNPTR C22 address; 8.5.1.8, table 8-26; table 8-17 inventory.
constexpr uint16_t REG_ANNPTR = 0x0007;
/// @brief ANNPTR.next_page; access RW; 8.5.1.8, table 8-26; table 8-17 inventory.
constexpr uint16_t MASK_ANNPTR_NEXT_PAGE = 0x8000;
/// @brief ANNPTR.message; access RW; 8.5.1.8, table 8-26; table 8-17 inventory.
constexpr uint16_t MASK_ANNPTR_MESSAGE = 0x2000;
/// @brief ANNPTR.ack2; access RW; 8.5.1.8, table 8-26; table 8-17 inventory.
constexpr uint16_t MASK_ANNPTR_ACK2 = 0x1000;
/// @brief ANNPTR.toggle; access R; 8.5.1.8, table 8-26; table 8-17 inventory.
constexpr uint16_t MASK_ANNPTR_TOGGLE = 0x0800;
/// @brief ANNPTR.code; access RW; 8.5.1.8, table 8-26; table 8-17 inventory.
constexpr uint16_t MASK_ANNPTR_CODE = 0x07FF;
/// @brief ANLNPTR C22 address; 8.5.1.9, table 8-27; table 8-17 inventory.
constexpr uint16_t REG_ANLNPTR = 0x0008;
/// @brief ANLNPTR.next_page; access R; 8.5.1.9, table 8-27; table 8-17 inventory.
constexpr uint16_t MASK_ANLNPTR_NEXT_PAGE = 0x8000;
/// @brief ANLNPTR.ack; access R; 8.5.1.9, table 8-27; table 8-17 inventory.
constexpr uint16_t MASK_ANLNPTR_ACK = 0x4000;
/// @brief ANLNPTR.message; access R; 8.5.1.9, table 8-27; table 8-17 inventory.
constexpr uint16_t MASK_ANLNPTR_MESSAGE = 0x2000;
/// @brief ANLNPTR.ack2; access R; 8.5.1.9, table 8-27; table 8-17 inventory.
constexpr uint16_t MASK_ANLNPTR_ACK2 = 0x1000;
/// @brief ANLNPTR.toggle; access R; 8.5.1.9, table 8-27; table 8-17 inventory.
constexpr uint16_t MASK_ANLNPTR_TOGGLE = 0x0800;
/// @brief ANLNPTR.code; access R; 8.5.1.9, table 8-27; table 8-17 inventory.
constexpr uint16_t MASK_ANLNPTR_CODE = 0x07FF;
/// @brief CR1 C22 address; 8.5.1.10, table 8-28; table 8-17 inventory.
constexpr uint16_t REG_CR1 = 0x0009;
/// @brief CR1.tdr_auto; access RW; 8.5.1.10, table 8-28; table 8-17 inventory.
constexpr uint16_t MASK_CR1_TDR_AUTO = 0x0100;
/// @brief CR1.link_recovery; access RW; 8.5.1.10, table 8-28; table 8-17 inventory.
constexpr uint16_t MASK_CR1_LINK_RECOVERY = 0x0080;
/// @brief CR1.robust_mdix; access RW; 8.5.1.10, table 8-28; table 8-17 inventory.
constexpr uint16_t MASK_CR1_ROBUST_MDIX = 0x0020;
/// @brief CR1.fast_rxdv; access RW; 8.5.1.10, table 8-28; table 8-17 inventory.
constexpr uint16_t MASK_CR1_FAST_RXDV = 0x0002;
/// @brief CR2 C22 address; 8.5.1.11, table 8-29; table 8-17 inventory.
constexpr uint16_t REG_CR2 = 0x000A;
/// @brief CR2.extended_duplex; access RW; 8.5.1.11, table 8-29; table 8-17 inventory.
constexpr uint16_t MASK_CR2_EXTENDED_DUPLEX = 0x0020;
/// @brief CR2.idle_rx_error; access RW; 8.5.1.11, table 8-29; table 8-17 inventory.
constexpr uint16_t MASK_CR2_IDLE_RX_ERROR = 0x0004;
/// @brief CR2.odd_nibble_disable; access RHW; 8.5.1.11, table 8-29; table 8-17 inventory.
constexpr uint16_t MASK_CR2_ODD_NIBBLE_DISABLE = 0x0002;
/// @brief CR3 C22 address; 8.5.1.12, table 8-30; table 8-17 inventory.
constexpr uint16_t REG_CR3 = 0x000B;
/// @brief CR3.descrambler_drop; access RW; 8.5.1.12, table 8-30; table 8-17 inventory.
constexpr uint16_t MASK_CR3_DESCRAMBLER_DROP = 0x0400;
/// @brief CR3.polarity_swap; access RW; 8.5.1.12, table 8-30; table 8-17 inventory.
constexpr uint16_t MASK_CR3_POLARITY_SWAP = 0x0040;
/// @brief CR3.pair_swap; access RW; 8.5.1.12, table 8-30; table 8-17 inventory.
constexpr uint16_t MASK_CR3_PAIR_SWAP = 0x0020;
/// @brief CR3.fast_drop_modes; access RHW; 8.5.1.12, table 8-30; table 8-17 inventory.
constexpr uint16_t MASK_CR3_FAST_DROP_MODES = 0x000F;
/// @brief REGCR.function; access RW; 8.5.1.13, table 8-31; table 8-17 inventory.
constexpr uint16_t MASK_REGCR_FUNCTION = 0xC000;
/// @brief REGCR.devad; access RW; 8.5.1.13, table 8-31; table 8-17 inventory.
constexpr uint16_t MASK_REGCR_DEVAD = 0x001F;
/// @brief ADDAR.address_data; access RW; 8.5.1.14, table 8-32; table 8-17 inventory.
constexpr uint16_t MASK_ADDAR_ADDRESS_DATA = 0xFFFF;
/// @brief FLDS C22 address; 8.5.1.15, table 8-33; table 8-17 inventory.
constexpr uint16_t REG_FLDS = 0x000F;
/// @brief FLDS.causes; access RC; Table 8-33 p63.
constexpr uint16_t MASK_FLDS_CAUSES = 0x01F0;
/// @brief PHYSTS C22 address; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t REG_PHYSTS = 0x0010;
/// @brief PHYSTS.mdix; access R; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_MDIX = 0x4000;
/// @brief PHYSTS.rx_error; access RC; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_RX_ERROR = 0x2000;
/// @brief PHYSTS.polarity; access RC; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_POLARITY = 0x1000;
/// @brief PHYSTS.false_carrier; access RC; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_FALSE_CARRIER = 0x0800;
/// @brief PHYSTS.signal; access RC; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_SIGNAL = 0x0400;
/// @brief PHYSTS.descrambler; access RC; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_DESCRAMBLER = 0x0200;
/// @brief PHYSTS.page; access RC; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_PAGE = 0x0100;
/// @brief PHYSTS.interrupt; access RC; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_INTERRUPT = 0x0080;
/// @brief PHYSTS.remote_fault; access RC; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_REMOTE_FAULT = 0x0040;
/// @brief PHYSTS.jabber; access RC; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_JABBER = 0x0020;
/// @brief PHYSTS.an_complete; access R; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_AN_COMPLETE = 0x0010;
/// @brief PHYSTS.loopback; access R; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_LOOPBACK = 0x0008;
/// @brief PHYSTS.duplex; access RH; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_DUPLEX = 0x0004;
/// @brief PHYSTS.speed; access RH; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_SPEED = 0x0002;
/// @brief PHYSTS.link; access R; 8.5.1.16, table 8-34; table 8-17 inventory.
constexpr uint16_t MASK_PHYSTS_LINK = 0x0001;
/// @brief PHYSCR C22 address; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t REG_PHYSCR = 0x0011;
/// @brief PHYSCR.disable_pll; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_DISABLE_PLL = 0x8000;
/// @brief PHYSCR.power_save_enable; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_POWER_SAVE_ENABLE = 0x4000;
/// @brief PHYSCR.power_save_mode; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_POWER_SAVE_MODE = 0x3000;
/// @brief PHYSCR.scrambler_bypass; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_SCRAMBLER_BYPASS = 0x0800;
/// @brief PHYSCR.loopback_fifo_depth; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_LOOPBACK_FIFO_DEPTH = 0x0300;
/// @brief PHYSCR.collision_full_duplex; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_COLLISION_FULL_DUPLEX = 0x0010;
/// @brief PHYSCR.interrupt_polarity; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_INTERRUPT_POLARITY = 0x0008;
/// @brief PHYSCR.test_interrupt; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_TEST_INTERRUPT = 0x0004;
/// @brief PHYSCR.interrupt_enable; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_INTERRUPT_ENABLE = 0x0002;
/// @brief PHYSCR.interrupt_output_enable; access RW; 8.5.1.17, table 8-35; table 8-17 inventory.
constexpr uint16_t MASK_PHYSCR_INTERRUPT_OUTPUT_ENABLE = 0x0001;
/// @brief MISR1 C22 address; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t REG_MISR1 = 0x0012;
/// @brief MISR1.link_quality_changed_event; access RC; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_LINK_QUALITY_CHANGED_EVENT = 0x8000;
/// @brief MISR1.energy_changed_event; access RC; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_ENERGY_CHANGED_EVENT = 0x4000;
/// @brief MISR1.link_changed_event; access RC; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_LINK_CHANGED_EVENT = 0x2000;
/// @brief MISR1.speed_changed_event; access RC; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_SPEED_CHANGED_EVENT = 0x1000;
/// @brief MISR1.duplex_changed_event; access RC; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_DUPLEX_CHANGED_EVENT = 0x0800;
/// @brief MISR1.an_complete_event; access RC; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_AN_COMPLETE_EVENT = 0x0400;
/// @brief MISR1.false_carrier_half_full_event; access RC; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_FALSE_CARRIER_HALF_FULL_EVENT = 0x0200;
/// @brief MISR1.receive_error_half_full_event; access RC; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_RECEIVE_ERROR_HALF_FULL_EVENT = 0x0100;
/// @brief MISR1.link_quality_changed_enable; access RW; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_LINK_QUALITY_CHANGED_ENABLE = 0x0080;
/// @brief MISR1.energy_changed_enable; access RW; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_ENERGY_CHANGED_ENABLE = 0x0040;
/// @brief MISR1.link_changed_enable; access RW; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_LINK_CHANGED_ENABLE = 0x0020;
/// @brief MISR1.speed_changed_enable; access RW; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_SPEED_CHANGED_ENABLE = 0x0010;
/// @brief MISR1.duplex_changed_enable; access RW; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_DUPLEX_CHANGED_ENABLE = 0x0008;
/// @brief MISR1.an_complete_enable; access RW; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_AN_COMPLETE_ENABLE = 0x0004;
/// @brief MISR1.false_carrier_half_full_enable; access RW; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_FALSE_CARRIER_HALF_FULL_ENABLE = 0x0002;
/// @brief MISR1.receive_error_half_full_enable; access RW; 8.5.1.18, table 8-36; table 8-17 inventory.
constexpr uint16_t MASK_MISR1_RECEIVE_ERROR_HALF_FULL_ENABLE = 0x0001;
/// @brief MISR2 C22 address; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t REG_MISR2 = 0x0013;
/// @brief MISR2.eee_error_event; access RC; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_EEE_ERROR_EVENT = 0x8000;
/// @brief MISR2.an_error_event; access RC; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_AN_ERROR_EVENT = 0x4000;
/// @brief MISR2.page_received_event; access RC; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_PAGE_RECEIVED_EVENT = 0x2000;
/// @brief MISR2.loopback_fifo_event; access RC; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_LOOPBACK_FIFO_EVENT = 0x1000;
/// @brief MISR2.mdix_changed_event; access RC; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_MDIX_CHANGED_EVENT = 0x0800;
/// @brief MISR2.sleep_event; access RC; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_SLEEP_EVENT = 0x0400;
/// @brief MISR2.polarity_or_wol_event; access RC; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_POLARITY_OR_WOL_EVENT = 0x0200;
/// @brief MISR2.jabber_event; access RC; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_JABBER_EVENT = 0x0100;
/// @brief MISR2.eee_error_enable; access RW; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_EEE_ERROR_ENABLE = 0x0080;
/// @brief MISR2.an_error_enable; access RW; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_AN_ERROR_ENABLE = 0x0040;
/// @brief MISR2.page_received_enable; access RW; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_PAGE_RECEIVED_ENABLE = 0x0020;
/// @brief MISR2.loopback_fifo_enable; access RW; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_LOOPBACK_FIFO_ENABLE = 0x0010;
/// @brief MISR2.mdix_changed_enable; access RW; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_MDIX_CHANGED_ENABLE = 0x0008;
/// @brief MISR2.sleep_enable; access RW; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_SLEEP_ENABLE = 0x0004;
/// @brief MISR2.polarity_or_wol_enable; access RW; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_POLARITY_OR_WOL_ENABLE = 0x0002;
/// @brief MISR2.jabber_enable; access RW; 8.5.1.19, table 8-37; table 8-17 inventory.
constexpr uint16_t MASK_MISR2_JABBER_ENABLE = 0x0001;
/// @brief FCSCR C22 address; 8.5.1.20, table 8-38; table 8-17 inventory.
constexpr uint16_t REG_FCSCR = 0x0014;
/// @brief FCSCR.false_carrier_count; access RC; 8.5.1.20, table 8-38; table 8-17 inventory.
constexpr uint16_t MASK_FCSCR_FALSE_CARRIER_COUNT = 0x00FF;
/// @brief RECR C22 address; 8.5.1.21, table 8-39; table 8-17 inventory.
constexpr uint16_t REG_RECR = 0x0015;
/// @brief RECR.receive_error_count; access RC; 8.5.1.21, table 8-39; table 8-17 inventory.
constexpr uint16_t MASK_RECR_RECEIVE_ERROR_COUNT = 0xFFFF;
/// @brief BISCR C22 address; 8.5.1.22, table 8-40; table 8-17 inventory.
constexpr uint16_t REG_BISCR = 0x0016;
/// @brief BISCR_errorCounterMode; access RW; 8.5.1.22, table 8-40; table 8-17 inventory.
constexpr uint16_t MASK_BISCR_ERRORCOUNTERMODE = 0x4000;
/// @brief BISCR_prbsMode; access RW; 8.5.1.22, table 8-40; table 8-17 inventory.
constexpr uint16_t MASK_BISCR_PRBSMODE = 0x3000;
/// @brief BISCR_checkerLock; access R; 8.5.1.22, table 8-40; table 8-17 inventory.
constexpr uint16_t MASK_BISCR_CHECKERLOCK = 0x0800;
/// @brief BISCR_checkerSyncLoss; access RH; 8.5.1.22, table 8-40; table 8-17 inventory.
constexpr uint16_t MASK_BISCR_CHECKERSYNCLOSS = 0x0400;
/// @brief BISCR_generatorActive; access R; 8.5.1.22, table 8-40; table 8-17 inventory.
constexpr uint16_t MASK_BISCR_GENERATORACTIVE = 0x0200;
/// @brief BISCR_normalPower; access R; 8.5.1.22, table 8-40; table 8-17 inventory.
constexpr uint16_t MASK_BISCR_NORMALPOWER = 0x0100;
/// @brief BISCR_miiLineTransmit; access RW; 8.5.1.22, table 8-40; table 8-17 inventory.
constexpr uint16_t MASK_BISCR_MIILINETRANSMIT = 0x0040;
/// @brief BISCR_loopbackMode; access RW; 8.5.1.22, table 8-40; table 8-17 inventory.
constexpr uint16_t MASK_BISCR_LOOPBACKMODE = 0x001F;
/// @brief RCSR C22 address; 8.5.1.23, table 8-41; table 8-17 inventory.
constexpr uint16_t REG_RCSR = 0x0017;
/// @brief RCSR.tx_clock_shift; access RW; 8.5.1.23, table 8-41; table 8-17 inventory.
constexpr uint16_t MASK_RCSR_TX_CLOCK_SHIFT = 0x0100;
/// @brief RCSR.clock_select; access RHW; 8.5.1.23, table 8-41; table 8-17 inventory.
constexpr uint16_t MASK_RCSR_CLOCK_SELECT = 0x0080;
/// @brief RCSR.rmii_mode; access RHW; 8.5.1.23, table 8-41; table 8-17 inventory.
constexpr uint16_t MASK_RCSR_RMII_MODE = 0x0020;
/// @brief RCSR.revision; access RW; 8.5.1.23, table 8-41; table 8-17 inventory.
constexpr uint16_t MASK_RCSR_REVISION = 0x0010;
/// @brief RCSR.overflow; access RC; 8.5.1.23, table 8-41; table 8-17 inventory.
constexpr uint16_t MASK_RCSR_OVERFLOW = 0x0008;
/// @brief RCSR.underflow; access RC; 8.5.1.23, table 8-41; table 8-17 inventory.
constexpr uint16_t MASK_RCSR_UNDERFLOW = 0x0004;
/// @brief RCSR.elasticity; access RW; 8.5.1.23, table 8-41; table 8-17 inventory.
constexpr uint16_t MASK_RCSR_ELASTICITY = 0x0003;
/// @brief LEDCR.blink; access RW; 8.5.1.24, table 8-42; table 8-17 inventory.
constexpr uint16_t MASK_LEDCR_BLINK = 0x0600;
/// @brief LEDCR.polarity; access RHW; 8.5.1.24, table 8-42; table 8-17 inventory.
constexpr uint16_t MASK_LEDCR_POLARITY = 0x0080;
/// @brief LEDCR.drive_enable; access RW; 8.5.1.24, table 8-42; table 8-17 inventory.
constexpr uint16_t MASK_LEDCR_DRIVE_ENABLE = 0x0010;
/// @brief LEDCR.drive_value; access RW; 8.5.1.24, table 8-42; table 8-17 inventory.
constexpr uint16_t MASK_LEDCR_DRIVE_VALUE = 0x0002;
/// @brief PHYCR C22 address; 8.5.1.25, table 8-43; table 8-17 inventory.
constexpr uint16_t REG_PHYCR = 0x0019;
/// @brief PHYCR.auto_mdix; access RHW; 8.5.1.25, table 8-43; table 8-17 inventory.
constexpr uint16_t MASK_PHYCR_AUTO_MDIX = 0x8000;
/// @brief PHYCR.force_mdix; access RHW; 8.5.1.25, table 8-43; table 8-17 inventory.
constexpr uint16_t MASK_PHYCR_FORCE_MDIX = 0x4000;
/// @brief PHYCR.pause_rx; access R; 8.5.1.25, table 8-43; table 8-17 inventory.
constexpr uint16_t MASK_PHYCR_PAUSE_RX = 0x2000;
/// @brief PHYCR.pause_tx; access R; 8.5.1.25, table 8-43; table 8-17 inventory.
constexpr uint16_t MASK_PHYCR_PAUSE_TX = 0x1000;
/// @brief PHYCR.mii_link; access R; 8.5.1.25, table 8-43; table 8-17 inventory.
constexpr uint16_t MASK_PHYCR_MII_LINK = 0x0800;
/// @brief PHYCR.led_stretch; access RW; 8.5.1.25, table 8-43; table 8-17 inventory.
constexpr uint16_t MASK_PHYCR_LED_STRETCH = 0x0080;
/// @brief PHYCR.led_config; access RW; 8.5.1.25, table 8-43; table 8-17 inventory.
constexpr uint16_t MASK_PHYCR_LED_CONFIG = 0x0020;
/// @brief PHYCR.phy_address; access RH; 8.5.1.25, table 8-43; table 8-17 inventory.
constexpr uint16_t MASK_PHYCR_PHY_ADDRESS = 0x001F;
/// @brief 10BTSCR C22 address; 8.5.1.26, table 8-44; table 8-17 inventory.
constexpr uint16_t REG_10BTSCR = 0x001A;
/// @brief 10BTSCR.lower_threshold_enable; access RW; 8.5.1.26, table 8-44; table 8-17 inventory.
constexpr uint16_t MASK_10BTSCR_LOWER_THRESHOLD_ENABLE = 0x2000;
/// @brief 10BTSCR.squelch; access RW; 8.5.1.26, table 8-44; table 8-17 inventory.
constexpr uint16_t MASK_10BTSCR_SQUELCH = 0x1E00;
/// @brief 10BTSCR.nlp_disable; access RW; 8.5.1.26, table 8-44; table 8-17 inventory.
constexpr uint16_t MASK_10BTSCR_NLP_DISABLE = 0x0080;
/// @brief 10BTSCR.polarity; access R; 8.5.1.26, table 8-44; table 8-17 inventory.
constexpr uint16_t MASK_10BTSCR_POLARITY = 0x0010;
/// @brief 10BTSCR.jabber_disable; access RW; 8.5.1.26, table 8-44; table 8-17 inventory.
constexpr uint16_t MASK_10BTSCR_JABBER_DISABLE = 0x0001;
/// @brief BICSR1 C22 address; 8.5.1.27, table 8-45; table 8-17 inventory.
constexpr uint16_t REG_BICSR1 = 0x001B;
/// @brief BICSR1_errorBytes; access R; 8.5.1.27, table 8-45; table 8-17 inventory.
constexpr uint16_t MASK_BICSR1_ERRORBYTES = 0xFF00;
/// @brief BICSR1_ipgLength; access RW; 8.5.1.27, table 8-45; table 8-17 inventory.
constexpr uint16_t MASK_BICSR1_IPGLENGTH = 0x00FF;
/// @brief BICSR2 C22 address; 8.5.1.28, table 8-46; table 8-17 inventory.
constexpr uint16_t REG_BICSR2 = 0x001C;
/// @brief BICSR2_packetLength; access RW; 8.5.1.28, table 8-46; table 8-17 inventory.
constexpr uint16_t MASK_BICSR2_PACKETLENGTH = 0x07FF;
/// @brief CDCR C22 address; 8.5.1.29, table 8-47; table 8-17 inventory.
constexpr uint16_t REG_CDCR = 0x001E;
/// @brief CDCR_start; access RW; 8.5.1.29, table 8-47; table 8-17 inventory.
constexpr uint16_t MASK_CDCR_START = 0x8000;
/// @brief CDCR_rescalEnable; access RW; 8.5.1.29, table 8-47; table 8-17 inventory.
constexpr uint16_t MASK_CDCR_RESCALENABLE = 0x4000;
/// @brief CDCR_done; access R; 8.5.1.29, table 8-47; table 8-17 inventory.
constexpr uint16_t MASK_CDCR_DONE = 0x0002;
/// @brief CDCR_failed; access R; 8.5.1.29, table 8-47; table 8-17 inventory.
constexpr uint16_t MASK_CDCR_FAILED = 0x0001;
/// @brief PHYRCR.restart; access W1S; 8.5.1.30, table 8-48; table 8-17 inventory.
constexpr uint16_t MASK_PHYRCR_RESTART = 0x4000;
/// @brief MLEDCR MMD address; 8.5.1.31, table 8-49; table 8-17 inventory.
constexpr uint16_t REG_MLEDCR = 0x0025;
/// @brief MLEDCR.polarity_swap; access RW; 8.5.1.31, table 8-49; table 8-17 inventory.
constexpr uint16_t MASK_MLEDCR_POLARITY_SWAP = 0x0200;
/// @brief MLEDCR.source; access RW; 8.5.1.31, table 8-49; table 8-17 inventory.
constexpr uint16_t MASK_MLEDCR_SOURCE = 0x0078;
/// @brief MLEDCR.route_enable; access RW; 8.5.1.31, table 8-49; table 8-17 inventory.
constexpr uint16_t MASK_MLEDCR_ROUTE_ENABLE = 0x0001;
/// @brief COMPT MMD address; 8.5.1.32, table 8-50; table 8-17 inventory.
constexpr uint16_t REG_COMPT = 0x0027;
/// @brief COMPT.pattern; access RW; 8.5.1.32, table 8-50; table 8-17 inventory.
constexpr uint16_t MASK_COMPT_PATTERN = 0x000F;
/// @brief 10M_CFG MMD address; 8.5.1.33, table 8-51; table 8-17 inventory.
constexpr uint16_t REG_10M_CFG = 0x002A;
/// @brief 10M_CFG.short_preamble; access RW; 8.5.1.33, table 8-51; table 8-17 inventory.
constexpr uint16_t MASK_10M_CFG_SHORT_PREAMBLE = 0x4000;
/// @brief FLD_CFG1 MMD address; 8.5.1.34, table 8-52; table 8-17 inventory.
constexpr uint16_t REG_FLD_CFG1 = 0x0117;
/// @brief FLD_CFG1.window; access RW; Tables 8-52/53 p76; 8.3.17 pp44-45.
constexpr uint16_t MASK_FLD_CFG1_WINDOW = 0xFC00;
/// @brief FLD_CFG1.error_count; access RW; Tables 8-52/53 p76; 8.3.17 pp44-45.
constexpr uint16_t MASK_FLD_CFG1_ERROR_COUNT = 0x03F0;
/// @brief FLD_CFG2 MMD address; 8.5.1.35, table 8-53; table 8-17 inventory.
constexpr uint16_t REG_FLD_CFG2 = 0x0131;
/// @brief FLD_CFG2.descrambler_window; access RW; Tables 8-52/53 p76; 8.3.17 pp44-45.
constexpr uint16_t MASK_FLD_CFG2_DESCRAMBLER_WINDOW = 0x003F;
/// @brief CDSCR MMD address; 8.5.1.36, table 8-54; table 8-17 inventory.
constexpr uint16_t REG_CDSCR = 0x0170;
/// @brief CDSCR_crossDisable; access RW; 8.5.1.36, table 8-54; table 8-17 inventory.
constexpr uint16_t MASK_CDSCR_CROSSDISABLE = 0x4000;
/// @brief CDSCR_txChannel; access RW; 8.5.1.36, table 8-54; table 8-17 inventory.
constexpr uint16_t MASK_CDSCR_TXCHANNEL = 0x2000;
/// @brief CDSCR_dcNoInit; access RW; 8.5.1.36, table 8-54; table 8-17 inventory.
constexpr uint16_t MASK_CDSCR_DCNOINIT = 0x1000;
/// @brief CDSCR_averageCycles; access RW; 8.5.1.36, table 8-54; table 8-17 inventory.
constexpr uint16_t MASK_CDSCR_AVERAGECYCLES = 0x0700;
/// @brief CDSCR_segment; access RW; 8.5.1.36, table 8-54; table 8-17 inventory.
constexpr uint16_t MASK_CDSCR_SEGMENT = 0x0070;
/// @brief CDSCR2 MMD address; 8.5.1.37, table 8-55; table 8-17 inventory.
constexpr uint16_t REG_CDSCR2 = 0x0171;
/// @brief CDSCR3 MMD address; 8.5.1.38, table 8-56; table 8-17 inventory.
constexpr uint16_t REG_CDSCR3 = 0x0173;
/// @brief CDSCR3_segmentDuration; access RW; 8.5.1.38, table 8-56; table 8-17 inventory.
constexpr uint16_t MASK_CDSCR3_SEGMENTDURATION = 0xFF00;
/// @brief CDSCR3_initialSkip; access RW; 8.5.1.38, table 8-56; table 8-17 inventory.
constexpr uint16_t MASK_CDSCR3_INITIALSKIP = 0x00FF;
/// @brief TDR_175 MMD address; 8.5.1.39, table 8-57; table 8-17 inventory.
constexpr uint16_t REG_TDR_175 = 0x0175;
/// @brief TDR_175_shadowAverageLocation; access RW; 8.5.1.39, table 8-57; table 8-17 inventory.
constexpr uint16_t MASK_TDR_175_SHADOWAVERAGELOCATION = 0x3800;
/// @brief TDR_175_forwardShadow; access RW; 8.5.1.39, table 8-57; table 8-17 inventory.
constexpr uint16_t MASK_TDR_175_FORWARDSHADOW = 0x000F;
/// @brief TDR_176 MMD address; 8.5.1.40, table 8-58; table 8-17 inventory.
constexpr uint16_t REG_TDR_176 = 0x0176;
/// @brief TDR_176_peakLocationThreshold; access RW; 8.5.1.40, table 8-58; table 8-17 inventory.
constexpr uint16_t MASK_TDR_176_PEAKLOCATIONTHRESHOLD = 0x001F;
/// @brief CDSCR4 MMD address; 8.5.1.41, table 8-59; table 8-17 inventory.
constexpr uint16_t REG_CDSCR4 = 0x0177;
/// @brief CDSCR4_shortCableThreshold; access RW; 8.5.1.41, table 8-59; table 8-17 inventory.
constexpr uint16_t MASK_CDSCR4_SHORTCABLETHRESHOLD = 0x1F00;
/// @brief TDR_178 MMD address; 8.5.1.42, table 8-60; table 8-17 inventory.
constexpr uint16_t REG_TDR_178 = 0x0178;
/// @brief TDR_178_pulseWidth; access RW; 8.5.1.42, table 8-60; table 8-17 inventory.
constexpr uint16_t MASK_TDR_178_PULSEWIDTH = 0x0007;
/// @brief CDLRR1 MMD address; 8.5.1.43, table 8-61; table 8-17 inventory.
constexpr uint16_t REG_CDLRR1 = 0x0180;
/// @brief CDLRR1_peakLocation; access R; 8.5.1.43, table 8-61; table 8-17 inventory.
constexpr uint16_t MASK_CDLRR1_PEAKLOCATION = 0x00FF;
/// @brief CDLRR2 MMD address; 8.5.1.44, table 8-62; table 8-17 inventory.
constexpr uint16_t REG_CDLRR2 = 0x0181;
/// @brief CDLRR3 MMD address; 8.5.1.45, table 8-63; table 8-17 inventory.
constexpr uint16_t REG_CDLRR3 = 0x0182;
/// @brief CDLRR4 MMD address; 8.5.1.46, table 8-64; table 8-17 inventory.
constexpr uint16_t REG_CDLRR4 = 0x0183;
/// @brief CDLRR5 MMD address; 8.5.1.47, table 8-65; table 8-17 inventory.
constexpr uint16_t REG_CDLRR5 = 0x0184;
/// @brief CDLAR1 MMD address; 8.5.1.48, table 8-66; table 8-17 inventory.
constexpr uint16_t REG_CDLAR1 = 0x0185;
/// @brief CDLAR1_peakAmplitude; access R; 8.5.1.48, table 8-66; table 8-17 inventory.
constexpr uint16_t MASK_CDLAR1_PEAKAMPLITUDE = 0x007F;
/// @brief CDLAR2 MMD address; 8.5.1.49, table 8-67; table 8-17 inventory.
constexpr uint16_t REG_CDLAR2 = 0x0186;
/// @brief CDLAR3 MMD address; 8.5.1.50, table 8-68; table 8-17 inventory.
constexpr uint16_t REG_CDLAR3 = 0x0187;
/// @brief CDLAR4 MMD address; 8.5.1.51, table 8-69; table 8-17 inventory.
constexpr uint16_t REG_CDLAR4 = 0x0188;
/// @brief CDLAR5 MMD address; 8.5.1.52, table 8-70; table 8-17 inventory.
constexpr uint16_t REG_CDLAR5 = 0x0189;
/// @brief CDLAR6 MMD address; 8.5.1.53, table 8-71; table 8-17 inventory.
constexpr uint16_t REG_CDLAR6 = 0x018A;
/// @brief CDLAR6_peakPolarity; access R; 8.5.1.53, table 8-71; table 8-17 inventory.
constexpr uint16_t MASK_CDLAR6_PEAKPOLARITY = 0x0800;
/// @brief CDLAR6_crossDetect; access R; 8.5.1.53, table 8-71; table 8-17 inventory.
constexpr uint16_t MASK_CDLAR6_CROSSDETECT = 0x0020;
/// @brief MSE_Val MMD address; 8.5.1.54, table 8-72; table 8-17 inventory.
constexpr uint16_t REG_MSE_Val = 0x0218;
/// @brief MSE_Val.mean_square_error; access R; 8.5.1.54, table 8-72; table 8-17 inventory.
constexpr uint16_t MASK_MSE_VAL_MEAN_SQUARE_ERROR = 0xFFFF;
/// @brief IO_CFG1 MMD address; 8.5.1.55, table 8-73; table 8-17 inventory.
constexpr uint16_t REG_IO_CFG1 = 0x0302;
/// @brief IO_CFG1.mac_impedance; access RW; 8.5.1.55, table 8-73; table 8-17 inventory.
constexpr uint16_t MASK_IO_CFG1_MAC_IMPEDANCE = 0xC000;
/// @brief IO_CFG1.rx_dv; access RHW; 8.5.1.55, table 8-73; table 8-17 inventory.
constexpr uint16_t MASK_IO_CFG1_RX_DV = 0x0100;
/// @brief IO_CFG1.clock_off; access RW; 8.5.1.55, table 8-73; table 8-17 inventory.
constexpr uint16_t MASK_IO_CFG1_CLOCK_OFF = 0x0040;
/// @brief LED0_GPIO_CFG MMD address; 8.5.1.56, table 8-74; table 8-17 inventory.
constexpr uint16_t REG_LED0_GPIO_CFG = 0x0303;
/// @brief LED0_GPIO_CFG.clock_source; access RW; 8.5.1.56, table 8-74; table 8-17 inventory.
constexpr uint16_t MASK_LED0_GPIO_CFG_CLOCK_SOURCE = 0x0038;
/// @brief LED0_GPIO_CFG.mux; access R; 8.5.1.56, table 8-74; table 8-17 inventory.
constexpr uint16_t MASK_LED0_GPIO_CFG_MUX = 0x0007;
/// @brief LED1_GPIO_CFG MMD address; 8.5.1.57, table 8-75; table 8-17 inventory.
constexpr uint16_t REG_LED1_GPIO_CFG = 0x0304;
/// @brief LED1_GPIO_CFG.clock_source; access RW; 8.5.1.57, table 8-75; table 8-17 inventory.
constexpr uint16_t MASK_LED1_GPIO_CFG_CLOCK_SOURCE = 0x0038;
/// @brief LED1_GPIO_CFG.mux; access RW; 8.5.1.57, table 8-75; table 8-17 inventory.
constexpr uint16_t MASK_LED1_GPIO_CFG_MUX = 0x0007;
/// @brief LED2_GPIO_CFG MMD address; 8.5.1.58, table 8-76; table 8-17 inventory.
constexpr uint16_t REG_LED2_GPIO_CFG = 0x0305;
/// @brief LED2_GPIO_CFG.mux; access RW; 8.5.1.58, table 8-76; table 8-17 inventory.
constexpr uint16_t MASK_LED2_GPIO_CFG_MUX = 0x0007;
/// @brief LED3_GPIO_CFG MMD address; 8.5.1.59, table 8-77; table 8-17 inventory.
constexpr uint16_t REG_LED3_GPIO_CFG = 0x0306;
/// @brief LED3_GPIO_CFG.mux; access R; 8.5.1.59, table 8-77; table 8-17 inventory.
constexpr uint16_t MASK_LED3_GPIO_CFG_MUX = 0x0007;
/// @brief CLK_OUT_LED_STATUS MMD address; 8.5.1.60, table 8-78; table 8-17 inventory.
constexpr uint16_t REG_CLK_OUT_LED_STATUS = 0x0308;
/// @brief CLK_OUT_LED_STATUS.clock_off_status; access R; 8.5.1.60, table 8-78; table 8-17 inventory.
constexpr uint16_t MASK_CLK_OUT_LED_STATUS_CLOCK_OFF_STATUS = 0x0001;
/// @brief VOD_CFG1 MMD address; 8.5.1.61, table 8-79; table 8-17 inventory.
constexpr uint16_t REG_VOD_CFG1 = 0x030B;
/// @brief VOD_CFG1.minus_mdix_high; access RW; 8.5.1.61, table 8-79; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG1_MINUS_MDIX_HIGH = 0x3000;
/// @brief VOD_CFG1.minus_mdi; access RW; 8.5.1.61, table 8-79; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG1_MINUS_MDI = 0x0FC0;
/// @brief VOD_CFG1.zero; access RW; 8.5.1.61, table 8-79; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG1_ZERO = 0x003F;
/// @brief VOD_CFG2 MMD address; 8.5.1.62, table 8-80; table 8-17 inventory.
constexpr uint16_t REG_VOD_CFG2 = 0x030C;
/// @brief VOD_CFG2.minus_mdix_low; access RW; 8.5.1.62, table 8-80; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG2_MINUS_MDIX_LOW = 0xF000;
/// @brief VOD_CFG2.plus_mdix; access RW; 8.5.1.62, table 8-80; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG2_PLUS_MDIX = 0x0FC0;
/// @brief VOD_CFG2.plus_mdi; access RW; 8.5.1.62, table 8-80; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG2_PLUS_MDI = 0x003F;
/// @brief VOD_CFG3 MMD address; 8.5.1.63, table 8-81; table 8-17 inventory.
constexpr uint16_t REG_VOD_CFG3 = 0x030E;
/// @brief VOD_CFG3.mdi_termination; access RW; 8.5.1.63, table 8-81; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG3_MDI_TERMINATION = 0xF000;
/// @brief VOD_CFG3.mdi_enable; access RW; 8.5.1.63, table 8-81; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG3_MDI_ENABLE = 0x0800;
/// @brief VOD_CFG3.mdix_termination; access RW; 8.5.1.63, table 8-81; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG3_MDIX_TERMINATION = 0x0780;
/// @brief VOD_CFG3.mdix_enable; access RW; 8.5.1.63, table 8-81; table 8-17 inventory.
constexpr uint16_t MASK_VOD_CFG3_MDIX_ENABLE = 0x0040;
/// @brief ANA_LD_PROG_SL MMD address; 8.5.1.64, table 8-82; table 8-17 inventory.
constexpr uint16_t REG_ANA_LD_PROG_SL = 0x0404;
/// @brief ANA_RX10BT_CTRL MMD address; 8.5.1.65, table 8-83; table 8-17 inventory.
constexpr uint16_t REG_ANA_RX10BT_CTRL = 0x040D;
/// @brief ANA_RX10BT_CTRL.receiver_current_gain; access RW; 8.5.1.65, table 8-83; table 8-17 inventory.
constexpr uint16_t MASK_ANA_RX10BT_CTRL_RECEIVER_CURRENT_GAIN = 0x001F;
/// @brief GENCFG MMD address; 8.5.1.66, table 8-84; table 8-17 inventory.
constexpr uint16_t REG_GENCFG = 0x0456;
/// @brief GENCFG.min_ipg; access RW; 8.5.1.66, table 8-84; table 8-17 inventory.
constexpr uint16_t MASK_GENCFG_MIN_IPG = 0x0008;
/// @brief LEDCFG MMD address; 8.5.1.67, table 8-85; table 8-17 inventory.
constexpr uint16_t REG_LEDCFG = 0x0460;
/// @brief LEDCFG.led3_source; access RW; 8.5.1.67, table 8-85; table 8-17 inventory.
constexpr uint16_t MASK_LEDCFG_LED3_SOURCE = 0xF000;
/// @brief LEDCFG.led2_source; access RW; 8.5.1.67, table 8-85; table 8-17 inventory.
constexpr uint16_t MASK_LEDCFG_LED2_SOURCE = 0x0F00;
/// @brief LEDCFG.led1_source; access RW; 8.5.1.67, table 8-85; table 8-17 inventory.
constexpr uint16_t MASK_LEDCFG_LED1_SOURCE = 0x00F0;
/// @brief IOCTRL MMD address; 8.5.1.68, table 8-86; table 8-17 inventory.
constexpr uint16_t REG_IOCTRL = 0x0461;
/// @brief IOCTRL.mac_slew; access RW; 8.5.1.68, table 8-86; table 8-17 inventory.
constexpr uint16_t MASK_IOCTRL_MAC_SLEW = 0x001F;
/// @brief SOR1 MMD address; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t REG_SOR1 = 0x0467;
/// @brief SOR1.strap0; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP0 = 0x0001;
/// @brief SOR1.strap1; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP1 = 0x0002;
/// @brief SOR1.strap2; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP2 = 0x0004;
/// @brief SOR1.strap3; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP3 = 0x0008;
/// @brief SOR1.strap4; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP4 = 0x0010;
/// @brief SOR1.strap5; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP5 = 0x0020;
/// @brief SOR1.strap6; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP6 = 0x0040;
/// @brief SOR1.strap7; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP7 = 0x0080;
/// @brief SOR1.strap8; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP8 = 0x0100;
/// @brief SOR1.strap9; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP9 = 0x0200;
/// @brief SOR1.strap10; access R; 8.5.1.69, table 8-87; table 8-17 inventory.
constexpr uint16_t MASK_SOR1_STRAP10 = 0x0400;
/// @brief SOR2 MMD address; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t REG_SOR2 = 0x0468;
/// @brief SOR2.isolate; access R; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_ISOLATE = 0x4000;
/// @brief SOR2.crs_rxdv; access RH; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_CRS_RXDV = 0x1000;
/// @brief SOR2.led3_polarity; access RH; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_LED3_POLARITY = 0x0800;
/// @brief SOR2.led2_polarity; access RH; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_LED2_POLARITY = 0x0400;
/// @brief SOR2.led0_polarity; access RH; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_LED0_POLARITY = 0x0200;
/// @brief SOR2.fast_drop; access RH; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_FAST_DROP = 0x0100;
/// @brief SOR2.auto_mdix; access RH; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_AUTO_MDIX = 0x0080;
/// @brief SOR2.led1_polarity; access RH; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_LED1_POLARITY = 0x0020;
/// @brief SOR2.rmii; access RH; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_RMII = 0x0010;
/// @brief SOR2.follower; access RH; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_FOLLOWER = 0x0008;
/// @brief SOR2.an1; access R; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_AN1 = 0x0004;
/// @brief SOR2.an0; access R; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_AN0 = 0x0002;
/// @brief SOR2.an_enable; access R; 8.5.1.70, table 8-88; table 8-17 inventory.
constexpr uint16_t MASK_SOR2_AN_ENABLE = 0x0001;
/// @brief LEDCFG2 MMD address; 8.5.1.71, table 8-89; table 8-17 inventory.
constexpr uint16_t REG_LEDCFG2 = 0x0469;
/// @brief LEDCFG2.led2_polarity; access RHW; 8.5.1.71, table 8-89; table 8-17 inventory.
constexpr uint16_t MASK_LEDCFG2_LED2_POLARITY = 0x0040;
/// @brief LEDCFG2.led2_drive_value; access RW; 8.5.1.71, table 8-89; table 8-17 inventory.
constexpr uint16_t MASK_LEDCFG2_LED2_DRIVE_VALUE = 0x0020;
/// @brief LEDCFG2.led2_drive_enable; access RW; 8.5.1.71, table 8-89; table 8-17 inventory.
constexpr uint16_t MASK_LEDCFG2_LED2_DRIVE_ENABLE = 0x0010;
/// @brief LEDCFG2.led1_polarity; access RHW; 8.5.1.71, table 8-89; table 8-17 inventory.
constexpr uint16_t MASK_LEDCFG2_LED1_POLARITY = 0x0004;
/// @brief LEDCFG2.led1_drive_value; access RW; 8.5.1.71, table 8-89; table 8-17 inventory.
constexpr uint16_t MASK_LEDCFG2_LED1_DRIVE_VALUE = 0x0002;
/// @brief LEDCFG2.led1_drive_enable; access RW; 8.5.1.71, table 8-89; table 8-17 inventory.
constexpr uint16_t MASK_LEDCFG2_LED1_DRIVE_ENABLE = 0x0001;
/// @brief RXFCFG1 MMD address; 8.5.1.72, table 8-90; table 8-17 inventory.
constexpr uint16_t REG_RXFCFG1 = 0x04A0;
/// @brief RXFCFG1_crcGate; access RW; 8.5.1.72, table 8-90; table 8-17 inventory.
constexpr uint16_t MASK_RXFCFG1_CRCGATE = 0x1000;
/// @brief RXFCFG1_levelAcknowledge; access W0C; 8.5.1.72, table 8-90; table 8-17 inventory.
constexpr uint16_t MASK_RXFCFG1_LEVELACKNOWLEDGE = 0x0800;
/// @brief RXFCFG1_pulseWidth; access RW; 8.5.1.72, table 8-90; table 8-17 inventory.
constexpr uint16_t MASK_RXFCFG1_PULSEWIDTH = 0x0600;
/// @brief RXFCFG1_levelMode; access RW; 8.5.1.72, table 8-90; table 8-17 inventory.
constexpr uint16_t MASK_RXFCFG1_LEVELMODE = 0x0100;
/// @brief RXFCFG1_enable; access RW; 8.5.1.72, table 8-90; table 8-17 inventory.
constexpr uint16_t MASK_RXFCFG1_ENABLE = 0x0080;
/// @brief RXFCFG1_bitMaskFlag; access RW; 8.5.1.72, table 8-90; table 8-17 inventory.
constexpr uint16_t MASK_RXFCFG1_BITMASKFLAG = 0x0040;
/// @brief RXFCFG1_secureOn; access RW; 8.5.1.72, table 8-90; table 8-17 inventory.
constexpr uint16_t MASK_RXFCFG1_SECUREON = 0x0020;
/// @brief RXFCFG1_magicPacketEnable; access RHW; 8.5.1.72, table 8-90; table 8-17 inventory.
constexpr uint16_t MASK_RXFCFG1_MAGICPACKETENABLE = 0x0001;
/// @brief RXFS MMD address; 8.5.1.73, table 8-91; table 8-17 inventory.
constexpr uint16_t REG_RXFS = 0x04A1;
/// @brief RXFS.interrupt_source; access RW; 8.5.1.73, table 8-91; table 8-17 inventory.
constexpr uint16_t MASK_RXFS_INTERRUPT_SOURCE = 0x1000;
/// @brief RXFS.sfd_error; access RCH; 8.5.1.73, table 8-91; table 8-17 inventory.
constexpr uint16_t MASK_RXFS_SFD_ERROR = 0x0080;
/// @brief RXFS.bad_crc; access RCH; 8.5.1.73, table 8-91; table 8-17 inventory.
constexpr uint16_t MASK_RXFS_BAD_CRC = 0x0040;
/// @brief RXFS.secure_on_error; access RCH; 8.5.1.73, table 8-91; table 8-17 inventory.
constexpr uint16_t MASK_RXFS_SECURE_ON_ERROR = 0x0020;
/// @brief RXFS.magic_packet; access RCH; 8.5.1.73, table 8-91; table 8-17 inventory.
constexpr uint16_t MASK_RXFS_MAGIC_PACKET = 0x0001;
/// @brief RXFPMD1 MMD address; 8.5.1.74, table 8-92; table 8-17 inventory.
constexpr uint16_t REG_RXFPMD1 = 0x04A2;
/// @brief RXFPMD1_byte4; access RW; 8.5.1.74, table 8-92; table 8-17 inventory.
constexpr uint16_t MASK_RXFPMD1_BYTE4 = 0xFF00;
/// @brief RXFPMD1_byte5; access RW; 8.5.1.74, table 8-92; table 8-17 inventory.
constexpr uint16_t MASK_RXFPMD1_BYTE5 = 0x00FF;
/// @brief RXFPMD2 MMD address; 8.5.1.75, table 8-93; table 8-17 inventory.
constexpr uint16_t REG_RXFPMD2 = 0x04A3;
/// @brief RXFPMD2_byte2; access RW; 8.5.1.75, table 8-93; table 8-17 inventory.
constexpr uint16_t MASK_RXFPMD2_BYTE2 = 0xFF00;
/// @brief RXFPMD2_byte3; access RW; 8.5.1.75, table 8-93; table 8-17 inventory.
constexpr uint16_t MASK_RXFPMD2_BYTE3 = 0x00FF;
/// @brief RXFPMD3 MMD address; 8.5.1.76, table 8-94; table 8-17 inventory.
constexpr uint16_t REG_RXFPMD3 = 0x04A4;
/// @brief RXFPMD3_byte0; access RW; 8.5.1.76, table 8-94; table 8-17 inventory.
constexpr uint16_t MASK_RXFPMD3_BYTE0 = 0xFF00;
/// @brief RXFPMD3_byte1; access RW; 8.5.1.76, table 8-94; table 8-17 inventory.
constexpr uint16_t MASK_RXFPMD3_BYTE1 = 0x00FF;
/// @brief RXFSOP1 MMD address; 8.5.1.77, table 8-95; table 8-17 inventory.
constexpr uint16_t REG_RXFSOP1 = 0x04A5;
/// @brief RXFSOP1_byte1; access RW; 8.5.1.77, table 8-95; table 8-17 inventory.
constexpr uint16_t MASK_RXFSOP1_BYTE1 = 0xFF00;
/// @brief RXFSOP1_byte0; access RW; 8.5.1.77, table 8-95; table 8-17 inventory.
constexpr uint16_t MASK_RXFSOP1_BYTE0 = 0x00FF;
/// @brief RXFSOP2 MMD address; 8.5.1.78, table 8-96; table 8-17 inventory.
constexpr uint16_t REG_RXFSOP2 = 0x04A6;
/// @brief RXFSOP2_byte3; access RW; 8.5.1.78, table 8-96; table 8-17 inventory.
constexpr uint16_t MASK_RXFSOP2_BYTE3 = 0xFF00;
/// @brief RXFSOP2_byte2; access RW; 8.5.1.78, table 8-96; table 8-17 inventory.
constexpr uint16_t MASK_RXFSOP2_BYTE2 = 0x00FF;
/// @brief RXFSOP3 MMD address; 8.5.1.79, table 8-97; table 8-17 inventory.
constexpr uint16_t REG_RXFSOP3 = 0x04A7;
/// @brief RXFSOP3_byte5; access RW; 8.5.1.79, table 8-97; table 8-17 inventory.
constexpr uint16_t MASK_RXFSOP3_BYTE5 = 0xFF00;
/// @brief RXFSOP3_byte4; access RW; 8.5.1.79, table 8-97; table 8-17 inventory.
constexpr uint16_t MASK_RXFSOP3_BYTE4 = 0x00FF;
/// @brief EEECFG3 MMD address; 8.3.3.1-2; 8.3.4.
constexpr uint16_t REG_EEECFG3 = 0x04D1;
/// @brief EEE_04D3 MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_04D3 = 0x04D3;
/// @brief EEE_04DF MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_04DF = 0x04DF;
/// @brief EEE_033E MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_033E = 0x033E;
/// @brief EEE_033F MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_033F = 0x033F;
/// @brief RECIPE_0123 MMD address; 8.3.3.1; 8.3.14.4.
constexpr uint16_t REG_RECIPE_0123 = 0x0123;
/// @brief EEE_031B MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_031B = 0x031B;
/// @brief EEE_0466 MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_0466 = 0x0466;
/// @brief EEE_04CF MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_04CF = 0x04CF;
/// @brief EEE_0416 MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_0416 = 0x0416;
/// @brief EEE_04F5 MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_04F5 = 0x04F5;
/// @brief EEE_04E0 MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_04E0 = 0x04E0;
/// @brief EEE_031F MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_031F = 0x031F;
/// @brief EEE_04F4 MMD address; 8.3.3.1.
constexpr uint16_t REG_EEE_04F4 = 0x04F4;
/// @brief RECIPE_0428 MMD address; 8.3.6.3 p29; table8-50 p75.
constexpr uint16_t REG_RECIPE_0428 = 0x0428;
/// @brief LOOP_0122 MMD address; 8.3.14.4.
constexpr uint16_t REG_LOOP_0122 = 0x0122;
/// @brief LOOP_0130 MMD address; 8.3.14.4.
constexpr uint16_t REG_LOOP_0130 = 0x0130;

/// @brief BMCR stable update excluding sampled reset/restart commands.
/// @param[in] sampled Prior control word.
/// @param[in] mask Caller-owned subset of MASK_BMCR_PERSISTENT.
/// @param[in] value Stable replacement bits inside mask.
/// @param[in] powerDown Desired software power-down, never sampled pin state.
/// @return Stable word with commands/reserved bits zero; table 8-19.
/// @pre Caller validates mask/value and feature prerequisites before use.
constexpr uint16_t composeBmcr(uint16_t sampled, uint16_t mask, uint16_t value,
                               bool powerDown) noexcept {
  return static_cast<uint16_t>(((sampled & MASK_BMCR_PERSISTENT & ~mask) | value) &
                               ~uint16_t{0x0800}) | (powerDown ? uint16_t{0x0800} : uint16_t{0});
}
/// @brief Neutral W0C composition for RXFCFG1; table 8-90.
/// @param[in] sampled Prior word, including reserved and undefined flag bits.
/// @param[in] mask Validated stable caller-owned subset; never bit 11.
/// @param[in] value Validated replacement subset of mask.
/// @param[in] acknowledge True deliberately writes zero to bit 11.
/// @return Per-target stable/reserved preservation with explicit W0C action.
constexpr uint16_t composeWakeControl(uint16_t sampled, uint16_t mask,
                                      uint16_t value, bool acknowledge) noexcept {
  return static_cast<uint16_t>((sampled & ~static_cast<uint16_t>(mask | 0x0800U)) |
                               value | (acknowledge ? 0U : 0x0800U));
}
/// @brief MISR stable-byte update; consumed upper-byte events must be retained.
/// @param[in] sampled Prior MISR1/2 word.
/// @param[in] mask Validated nonzero low-byte enable mask.
/// @param[in] value Validated replacement bits inside mask.
/// @return Low-byte control word; table 8-36/37 status bits have no write action.
constexpr uint16_t composeInterruptEnables(uint16_t sampled, uint16_t mask,
                                           uint16_t value) noexcept {
  return static_cast<uint16_t>((sampled & 0x00FFU & ~mask) | value);
}

} } // namespace DP8382x::cmd
