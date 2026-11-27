# Prompt 63 · DP83822 copper identity, reset and link

**COMPLETE for revision-0 copper core software.** The [final verification](#phase-d-final-verification)
records the results for I, H, IF and HF and the limits on interrupt and counter evidence.

## Phase (a) assessment

**IN PROGRESS.** Phase (b) can implement revision-0 copper operation on
DP83822I, H, IF and HF. The current driver has one private, unselectable
DP83822 profile. Turning it into four selectable rows alone would expose
DP83826 startup writes, IRQ routing and reset timing to DP83822 callers.
Add the exact-map paths and their rejection tests together. Prompt 62
completed at `d9671163249180df8ccd94f720ef1ba54f48b29b`. The worktree
and index were clean on main, the wrapper verified origin/main at that
commit, and the local comparison was 0 ahead, 0 behind.
[Prompt requirements](../../prompts/63-p22-copper-core.md) ·
[Prerequisite research](62-p22-source-subtypes.md#phase-d-final-verification)

The ten procedure IDs are `identity`, `straps`, `management`, `reset`,
`copper-link`, `negotiation`, `mdix`, `link-loss`, `irq` and `counters`.
Their field assignments cover 145 named fields in 27 tables. Count the
cross-procedure requirements separately, including the 10-Mbit receiver
controls, RXFS selector and destructive reads in diagnostic and EEE words.
Use one collector, one application owner and the existing terminal MDIO
batch, aggregate deadline and separate restoration contracts. The lead owns
public types, map selection, reset/link contracts, integration and progress.
Two parallel read-only assessments used the session's high-reasoning model
to check independent silicon questions about reset domains and read effects.
[Procedure and field assignments](../../registers/dp83822.json) ·
[API contracts](../../api-contracts.md) · [Verification](../../prompts/VERIFICATION.md)

### Admission and startup

1. Append four exact declarations to `Variant` and key their common copper
   behavior by register map. Exercise each declaration on MII, RMII and
   RGMII. ID registers 02/03 return `2000 A240` for all four (OUI 080028,
   model 24 hex, revision 0). Retain the declared suffix, observed UNKNOWN
   operating mode, declared interface and medium separately. Invalid enums,
   masks, BASIC/ENHANCED requirements, address 32 and incompatible media
   must fail with zero callbacks. Test address 0 and 31. Exercise either
   zero/ones word, wrong OUI/model, revisions 1–15 and a failed second ID
   read. After a mismatch allow a bounded identity retry and fence raw,
   collector, feature and reset I/O until identity is accepted.
2. Decode latched `31/0467` and `31/0468` using each pin's four-level
   table. The default words `0FC3 0000` give address 1, copper MII and all
   four advertised abilities. SOR1 exposes LED_0. LED_1 modes 2/3 are
   test straps with no LED_1 mode field in SOR2. Distinguish what the board
   must attest from what the registers can verify. Compare strap state with
   current CR2[14] and the declared copper medium before a copper feature
   write. A retained or externally set FX bit can disagree with the straps.
   I/H lack FX. IF/HF MII/RMII FX execution belongs to 65, and IF/HF
   FX+RGMII awaits evidence. Return distinct reasons for those cases.
3. Give `NONE` startup a bounded read/verify path and replay only persistent
   owned values. Trace each startup entry, including verification, through
   its last read. Today's `InitStep::APPLY/VERIFY` uses vendor
   `030E=4A40`. Exclude it, the DP83825 functional-mode recipe and all
   unimplemented optional-feature paths when dispatching this map.
   The portable interface declaration can describe strapped copper RGMII.
   Interface programming belongs to 64 and the configured native MAC must
   accept its own wiring.

These cases come from SNLS505H §4, §§7.4–7.5 and tables 7-10..13,
8-3/4/11/25/84/85. The original July 2026 PDF SHA-256 is
`d73d83c018e6dab3f935ea45ca1771bbc34c030b6b24de45fccd1dfa6998a2d3`.
[Original datasheet](../../reference/source/TI-DP83822-SNLS505H.pdf) ·
[Part selection](../../../src/PartTable.h) · [Binding and initialization](../../../src/DP8382x.cpp)

### Reset and management transactions

| Trigger | Required trace and postcondition |
| --- | --- |
| POR or external RESET_N | Both reset all domains and relatch straps. POR permits up to 200 ms from power application to SMI readiness. For RESET_N, enforce at least 10 µs low and allow 2 ms from release to SMI preamble readiness. Respect one idle MDC period, MDIO high for the 32-bit preamble, and the separate XI conditions in §§6.6–6.9. Figure 6-1 requires 100 µs stable external clock to release when absent at power-up. Section 6.7 gives 1 µs from stable XI to subsequent assertion, §6.8 requires 1 µs stable during low. The 1.5 s FLP time is typical. |
| C22 1F = 8000 | Use pin-equivalent hard reset with strap relatch per §7.5.1 and table 8-30. Record the narrower table 8-1 wording. Anchor silence to observed completion or a conservative later bound, re-identify, sample straps, then restore. |
| C22 1F = 4000 | Restart digital logic, retain registers and strap settings, invalidate link freshness. Mapping the “soft reset” in §7.4.4.2 to this word is inferred. |
| C22 00[15] | PCS reset, vendor settings and straps retained. Reread owned direct fields because table 8-1 leaves PCS membership unspecified. |
| DEVAD 3 offset 0000[15] | Reset MMD 3, MMD 7 and PCS, retain vendor registers. Invalidate affected standard observations and counter intervals, verify before replay or publishing trust. |

`Phy::prepareBoard` currently uses a 25 µs pulse floor and 50 ms POR
allowance. Select limits by part and keep the shared bus quiet through
uncertain assertion/release, including another owner's access. Test pulse
9/10 µs, the POR and release boundaries, clock regression/lag and a slow
declared MDC where a full cycle exceeds 2 ms. DP83822's maximum MDC is
25 MHz. External power and pin events remain board-owned. Exercise each
command poll, cancellation after a possible write, primary failure, lock
release and separately budgeted restoration. A reread of retained state
must preserve an outstanding cleanup obligation.
[Timing diagrams and reset tables](../../reference/source/TI-DP83822-SNLS505H.pdf) ·
[Native board preparation](../../../src/esp_idf/Phy.cpp)

Keep C22, vendor DEVAD 31 and IEEE DEVAD 3/7 distinct. For printed 703D
expect `0D=0007, 0E=003D, 0D=4007, read 0E` within one controller hold.
For the vendor alias of 0017 the selector values are `001F,0017,401F`.
Its RCSR[3:2] FIFO status is destructive. Exercise this trace through the
collector and reject a raw bypass through either spelling. Protect direct
and indirect selector access, disallow post-increment, and keep holes,
unlisted bits and recipe-only words out of ordinary writes. Inject failure
at each selector/data frame and at release. Section 7.4.2.1 conflicts with
the later post-increment examples. Address mode 00 and data mode 01 suffice.
[Source decisions](../../registers/dp83822.json)

### Copper configuration and observation

| Procedure | Phase (b) acceptance cases |
| --- | --- |
| Forced and advertised operation | Force 10/half, 10/full, 100/half and 100/full. Cover all legal advertised subsets, symmetric/asymmetric pause intersections, half duplex, no common ability, parallel detect, extended full duplex and manual next pages (ACK, toggle, invalid sequence, deadline). Keep local, partner and resolved facts separate. Table 8-5 permits local ANAR[13] remote-fault advertisement on this map. Existing advertisement composition forces zero. Exercise the part difference in both directions. |
| MDIX and fast AN | CR1[5] robust can be strap-set on 100-Mbit Auto-MDIX. CR1[6] fast applies when both ends force 100. Either can set CR2[6] in hardware. Verify PHYCR[15:14] auto/manual MDI/manual MDIX and each independent CR3[6:5] swap bit. Cover partial writes and the first backend error. Fast AN CR1[4] uses codes [3:2] 00/01/10 with timer triples 80/50/35, 120/75/50, 240/150/100. Code 11 is NA. Require compatible peer settings. Table 8-10 gives no units for deadline arithmetic. |
| 10BASE-Te and fast link drop | Test 001A mask 3E81, NLP, jabber, reduced receiver threshold and squelch codes 0–8 (200–600 mV in 50 mV steps). Reject 9–15. Reading 001A clears polarity evidence also visible at PHYSTS[12]. Share the threshold/squelch implementation with the electrical field owner at 66. Exercise CR3 enables [10,3,2,1,0] against FLDS masks 0100,0080,0040,0020,0010, including combined causes, link recovery and the 100BASE-TX far-end drop control. Exclude the older map's 002A preamble and 0117/0131 threshold writes. |
| Link and pause | PHYSTS[2:0] words 0001,0005,0003,0007 while linked resolve to 100/half, 100/full, 10/half, 10/full. In forced mode check actual PHYSTS even with contradictory programmed BMCR. Keep no-link, unstable link and read failure distinct. Read PHYSTS mirrors before their underlying destructive sources, retain both latched-low and current BMSR reads, and reject incoherent resolution across a flap. Use full-duplex, completed negotiation evidence for resolved pause. Cached reads issue zero callbacks. |
| Interrupts | Exercise all 16 MISR sources, independent masks, the global enable, polarity and sustained test on PHYSCR[3:0]. Use the dedicated INT/PWDN_N route. GPIO mux code 4 routes a TX timestamp indication. Test invalid route with zero callbacks. Table 8-88 RXFS[12] resets 0 (polarity), selects WoL at 1 and can change automatically on WoL enable. Attribute MISR2[9] only when the selector is known for the sample. Test unknown initial state, reset, crossing and uncertain selector writes. Write 0000/1000 for selector updates with all eight low W1S bits zero, including reserved [4:2]. Retain raw RXFS and named status mask 00E3. |
| Counter and IRQ evidence | Test FCSCR saturation at 00FF, RECR at FFFF, and DEVAD 3/0016 read-clear wake errors. Preserve actual MISR threshold events. The RECR half-full description says 7Fh despite its 16-bit width. Do not calculate a threshold from it. MISR high bytes are read-clear. The low enable bytes are RC/W and their retention on read is unspecified. Keep desired mask, sampled mask and confidence in the installed mask separate after *every* read, including verification. Simulate both retained and cleared enables. A failed second MISR read or release preserves the first consumed sample, uncertainty and incomplete service status. TI clarification or independent events on an identified fixture must establish retention and rearm behavior. |

The TI RTOS source at `94bf4343004732983b0b8e1eec123d88ff70453b`
corroborates the ID and PHYSTS decoding. Its MDIX helper discards RMW
statuses and its diagnostic dump reads destructive registers. Reuse the
facts through the existing collector and terminal-result contract.
[Pinned TI source and header comparison](../../reference/library-family.md#ti-rtos-dp83822-concrete-behavior) ·
[SNLS505H §§7.4.5–7.4.11 and tables 8-1..26, 8-81/82/88/140](../../reference/source/TI-DP83822-SNLS505H.pdf)

### Implementation and verification ownership

The lead should integrate `Config.h`, `DP8382x.h`, `PartTable.h`,
map-specific raw-access policy, startup/replay in `DP8382x.cpp` and
native board preparation. Freeze the new capability and evidence semantics
before splitting work. One implementation unit can own
`DP8382xConfiguration.cpp`, `DP8382xQuality.cpp` and their configuration
vectors. The other can own `DP8382xLink.cpp`, `DP8382xInterrupts.cpp`,
`DP8382xEvidence.h` and their collector vectors. Route changes needed in
the shared transaction/retention code through the lead. Audit every public
entry that branches on “other than DP83825I” before enabling any DP83822
row, including raw aliases, CLI features and replay of owned state.

Extend the existing CLI, compiled fixture and Python protocol/campaign
checks. The wire protocol currently lists five declarations and handles
UNKNOWN mode specially for DP83825I. Record each DP83822 suffix, medium,
interface, address and firmware identity, and exercise incompatible and
unknown records plus replay. Register Release-active literal traces by
suffix/interface in root CMake. Include failed preflight, exact callback
count and order, deadline equality, partial effects, cancellation, lock
release, output validity and zero-I/O snapshots. Keep the application's
100/full default independent of the driver's capabilities.
[CLI](../../../examples/common/Dp8382xCli.cpp) ·
[Protocol](../../../tools/dp8382x_hil_protocol.py) ·
[Compiled fixture](../../../test/cli/dp8382x_cli_fixture.cpp)

With the runtime change, update public Doxygen, identity/variant, API,
feature coverage, CLI and fixture guidance and review README/CHANGELOG.
Map all ten procedures to their actual tests and command routes, including
open evidence. Run the original-family regressions and exact-part suites in
Release and focused UBSan, compiled CLI/replay, the tests-OFF consumer and
package checks for changed headers. Link the real P4 adapter fixture and
both affected applications with `scripts\idf.cmd` at v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643` in short build paths under
`C:\esp\build\dp8382x-loop`. Track MAC interface and clock timing at 64, optical
behavior at 65 (Q14 per F grade), diagnostics at 66 and optional controls
at 67. The full copper software disposition must retain any essential
unresolved silicon behavior. Each grade requires its own physical receipt.

### Sibling audit

All three sibling worktrees and indexes were clean on main tracking
origin/main. The audit included each sibling's AGENTS and CI definition.

| Sibling and revision | Sources reopened and decision |
| --- | --- |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `include/MB85RC/CommandTable.h`, `src/MB85RC.cpp`, `test/test_basic.cpp`, `examples/common/DiagnosticCore.h`, `tools/hil_runner.py`. Adopt explicit capability rows and mismatch cases. Product-ID AUTO selection cannot distinguish A240 suffixes. |
| TMP1x2 `0791429f8fce1170e2082f1b011456fc461318b8` | `include/TMP1x2/Config.h`, `src/TMP1x2Configuration.cpp`, `test/variants/test_variants.cpp`, root CMake registration, `examples/common/Tmp1x2CliDiagnostics.cpp`, `tools/hil_tmp1x2_runner.py`. Bind rejects a declared ALERT pin on ADD0 packages without I/O while retaining threshold controls. Apply this distinction to IRQ wiring, internal events and the board's declared medium. |
| TUSB32x `6989c77fad3bc4651403942a4c9f7ca3940f859b` | `include/TUSB32x/Config.h`, `src/TUSB32x.cpp`, `src/TUSB32xProtocol.cpp`, `test/test_validators.cpp`, `test/CMakeLists.txt`, `examples/common/Tusb32xCli.cpp`, `tools/tusb32x_hil_runner.py`. Use separate declared-profile and observed values, strict local validation and independently registered observations. Keep MDIO access and ESP32-P4 verification specific to this driver. |

### Phase (a) check receipts

On Windows 11 AMD64, GCC 15.1.0, CMake 4.0.1, Ninja 1.12.1, Python
3.12.10 and PyMuPDF 1.27.2.2, the following commands reproduce the host
baseline from the repository root.

```powershell
cmake -S . -B build/p63-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON -DDP8382X_BUILD_PINNED_IDF_TESTS=OFF
cmake --build build/p63-baseline --parallel 4 --target dp8382x_identity_tests dp8382x_reset_tests dp8382x_link_tests dp8382x_negotiation_tests dp8382x_interrupt_tests dp8382x_forced_tests dp8382x_events_tests dp8382x_interface_tests
ctest --test-dir build/p63-baseline -R '^((e_|ai_|ae_|p25_)?(identity|reset|link|negotiation|interrupt|forced|events)(_.*)?)$' --output-on-failure --no-tests=error -j 3 --output-log build/p63-baseline/regressions.log
```

| Evidence | Phase (a) result |
| --- | --- |
| Host C++ baseline | PASS 197/197 existing-family registrations. The first selection ran 196 and missed the executable that owns `p25_events`. Building `dp8382x_interface_tests` and repeating the identical selection passed. DP83822 runtime tests await phase (b). |
| Offline research and Python | PASS. `python -B tools/verify_ledger.py` passed all family inventories. `python -B -m unittest discover -s test -p 'test_p22_ledger.py' -q` passed 16/16, including comparison to all 576 printed field rows. Full discovery with `test_*.py` ran 296 tests, 237 passed and 59 fixture-dependent tests skipped. |
| Documentation and source integrity | PASS. `python -B tools/verify_references.py` checked 29 vendor payloads, 27 extracts and 2,505 local links. `python -B tools/check_doxygen.py` checked 296 pages and rejected both negative fixtures. `python -B scripts/generate_version.py check`, `git diff --check` and whitespace checks on the new report passed. |
| Portable and exported consumers | NOT RUN for this assessment. |
| SDK links | NOT RUN for this assessment. |
| Remote CI | NOT RUN. Assigned to prompt 71. |
| Physical | NOT RUN on any grade. The outstanding observations include reset/strap retention, successive MISR events and the RECR threshold. |

## Phase (b) offline strap decoding

The first implementation slice covers the decoding part of `straps` for
all four DP83822 declarations. `decodeDp83822Straps` converts two current
latches to pin functions without any bus access. Each two-bit field gives
a mode number. Each pin has its own mapping from that number to functions.
The decoder preserves the independent observations when LED_0 selects a
test mode or XI_50 selects a reserved interface. A partial or stale pair
cannot replace a prior result. LED_1 requires board evidence, and live
copper startup still requires an independent CR2[14] read. Tables 7-10
through 7-13 and 8-84/85 on PDF pages 49–51 and 90–91 supply the vectors.
[Original tables](../../reference/source/TI-DP83822-SNLS505H.pdf)

The wrapper verified a clean assessment checkpoint at
`571751e833abd139c699a81df0493f58ce0280d0`. The lead owned the API,
decoder and integration. One agent independently checked the PDF and
reopened the three siblings' files listed above. Their revisions and clean
status matched phase (a). TUSB32x's distinction between declared and
observed values applies to this decoder. A second agent wrote the literal
test vectors in `test/identity.cpp`. Both used the session model to check
the per-pin encodings against the original tables.

| Evidence | Slice result |
| --- | --- |
| Host | PASS, Windows x86-64, GNU 15.1.0 Release. Built `dp8382x_identity_tests` and `dp8382x_header_checks` in the phase (a) Ninja tree. The selection below passed 21/21, including all eight interface input combinations, the literal defaults and all seven stale/incomplete flag combinations. |
| Portable consumer | PASS, tests-OFF Release build of `test/consumer`. Both existing consumer CTests passed. |
| Exported package, SDK link, remote CI, physical | NOT RUN for this slice. CI belongs to prompt 71. Physical evidence remains pending per suffix. |

```powershell
cmake --build build/p63-baseline --parallel 4 --target dp8382x_header_checks dp8382x_identity_tests
ctest --test-dir build/p63-baseline -R '^(p22_straps|(e_|ai_|ae_|p25_)?identity(_.*)?|dp8382x_identity_tests_failure_path)$' --output-on-failure --no-tests=error -j 3
cmake -S test/consumer -B build/p63-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/p63-consumer --parallel 4
ctest --test-dir build/p63-consumer --output-on-failure --no-tests=error
```

Continue phase (b) with exact-part identity and rejection fences. Keep startup
and maintenance guarded while implementing live strap capture, CR2 medium
verification and the remaining map-specific paths.

## Phase (b) portable identity

The portable core admits a two-read Clause 22 identity probe for each
board-declared copper DP83822I, H, IF and HF on MII, RMII and RGMII. All
twelve combinations accept `2000 A240` with UNKNOWN operating mode.
Capabilities describe silicon by declared grade. Only IF and HF report
fiber. For fiber bindings the core distinguishes absent hardware on I/H,
a pending procedure on IF/HF, and the open IF/HF RGMII source question.
The declared MDC maximum is 25 MHz. [SNLS505H section 4, section 6.9
and tables 8-3/4](../../reference/source/TI-DP83822-SNLS505H.pdf)

The test matrix covers addresses 0 and 31, both single-word zero and
ones cases, OUI and model mismatches, all 15 unaccepted revisions, a
partial second read, and a successful retry. Each row checks startup,
raw access, link settings, wake observation and collector rejection with
zero clock and MDIO callbacks. The native factory and shared CLI reject
these declarations until board preparation and runtime operations exist.

The wrapper verified the previous strap checkpoint at
`c27806fc474a66d1612a2ac36aa489c0568f9534` on main, equal to origin/main.
The lead owned the contracts, admission, integrations and report. One agent
wrote literal identity tests and a read-only agent audited portable and
native access, the original PDF and the sibling sources listed in phase (a).
Both used the session model for silicon and admission reasoning. All three
sibling revisions and clean states matched phase (a).

| Evidence | Slice result |
| --- | --- |
| Host | PASS on Windows x86-64, GNU 15.1.0, Release/Ninja. 44/44 identity, strap, foundation and CLI tests. The two native factory regressions passed with the pinned SDK host harness. |
| Portable consumer | PASS, 2/2 from the tests-OFF Release consumer. Exported package consumer NOT RUN this slice. |
| SDK link | PASS, 3/3 existing ESP32-P4 images, ESP-IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`. Adapter fixture (DP83825I), basic and owner applications (DP83826I) linked against the changed inputs. The first attempts encountered SDK checkout ownership and clangd activation errors. Reusing the prompt 53 process-local Git, cached clangd and ccache settings resolved both. Logs are in `build/p63-baseline/final-idf-{56l,56b,56o}.log`. |
| Research and docs | PASS, ledger verifier, 16/16 DP83822 ledger tests, Doxygen (298 pages and two rejected negative fixtures), generated version, local reference and whitespace checks. |
| Remote CI and physical | NOT RUN. CI belongs to prompt 71. Each suffix still requires independent physical evidence. |

```powershell
cmake --build build/p63-baseline --parallel 4 --target dp8382x_identity_tests dp8382x_foundation_tests dp8382x_header_checks dp8382x_cli_tests
ctest --test-dir build/p63-baseline -R '^(p22_.*|(e_|ai_|ae_|p25_)?identity(_.*)?|foundation_.*|cli_processor_.*|dp8382x_identity_tests_failure_path)$' --output-on-failure --no-tests=error -j 3
cmake --build build/p60-tests --target dp8382x_native_host_tests --parallel 4
ctest --test-dir build/p60-tests -R '^(e_)?native_factory$' --output-on-failure --no-tests=error
cmake --build build/p63-consumer --parallel 3
ctest --test-dir build/p63-consumer --output-on-failure --no-tests=error
```

The SDK commands and scoped environment are in the [prior SDK link
receipt](61-p25-feature-closure.md#phase-d-esp32-p4-sdk-links). Continue phase
(b) with bounded live SOR1/SOR2 reads and a separate CR2[14] observation.
Test partial capture, stale evidence, strap/active-medium disagreement and
board prerequisites before admitting copper startup or any feature write.

## Phase (b) live strap capture

The wrapper verified `508f5d43b15d0ace17a3aaabf2d3720956b5afee` equal to
`origin/main`. The tree and index were clean. The lead owned the API and
implementation. One agent independently checked the original PDF and the
three sibling revisions and paths listed in phase (a), all still clean.
Another wrote the literal vectors in `test/identity.cpp`. Both used the
session model for the MDIO semantics and partial-transfer cases.

After identity acceptance, `captureDp83822Straps` reads vendor MMD 31
offsets 0467h and 0468h under one hold. Each read selects the address and
nonincrementing data mode. Only an in-budget success with successful
release publishes a current pair. A failed callback replaces the previous
capture with the returned words from its own completed prefix. A local
denial preserves the previous snapshot. SOR1 can report strapped FX on a
copper binding. Startup still needs a CR2[14] observation and board
evidence for LED_1. [SNLS505H §7.4.2.5 and tables 8-11, 8-84/85](../../reference/source/TI-DP83822-SNLS505H.pdf)

| Evidence | Result for this slice |
| --- | --- |
| Host | PASS, Windows x86-64, GCC 15.1.0 Release/Ninja. All 62 selected CTests passed, including twelve grade/interface rows checking the eight selector/data frame cuts, frame and deadline boundaries, stale evidence, external reset, failed release and retry. |
| Portable and source-package consumers | PASS, 2/2 in `build/p63-consumer` and 1/1 in `build/p63-export/out`. The tests-OFF DP83825I package consumer used 29 byte-checked working-tree inputs at the paths in the phase 61 export. |
| SDK links | PASS, all three existing ESP32-P4 images linked with ESP-IDF v6.1 `fff9895c` using the phase 61 commands and scoped environment above. Logs are `build/p63-baseline/capture-56{l,b,o}.log`. |
| Research and documentation | PASS, ledger verifier, 16/16 exact-part ledger tests, Doxygen (298 pages and two rejected negative fixtures) and generated version check. |
| Remote CI and physical | NOT RUN. Prompt 71 owns CI. Each suffix still needs physical qualification. |

```powershell
cmake --build build/p63-baseline --parallel 4 --target dp8382x_header_checks dp8382x_identity_tests dp8382x_reset_tests dp8382x_cli_tests
ctest --test-dir build/p63-baseline -R '^(p22_.*|(e_|ai_|ae_|p25_)?(identity|reset)(_.*)?|cli_processor_.*|dp8382x_identity_tests_failure_path)$' --output-on-failure --no-tests=error -j 3 --output-log build/p63-baseline/capture-test.log
```

Continue phase (b) with the separate current-medium observation and
strap/board checks before permitting copper writes. The native adapter,
CLI and subsequent copper operations retain their gates.

## Phase (b) copper implementation

Entry was clean on `main` at `a9cdb23c7d53db998a4f9054d059323cde6acdfd`, equal to the locally tracked upstream after wrapper synchronization. The [sibling audit](#sibling-audit) still applies at the listed clean revisions. Two implementation agents used the session model for separate register-level tasks, one for configuration and quality, the other for link, interrupt and counter evidence. The lead integrated the shared contracts, startup, native adapter, CLI, runner, tests and documentation. Independent review is pending.

| Procedure | Implemented behavior and executing evidence |
| --- | --- |
| Identity, straps, management | The revision-0 A240 probe preserves the declared suffix. A single hold captures the two strap words. A separate CR2 read records the live medium. Startup requires current matching interface, address and copper evidence, plus board LED_1 mode 1 or 4. A live fiber mismatch retains the CR2 word and mask 4000h. Tests cover I/H/IF/HF on MII/RMII/RGMII, stale and partial observations, namespaces and interrupted indirect transfers. See `p22_identity_*`, `p22_straps`, `p22_startup` and `p22_collector`. |
| Reset | NONE checks all three reset registers. The two PHYRCR commands distinguish pin-equivalent hard reset from retained-register restart. Hard reset rechecks identity and samples relatched straps. PCS and MMD3 resets preserve vendor registers and recheck copper state. Native board preparation uses the 200 ms POR allowance, 10-microsecond pulse floor, 2 ms release allowance and declared idle MDC time. Tests exercise the five startup choices, raw command traces, failure prefixes and board admission. The prose soft-reset interpretation remains inferred. See `p22_startup` and `native_factory`. |
| Copper link, negotiation and MDIX | Forced and advertised profiles use the DP83822 masks, including ANAR remote fault, independent CR3 swap bits, CR1 robust/fast MDIX, fast AN and effective CR2 parallel detection. The owner supplies peer compatibility for fast profiles. Link tests use independent PHYSTS words 0001/0005/0003/0007 and check consistency across destructive reads. Typed tests cover forced speeds and duplex, pause, next pages, flaps and read failures. See `p22_copper_*` and `p22_link`. |
| Link loss | DP83822 TP10CR and fast-drop fields use their own masks and read effects. Tests exercise all legal squelch codes, reject reserved codes, preserve interrupted writes and check the cause mapping. The 26-family preamble and threshold recipes remain outside these paths. See `p22_copper_*`. |
| IRQ and counters | One collector retains consumed prefixes, including MISR1/2, polarity, FIFO and PRBS sync loss. It also records the passive LPI words. RXFS selector writes keep W1S fields zero. Installation verifies and rewrites the interrupt masks before enabling delivery. Later consuming reads withdraw confidence in installed masks. Tests include retained and cleared mask models, failure and release boundaries, 8-bit FCS and 16-bit receive-error saturation, vendor FIFO and MMD7 partner traces. The RECR half-full threshold and hardware MISR enable/rearm behavior remain open. See `p22_collector`. |

The native factory, shared CLI and offline runner accept declared copper operation. Twelve compiled fixture sessions check declaration and replay. Four campaign receipts cover observation, hard reset, forced 10/half and advertised 100/full. Host fixture transcripts use simulated firmware identity. Interface timing, fiber, diagnostic stimulus and optional features keep their separate procedure gates. Qualification requires an identified physical fixture per suffix. [Copper source dispositions](../../registers/dp83822.json) and [original tables](../../reference/source/TI-DP83822-SNLS505H.pdf).

| Local evidence | Phase (b) result |
| --- | --- |
| Windows x86-64, GCC 15.1.0, Ninja, Release | PASS 265/265 focused core, original-family, CLI, runner and campaign CTests. Release UBSan with trap-on-error passed all 28 DP83822 CTests. |
| Python, sources and documentation | Python 3.12 discovery ran 298 tests, 237 passed and 61 skipped. The compiled fixture suites also ran under CTest with their executable supplied. Ledger and generated version passed. Doxygen checked 300 generated pages and rejected both negative fixtures. |
| Portable and packaged consumers | PASS 2/2 separate tests-OFF consumers and 1/1 source-package consumer using 29 refreshed and byte-compared input files. |
| ESP-IDF | PASS 2/2 native factory host tests built against pinned SDK sources. PASS 3/3 ESP32-P4 image links using v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`. These are the existing adapter-link, basic and owner images. |
| Remote CI and physical | NOT RUN. Prompt 71 owns remote CI. All sessions here used simulated transports. |

Reproduce from the repository root with the existing build configurations below. For the native host adapter and SDK links use the scoped environment and project arguments in the [SDK link receipt](61-p25-feature-closure.md#phase-d-esp32-p4-sdk-links). The SDK build paths end in `56l`, `56b` and `56o`. Logs under `build/p63-baseline` include `p63b-56*.log`, `p63b-native-final.log`, `final-copper.log` and `python-final.log`.

```powershell
cmake --build build/p63-baseline --parallel 6
ctest --test-dir build/p63-baseline -R '^(p22_.*|(e_|ai_|ae_|p25_)?(identity|reset|link|interrupt|forced|quality|negotiation|events|cli_processor)(_.*)?|cli_hil_(campaign|runner)|cli_(fixture|control|diagnostic|pin|selftest)_transcript)$' -j 3 --output-on-failure --no-tests=error
cmake --build build/p63b-ubsan --target dp8382x_identity_tests dp8382x_reset_tests dp8382x_quality_tests dp8382x_link_tests dp8382x_interrupt_tests --parallel 4
ctest --test-dir build/p63b-ubsan -R '^p22_' --output-on-failure --no-tests=error
cmake --build build/p60-tests --target dp8382x_native_host_tests --parallel 4
ctest --test-dir build/p60-tests -R '^(e_)?native_factory$' --output-on-failure --no-tests=error
cmake --build build/p63-consumer
ctest --test-dir build/p63-consumer --output-on-failure --no-tests=error
cmake --build build/p63-export/out
ctest --test-dir build/p63-export/out --output-on-failure --no-tests=error
python -B -m unittest discover -s test -p "test_*.py" -v
python -B tools/check_doxygen.py
python -B tools/verify_references.py
python -B tools/verify_ledger.py
python -B scripts/generate_version.py check
git diff --check
```

## Phase (c) independent review

On 2026-11-28 the worktree and index were clean on `main` at
`4a2ddf17ff5164cb8bbfcf25bca966ec97b07305`. The wrapper verified equality
with `origin/main` and the local comparison returned 0 ahead, 0 behind.
Two parallel read-only reviewers used the session's high-reasoning model for
reset and configuration ownership, and for link and destructive register
semantics. Both finished. The lead reviewed integration, the native board
timing, and the compiled CLI and Python replay paths. The review covered
the changes since `571751e`, including the earlier strap and identity commits.
The [sibling audit](#sibling-audit) paths and three clean revisions still
match. Their explicit capability rows, declaration checks and independent
test registration apply here. A240 still requires a board-declared suffix.

| Finding | Correction and evidence |
| --- | --- |
| A raw CR2 read between cooperative polls could reveal fiber after admission. HARD could still issue its command and NONE could publish readiness. A raw strap read could also contradict the saved pair. | The core checks current board proof at reset submission and before completing startup. A changed SOR1 or SOR2 retains the observed word and invalidates the pair. A changed ID word withdraws identity until a new two-word probe. `p22_startup` interrupts HARD and NONE with live FX, either changed latch or revision 1. It checks effects, readiness and the returned mismatch. SNLS505H tables 8-3/4, 8-11, 8-30 and 8-84/85. |
| A later PHYSTS or BMSR read could contradict a current cached link. | The collector marks conflicting link, mode or negotiation evidence stale. `p22_link` exercises up/down transitions in both directions, including a down sample followed by failed release, and counts callbacks around cached reads. Tables 8-2 and 8-16. |
| Changing a link profile after installing an IRQ policy could always fail. Its verification tail consumed the two MISR enable bytes whose post-read retention is unknown. A separate raw PHYSCR read could also contradict the installed control while trust stayed VERIFIED. | A mutating link profile reuses the budgeted IRQ installation procedure for its owned masks. Read-only verification still reports the retention gate. A direct or vendor-alias control read retains the raw word and withdraws trust on contradiction. `p22_collector` exercises both possible mask-retention models after IRQ service, verifies the actual mask bytes and checks both PHYSCR read paths. Tables 8-17 through 8-19. |
| Offline replay could accept a successful reset with incompatible strap words, or a saved profile with flag combinations that the CLI cannot restore. | The runner and campaign share an independent board check for address, functional LED_0, copper and interface. Tests alter actual compiled receipts, including FX at the correct address and a reserved interface encoding. Saved-profile tests cover timer codes, fast MDIX and peer confirmation. Tables 7-10 through 7-13, 8-10 and 8-84/85. |

The sources are the [original SNLS505H PDF](../../reference/source/TI-DP83822-SNLS505H.pdf)
with the phase (a) hash, the [exact-part procedure ledger](../../registers/dp83822.json)
and the [pinned TI implementation comparison](../../reference/library-family.md#ti-rtos-dp83822-concrete-behavior).
The public counter contract now includes the full-word sum for MMD 3/0016
wake errors from table 8-140.

| Local layer | Stage (c) receipt |
| --- | --- |
| Windows x86-64 host, GCC 15.1.0, Ninja Release | PASS 265/265 focused CTests using the phase (b) selection. Release UBSan trap passed 28/28 DP83822 registrations. The pinned native factory host tests passed 2/2. |
| Offline runner | PASS. After moving the strap check into the common protocol module, the first runner import exposed an extra closing parenthesis. It was corrected. Both compiled runner/campaign CTests passed on the final files. Python 3.12 discovery ran 299 tests with 61 skips for fixture-dependent cases. |
| Portable and package consumers | PASS 2/2 tests-OFF build-tree consumers and 1/1 source-package consumer. All 29 files in the existing package were compared with the worktree after refreshing changed inputs. |
| ESP32-P4 SDK links | PASS 3/3 with ESP-IDF v6.1 `fff9895c`. The existing adapter-link, basic and owner images were 421376, 732432 and 610656 bytes respectively. DP83822 factory execution used simulated hardware on the host. |
| Documentation and sources | Doxygen passed 300 pages and two rejected negative fixtures. Reference integrity, the register ledger, generated version and whitespace checks passed. |
| Remote CI and physical qualification | NOT RUN. CI belongs to prompt 71. Each grade still needs identified fixture evidence, including real mask retention/rearm and the RECR threshold. |

Reproduce with the phase (b) commands above. Rerun the compiled replay checks
with `ctest --test-dir build/p63-baseline -R '^cli_hil_(runner|campaign)$' -j 2 --output-on-failure --no-tests=error`.
The host, UBSan, Python, replay and SDK logs are under `build/p63-baseline`
with prefix `p63c-`. Prompt 63 remains in progress for phase (d).

## Phase (d) final verification

On 2026-11-28 the wrapper verified that `main` and `origin/main` both pointed
to `836223268034b7612b33ddc4f5f929d7078beb61`, the reviewed implementation.
The worktree and index were clean at entry. Two agents used the session model
for independent work. One checked the ten procedures against the original tables,
tests and the three clean [sibling revisions](#sibling-audit). The other built
the pinned native host adapter and three SDK applications. The lead ran the
portable checks, closed an address-test gap and integrated both receipts.

| Ledger procedures | Copper acceptance and executing evidence |
| --- | --- |
| `identity`, `straps` | All twelve declared grade/interface combinations accept revision 0 and preserve the board's suffix. The identity rows check rejection and retry. The strap test already checked the five address bits individually and the unstrapped default of 1. It now decodes all 32 copper addresses from independent SOR1/SOR2 words based on tables 7-10 and 8-84/85. |
| `management`, `reset` | `p22_collector` checks C22, vendor and IEEE selector/data traces, destructive access and partial reads. `p22_startup` checks all twelve combinations across NONE, HARD, PCS, RESTART and EEE_PCS, including the live medium check before writes. The native factory tests exercise POR, RESET_N and idle MDC timing with simulated clocks for each grade. |
| `copper-link`, `negotiation`, `mdix`, `link-loss` | The twelve `p22_copper_*` rows exercise all 15 nonempty advertised ability masks, four forced speed/duplex settings, crossover, next pages, 10BASE-Te squelch and 32 combinations of fast-drop causes. `p22_link` checks PHYSTS 0001/0005/0003/0007, pause, flaps, deadlines and the distinction between link down and MDIO failure. The CLI uses `configuration apply`, `controls quality` and `link read`. |
| `irq`, `counters` | `p22_collector` exercises all 16 interrupt sources under both possible enable-retention behaviors, RXFS selector writes, saturated counts and partial acquisitions. `irq configure`, `irq service` and `link collect` use the same owner. A consuming read preserves desired masks and withdraws confidence in installation. The collector reports the actual receive-error threshold event. |

The archived SNLS505H PDF still matches the phase (a) SHA-256.
The ledger's two open observations require TI clarification or independent
measurements on identified silicon. Successive interrupts must establish
MISR enable retention and rearm behavior. An observed threshold crossing must
establish the numeric RECR threshold. The section 7.4.4.2 soft-reset name
remains an inference. Reset selection uses the register and command word.
[Source decisions](../../registers/dp83822.json) · [Original tables](../../reference/source/TI-DP83822-SNLS505H.pdf)

| Layer | Local result on Windows 11 x86-64 |
| --- | --- |
| Host behavior and simulated CLI | PASS. GCC 15.1.0, Release, CMake 4.0.1, Ninja 1.12.1. All 636 registered CTests passed, including original-family regressions, compiled runner, campaign and interruption tests. After the address-test addition, all 48 identity and DP83822 registrations passed again. Release UBSan trap passed all 28 DP83822 registrations on the final test source. |
| Offline Python and research | PASS. Python 3.12.10 discovery ran 299 tests, with 238 passes and 61 fixture-dependent skips. The exact-part ledger suite passed 16/16, including its comparison to all 576 printed field rows. The family ledger verifier passed. |
| Portable and source-package consumers | PASS. Both tests-OFF build-tree consumers passed. All 29 inputs in the source package matched the worktree by SHA-256. A fresh tests-OFF Ninja build of its existing DP83825I application passed 1/1. |
| Pinned native host | PASS. The first native build encountered Git checkout ownership on the SDK pin check. The documented process-local settings allowed verification of all 45 SDK files. Both factory tests and ten board regressions passed (12/12). The factory exercises the four declared DP83822 grades on simulated copper RMII. |
| ESP32-P4 SDK links | PASS. ESP-IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643` relinked three images with clean firmware identity `8362232`. The existing DP83825I adapter-link image was 421376 bytes. The DP83826I basic and owner images were 732432 and 610656 bytes. |
| Documentation and integrity | PASS. Doxygen checked 300 generated pages and rejected both negative fixtures. The generated version, reference payloads, Markdown links and final diff whitespace passed. |
| Remote CI | NOT RUN. Prompt 71 owns the remote checkpoint. |
| Physical qualification | NOT RUN on I, H, IF or HF. Each suffix requires its own identified board, strap, link and event evidence. |

Use the phase (b) build and Python commands above and the [SDK commands and scoped environment](61-p25-feature-closure.md#phase-d-esp32-p4-sdk-links).
For the final package check, configure `build/p63-export/application` into
`build/p63-export/p63d-out` with Release, Ninja and `DP8382X_BUILD_TESTS=OFF`,
then build and run CTest there. These commands reproduce the host selections.

```powershell
ctest --test-dir build/p63-baseline -j 4 --output-on-failure --no-tests=error
ctest --test-dir build/p63-baseline -R '(identity|^p22_)' -j 3 --output-on-failure --no-tests=error
ctest --test-dir build/p63b-ubsan -R '^p22_' --output-on-failure --no-tests=error
ctest --test-dir build/p60-tests -R '^((e_)?native_factory|native_board_[a-z_]+)$' --output-on-failure --no-tests=error
```

The local logs are in `build/p63-baseline/p63d-*`. README, CHANGELOG, the
API, variant, CLI and fixture guides describe the reviewed copper behavior.
The coverage guide now records the full address test. Prompt 64 owns MAC
interface and clock programming, 65 owns fiber and optical Q14 for IF and HF,
66 owns diagnostics and electrical features, and 67 owns optional controls.
The wrapper records the completion commit and verifies its upstream SHA after this run.
