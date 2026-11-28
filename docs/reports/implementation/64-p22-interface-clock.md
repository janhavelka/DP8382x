# Prompt 64 — DP83822 interface and clock control

## Phase (a) assessment

Prompt 64 is **IN PROGRESS**. The two assigned procedures are
`mac-interface` and `clock-phase` for revision-0 DP83822I, H, IF and HF.
On 2026-11-28 the wrapper verified the prompt 63 completion commit
`8d01295baa3be81241c4dce824d477724c935ebc` on both `main` and
`origin/main`. Entry had a clean worktree and index and zero ahead/behind.
Prompt 62 supplies the exact-part field inventory. Prompt 63 supplies copper
identity, straps, reset, link and destructive-read collection. The existing
interface API supplies bounded configuration, verification and restoration
for the earlier parts. [Numbered prompt](../../prompts/64-p22-interface-clock.md)
· [Exact-part ledger](../../registers/dp83822.json)
· [Prompt 63 verification](63-p22-copper-core.md#phase-d-final-verification)
· [Existing interface contract](../../interface-clock.md)

The lead traced the public contract, transport admission and replay. Two
parallel read-only assessments used the session model for silicon interpretation
and cross-layer admission. Sources included the original TI tables, pinned
drivers and SDK, peer tests, actual CLI and runner. Both assessments finished.
The dispositions below also cover G9-6, G9-13, G9-15 and G9-17.
[Audit dispositions](../../reports/promptset-audit-dispositions.md)

## Current behavior and integration decisions

- `src/PartTable.h` advertises portable MII, RMII and RGMII for the four
  grades. `src/DP8382xInterface.cpp::_validateInterface` gates all DP83822
  configuration. Its existing eight-row pass contains addresses, masks and
  clock choices specific to other parts. Keep their public behavior and
  tests. Supply exact-part settings, observations and pass costs for DP83822.
- In `src/DP8382x.cpp`, `_dp83822BoardStatus` checks accepted identity,
  fresh SOR1/SOR2, LED test modes, address, observed CR2 medium and strapped
  interface/role. Preserve that default contract. A board deliberately
  strapped for one interface and wired to run another needs an explicit
  declaration of its expected boot straps and intended wiring. Test that path
  from bind through startup and HARD recovery. Keep the decoded boot
  interface, requested target and observed RCSR distinct in snapshots and
  runner records. A silent relaxation of the strap check would lose prompt
  63's wrong-board protection.
- Keep the existing direct-call contract for earlier parts. A DP83822 change
  that needs a digital restart needs a bounded operation that can yield while
  the device acts. Extend the interface owner and shared replay steps for
  preflight, apply, restart completion and verification. Release the controller
  before each wait. Reserve the complete applicable path before its first
  consuming read and recheck at submission. For cooperative execution budget
  each complete transaction and the remaining operation. Reuse restart
  command semantics and conservative post-callback timestamps.
- Check both direct and poll dispatch in `_execute`, the shared collector,
  reset invalidation, `_admitProfilePass`, `_addOwnedReplayCost` and the
  `_interfaceTailPending` path. Restoration temporarily clears
  `_initialized` while re-establishing owned state. Check current board
  evidence at that boundary and reserve initialized-only admission for the
  public mutation entry. A changed medium or invalid strap sample must withdraw
  interface trust along with the existing copper evidence.

## Silicon acceptance for phase (b)

The original SNLS505H PDF has SHA-256
`d73d83c018e6dab3f935ea45ca1771bbc34c030b6b24de45fccd1dfa6998a2d3`.
Use the printed field tables for independent test literals. TI RTOS
`94bf4343` in `rtos_drivers/src/dp83822.c` admits all three MAC modes but
its configuration path handles MDIX. TI Linux `97becb13` in
`drivers/net/phy/dp83822.c` updates the two shift bits under a mask. The
combined TI Linux tree at `450af114`, `linux_drivers/dp83822.c`, sets
nonzero bits and can leave an old shift enabled. The comparison motivates the
sixteen prior-to-target tests below. [Original datasheet](../../reference/source/TI-DP83822-SNLS505H.pdf)
· [Pinned source comparisons](../../reference/library-family.md#interface-clock-analog-and-led-facts)

| Area | Source and independent acceptance |
| --- | --- |
| Board, medium and direction | Section 4 and tables 5-1, 7-10 and 7-13. For each grade exercise MII and RGMII with 25 MHz, RMII leader with 25 MHz input and 50 MHz output, and RMII follower with external 50 MHz CMOS. Reject reserved XI/interface rows and conflicting clock drivers. IF/HF FX over MII/RMII is documented in §§7.4.1.1/.2. RX_ER mode 3 can encode FX plus RGMII, but applicability is open. Keep optical operation with prompt 65. |
| RCSR mode and phase | Table 8-23 at 0017. Bit 9 takes precedence over bit 5. Bits 12/11 select fixed 3.5 ns RX/TX delay with opposite polarity. For off/off, on/off, off/on and on/on use 0800, 1800, 0000 and 1000 under mask 1800, from **each** prior combination. Default shift bits 00 mean RX aligned and TX delayed. Bit 10 requires a shared reference for synced TX. Independently exercise RMII TX shift [8], reference [7], recovered FIFO use/bypass [6], revision [4] and elasticity [1:0]. Ordinary default before strap overrides is 0041. |
| RMII elasticity and clocks | Tables 7-3 and 8-23. Codes 0/1/2/3 have tolerance 14/2/6/10 bits and packet limits 16800/2400/7200/12000 bytes at ±50 ppm, halved at ±100 ppm. Test each limit and one byte over. Distinguish FIFO tolerance from start threshold. Check the RX_CLK/XI receive selection and inverted FIFO-bypass polarity. The repeater layouts in §7.4.1.3 are board wiring between two PHYs. |
| Companion controls | Tables 8-1/10/11/17/78. Cover BMCR[10] isolation, CR1[9] enhanced RMII and [1] fast RX_DV, CR2[3:0] conditional MII isolation, idle RX_ER, odd-nibble disable and RMII receive clock, PHYSCR[4] full-duplex COL and GENCFG 0456[3] 120/200 ns minimum IPG. Bit 3 defaults to 1 and sub-200 ns is MII-only. Exercise pre-existing intent and defaults during transition. Compose neutral values for mixed access and preserve prompt 63's CR1/CR2 MDIX and IRQ ownership. |
| GPIO clock routing | Tables 8-81/82 and §7.3.4. Cover GPIO3 on 0462[14:12], GPIO1 on 0462[6:4] and GPIO2 on 0463[6:4], each with its own function field set to 011. Selectors 0/1/2/4/6/7 mean MAC clock, XI, internal 25 MHz, RMII leader 50 MHz, free-running 125 MHz and recovered 125 MHz. Reject 3/5. Test pin/data/LED/WoL reservations, GPIO1 versus FX signal detect on pin 24 (inferred), leader output direction and the recovered-clock partner prerequisite. Preserve unrelated pin fields and reserved bits on ordinary updates. |
| MII TX phase | Table 8-36 at vendor 0042. Use a four-bit code 0–15 and a distinct [4] W1S apply action. Exercise every code, including downward changes and the 2→9 example (+28 ns at 4 ns per count). Preserve the conflict between enable prose [4:0] and the operand row [3:0]. Verify stable fields separately from acceptance of the action. Record uncertainty after a failed action without blind retrigger. Physical phase and trigger ordering need qualification. |
| Pin-only status and timing | §7.4.1.4 table 7-5. Under RX_DV low, nibble 9 denotes 10/full/up and 3 denotes 100/half/up. Exercise the reserved speed codes and RX_DV-high validity rule as MAC-side vectors. The MAC samples pins, so a PHY register poll cannot deliver this status. Account for RX_CLK stretching. §§6.20/.21 give qualified 100BASE-TX latency figures. Only the RGMII timing footnotes add 0457=0410 to CR1=0002. This opaque setting lacks a documented inverse. Keep ordinary interface programming clear of 0457 and record the measurement conditions and open restoration gate. |

### Transition, failure and restoration vectors

1. Reject invalid enum values, wrong declared medium/interface, missing
   maintenance/MAC-stop/reference grants and known pin conflicts before clock
   or MDIO callbacks. Preserve prior desired settings, trust, token and caller
   output. A conflict discoverable only from a live preflight may consume a
   read. Keep its FIFO evidence and make zero target writes.
2. Read live RCSR and 0462 before any mode write. For software RCSR[5] 0→1
   entering RMII, check permission to temporarily drive GPIO3/RX_D3 and
   tri-state GPIO1/LED_1. Upper-field owners must permit [14:12]=4 and
   [10:8]=3. A board needing uninterrupted lower-pin service cannot admit
   that interval. Write the new RCSR, then **literal** vendor 0462=4300,
   then C22 001F=4000. Retain the lower [6:4]/[2:0] values before the
   literal. After restart, restore applicable nonzero, owned or strap-loaded
   fields by mask and verify them (inferred). For example, starting with
   compatible upper fields and lower 0063, the final word should have 4363
   under mask 7777. The literal selects the leader clock even for software
   follower entry. Record this open electrical question and require the board
   to admit the transient. Hold the application's maintenance and mux ownership
   across preflight, restart and restore so another writer cannot take the pins
   between controller transactions. Strapped RMII already active skips the recipe.
3. On live RMII exit ([5] 1→0 or [9] 0→1), restore 0462[10:8] to normal
   data function by masked write, then digital restart. This sequence is
   inferred from table 5-1. Test both edges, upper clock-selector retention,
   unaffected lower fields, already-active/no-edge cases and restored readback.
   Restart retains registers/straps and invalidates live link evidence.
4. Exercise C22 and vendor-31 alias access to RCSR. The indirect read is
   W0D=001F, W0E=0017, W0D=401F, R0E in one controller hold. Include the
   dependent W0E in the same hold for RMW. Every completed RCSR read consumes
   FIFO [3:2] through the one collector, even on later write or release
   failure. Force those bits to zero in the write. Keep 0D and 0E direct
   only (§7.4.2.1). Exercise competing access and interrupted prefixes.
5. Inject errors before and after each target write, including effect-possible
   errors, late or failed release, insufficient frames, deadlines and frozen
   clocks. In particular, if bit 5 already changed but 4300 or 4000 failed,
   the next RCSR sample alone cannot prove that the recipe completed. Retain
   the saved lower fields and the completed/uncertain step through OP-9.
   Exercise cancellation, retry from the retained state, independently
   budgeted restoration, mismatch after restore and silent cached snapshots.
   Include HARD relatching the original straps, RESTART retaining the words
   and a read-only verify pass. Check separate primary, controller-release and
   later restoration results. [Verification contract](../../prompts/VERIFICATION.md)

## SDK, CLI and work ownership

At pinned ESP-IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`,
`components/esp_eth/src/mac/esp_eth_mac_esp.c` has MII/RMII paths. Its
RGMII path requires `SOC_EMAC_SUPPORT_1000M`, absent from the P4
`components/soc/esp32p4/include/soc/soc_caps.h`. The P4 factory currently
checks the DP83822 LED_1 attestation and delegates interface admission to
portable bind. Add an explicit native diagnostic before resources or bus
callbacks for an unavailable interface. Cover native maintenance entry too.
Keep the portable interface mask 7 and preserve output on rejection. Run
the actual pinned native factory and supported MAC image paths. The adapter
link project's current part allowlist omits DP83822. Include an identified
DP83822 build on supported wiring when reporting a DP83822 link.

For phase (b) the lead owns the public contracts in `include/DP8382x/`,
boot/runtime declarations, `src/DP8382x.cpp` integration, native admission,
registration, progress and final documentation. Assign the interface procedure
and independent literal tests in `src/DP8382xInterface.cpp` and
`test/interface.cpp` to one implementer after agreeing the types. Assign
the shared CLI, compiled fixture and Python runner/replay changes to the
other. Keep files disjoint and route every entry through the same driver owner.

The legacy CLI parser in `examples/common/Dp8382xCliPins.h` accepts MII and
the two RMII roles. `test/cli/dp8382x_cli_fixture.cpp` already supports
`--rgmii`. Extend its exact-part reset, selector and RC effects alongside the
parser, help and status records. In `tools/dp8382x_hil_protocol.py`,
preserve checks against the original strap declaration. Extend the specific
control cases in `tools/dp8382x_hil_controls.py` and the per-part feature
gate in `tools/dp8382x_hil_campaign.py`. Validate grade, target and observed
interface, shift polarity, mux, operation outcome and restored profile against
real compiled transcripts, including altered saved receipts. Update public
Doxygen, the API/interface, variant, adapter, CLI and fixture guides and the
two coverage rows with the executing paths. README and CHANGELOG currently
describe prompt 63 copper support.

### Read-only sibling audit

All three peers were clean on `main`. The parent and peer AGENTS guidance,
example and runner conventions were checked against this repository's
serialized MDIO, clock and SDK contracts.

| Peer revision | Paths reopened and convention selected |
| --- | --- |
| TMP1x2 `0791429f8fce1170e2082f1b011456fc461318b8` | `include/TMP1x2/Config.h`, `src/TMP1x2Configuration.cpp`, `test/helpers/test_helpers.cpp`, `test/variants/test_variants.cpp`, `examples/common/Tmp1x2Cli.cpp`, `tools/hil_tmp1x2_runner.py`. Adopt pure board/enum validation and literal per-variant adverse cases. |
| TUSB32x `6989c77fad3bc4651403942a4c9f7ca3940f859b` | `include/TUSB32x/Config.h::DeviceProfile`, `src/TUSB32xOperations.cpp`, `test/test_profile_application.cpp`, `test/test_profile_observation.cpp`, `examples/common/Tusb32xCli.cpp`, `tools/tusb32x_hil_runner.py`. Its 256-encoding admission sweep checks token, desired generation and verified state. Adapt complete-profile rejection to uncertain writes and destructive PHY reads. |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `include/MB85RC/CommandTable.h::VariantInfo`, `src/MB85RC.cpp::variantForExpected`, `test/test_basic.cpp`, `examples/common/DiagnosticCore.h`, `tools/hil_runner.py`. Adopt explicit declared capability rows and zero-transfer unsupported operations. Retain board-declared suffixes under the common PHY ID. Sibling I2C build targets do not establish EMAC support. |

## Entry verification and remaining gates

Windows x86-64, GCC 15.1.0, Ninja Release, Python 3.12.10. These checks
establish the entry behavior and source evidence for the brief.

```powershell
cmake --build build/p63-baseline --parallel 4
ctest --test-dir build/p63-baseline -R '^(p22_.*|(ai_|ae_)?interface(_.*)?|cli_controls.*)$' -j 3 --output-on-failure --no-tests=error
ctest --test-dir build/p63-baseline -R '^cli_(pin_transcript|control_transcript|hil_runner|hil_campaign)$' -j 3 --output-on-failure --no-tests=error
python -m unittest discover -s test -p test_p22_ledger.py -v
python tools/verify_ledger.py
```

| Layer | Phase (a) result |
| --- | --- |
| Host behavior | PASS. Release build current, 38/38 DP83822 and existing interface CTests, then 4/4 compiled CLI/runner/campaign tests. In the existing fixture on DP83822HF with `--rgmii --attest-features`, `caps` reported interface mask 7, and `controls interface rgmii 0 0 1518 50 0 keep keep` returned INVALID_PARAM. The MII form returned UNSUPPORTED. Both transport snapshots showed 8 batches and 27 completed frames. This is the missing programming path at entry. |
| Research | PASS. 16/16 DP83822 ledger tests, including original PDF field cells, plus the family ledger verifier. |
| Report and source integrity | PASS. The reference checker verified 29 archived payloads, 27 text extracts and 2536 local links. Tracked/staged whitespace and the new report's UTF-8, whitespace, eight local link destinations and two procedure IDs passed. |
| Portable/package consumer | NOT RUN in phase (a). Exercise the changed exports through both ordinary and clean package consumers in implementation verification. |
| SDK link | NOT RUN in phase (a). P4 MAC source admission was assessed at the recorded pin. |
| Remote CI | NOT RUN. No tracked workflow exists at entry. Prompt 71 owns the checkpoint. |
| Physical | NOT RUN on all four grades. Record identified boards, direction/frequency, scope traces for fixed shifts and TX phase, GPIO contention, FIFO and transition behavior, including follower entry, reverse transition and restore. FX/RGMII applicability remains open for IF/HF. |

Phase (b) can proceed with both assigned software procedures. The wrapper
owns this planning checkpoint and upstream verification.

## Phase (b) ESP32-P4 interface admission

The wrapper verified clean `main` and `origin/main` at
`0f73f32f929a36bc756e6d39383170b9ecdba002`. This slice implements the
native MAC admission requirement of `mac-interface`. Both portable
procedures remain in progress.

At ESP-IDF v6.1 `fff9895c`, lines 905 and 922 of
`components/esp_eth/src/mac/esp_eth_mac_esp.c` implement MII and RMII.
Line 1001 guards the RGMII case with `SOC_EMAC_SUPPORT_1000M`, absent in
`components/soc/esp32p4/include/soc/soc_caps.h`. The component manifest
selects ESP32-P4. The factory now reports `ESP_ERR_NOT_SUPPORTED` before
allocating the owner, gate or fence for an LED_1-attested DP83822 RGMII
request. The installed, stopped interface entry checks its existing owner
and identity requirements, then rejects RGMII before invoking the portable
procedure. The capability snapshot continues to report interface mask 7.
[Native API and guide](../../esp-idf.md)

The lead implemented the native gate and updated its public Doxygen, README,
changelog and adapter guide. One agent added regressions in
`test/esp_idf/host/native.cpp`. In parallel, a read-only agent checked the
SDK and reopened the three clean peer revisions and paths in the phase (a)
sibling table. Their explicit declaration checks and pre-I/O rejection tests
apply here. Both used the session model to trace native ownership and finished.

Windows x86-64 Release, GCC 15.1.0, Ninja. With the process-local SDK Git
setting from [prompt 61](61-p25-feature-closure.md#phase-d-esp32-p4-sdk-links),
run the following from the repository root.

```powershell
cmake --build build/p60-tests --parallel 4 --target dp8382x_native_host_tests dp8382x_native_host_100hz_tests dp8382x_native_host_333hz_tests dp8382x_native_cli_tests
ctest --test-dir build/p60-tests -R '^((e_)?native_factory(_(100|333)hz)?|native_cli_.*|native_board_.*|native_feature_.*)$' -j 3 --output-on-failure --no-tests=error
python tools/check_doxygen.py
python tools/verify_references.py
git diff --check
```

- Host PASS. The build verified 45 pinned SDK inputs. All 47 selected CTests
  passed, including all four factory registrations. For each I/H/IF/HF
  declaration the factory test rejects RGMII under both MAC attestations
  with the output sentinel, clock count, frame trace and unconsumed mutex
  failure intact. It creates an MII owner, initializes an RMII owner and
  verifies the P4-specific stopped diagnostic, preserved result and desired
  state, and zero additional clock samples or MDIO frames.
- SDK link PASS for the existing DP83826I RMII owner regression. Using the
  scoped SDK environment from prompt 61, `scripts\idf.cmd -C examples/esp_idf/owner -B C:\esp\build\dp8382x-loop\56o build`
  linked a 611072-byte image from `0f73f32` plus working changes. SHA-256
  `55529c82875593c55033cd84b98f0cf1308f8f76b7cdf7b04709376f459bc275`.
  The DP83822-declared supported-interface image is still due.
- Doxygen PASS for 300 pages and two negative fixtures. Portable/package
  consumers, remote CI and physical qualification NOT RUN in this slice.

Continue phase (b) with the portable board declaration and independent
bind/startup tests for expected boot straps versus intended copper wiring.
Then implement RCSR collection and interface/clock programming, interruption,
restoration and CLI/runner evidence from the matrix above. Keep the native
MAC check on any new cooperative interface entry. The wrapper owns the
checked commit and upstream verification.

## Phase (b) portable boot interface

The wrapper verified the native admission checkpoint at
`4bb6509c7f6feefa069a73ddfe3d714d399e0e6f` on both `main` and
`origin/main`. The worktree and index were clean at entry. This slice of
`mac-interface` implements the expected boot row for a copper binding.
`clock-phase` and runtime interface programming remain in progress.

SNLS505H table 7-13 (printed pp. 50–51) gives a 25 MHz XI for MII, RGMII
and RMII leader, and 50 MHz for RMII follower. RGMII takes priority over
RMII_EN. The explicit declaration models a board with a fixed XI reference.
Bind rejects clock-frequency crossings before installing the new configuration.
Existing boards default to `AS_WIRED`. The cached strap snapshot continues
to reflect the words actually read. An unexpected boot row fails the board
check even when the observed interface happens to match the target. A correct
boot row requiring a transition reaches a separate `UNSUPPORTED/FEATURE`
gate before reset or configuration writes. This keeps native readiness closed
until the portable owner can establish and verify the target.
[Board declaration](../../interface-clock.md#dp83822-boot-interface)

The lead owned the public type, bind and startup checks, integration and
guides. One agent implemented the independent literal matrix in
`test/reset.cpp`. A second, read-only agent checked the original table, native
readiness and the six paths per peer in the phase (a) audit. TMP1x2, TUSB32x
and MB85RC remained clean at the revisions recorded there. The session model
was used for the cross-product tests and silicon interpretation. Both agents
finished.

| Layer | Windows x86-64, GCC 15.1.0, Ninja Release |
| --- | --- |
| Host | PASS. Rebuilt reset, quality, identity, interrupt and link test targets in `build/p63-baseline`. CTest with `-R '^p22_\|^reset_'` passed 34/34. The four-grade test varies declared boot row, target, observed SOR words and NONE/HARD startup. It verifies the 11-frame rejection trace, retained straps, initialized state and zero-callback bind failures. The native host build verified 45 pinned SDK inputs. Both `e_native_factory` and `native_factory` passed in `build/p60-tests`. |
| Portable/package consumers | PASS. Both tests-OFF consumers passed in `build/p63-consumer`. Copied the 29 source-package paths from the prompt 63 export list, byte-checked each against the working tree and built a fresh tests-OFF application in `build/p64-boot-export/out`. Its 1/1 CTest checks the public default, a compatible DP83822IF MII-to-RMII leader declaration and rejection of a 50 MHz target. |
| Documentation | PASS. Doxygen verified 300 pages and two negative fixtures. README, changelog and the interface guide document board admission. |
| SDK link / remote CI / physical | NOT RUN in this slice. The supported DP83822 P4 link remains due. Prompt 71 owns remote CI. Clock and transition measurements require an identified authorized fixture. |

Continue phase (b) with RCSR access through the shared FIFO collector. Trace
both direct and vendor-31 reads, including failed releases and the dependent
RMW write, before enabling the runtime transition. Keep the explicit boot row
as the identity of the expected board across HARD recovery.
