# Prompt 4 event, counter and signal-quality source review

Reviewed 2026-10-28 on Windows with Python 3.12/PyMuPDF 1.27.2.2.
Execution branch: `promptset/dp8382x-implementation-20261028`; entry checkpoint
`42a908b28d9ad471a60b5ca220127eb9e7dbf9ec`. This delegated review writes only this
report and a temporary integration fragment. The lead owns the ledger, shared
contracts, final checks and Git. Stronger reasoning was selected for destructive
reads and contradictory source tables. No implementation or fixture operation
is performed here.

Read workspace/parent agent rules, COMMON, prompt 4, EVIDENCE, FULL_COVERAGE,
README, architecture, variants, reference index and ledger schema. Adopt one
application owner, retained partial evidence, distinct desired/observed/trusted
state, finite primary/cleanup budgets and independent physical qualification.
The first-execution remote-CI exception reserves CI work for prompt 71.

## Original-source audit and independent literals

The following literals were derived from archived SNLS647I tables and original
PDF renderings, then compared with existing core records. Original pages
64–70, 72–73, 81 and 92 were visually inspected; table 8-44 starts on p71 and
table 8-90 is on p91. Access notation is table 8-18 p54: RC means read to clear;
RCH additionally allows hardware set/clear. All numeric words below are hex.
Vendor payloads/provenance are unchanged.

| Register / table | Address | Reset / known mask | Reserved mask | Independent field/access partition |
| --- | --- | --- | --- | --- |
| PHYSCR / 8-35 | C22 `0011` | `0108 / FFFF` | `04E0` | RW `8000,4000,3000,0800,0300,0010,0008,0004,0002,0001` |
| MISR1 / 8-36 | C22 `0012` | `0000 / FFFF` | `0000` | Eight individual RC event bits `8000..0100`; eight RW enables `0080..0001` |
| MISR2 / 8-37 | C22 `0013` | `0000 / FFFF` | `0000` | Same access partition; different event meanings below |
| FCSCR / 8-38 | C22 `0014` | `0000 / FFFF` | `FF00` | RC `00FF`; 8-bit, saturates `FF`, half-full exceeded at `80` |
| RECR / 8-39 | C22 `0015` | `0000 / FFFF` | `0000` | RC `FFFF`; explicit 16-bit field conflicts with maximum `FF` / half-full `7F` prose |
| RCSR / 8-41 | C22 `0017` | `0041 / FF5F` | `FE40` | RW `0100,0010,0003`; RHW `0080,0020`; RC `0008,0004` |
| 10BTSCR / 8-44 | C22 `001A` | `0000 / FFFF` | `C16E` | RW `2000,1E00,0080,0001`; polarity `0010` typed R but explicitly clears on read |
| MSE_Val / 8-72 | DEVAD `1F`: `0218` | `0000 / FFFF` | `0000` | R `FFFF`; raw, unscaled sample |
| RXFS / 8-91 | DEVAD `1F`: `04A1` | `1000 / FFFF` | `EF1E` | RW source `1000`; RCH `0080,0040,0020,0001` |

These C22 registers have the already established DEVAD-31 aliases. Extended
MSE has no `0225` alias. A reset value is not a generic whole-word write or a
stable dynamic-field expectation. RXFS reserved bits 4:1 are typed RCH in the
table but remain reserved; no extra event decoder follows from their access.

PHYSCR details: `8000` disables internal clocks only in IEEE power-down;
`4000` enables power-save selection. Selector `3000` has unshifted 0 normal,
2 active sleep; 1 reserved and 3 undocumented are rejected. `0800` bypasses
scrambling; `0300` selects 4/5/6/8 nibbles for codes 0/1/2/3. `0010` enables
collision signaling in full duplex. IRQ polarity `0008`: 0 idle-low/active-high,
1 idle-high/active-low. `0004` continuously forces an IRQ until cleared;
`0002` is global event enable. ENHANCED `0001` changes INTR/PWRDN from power-down
input to IRQ output and therefore requires board pin ownership.

MISR source mapping is literal and independent of the ledger:

| Event bit / enable bit | MISR1 | MISR2 |
| --- | --- | --- |
| `8000 / 0080` | Link-quality change | EEE error |
| `4000 / 0040` | Energy change | AN error |
| `2000 / 0020` | Link change | Page received |
| `1000 / 0010` | Speed change | Loopback FIFO overflow/underflow |
| `0800 / 0008` | Duplex change | MDI/MDIX change |
| `0400 / 0004` | AN complete | Sleep event |
| `0200 / 0002` | False-carrier half-full | Polarity or WoL event |
| `0100 / 0001` | Receive-error half-full | Jabber |

