# Re-audit source map

Sibling paths are relative to `Projects/` and intentionally written as paths,
not dependencies or links outside this repository. Start with their applicable
`AGENTS.md`; inspect live files because these repositories evolve independently.
The reports below are dated observations, not permanent compatibility contracts.

| Topic | Primary local sources to reopen | Use and limit |
| --- | --- | --- |
| Portable API/layout | `TMP1x2/include/TMP1x2/`, `INA228/include/INA228/`, `PCA9555/include/PCA9555/`, corresponding `src/` | Names, terminal Status, typed config; not I2C framing |
| Ownership/effects | `MB85RC/include/MB85RC/`, `PCA9555/src/PCA9555.cpp`, `TMP1x2/docs/owner-operations.md`, `24Cxx/include/EEPROM24Cxx/` | Borrowed transport, uncertainty, retained operation evidence; no EEPROM timing or split-MMD polling |
| Health | INA228, SHT3x-main, PCA9555, TCA9548A public snapshots and implementations | Vocabulary differs; distinguish MDIO health, config trust and link |
| Shared CLI | `TMP1x2/examples/common/`, `SHT3x-main/examples/common/`, `24Cxx/examples/common/`, LDC1614 dispatcher/tests | Actual help, strict parsing, owner ingress, bounded output |
| Native IDF lifecycle | INA228 `examples/esp_idf/basic/`, TCA9548A/PCA9555 `examples/espidf_basic/` | Cleanup, ownership and diagnostics; their I2C drivers are not PHY/MAC backends |
| Python HIL | `TMP1x2/tools/hil_tmp1x2_runner.py`, INA228/PCA9555 `tools/run_i2c_hil.py`, TCA9548A `tools/tca9548a_hil.py`, MB85RC `tools/hil_runner.py` | Provenance, strict outcomes, restoration and parser tests; inspect current grammar |
| Host/package/CI | TMP1x2/24Cxx CMake, tools and workflows; SCD41 package consumers; MB85RC metadata tools | Test registration, clean consumption and version source; no copied S2/S3 Ethernet support claim |
| Family expansion | TMP1x2 model validation, MB85RC family/geometry selection, TUSB32x capability planning | Per-variant evidence and unsupported cases, not cross-family register assumptions |
| Prompt organization | `FieldCore-node/docs/prompts/`, guidelines and implementation plan; `TUSB32x/docs/prompts/` | Contracts, source maps, scope/ownership, closure; no inherited Git/CI/model authorizations |
| Consumer integration | `FieldCore-node/src/devices/i2c/Ina228Module.h` and `.cpp`, `FieldCore-node/include/TunnelMonitor/contracts/`, current integration guidelines | Module-owned I/O, cached projection, status translation; no existing Ethernet module claim |

Broaden the initial inventory to ADS1115, INA3221, LDC1614, LSM6DS3TR, OPT4001,
RV3032-C7, MCP45HVX1, SSD1315 and applicable non-I2C drivers such as MAX31865.
Use their actual current directory names discovered by `rg --files`. Do not
copy obsolete reports or claimed API names without finding the implementation.
A targeted stage revisits the applicable subset; it need not reread every line
of every library. Record why the selected evidence is relevant.
Use [CONVENTIONS](CONVENTIONS.md) for the concrete selected baseline. Inspect the
named symbol and its implementation/tests before adopting its behavior; peers
differ on health admission, lifecycle aliases and result retention.

The [sibling audit](../reports/sibling-driver-audit.md),
[FieldCore audit](../reports/fieldcore-conventions-audit.md),
[additional family/tooling audit](../reports/family-tooling-audit.md) and
[PHY audit](../reports/phy-implementation-audit.md) document this planning run.

## Silicon and SDK authority

The [extracted library evidence](../reference/library-evidence.md) contains
register/mask/value/procedure/timing tables from the pinned TI and other primary
drivers, including rejected code behavior and source conflicts. Reopen the
specific rows and source lines relevant to each prompt. Follow
[EVIDENCE](EVIDENCE.md): compare with exact datasheets, use independent vectors,
preserve unknowns and correct the prior findings rather than trusting them.

Read [reference index](../reference/README.md),
[source-code review](../reference/source-code.md),
[variant scope](../variants.md), [architecture](../architecture.md) and
[initial board context](../esp32p4-board.md).

Apply [FULL_COVERAGE](FULL_COVERAGE.md), the
[expanded public research catalog](../reference/public-research.md) and the
[DP83826I register/feature inventory](../reference/dp83826i-feature-inventory.md).
These add manufacturer GitHub/SDK and independent primary-source comparisons;
the existing TI driver is not a complete register or feature specification.

- DP83826I/E SNLS647I is first silicon authority; review identity/modes,
  register access/reset, 10BASE-Te, link observation and diagnostic procedures.
- SNLS783A plus SDAA067A govern the A-generation comparison, SNLS638C governs
  DP83825I, SNLS505H governs each exact DP83822 variant. Document differences.
- SPRACC8A is MDIO context; exact mapping remains device-specific. SNLA423A and
  relevant EVM material guide the initial fixture. Feature app notes require an
  exact-variant applicability check and cannot override its datasheet.
- SNLA344C/SNLA494/SNLA495 explain EtherCAT/ESC/C2000 use cases. Do not transpose
  their ESC reset/clock/control assumptions to an ESP32-P4 master.
- Review pinned TI BSD source critically; existing review identifies reset
  address/mask confusion and unestablished 10BASE-Te setup. Linux GPL code is a
  reference for behavior investigation, never code to paste into MIT sources.
- Recheck pinned Espressif `esp_eth_phy.h`, `esp_eth_phy_802_3.h/.c`, generic PHY
  and P4 Ethernet documentation when implementing the adapter. Existing MMD
  helpers need complete-transaction serialization, not blind duplication/reuse.

When a claim is uncertain, state the unresolved source conflict and gate the
affected feature. Capture official source URL, revision, section/table and exact
interpretation. Do not solve gaps by broadening a mask until a test fake passes.
