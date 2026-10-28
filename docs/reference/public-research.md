# Public sources for complete PHY coverage

Research expanded on **2026-10-26** following the request for complete chip
functionality beyond the first application. These are inspected source leads and
findings, not imported driver code or implemented support. The
[full-coverage contract](../prompts/FULL_COVERAGE.md) governs their use; the
[DP83826I inventory](dp83826i-feature-inventory.md) accounts for the complete
datasheet register catalogue and feature families.
The subsequent [library-data extraction](library-evidence.md) follows these leads
into actual constants, procedures, timings and defects, and supplies the relevant
numeric anchors to the implementation prompts. Recheck those observations against
exact source/datasheet evidence; they are not a replacement specification.

Exact-part datasheets remain silicon authority. Manufacturer software provides
useful register names, settings, procedures and comparison cases, but several
reviewed implementations are partial or contain defects. Existing immutable
baselines and license boundaries remain in [source-code.md](source-code.md).

The [prompt-5 audit](../reports/implementation/5-coverage-source-review.md)
rechecks current manufacturer default branches, per-file licenses, product
material and exact-family boundaries. Its [power review](../reports/implementation/5-power-source-review.md)
and [pin review](../reports/implementation/5-pin-source-review.md) reconcile the
remaining originals into the authoritative ledger; the old source leads below
are not another numeric map.

## Manufacturer GitHub and SDK sources

### TI dedicated Ethernet RTOS drivers

Reopened the DP83826 source, public/private headers, directory inventories and
license at commit `94bf4343004732983b0b8e1eec123d88ff70453b`:

- [DP83826 implementation](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c)
- [Register definitions](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826_priv.h)
- [Public configuration and LED modes](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/include/dp83826.h)
- [DP83822 implementation](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83822.c)
- [Generic PHY helpers](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/generic_phy.c)
- [BSD-3-Clause license](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/LICENSE)

Useful for identity, Auto-MDIX, reset/restart, link and LED configuration. Its
private header is a subset of the full datasheet register map. The previously
recorded reset address/mask issue and omitted established 10BASE-Te setup remain
review items. Empty callbacks in a generic driver table do not establish PHY
features such as precision timestamping. Do not wrap this file and call the
result complete chip support.

### TI MCU+ SDK ICSS-EMAC implementation

Additional implementation at `4b9a66f4e18e02a834961f5f834155c846cab041`:

