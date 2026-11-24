/// @file PartTable.h
/// @brief Private ordering-part admission and map selection.
#pragma once

#include <DP8382x/Config.h>

namespace DP8382x::detail {

enum class RegisterMap : uint8_t { DP83826, DP83826A, DP83825, DP83822 };

struct PartProfile {
  Variant variant;
  RegisterMap map;
  uint8_t modelCount;
  uint8_t models[2];
  uint16_t revisionMask;
  uint8_t interfaceMask;
  uint32_t featureMask;
  bool hasOperatingMode;
  uint8_t strapAddressMax;
  bool forcedRequiresRobust;
};

// The low four feature bits are the public LINK_* modes. The original map
// has copper media and no gigabit or fiber procedure (SNLS647I, sections 4/8).
constexpr uint32_t FEATURE_COPPER = 1U << 4;
constexpr PartProfile copper826(Variant variant, RegisterMap map, uint16_t revisions) noexcept {
  return {variant, map, 2, {0x11, 0x13}, revisions,
          INTERFACE_MII | INTERFACE_RMII, LINK_TECHNOLOGY_MASK | FEATURE_COPPER, true, 7, true};
}

// UNKNOWN rows record the frozen future map contracts and have no selectable
// ordering part or implemented features. Admission is by declaration only.
inline constexpr PartProfile PART_TABLE[] = {
  copper826(Variant::DP83826I, RegisterMap::DP83826, 0x0002),
  copper826(Variant::DP83826E, RegisterMap::DP83826, 0x0002),
  copper826(Variant::DP83826AI, RegisterMap::DP83826A, 0x0010),
  copper826(Variant::DP83826AE, RegisterMap::DP83826A, 0x0010),
  {Variant::UNKNOWN, RegisterMap::DP83825, 1, {0x14, 0}, 0x0001,
   INTERFACE_RMII, 0, false, 3, false},
  {Variant::UNKNOWN, RegisterMap::DP83822, 1, {0x24, 0}, 0x0001,
   INTERFACE_MII | INTERFACE_RMII | INTERFACE_RGMII, 0, false, 31, false}
};

inline constexpr const PartProfile* findPart(Variant variant) noexcept {
  if (variant != Variant::UNKNOWN) {
    for (const auto& part : PART_TABLE) {
      if (part.variant == variant) { return &part; }
    }
  }
  return nullptr;
}

} // namespace DP8382x::detail
