# Prompt 3 source and sibling re-audit

Reviewed on 2026-10-28 for the DP83826I/SNLS647I core ledger. This report
records source observations, not implemented driver behavior or hardware results.
The lead owns the ledger, shared guides, progress and Git. This delegated audit
owns only this report; siblings and vendor payloads remained read-only. The
inherited capable model was used for source comparison; the separate stronger
reasoning reviewer owns independent original-PDF arithmetic and access review.

The prerequisite decisions are the [core audit](1-core-api-state-ownership.md)
and [tooling audit](2-ecosystem-tooling-audit.md). Apply
[EVIDENCE](../../prompts/EVIDENCE.md), including exact-part authority and licensing,
and the [TI core catalogue](../../reference/library-ti-core.md) for detailed
source-code defect records. No vendor implementation was copied into MIT code.

## Live sibling evidence

Paths below are relative to `C:/Users/Honza/Documents/Projects/`. HEAD and status
were read without fetching, changing branches or executing sibling tests.

| Repository | Inspected HEAD | Working tree at inspection |
| --- | --- | --- |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | Clean |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | Clean |
| TUSB32x | `4445cdc9a67aceed1347b8685aa2a2a347a79743` | Concurrent modifications to `test/CMakeLists.txt`; new native-IDF example, identity/tool/test and audit files. These were not edited or adopted as completed evidence. |

Reopened parent `Projects/AGENTS.md` and each sibling's root `AGENTS.md`.
Their workflow preferences are reference context, not authority to run their CI
or mutate their repositories. This execution retains DP8382x's prompt-71 CI rule.

| Inspected paths and symbols | Selected convention and limit |
| --- | --- |
| `TMP1x2/include/TMP1x2/Config.h`: `Model`, `isValidModel`, `isValidAddress`, `hasAlertOutput`; `src/TMP1x2Configuration.cpp`: `validateConfig`, `validateSettings`, `defaultConfig` | Closed exact-model enums, per-model legal address ranges and pure preflight. Reject invalid enum values and model-inapplicable physical controls before I/O. Do not transfer I2C addresses or temperature encoding. |
| `TMP1x2/test/variants/test_variants.cpp` | Tests all 128 candidate addresses for each model, asserts zero bus calls on bind, rejects model 255 and physical ALERT on the address-select package, while retaining its register thermostat functionality. Adopt independent literals and distinction between missing pin and available silicon function. |
| `MB85RC/include/MB85RC/CommandTable.h`: `VariantInfo`, `KNOWN_VARIANTS`, `findVariantByProductId`; `src/MB85RC.cpp`: `isSupportedRuntimeVariant`, `_selectVariant`, `_validateActiveDeviceId`, `_encodeMemoryAddress` | Separate readable identity, declared variant, addressing model, implemented support and optional capabilities. Unknown products fail selection; re-identification invalidates variant-dependent state. PHY IDR1 alone cannot supply the corresponding exact-part selection. |
| `MB85RC/test/test_basic.cpp`: `test_begin_auto_cannot_select_no_device_id_variant`, `test_begin_auto_rejects_unknown_device_id_product`, `test_auto_reidentification_resets_variant_dependent_state`; expected-variant mismatch cases and `findVariantByProductId(0x123/0)` checks | Reject mismatched or unknown identities; a device without readable identity cannot be auto-selected. Preserve raw identity evidence separately from usable capability state. These are inspected tests, not tests run in this task. |
| `TMP1x2/examples/common/Tmp1x2Cli.cpp`: help and model/address handler; `MB85RC/examples/common/DiagnosticCore.h`: variant formatting | TMP explicitly says selected model is a BOM declaration because it has no unique ID; model changes require uninitialized state. DP83826 adds real readable OUI/model/revision, but still cannot infer I/E temperature suffix from shared identity. CLI must display declared part and observed mode/revision separately. |
| `TMP1x2/tools/hil_tmp1x2_runner.py`, `tools/test_hil_tmp1x2_runner.py`; `MB85RC/tools/hil_runner.py`, `tools/test_hil_runner.py` | Inspected session/parser definitions, identity patterns, fake serial boundary tests and finite response limits. Keep identity mismatch and incomplete evidence visible; parser success is not physical qualification. Do not copy either runner's protocol grammar as the future PHY CLI. |
| `TMP1x2/.github/workflows/ci.yml`, `MB85RC/.github/workflows/ci.yml` | Reopened actual test/build registration. TMP uses CMake/CTest, contract checks, pinned checkout and PlatformIO 6.1.19; MB uses native PlatformIO plus parser/metadata/timing checks and native-IDF builds. Their S2/S3 builds do not establish Ethernet/P4 support. No CI status was queried. |
| `TUSB32x/docs/prompts/2-family-capabilities.md` and root rules | Reference planning explicitly separates declared variant, signature and revision and does not infer suffix/capability from a shared identity. Only this convention is adopted; the existence of its plan is not evidence that its entire implementation exists. |