- [DP83826E source](https://github.com/TexasInstruments/mcupsdk-core/blob/4b9a66f4e18e02a834961f5f834155c846cab041/source/board/ethphy/icss_emac/ethphy_dp83826e.c)
- [DP83826E header](https://github.com/TexasInstruments/mcupsdk-core/blob/4b9a66f4e18e02a834961f5f834155c846cab041/source/board/ethphy/icss_emac/ethphy_dp83826e.h)

The inspected files retain BSD-3-Clause notices. This is a separate useful
feature/mask source: odd-nibble detection, extended full-duplex behavior,
enhanced IPG, configurable Fast Link Drop causes, LED source/mux/blink controls,
speed/duplex and IEEE power-down. Reuse the facts only after checking exact
DP83826I/E mode applicability; ICSS ownership is not the portable core design.

Its ID1-only identity check is too broad for exact device selection. Setters that
mask/shift values still require independent legal-range validation. These are
negative-test leads, not behavior to preserve blindly. Observed Git blob IDs:
source `b320d30c92f7f49885a5249b560744f8b063814d`, header
`7aab53d4eb7bf507684d7e5e7f894dcdc7ebb777`.

### TI DP83826/A Zephyr driver and binding

The combined TI repository contains additional material at
`450af1144b8caeb58724bcbae081b409f6bf5c3a`:

- [DP83826x source](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c)
- [Device binding](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/dts/bindings/ethernet/phy/ti,dp83826x.yaml)
- [Integration README](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/readme.md)

The source/binding have Apache-2.0 identifiers despite the repository's GPL root;
actual reuse requires per-file review and notices. Useful leads include MII/RMII
clock roles, DAC swing and IRQ integration. It is not proof of upstream Zephyr
support or complete features.

Reviewed problems include the indirect selector expression `(0x4000 & devad)`
instead of setting data mode, identity naming conflicting with the Linux source,
unbounded mutex acquisition, fixed busy waits, unestablished VOD_CFG3 programming and rejected
forced negotiation-off operation. The PHY library must validate these paths
against TI specifications and bounded ownership rules, not inherit them.

### TI Linux family/revision evidence

At the same combined-repository commit:

- [DP83822/25/26 driver](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/linux_drivers/dp83822.c)
- [Linux driver release notes](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/linux_drivers/release_notes.md)

This GPL-2.0 source provides comparison evidence for original DP83826 basic/
enhanced IDs, revision-B variants and A-generation identities. Its labels must
be reconciled with exact orderable parts, modes and revisions; the TI Zephyr
source's NC label disagrees. Use differences as identity/masking regression
cases, not as permission to accept all related IDs. No GPL implementation is
copied into the MIT library. The separately pinned TI Linux mirror remains a
second snapshot for comparing fixes.

### TI SDK integration documentation and mutable pages

The [TI PHY integration guide](https://github.com/TexasInstruments/mcupsdk-enet-lld/blob/bdefb871d725434aecad5a32ad10d704e797e796/docs/networking/enet_lld/phy_integration.md)
is an architectural comparison for generic/device-specific responsibilities.
Its partial driver list is not an exclusion list for supported silicon.

The [AM64x DP83826 API page](https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/latest/exports/docs/api_guide_am64x/group__ENETPHY__DP83826.html)
returned **MCU+ SDK 12.01.00** in this follow-up, whereas the 2026-10-24 review
recorded 11.02.00. The URL is mutable. Record exact returned versions and pin
implementation source; neither page version is this library's tested SDK range.
TI's [AM261x release notes](https://software-dl.ti.com/mcu-plus-sdk/esd/AM261X/26_00_00_06/exports/docs/api_guide_am261x/RELEASE_NOTES_26_00_00_PAGE.html)
also identify public MCU+ SDK repositories and actual evaluation examples; use
them as additional source-discovery paths, not proof of P4 compatibility.

## Independent primary implementation comparison

Upstream Zephyr commit `bc5d59d1b6657cfeef30124b818c8fbd69da68ac` has:

- [DP83825 driver](https://github.com/zephyrproject-rtos/zephyr/blob/bc5d59d1b6657cfeef30124b818c8fbd69da68ac/drivers/ethernet/phy/phy_ti_dp83825.c)
- [DP83825 binding](https://github.com/zephyrproject-rtos/zephyr/blob/bc5d59d1b6657cfeef30124b818c8fbd69da68ac/dts/bindings/ethernet/phy/ti,dp83825.yaml)
- [Teensy 4.1 board configuration](https://github.com/zephyrproject-rtos/zephyr/blob/bc5d59d1b6657cfeef30124b818c8fbd69da68ac/boards/pjrc/teensy4/teensy41.dts)

The Apache-2.0 driver supplies clock-role and IRQ/callback comparison evidence for
the DP83825I stage. It also has framework-specific waits and limited forced-mode
behavior; do not copy those limits into generic chip support. A real board source
is useful integration evidence, not this project's linked or physical test.
The bounded upstream search did not establish a DP83826 upstream implementation;
unrelated DP83867 symbols were false positives. Targeted ST/NXP searches did not
produce additional useful exact-family implementations in this pass. These are
search limits, not statements that no public implementation exists.

## Register tools and manufacturer procedures

- [TI Ethernet software catalog](https://www.ti.com/tool/ETHERNET-SW) lists
  register/device-description tooling, including DIEP material for this family.
  Device files and examples can fill source leads, but their contents were not
  inspected through an installer in this review.
- [USB-2-MDIO](https://www.ti.com/tool/USB-2-MDIO) and its
  [user guide SNLU197A](https://www.ti.com/lit/pdf/snlu197) are useful for independent
  register access, scripted reads/writes and log-export workflows. The inspected
  guide identifies an August 2026 revision and 12 pages. No installer was run or
  downloaded. The landing-page version and revised guide differ; guide pin-label
  inconsistencies require schematic/firmware confirmation before wiring advice.
- [DP83826I product documentation](https://www.ti.com/product/DP83826I) is a
  discovery index for current documents, application notes and support. Its
  broad feature list is not a replacement for mode-specific restrictions in
  the exact datasheet.

TI E2E searches should target remaining register/sequence contradictions from
the inventory. Identify TI engineer replies versus community suggestions and
record exact part, mode, date and stated scope. No specific unverified forum
recipe is promoted to a mandatory initialization sequence here.

## What this research adds to implementation

Research must cover full settings and procedures, not just reset/link-up:
next-page/pause, all pair/polarity controls, interface timing, link/error/MSE
observations, every IRQ source, full LED/GPIO muxing, all loopback/BIST/PRBS and
TDR settings, EEE/LPI, supported WoL matching, PHY power states, RMII repeater,
Fast Link Drop, compliance stimulus and documented analog controls. The inventory
maps the authoritative sections and exposes procedure-only addresses and conflicts.

Maintain an exhaustive source-to-register/field/procedure-to-API/CLI/test ledger.
Use applicable manufacturer code to discover omissions and independent negative
cases. Unsupported exact silicon, hardware-only properties and unresolved evidence
remain distinct from an implemented software feature. First-board limitations
affect defaults and physical qualification, never silently shrink the library.

This is a focused useful-source sweep, not a claim to have exhausted the internet.
No new source-code payloads or installers were imported. Existing archived
vendor documents and their hashes remain unchanged.