MISR `2020` is link-event plus enabled source in MISR1, not live link up.
MISR1 `2000` still contains consumed link history even when enable `0020` is
clear. MISR2 `0202` identifies a shared-source event; RXFS `1000` context means
WoL, `0000` means polarity. Unknown/changing selector context remains ambiguous,
not two simultaneous labels. WoL enabling automatically changes the selector.
Configuration reads and readback consume events just as service reads do.

10BTSCR squelch codes 0..8 yield 200,250,300,350,400,450,500,550,600 mV;
shifted endpoints are `0000` and `1000`. Codes 9..15 are undocumented and
rejected. Lower-threshold enable is `2000`, NLP disable `0080`, jabber disable
`0001` (10BASE-Te only). A read clears polarity `0010` and PHYSTS `1000`.
The explicit side effect overrides the naive assumption that the R label makes
this a passive register.

## Shared core observations and conflicts

PHYSTS table 8-34 remains partitioned correctly by prompt 3: reserved `8000`,
reset `0002`, speed `0002` (0=100, 1=10), duplex `0004` (0=half, 1=full).
BASIC AN-disabled values are described as strap-latched; ENHANCED AN-disabled
speed and duplex both read 1 by the table. This cannot universally represent
software-forced BMCR 100/half settings. Keep the existing `forced_physts` gate;
configured intent and independently resolved link remain separate.

PHYSTS mirror-clear ordering from original p64:

| PHYSTS mask | Clearing access / meaning |
| --- | --- |
| `2000` | RECR read clears receive-error latch |
| `1000` | 10BTSCR read clears polarity; PHYSTS read explicitly does not |
| `0800` | FCSCR read clears false-carrier latch |
| `0400,0200` | Signal detect / descrambler lock typed RC; retain raw before later read |
| `0100` | ANER read clears page-received mirror |
| `0080` | MISR read clears pending indication; table specifically names `0012` |
| `0040` | BMSR read or reset clears remote fault |
| `0020` | BMSR jabber duplicate, 10 Mbit/s only; PHYSTS read does not clear it |
| `0001` | Link duplicate, not cleared by PHYSTS read |

Capture requested PHYSTS mirrors before their clearing source reads, but never
claim the resulting multiframe group is atomic. BMSR first/second reads retain
historical and current evidence separately. Old latched events are not current
link state. A reset/restart creates a new observation epoch; it cannot restore
consumed hardware history or justify clearing retained host history.

RCSR table 8-41 p70 explicitly says `0008` overflow and `0004` underflow are RC,
reset zero, with **0 fault / 1 normal**. `000C` thus represents both normal in
the printed encoding; `0000` is both fault encodings, but reset-zero alone is
not a newly observed fault. Clear/rearm timing remains unexplained. Keep
`rmii_rc`; do not reverse polarity to make a fake convenient.
The reset header `00X1` is refined only from explicit table rows: reserved bit6
is one, elasticity is code1, while strap-loaded bits7/5 stay unknown. Thus
partial reset `0041`, known mask `FF5F`. Elasticity codes0/1/2/3 give14/2/6/10
bits and16800/2400/7200/12000 bytes at +/-50ppm; halve the stated lengths at
+/-100ppm. Clock shift is follower-only; physical clock wiring remains required.

RECR preserves all 16 raw bits, including `0100`, `8000`, `FFFF`, without
claiming those counts are reachable on hardware. It counts receive errors with
valid carrier/RXDV and does not count in MII loopback. The width/saturation
conflict remains `recr_width`; do not silently truncate to 8 bits, assume
wraparound, or promote raw sums to exact lifetime errors. FCSCR `FF` is a
saturated lower bound; repeated sampling cannot recover missed excess events.

RXFS `0080` SFD-error prose references RXFCFG1 bit 13, which table 8-90 marks
reserved. `rxfs_sfd_reference` retains this discrepancy: raw SFD error remains
observable, but there is no established SFD-byte customization write. RXFS
`0040` bad CRC, `0020` invalid Secure-ON and `0001` Magic Packet are separate
consumed evidence. CRC gating and WoL policy affect interpretation.

## MSE and application-note applicability

[SNLA423A](../../reference/source/TI-DP83826-Troubleshooting-SNLA423A.pdf),
June 2025, table 2-6 p12 uses `0225`, while the exact datasheet uses `0218`.
The original PDF formula is `10 log10(0.5) - 10 log10(MSE / 2^17)`;
the denominator is **2 to the power 17**, not 217. Table 2-7 leaves exact
19.5/22 dB boundaries unassigned and says nothing about zero input. This is
recorded source arithmetic, not an accepted conversion for raw `0218`.