For DP83826, combine pure declaration validation with a separately bounded read
of both PHY ID registers. Keep raw IDs, decoded OUI, model/mode, revision,
declared DP83826I and revision acceptance separate. A shared mode ID does not
prove DP83826E, DP83826I or A-generation equivalence. Revisions not covered by the
selected exact-part evidence remain explicit acceptance gates, not silently
enabled capabilities. No sibling numerical register facts enter the PHY ledger.

## Refreshed public source pins

GitHub's default-branch commit API was queried and actual raw files were retrieved
at the immutable commits below. Source bytes were kept only in a temporary audit
directory outside the repository. The archived TI PDFs, ZIPs, provenance and
hashes were not changed. Per-file notices were inspected; license labels below
apply to the named files, not automatically to every repository file.

| Source and immutable commit | Files actually inspected | License and findings |
| --- | --- | --- |
| [TI dedicated RTOS](https://github.com/TexasInstruments/ethernet-rtos-drivers/tree/94bf4343004732983b0b8e1eec123d88ff70453b), `94bf4343004732983b0b8e1eec123d88ff70453b` | `rtos_drivers/src/dp83826.c`, `dp83826_priv.h`, `generic_phy.c`, `rtos_drivers/include/dp83826.h`; identity, config/MDIX, indirect RMW, reset/restart, link functions and definitions | BSD-3-Clause notices in all four files. Current default HEAD remains the catalogue pin. PHYRCR completion still passes its bit mask as the address; multi-write RMW still ignores write errors; generic link query still leaves output untouched after first-low BMSR. These defects remain rejected. |
| [TI MCU+ ICSS-EMAC](https://github.com/TexasInstruments/mcupsdk-core/tree/4b9a66f4e18e02a834961f5f834155c846cab041), `4b9a66f4e18e02a834961f5f834155c846cab041` | `source/board/ethphy/icss_emac/ethphy_dp83826e.c` and `.h`; identifier, MDIX, extended duplex, odd-nibble functions and declarations | BSD-3-Clause notices in both. Current default HEAD unchanged. Identity check reads only IDR1. The odd-nibble enable-named method sets a disable mask. Neither behavior defines the portable API. |
| [TI combined Linux material](https://github.com/TexasInstruments/ti-ethernet-software/tree/450af1144b8caeb58724bcbae081b409f6bf5c3a), `450af1144b8caeb58724bcbae081b409f6bf5c3a` | `linux_drivers/dp83822.c` identity definitions and `release_notes.md` | C file SPDX GPL-2.0; release notes have no file-specific license declaration inspected. HEAD unchanged. Original BASIC/ENHANCED, revision-B and A-generation labels differ explicitly; these are comparison clues, not permission to infer temperature suffix. Notes record June 2024 a131 support and September 2025 A-generation IDs. GPL source stays reference-only. |
| [TI combined Zephyr material](https://github.com/TexasInstruments/ti-ethernet-software/tree/450af1144b8caeb58724bcbae081b409f6bf5c3a), same commit | `zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c`, `driver_files/dts/bindings/ethernet/phy/ti,dp83826x.yaml`, integration `readme.md`; identity, MMD, cfg_link and binding | Source and binding SPDX Apache-2.0; README no file-specific declaration inspected. Per-file terms differ from GPL root. Incorrect indirect selector AND, conflicting NC identity label, unlimited mutex waits and rejected negotiation-off remain visible. VOD_CFG3 is defined but the inspected configuration does not establish the required original-part initialization. |
| [Upstream Linux](https://github.com/torvalds/linux/blob/65ba8f34ec886ec72b2c07ddc8cf3d815c32869d/drivers/net/phy/dp83822.c), `65ba8f34ec886ec72b2c07ddc8cf3d815c32869d` | `drivers/net/phy/dp83822.c`: identity, DP83826 configuration/reset and registration | SPDX GPL-2.0. Current upstream comparison has C/NC IDs and low-nibble masking; it does not explicitly distinguish the TI combined tree's A-generation IDs. Uses vendor MMD for RCSR and DAC setup. Software reset writes the circuitry-restart command then calls config_init; this is not a complete bounded reset/replay contract for this project. |
| [Upstream Zephyr](https://github.com/zephyrproject-rtos/zephyr/tree/5c71a733110403a2e01f6cabd922b5f0198e9981), `5c71a733110403a2e01f6cabd922b5f0198e9981` | `drivers/ethernet/phy/` directory, `phy_ti_dp83825.c`, `dts/bindings/ethernet/phy/ti,dp83825.yaml`; notices, clock/interrupt binding and mutex/configuration paths | Source and binding SPDX Apache-2.0. Directory lists TI DP83825 and DP83867, no DP83826 driver. This is a bounded directory finding, not a claim that no public DP83826 implementation exists. DP83825 is later-variant context only. |
| [ESP-IDF v6.1 baseline](https://github.com/espressif/esp-idf/tree/fff9895c82d744c7237be8847347bdd1b07c6643), `fff9895c82d744c7237be8847347bdd1b07c6643` | `components/esp_eth/include/esp_eth_phy.h`, `esp_eth_phy_802_3.h`, `src/phy/esp_eth_phy_802_3.c`, `esp_eth_phy_generic.c`; identity, pause, MMD and base construction | All four Apache-2.0. Retain the already selected immutable baseline. MMD helpers compose the expected selector phases but contain no encompassing bus lock. Pause advertisement maps nonzero to both bits; reported peer pause is one Boolean. Full portable pause resolution and shared transaction ownership remain necessary. |
| [ESP-IDF current comparison](https://github.com/espressif/esp-idf/blob/4d80c1b29aeabc8e735ffc6e1be9bb23e95d1d5d/components/esp_eth/src/phy/esp_eth_phy_802_3.c), `4d80c1b29aeabc8e735ffc6e1be9bb23e95d1d5d` | `components/esp_eth/src/phy/esp_eth_phy_802_3.c`, diff against baseline | Apache-2.0. Adds 1000BASE-T master-mode methods and changes hardware-reset waits to a minimum-delay helper with an extra RTOS tick. MMD methods unchanged. This does not change the selected SDK baseline or add gigabit support to DP83826. Prompt 6/37 must audit v6.1 reset delay quantization against required minima. |

The current TI RTOS file history was also inspected: latest file commits include
`802292840e403de50496118adcad5a652068b15a` (generic PHY functionality) and
`58168a427315d14b78b7fb5ce0f2870da127b45d` (link check in speed/duplex).
Neither fixes the observed reset-address call at the inspected HEAD. The TI
Zephyr file history contains `440797b822d7a9ebea21a810a74e99431abbc95a`
(826A/510 drivers). Current upstream Linux file history includes
`8a9f9bd2d070f55bc892e7cb6879ce25e6fe1d62` (optional external clock),
`d790b7064387e5a38f6feec2cb386f1941332dc8` (probe readability) and
`333c29a27f96bfe1577af68fc3856bb17034f755` (phy_port). These are source-history
observations, not local or remote test results.

## Manufacturer documents and unresolved search results

The [current product index](https://www.ti.com/product/DP83826I) still lists
SNLS647I, August 2025, as its datasheet. Its visible documentation list includes
SNLA423A and rollover material; no separate silicon errata document was located
in that listing or the targeted errata search. This finite search does not prove
that no erratum or unpublished clarification exists. The page's feature list
does not override the archived exact mode/field definitions.

The [current EVM page](https://www.ti.com/tool/DP83826EVM) identifies MII/RMII and
on-board MSP430 USB-to-MDIO access. Reopened archived
[SNLU262A](../../reference/source/TI-DP83826EVM-SNLU262A.pdf), June 2023,
through its searchable introduction and MDIO-access sections as fixture context.
It is not a measurement of this board. DIEP is linked as version 1.0; no installer
was run, and no claim is made to have inspected its device-description payload.
The [mutable AM64x SDK API page](https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/latest/exports/docs/api_guide_am64x/group__ENETPHY__DP83826.html)
again returned 12.01.00; immutable source pins, not `latest`, govern comparison.

Targeted searches included `site.e2e.ti.com DP83826 PHYSTS forced mode speed
duplex`, `DP83826E VOD_CFG3 E2E`, `DP83826 revision 0x2000 E2E`, and
`site.ti.com DP83826 errata`. They did not establish an applicable TI engineer
resolution of the ENHANCED forced-mode PHYSTS contradiction. An older DP83822EVM
thread on latch behavior concerns another part and was not promoted into DP83826
authority. No unverified forum recipe is accepted. Next evidence remains an
exact-part TI clarification or identified physical observations covering mode,
revision, forced BMCR settings, status readback and reset state.

## Consequences for this stage

The independent silicon reviewer also visually checked the original IEEE
802.3-2012 Section Two, printed pages 731–732, especially Table 28B-3, through
an [identified document mirror](https://www.my-wire.de/schnittstellenbeschreibung/ethernet.pdf).
Retrieval on 2026-10-28: 20,249,231 bytes, SHA-256
`29774aba6d5b82524f4323214b1035603b0aa901c5f971d770049776b0a6c2af`.
This is an original IEEE document hosted by a nonpublisher, not a claim about the
latest standard. The payload and rendered page stayed in system temporary
storage; the repository records only factual pause-resolution results and
provenance. IEEE copyright remains applicable. TI table 8-43 expressly refers
to Annex 28B's full-duplex pause resolution. The
[IEEE-hosted 2004 interpretation](https://www.ieee802.org/3/interp/interp-1-1104.pdf)
was additionally read for symmetric-pause priority context; its Clause 37 focus
does not make it the exact DP83826 technology authority.

The same original IEEE PDF was later checked visually for next-page sequencing:
28.2.3.4 pp294–298, 28.2.4.1.6/Table28-8 pp303–304, Figure28-18 p320 and
Annex28C p734. Restart clears the page-loaded flag, register7 writing loads it,
initial page receipt is the base page, and termination requires both peers to
finish. Null pages continue after local payload exhaustion while the peer has
more; reserved message codes cannot be transmitted. These correct a possible
pre-restart-load/every-event-is-next-page assumption. Exact DP83826I management
event/load boundaries remain the ledger's `next_page_handshake` implementation
gate, with TI clarification or identified traces as next evidence.

Retain the catalogue's confirmed defects as negative regression inputs, and
derive expected masks and sequences from original SNLS647I tables independently.
Do not treat repeated vendor code as independent corroboration. Required
preflight distinctions are C22 address versus mask, MMD namespace versus printed
address, declared suffix versus readable model, restart-clear versus completed
negotiation, and local pause policy versus SDK Boolean projection.

Source review: PASS for the bounded paths above. Sibling test execution,
portable/package consumers, SDK link, remote CI and physical hardware:
NOT RUN in this delegated audit. The lead records the stage's actual checker,
unit-test and reference/link results and final commit/sync. Vendor archive
integrity remains checked by the repository's existing reference verifier.

## Fresh independent follow-up

After checkpoint `01ce198`, a new reviewer reopened the original TI core tables
and IEEE next-page text rather than inheriting the prior conclusions. TI table
8-81 p86 shows VOD_CFG3 bits 15:12 as **R/W**; the earlier ledger's R label and
claimed conflict were wrong and have been removed. Section 7.6 p15's nominal
100M-signaling and RMII-leader-clock intervals apply to reset, not a separately
specified power-up interval. IEEE Annex 28B.4 p732 and 28.2.3.4.13g p298 explicitly
require a defining message before an unformatted series, with its referenced
pages following immediately in order. These corrections change no source bytes.

The fresh sibling re-audit reopened the relevant TMP1x2/MB85RC declaration,
implementation, rejection-test, CLI, runner, workflow and rule paths listed above.
Both remain clean at their listed revisions. TUSB32x has advanced to
`9e4b08a62303dbb9a6bbbfdbacb6aeb42154533c` with concurrent changes to rules,
docs, examples, metadata and tests, plus untracked
`docs/audit/execution-27-peers.md` and `docs/integration.md`. Its current rules and
capability-planning prompt were reopened as read-only references. The earlier
`4445cdc` observation remains historical; neither observation proves full TUSB32x
implementation. No sibling or vendor payload was modified by either audit.
