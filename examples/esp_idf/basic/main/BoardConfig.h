/// @file BoardConfig.h
/// @brief Application-owned wiring selected from the latest saved P4 netlist.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "esp_eth_mac_esp.h"

namespace Board {
/// @brief Saved netlist mapping, not a measurement of voltage, straps or clock.
constexpr int MDC = 40, MDIO = 39, REF_CLK = 44, TX_EN = 49;
constexpr int TXD0 = 41, TXD1 = 42, CRS_DV = 51, RXD0 = 52, RXD1 = 53;
constexpr int RESET_N = 54;
constexpr uint32_t PHY_ADDRESS = 0; // Expected strap value; live identity is checked by the driver.
constexpr uint32_t LINK_POLL_MS = 1000; // Application policy; not a PHY response guarantee.

/// @brief Fill every RMII field used by the pinned P4 MAC.
/// @param[out] config SDK MAC wiring, explicitly overwritten for this board.
inline void apply(eth_esp32_emac_config_t& config) {
  config.smi_gpio.mdc_num = MDC;
  config.smi_gpio.mdio_num = MDIO;
  config.interface = EMAC_DATA_INTERFACE_RMII;
  config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
  config.clock_config.rmii.clock_gpio = REF_CLK;
  config.emac_dataif_gpio.rmii = {TX_EN, TXD0, TXD1, CRS_DV, RXD0, RXD1};
}

/// @brief Reject a silently retained SDK default or duplicate board pad.
/// @param[in] config Fully configured SDK MAC input.
/// @return True only for the selected mapping and unique SMI/RMII/reset pads.
inline bool valid(const eth_esp32_emac_config_t& config) {
  const auto& data = config.emac_dataif_gpio.rmii;
  if (config.smi_gpio.mdc_num != MDC || config.smi_gpio.mdio_num != MDIO ||
      config.interface != EMAC_DATA_INTERFACE_RMII ||
      config.clock_config.rmii.clock_mode != EMAC_CLK_EXT_IN ||
      config.clock_config.rmii.clock_gpio != REF_CLK ||
      data.tx_en_num != TX_EN || data.txd0_num != TXD0 || data.txd1_num != TXD1 ||
      data.crs_dv_num != CRS_DV || data.rxd0_num != RXD0 || data.rxd1_num != RXD1) return false;
  constexpr std::array<int, 10> pins{MDC, MDIO, REF_CLK, TX_EN, TXD0, TXD1,
                                      CRS_DV, RXD0, RXD1, RESET_N};
  for (std::size_t i = 0; i < pins.size(); ++i) {
    for (std::size_t j = i + 1; j < pins.size(); ++j) {
      if (pins[i] == pins[j]) return false;
    }
  }
  return PHY_ADDRESS <= 31 && LINK_POLL_MS > 0;
}
} // namespace Board
