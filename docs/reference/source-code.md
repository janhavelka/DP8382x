# Official source-code references

Verified on 2026-10-24. These links point to immutable Git commits. Source code
is linked for engineering review; none has been imported into the implementation.
The [exact-device datasheets](README.md) remain authoritative for hardware behavior.

The [2026-10-26 public research follow-up](public-research.md) adds TI ICSS-EMAC
and Zephyr implementations, additional Linux identity evidence, upstream Zephyr
DP83825, register tooling and source conflicts. It also records the changed
version returned by the mutable MCU+ SDK page. These sources extend the comparison;
none is a complete substitute for the exact chip's register and feature inventory.

## Texas Instruments RTOS drivers

Repository: [TexasInstruments/ethernet-rtos-drivers](https://github.com/TexasInstruments/ethernet-rtos-drivers).
Snapshot: `94bf4343004732983b0b8e1eec123d88ff70453b`, main, 2026-07-29.
License: [BSD-3-Clause](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/LICENSE),
also present as notices in the inspected PHY source files.

| Reference | Use |
| --- | --- |
| [DP83826 register definitions](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826_priv.h) | Named registers and masks; not a complete datasheet register map |
| [DP83826 implementation](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c) | Identity, mode setup, Auto-MDIX, reset/restart and link status |
| [DP83826 public API](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/include/dp83826.h) | Configuration representation and SDK-facing interface |
| [DP83822 register definitions](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83822_priv.h) | Family differences |
| [DP83822 implementation](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83822.c) | Variant-specific handling |
| [Generic PHY helpers](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/generic_phy.c) | Common control and management patterns |

No dedicated DP83825 driver was present in this snapshot. TI's
[MCU+ SDK DP83826 API guide](https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/latest/exports/docs/api_guide_am64x/group__ENETPHY__DP83826.html)
is an additional integration reference; the retrieved page identified SDK
11.02.00. Its `latest` URL is mutable, unlike the Git links above.

### Review findings for future implementation

- The reference accepts OUI `0x080028`, models `0x11`/`0x13`, and revision exactly
  1. The revision restriction is not proof that all other revisions are invalid.
- In `Dp83826_isResetComplete()`, the
  [read at line 349](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c#L349)
  passes `PHYRCR_SWRESET` as the register address. That symbol is a bit-15 mask;
  the header identifies `DP83826_PHYRCR` as address `0x1F`. This appears to be an
  upstream address/mask error. Check against the datasheet before adopting any
  reset sequence.
- An RGMII comment conflicts with this device's MII/RMII capabilities. Some
  callbacks also omit transport-error handling. Do not inherit those behaviors.
- The reviewed implementation did not establish the datasheet's DP83826
  10BASE-Te `VOD_CFG3` programming. Use the primary datasheet, section 8.3.13,
  when implementing the general 10 Mbit/s path.

The older combined
[TI Ethernet software repository](https://github.com/TexasInstruments/ti-ethernet-software/tree/450af1144b8caeb58724bcbae081b409f6bf5c3a)
is a secondary reference at `450af1144b8caeb58724bcbae081b409f6bf5c3a` (2026-04-29).
Its RTOS files contain BSD-style notices while its root license is GPL-2.0;
prefer the dedicated RTOS repository and inspect per-file terms for actual reuse.

## Texas Instruments Linux mirror

[DP83822/25/26 driver](https://github.com/TexasInstruments/ti-linux-kernel/blob/97becb13b81c314c1d17fcf2226aac9a32086f3d/drivers/net/phy/dp83822.c),
snapshot `97becb13b81c314c1d17fcf2226aac9a32086f3d`, ti-linux-6.18.y, 2026-10-22.
The file declares SPDX `GPL-2.0` and module license `GPL v2`.

Use it to cross-check family IDs and variant-specific configuration, interrupts,
WoL, LEDs, fiber and analog controls. It is an external GPL reference, not source
for copying into this project's MIT implementation.

Its model labels include DP83822, DP83825I/S/CM/CS and DP83826C/NC. Software labels
do not by themselves establish public orderable-part coverage. See the
[device coverage notes](../variants.md).

## Espressif integration

Baseline: **ESP-IDF v6.1**, tag commit
`fff9895c82d744c7237be8847347bdd1b07c6643`. Inspected files declare Apache-2.0.

| Reference | Use |
| --- | --- |
| [PHY interface](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/include/esp_eth_phy.h) | Driver contract |
| [IEEE 802.3 base interface](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/include/esp_eth_phy_802_3.h) | Base object and common methods |
| [IEEE 802.3 implementation](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/phy/esp_eth_phy_802_3.c) | Standard control, status and indirect MMD access |
| [Generic PHY implementation](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/phy/esp_eth_phy_generic.c) | Minimal use of the base |
| [ESP32-P4 Ethernet documentation](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32p4/api-reference/network/esp_eth.html) | MAC setup and custom PHY integration |

Use `phy_802_3_t` and `esp_eth_phy_802_3_obj_config_init()` as the starting
integration pattern. The base supplies `read_mmd_register` and
`write_mmd_register`; assess and reuse them with suitable serialization before
adding another indirect-access implementation.

Espressif's additional-driver repository was inspected at
`fe780da4c4efa4ef28dc6f0023e3a80a1c15bc5d` (master, 2026-10-21):

- [DP83848 component](https://github.com/espressif/esp-eth-drivers/tree/fe780da4c4efa4ef28dc6f0023e3a80a1c15bc5d/dp83848)
  and [implementation](https://github.com/espressif/esp-eth-drivers/blob/fe780da4c4efa4ef28dc6f0023e3a80a1c15bc5d/dp83848/src/esp_eth_phy_dp83848.c),
  Apache-2.0: useful component structure, but its OUI/model check targets DP83848.
  It cannot be used unchanged as the DP83826 component.
- [Ethernet initializer documentation](https://github.com/espressif/esp-eth-drivers/blob/fe780da4c4efa4ef28dc6f0023e3a80a1c15bc5d/ethernet_init/README.md):
  integration examples and generic PHY option.

No DP83822/25/26 component directory was present in that inspected snapshot.
Recheck available upstream support when implementation starts; these findings
describe pinned snapshots, not a permanent absence of support.
