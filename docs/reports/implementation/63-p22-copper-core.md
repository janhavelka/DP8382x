# Prompt 63 · DP83822 copper identity, reset and link

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