[SNLA495](../../reference/source/TI-DP83826-EtherCAT-Link-Down-SNLA495.pdf),
February 2026, table 3-3 **p7**, independently uses `0218` and gives `MSE > 033B`
as a reason to investigate the MDI path. This corroborates the datasheet address
without establishing `0225` as an alias, universal quality classification, a
sampling/update law, or measured dB accuracy. SNLA495 also identifies RECR and
RCSR error observations, but resolves neither counter width nor FIFO polarity.
Its ESC `0110`, `0141`, `0301/3/5/7` and `0510...` addresses belong to the ESC,
not to the PHY MDIO map. Its EtherCAT AN/100-full/Auto-MDIX policy is contextual.

[SNLA494](../../reference/source/TI-DP83826-EtherCAT-Frame-Errors-SNLA494.pdf),
February 2026, sections 2.2 and 3, distinguishes ESC receive/invalid/forward
error counters and uses MII loopback to isolate failures. ESC counters start
under the described port-open conditions. Neither these counters nor WKC are
RECR/FCSCR, and EtherCAT frame success is not PHY signal-quality calibration.

[SNLA344C](../../reference/source/TI-DP83826-EtherCAT-SNLA344C.pdf), October 2023,
interface/clock requirements and register setup, addresses EtherCAT slave
integration. Its RMII latency qualification and MII-oriented recommendations
cannot be imported into the P4 RMII leader application as proven behavior.
[SNLU262A](../../reference/source/TI-DP83826EVM-SNLU262A.pdf), June 2023,
sections 3–5, supplies EVM/clock/traffic/instrumentation context; its EVM test
results do not qualify another board's collector, IRQ, counter or MSE behavior.

TI E2E searches on 2026-10-28 used exact DP83826/RECR/16-bit, RMII overflow,
MSE/0218/0225 and MSE/SNR queries. No inspected exact-part answer closed RECR or
RCSR gates. A search-index snippet for a Gerome Cacho DP83826E MSE/SNR FAQ said
`0218`; reopening the forum listing returned different content and did not
expose that FAQ. It is **not** treated as inspected clarification or a scaling
authority. Existing exact datasheet plus SNLA495 evidence stands independently.

## Current driver and sibling comparison

GitHub connector rechecked the actual branch SHAs and read immutable files:

- TI `ti-ethernet-software/main` remained
  `450af1144b8caeb58724bcbae081b409f6bf5c3a`. The
  [Zephyr PHY file](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c)
  declares Apache-2.0. Lines 194–217 read MISR1 and discard the word, with
  `K_FOREVER`; lines 493–526 configure only link IRQ and consume further MISR
  reads. Adopt the field comparison, reject event loss and infinite lock policy.
- TI `ti-linux-kernel/ti-linux-6.18.y` remained
  `97becb13b81c314c1d17fcf2226aac9a32086f3d`. The
  [PHY source](https://github.com/TexasInstruments/ti-linux-kernel/blob/97becb13b81c314c1d17fcf2226aac9a32086f3d/drivers/net/phy/dp83822.c)
  declares GPL-2.0. Lines 324–390 configure both MISRs; lines 393–425 read
  MISR1 then MISR2, correlate status with enable bits, and return on MISR2
  failure without a retained MISR1 sample. DP83826 entries use these callbacks
  at lines 1193–1216. This is comparison evidence only; no GPL code is copied.
  Disabled-source events are still consumed history and must be retained here.

Read-only sibling audit used clean trees at:

| Repository | Revision | Reopened paths / selection |
| --- | --- | --- |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | `AGENTS.md`, `include/TMP1x2/TMP1x2.h`, `src/TMP1x2Operations.cpp`, `test/jobs/test_jobs.cpp`, `examples/common/Tmp1x2Cli*.cpp`, `tools/hil_tmp1x2_runner.py` |
| PCA9555 | `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | `AGENTS.md`, `include/PCA9555/PCA9555.h`, `src/PCA9555.cpp`, `test/test_basic.cpp`, `examples/01_basic_bringup_cli/main.cpp`, `tools/run_i2c_hil.py` |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | `AGENTS.md`, `include/MB85RC/MB85RC.h`, `src/MB85RC.cpp`, `test/test_basic.cpp`, `tools/hil_runner.py` |

TMP1x2 provides retained token/results, passive snapshots and bounded callback
accounting. Its ordinary staged snapshot publication cannot erase PHY words
already consumed by destructive reads, including a successful late callback.
PCA9555 lines 770–811 explicitly preserve an input observation when later pointer
cleanup fails; adopt independent primary/cleanup results, adapt pointer cleanup
to actual MDIO transaction policy. MB85RC's retained result and completed-prefix
uncertainty patterns are useful; memory readback reconciliation cannot recover
a lost read-clear PHY sample. Runner verdicts require bounded, correlated,
identified evidence; none of the sensor/memory timings specify PHY timing.

## Procedure and acceptance handoff

The temporary fragment supplies nine completed register records and five
procedure contracts: `interrupt_configuration`, `interrupt_test`,
`collect_event_counters`, `signal_quality_sample`, `calibration_recovery`.
The shared-core PHYSTS numeric record needs no replacement; its gate remains.
Power, transmit and WoL fields numerically partitioned here retain stage-5
feature ownership. The lead integrates tests/docs and feature evidence.

Collector host acceptance must independently inject failure at every terminal
read, including MISR1 success followed by MISR2 failure, FCSCR success followed
by RECR failure, and RXFS/10BTSCR consumption during RMW. Retain raw consumed
prefix, enabled/source context, timestamps and history/reset epochs; ambiguous
failed target reads create a history gap. Selector setup failure before target
access does not itself consume the target. Cancellation/deadline stop new work,
and cleanup never replaces primary error or discards retained words. Cached
health/link/configuration views issue no MDIO. A software total saturates visibly
and retains incomplete/lower-bound status instead of wrapping silently.

Calibration table 8-47 p73: only if CDCR `4000` is observed high, clear it and
immediately issue PHYRCR C22 `001F=8000`. Do not start calibration or reissue
CDCR cable trigger `8000`. No numeric maximum for "immediately" is specified:
admit/reserve the adjacent write pair under one transaction exclusion, with no
wait/output/unrelated access between them. If deassertion may have occurred,
reset remains owed even after cancellation, under separate finite cleanup.
Reset failure leaves recovery/configuration uncertainty. Follow core hard-reset
completion/restoration and the `hard_reset_straps` gate; retain consumed history,
replay persistent state, never temporary test settings or event bits.

Independent FLD cross-review for the lead also inspected original pp63/76:
FLDS reset `0000`, known `FFFF`, reserved `FE0F`, RC `01F0`; FLD_CFG1 reset
`8147`, reserved `000F`, RW `FC00/03F0`, both zero codes prohibited; FLD_CFG2
reset `2284`, reserved `FFC0`, RW `003F`. Section 8.3.17 literal `0117=0417`
decodes under table 8-52 as window code 1 (two 8 ns clocks = 16 ns) and threshold
code 1 (one error), inconsistent with the feature prose's 20 errors/10 us.
Retain the exact recipe and explicit gate; do not promise the prose detector
calibration from that literal. Table says the counter runs in steady state.

Independent diagnostics-fragment cross-review also checked original pp54,
68–73 and76–81: all BISCR/BICSR/CDCR/TDR reset, reserved and access partitions
match the tables, including entirely reserved result banks. Segment words
`0D07/0D14/1A21/343B/8F6F`, shadow words `1004/1004/1005/1008/100B` and pulse
codes `2/2/2/2/6` agree. The separate BICSR1 write15 action correctly avoids
making the read-only error byte generally writable. Review requested explicit
PHYRCR restart ownership for digital loopback, BMCR loopback exclusion for
reverse mode, and internal-loop exclusion before external fixture claims.
The lead integrated these procedure clarifications. Readback review confirmed
digital ownership `BMCR 7D80 / BISCR 705F / PHYRCR 4000`, reverse exclusion of
BMCR bit14, and external-path exclusion of BMCR bit14 and BISCR low5/bit6.
Autorun records possible contamination by continued partner activity. No
blocking review findings remain; raw TDR freshness/abort and BIST capture-lifecycle
gates remain necessary.

Physical acceptance needs identified part/mode/board/firmware, MDIO traces,
controlled invalid-symbol/false-carrier injection, IRQ pin capture, RMII clock
offset/FIFO fault stimulation, reset traces and calibrated noise/cable fixtures.
RECR threshold tests must cross `7F/80`, `FF/100`, `7FFF/8000/FFFF`; RCSR tests
must record idle/reset, injected event, first/second reads and rearm. MSE needs
exact-source scale clarification before a dB accuracy verdict. All physical
evidence is NOT RUN. Portable/package consumer, SDK link and remote CI are
NOT RUN in this source-review work unit; host ledger tests are run by the lead
after integration. Source review establishes research facts and gates only.

Integration readback compared all nine event register records, five procedures,
four gates and two source entries against the submitted fragment without
differences; feature changes were explanatory notes and additional shared gates.
Local `python tools/verify_ledger.py` passed with 96 registers, 28 procedures and 35
features (structural research validation only). `python tools/verify_references.py`
passed with 27 vendor artifacts, 25 extracts and 1185 links in 138 Markdown files.
The report's `git diff --check` passed. Driver host tests, consumer builds, SDK
link, remote CI and physical measurements remain NOT RUN in this review.
