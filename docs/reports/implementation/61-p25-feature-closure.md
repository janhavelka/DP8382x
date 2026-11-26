# Prompt 61 · DP83825I feature and integration coverage

## Phase (a) assessment

**IN PROGRESS.** On 2026-11-26 the wrapper verified prompt 60 at
`30500a369abe1d794c2207d925ba37f622b7017c` on `main`. The index and
worktree were clean, with zero ahead and behind `origin/main`. The last
production and test change was `4412a2db233defcd4542c2d63d58dc3663fee00b`.
Prompt 60 supplies the 18 core procedures, exact revision-0 identity
`2000:A140`, declared RMII roles, clock-loss admission, guarded MDIO and
the compiled fixture. Its adapter link used ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`.
[Prompt 60 verification](60-p25-source-core.md#phase-d-final-verification)

The 15 assigned IDs are `functional_mode`, `eee`, `legacy_lpi`, `power`,
`irq`, `counters`, `quality`, `loopback`, `bist`, `tdr`, `led_pin`,
`wake`, `vod`, `analog` and `compliance`. Of 570 printed field rows in
101 register tables, 252 name these owners, including 92 for analog.
201 rows are explicitly reserved and eight more are reserved by description.
`quality` has zero printed field rows. Its input is a separately documented
read. At closure, join every field to its disposition and check **all 33
procedures and all ten address exceptions** against the one coverage matrix.
Count aliases by address space and route. Check exceptional writes within
listed registers as well. [Exact ledger](../../registers/dp83825i.json) ·
[Feature matrix](../../feature-coverage.md#dp83825i)

### Phase (b) work and acceptance

Hex values below are register words or addresses. Vendor offsets use DEVAD
31, IEEE PCS uses 3 and auto-negotiation uses 7. The lead owns the C++17
public declarations, part profile, operation and restoration ordering,
adapter integration, coverage and commit request. Extend the existing
feature owners and CLI. Two read-only assessors used the session's
high-reasoning model in parallel to check diagnostic recipes and power/pin
interactions against the PDFs. The lead checked register ownership, code,
runner admission, sibling conventions and test registration.

| Procedure | Exact-part decision and independent acceptance |
| --- | --- |
| `irq`, `counters` | Extend the existing collector in `src/DP8382x.cpp` and `DP8382xInterrupts.cpp`. SNLS638C tables 7-15 and 7-18 through 7-26 protect consuming C22 000C, FLDS 000F, PHYSTS, MISR1/2 and counter/FIFO reads. Protect the vendor aliases and incidental RMW reads too. Service IRQs through MISR1/2 only. Their upper-byte events correspond to lower-byte enables despite the R marking. The relationship to 000C is unknown. Capture PHYSTS before its clearers, retain each consumed word after a failed second read, target write or controller release. RCSR bits 3:2 are RC and bit 8 is follower-only. Collect RXFS and PCS status/wake faults through that same owner. Table 7-24 labels RECR a 16-bit sample and describes FF saturation and a 7F threshold. Keep its raw width and record that discrepancy. Exercise all interrupt masks, source selection, polarity, stimulus and bounded stop. |
| `functional_mode` | One finite typed procedure owns default, power optimized, extended reach and extended reach with EEE. Tables 6-5 through 6-7 prescribe 13, 13 and 32 writes including final direct 001F=4000. Default needs zero mode writes from a known reset baseline. It cannot undo another mode by doing nothing. The two shorter scripts differ at 0404=0000/0080. Compare every write to independently transcribed literals. Allow 031B, 033A and 033D only at the specified recipe positions. Account for simultaneous ownership of 030B/C (VOD) and 0308 (clock controls) before mutation. For each prefix, including uncertain final RESTART, retain completed/possible effects and dirty trust. Poll restart and verify or separately recover before declaring the new state. |
| `eee`, `legacy_lpi` | Share the extended-reach-with-EEE executor rather than replaying the existing I/E or A recipe. Sections 6.3.3.2 and 6.3.4 and table 6-7 require 04D1=00C2 with bits 0 and 3 clear despite table 7-92's access/polarity conflict. Set 7:003C[1], restart AN and observe partner ability and PCS. Test the six IEEE tuples individually, including 3:0000[15] PCS reset and [10] RX clock-stoppable. Retain PCS/MMD3/7 reset effects independently from vendor state and BMCR. Distinguish advertised LPI from bilateral force code 3, reject codes 1 and 2. Cover failure between advertisement and restart, the request/stop at bit 12, and the collector's saturating read-clear waking fault count. SNLA328 §§2.2.1–2 require at least 36 µs of MAC Normal Inter-Frame after exit before data. Verify the stop and require application evidence for MAC readiness. |
| `power` | Sections 6.3.6–8 and tables 7-3, 7-20 and 7-75 distinguish normal, active sleep (0011[14:12]=6), IEEE down (BMCR[11] or board input), PLL disable while in IEEE down and deep power (IEEE first, then 0428[2]). Prove management, recovery and clock-loss authority before a disruptive transition. Distinguish attempted deep entry, observed down and verified recovery. Exercise both declared clock roles and simultaneous compliance ownership of 0428. |
| `led_pin`, `wake` | Implement LED0 and LED2 from tables 7-27/28/34/60–62/78/81–87. Test LED source codes 0–14 and reject 15. Pin 2 mux 0305[2:0] has a role-dependent default, LED2=1, wake=2, MDINT=4, high=6 and low=3/5/7. Reuse the prompt 60 clock owner for every route which moves that pin. LED2 source is 0460[7:4] and its manual drive/polarity is in 0469[6:4]. Preserve undefined PHYCR[5]. Check data before enabling drive, conflicts and each interrupted restoration. For wake, verify disable before rekeying. MAC octets 11:22:33:44:55:66 pack to 2211/4433/6655 at 04A2–4. Password 3C:41:9D:44:BB:5E packs to 413C/449D/5EBB at 04A5–7. SNLA261A table 2-1 and §2.2.2 supply those three otherwise unlisted addresses for Secure-ON. Restrict them to this procedure. Exercise CRC gate, pulse widths, level and direct/IRQ indication, source attribution, RXFS/MISR2 harvest before write-zero acknowledge, and neutral bit 11 in other writes. Keep DP83822 pattern filters outside exact-I admission. |
| `quality` | SDAA081 §2.3.3 table 2-6 gives one vendor 0218 read on a powered PHY with connected partner. Feed it 0209/020A/020B/033A/033B/033C and expect excellent/unclassified/good/good/unclassified/poor. Preserve the raw value. Separate unavailable link, failed read and successful measurement. The existing `readSignalQuality` only returns raw MSE plus context on earlier parts. |
| `loopback`, `bist` | Sections 6.3.14–15 and tables 7-3/25/30/31 govern admission. Disable AN for every internal loop, including reverse. Local RMII uses BMCR[14] plus BISCR selector 1 at 10 or 4 at 100. Check each one-hot PCS, digital, analog and reverse mode, 100 Mbit/s transmit option, fixture terminations and partner. Bypass the DP83826-specific digital writes to 0122/0123/0130. PRBS15 tests must distinguish generator, checker, both, single and continuous counting. SDAA081's 5000 is generator-only, with a one-second wait in its example. The 3108 analog case combines generator and checker. Test 11-bit packet bytes, IPG in four-byte units, bit-15 freeze/clear and interruption before the following count read. Save and restore actual packet/IPG words. |
| `tdr` | Use quiet/disconnected partner, CR1[8] auto-run or CDCR[15] manual start under §6.3.16.1 and tables 7-12/32/40–59. Cover channel, thresholds, segments 1–5, averages 0–6, done/fail and interrupted result reads. Detailed tables support 0180[7:0], 0185[6:0] and 018A[11,5]. Retain valid first-peak, sign and cross data. Peaks 2–5 are reserved. Cable propagation evidence is a prerequisite for distance conversion. CDCR[14] calibration completion/recovery needs clarification. Exclude the DP83826 SNLA428 recipe and DP83822 calibration. |
| `vod`, `analog` | Tables 8-2/3 and SNLA266B §4.1 give 17 coarse pairs and unit-specific 0333 trim for a whole 030E write. The 50% pair is 00E0/0008. At 0333=520B, A=10, B=8, clamped C=6 and offset −1 give 5800 after 0C40/000F. Exercise both clamp bounds, offsets −2 through +2 and unrepresentable results. Remove the earlier-part 4A40 fallback from the DP83825I path. Use documented 001A squelch codes 0–8 and 0302[15:14] impedance codes 0/1 for 50/25 Ω. Inspect every named DSP/analog field, including subcontrols in 0308, source-specific reset values and recipe overlaps. Give each a typed route or a cited limitation. A field width alone supplies no electrical operating range. |
| `compliance` | Table 7-35 supplies 10BASE-T selectors 0–13 and the five-bit 100BASE-TX count from 0428[5] and 0027[3:0]. Code 0 clears the shift register, 1–31 select zero counts. Force the matching speed, clear then reassert 0428[4] to change the pattern, keep deep-power bit 2 separate and admit reserved-bit exceptions solely for this test. Cover collision and scrambler controls, the SNLA266B §5 transmit, link-pulse, data and harmonic procedures, explicit stop and partial restoration. Preserve strap address and reserved fields in place of blindly writing the example PHYCR. Distinguish register control from an observable collision pin on an RMII board. |

Primary sources are the archived [SNLS638C datasheet](../../reference/source/TI-DP83825I-SNLS638C.pdf),
[SDAA081 diagnostic guide](../../reference/source/TI-DP83825-Troubleshooting-SDAA081.pdf),
[SNLA328 EEE guide](../../reference/source/TI-Energy-Efficient-Ethernet-SNLA328.pdf),
[SNLA261A wake guide](../../reference/source/TI-Wake-on-LAN-SNLA261A.pdf),
[SNLA266B compliance guide](../../reference/source/TI-DP8382x-Compliance-SNLA266B.pdf)
and [SNLU239A EVM guide](../../reference/source/TI-DP83825EVM-SNLU239A.pdf).
TI Linux uses common IRQ and wake callbacks, including a set-bit acknowledge
which conflicts with table 7-83. Zephyr enables PHYSCR[1:0] and MISR1[5],
and reads MISR1 for link IRQ service. Use the numeric comparison and exact
silicon semantics. [TI Linux at 97becb13](https://github.com/TexasInstruments/ti-linux-kernel/blob/97becb13b81c314c1d17fcf2226aac9a32086f3d/drivers/net/phy/dp83822.c) ·
[Zephyr at bc5d59d1](https://github.com/zephyrproject-rtos/zephyr/blob/bc5d59d1b6657cfeef30124b818c8fbd69da68ac/drivers/ethernet/phy/phy_ti_dp83825.c)

### Integration order

1. Start with exact access policy, collector and fault tests in
   `src/DP8382x.cpp`, `src/DP8382xInterrupts.cpp`, `test/access.cpp`,
   `test/events.cpp` and `test/interrupts.cpp`. The core currently limits
   `acquireEvidence` to seven sources for this part. `readableMmd` allows
   only vendor aliases of admitted addresses. Separate ordinary safe reads,
   owner-only reads and recipe-only actions. Test selectors, holes, direct
   aliases, retained status and release errors through CLI dump and SDK access.
2. Add the mode procedure and define recovery from an interrupted script
   before enabling dependent EEE, power, pin and analog operations. Keep
   desired, observed, trust, partial effects and the OP-9 recovery barrier
   distinct. Reuse the state machines in `src/DP8382x*.cpp`. Document
   bounds, valid state, units and output lifetime as public APIs change.
   Keep retained results independent of later mutable snapshots and respect
   COMMON's limit on by-value feature additions to `OperationResult`.
3. Integrate each verified operation through the shared commands in
   `examples/common/`, portable and installed adapter entry points,
   `examples/esp_idf/common/Dp8382xCliNative.h`, the compiled fixture and
   `tools/dp8382x_hil_{campaign,controls,diagnostics}.py`. Today
   `Campaign.feature_gate` blocks every optional DP83825I campaign and its
   constructor discards feature baselines. Select explicit cases and known
   restoration baselines for both roles. Preserve native application grants.
   Replace the corresponding blanket denials in `test/p25_profiles.inc`
   with exact positive cases and specific zero-callback negatives.
4. Add independent literal vectors in the existing family suites and
   register actual executing DP83825I tests in CMake. After each transfer
   and lock-release cut, verify retained data, possible write effects,
   token/output validity, deadline and cancellation behavior, and separately
   budgeted recovery. Check reset/replay, stale/bus-silent snapshots, and
   the no-link outcome separately from transport failure. Compare compiled
   command traces with offline replay, including failures. Extend
   `test/test_cli_coverage.py` to check the DP83825I matrix join and reject
   missing or cross-part evidence. Keep earlier-part tests registered.
5. With the behavior, update README, CHANGELOG, variants, API, feature,
   CLI, ESP-IDF, runner and RMII fixture guides. Run the full Release and
   focused UBSan host suites, Python/Doxygen/ledger/version checks, tests-OFF
   and exported consumers, the DP83825I adapter link and both P4 example
   links using the prompt 60 wrapper commands. Record reopened or unresolved
   software rows explicitly. Q15 requires identified board/role, clock loss
   and recovery, Secure-ON recognition, 000C/MISR interaction, partner and
   measured electrical behavior. Phase (a) grants no fixture operation.

### Sibling audit and phase receipts

Both sibling worktrees and indexes were clean on `main`. TMP1x2 at
`0791429f8fce1170e2082f1b011456fc461318b8` tests model capability and
every address against literal expectations, counting bus and GPIO callbacks.
Use that approach for part/role rejection. Its CLI restores the captured
profile and its runner tests loss of framing and reboot during cleanup.
Reviewed `include/TMP1x2/Config.h`, `src/TMP1x2Configuration.cpp`,
`test/variants/test_variants.cpp`, `examples/common/Tmp1x2CliDiagnostics.cpp`
and `tools/test_hil_tmp1x2_runner.py`.

MB85RC at `a9a83cafd6cd84cd41e7233332e854d9c32cde47` validates actual
manufacturer and product ID against selected metadata. Keep the declared
PHY and independent observed identity even when comparator drivers assign
different suffixes. Its runner requires evidence that restoration succeeded.
Reviewed `include/MB85RC/CommandTable.h`, `src/MB85RC.cpp`,
`test/test_public_contracts.cpp`, `examples/common/DiagnosticCore.h`,
`tools/hil_runner.py` and `tools/test_hil_runner.py`. Both siblings'
`AGENTS.md` and `.github/workflows/ci.yml` were inspected. The PHY uses its
own MDIO ownership and CMake/CTest registrations. This repository has no
tracked workflow at entry.

**Host PASS for the existing baseline.** Windows x86-64, GNU 15.1.0,
CMake/CTest 4.0.1, Ninja Release, Python 3.12.10. Reused prompt 60's
verified build. Only documentation changed between that tested source and
entry. The 22 DP83825I core tests and 17 existing event/IRQ/CLI tests passed.
Four ledger tests passed, including the original-PDF comparison.
Nine CLI coverage/registration tests passed against the current matrix.
The ledger verifier confirmed the 101 tables, 570 fields and 33 assignments.
An independent extraction from original PDF pages 33–35 matched all
13/13/32 ordered mode writes. Page 85 matched all 17 VOD coarse triples.
These checks compare the ledger with silicon documentation. The compiled
feature procedures and their fault tests are phase (b) deliverables.
Reference verification passed for 29 payloads, 27 extracts and 2,438 local
links. Doxygen passed for 294 pages and two negative fixtures. Version and
whitespace checks passed.

```powershell
ctest --test-dir build/p60-tests -R '^p25_' --output-on-failure --no-tests=error -j 3
ctest --test-dir build/p60-tests -R '^(events_|interrupt_|cli_(diagnostic|control|pin)_transcript$)' -j 2 --output-on-failure --no-tests=error
python -B -m unittest discover -s test -p 'test_p25_ledger.py' -v
python -B test/test_cli_coverage.py --ctest ctest build/p60-tests
python -B tools/verify_ledger.py
python -B tools/verify_references.py
python -B tools/check_doxygen.py
python -B scripts/generate_version.py check
git diff --check
```

**Portable/package consumer, SDK link, remote CI and physical evidence
NOT RUN in phase (a).** Prompt 71 owns remote CI. The historical build
and fixture receipts remain in prompt 60. Phase (b) can begin with access
and collector behavior. Full DP83825I software and physical qualification
remain open.

## Phase (b) implementation

**IN PROGRESS.** Entry on main was the wrapper-synchronized phase (a) commit
`594e8c158578fb9f92c623bc10a0cfc4dcab0117` with a clean index and tree.
The lead retained the public contracts, collector, integration and matrix.
Two parallel implementers used the session model for the timing/ownership
and silicon register work. Their disjoint assignments covered functional
modes, EEE and power, and diagnostics, quality and DSP controls. The exact
source and sibling revisions in phase (a) still apply.

- `DP8382x.cpp` guards both direct and indirect consuming addresses and
  retains complete samples on partial transactions. Register_12 has its own
  acquisition route. IRQ service uses MISR1/2. Wake and event configuration
  share the collector; LED0/LED2 and interrupt routing respect the clock role.
- The existing initialization owner runs the exact 13/13/32 mode scripts
  from a hard-reset baseline and retains accepted or uncertain prefixes.
  PCS reset, clock-stoppable policy, EEE advertisement and restart,
  next-page reception, Quiet RX shutdown, bilateral LPI and the post-exit MAC interval have separate evidence.
  Deep power has an explicit recovery admission and distinct control receipt.
- The common diagnostic owners now execute the exact-part loopback, BIST,
  TDR, 100M compliance count, scrambler and collision controls. MSE preserves
  the raw sample, earlier link context and the two undefined classification
  boundaries. The analog owner computes the VOD word from each unit's trim.
  Seven standalone DSP groups and impedance use bounded masked transactions.
- The shared CLI, native adapter declarations and Python admission/replay
  use the declared part and RMII role. The feature matrix joins all 33 ledger
  procedures, 101 tables, 570 fields and ten address exceptions to their
  dispositions. Link, pin, power and electrical measurements remain Q15.
  Register_12 versus MISR latch relationships, independent tuning ranges,
  the CDCR calibration completion condition and access conflicts remain open
  as recorded in the matrix. Software closure is subject to phase (c) review.

### Reproducing phase (b)

Windows x86-64, GNU C++ 15.1.0, Python 3.12.10, CMake 4.0.1 and Ninja.
Use the same configured combined Release and UBSan directories as the
preceding prompts. The pinned SDK is ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`, target esp32p4.
The three SDK commands use `scripts/idf.cmd -C <project> -B <dir> build`
with the existing configurations under `C:/esp/build/dp8382x-loop`.
Use `56l` for `test/esp_idf/adapter_link` (DP83825I), `56b` for
`examples/esp_idf/basic` and `56o` for `examples/esp_idf/owner`.

```powershell
cmake --build build/p60-tests
ctest --test-dir build/p60-tests --output-on-failure --no-tests=error -j 2
ctest --test-dir build/p55-ubsan -R '^p25_(events|outputs|core|profiles|diagnostics|functional_modes)$' --output-on-failure --no-tests=error
ctest --test-dir build/p61-consumer --output-on-failure --no-tests=error
ctest --test-dir build/p61-export/out --output-on-failure --no-tests=error
python -B test/test_cli_coverage.py --ctest ctest build/p60-tests
python -B -m unittest discover -s test -p 'test_*.py' -q
python -B tools/verify_ledger.py
python -B tools/verify_references.py
python -B tools/check_doxygen.py
python -B scripts/generate_version.py check
git diff --check
```

### Phase (b) evidence

| Layer | Result and scope |
| --- | --- |
| Host, combined Release | PASS, 1,017/1,017 CTests (140.41 s) in `build/p60-tests` with the pinned native-host SDK tests and earlier-part regressions. A prior run saw the new CLI transcript before its fixture had been rebuilt. Rebuilding the fixture and running the full suite on the final inputs resolved that failure. |
| Host, focused | PASS. The six combined UBSan cases passed. The diagnostics and analog selection passed 20/20 in each of Release and UBSan. The diagnostic binary executed 278,724 assertions. Functional-mode tests executed 103,340 assertions. Both declared roles exercised all 37 admitted campaigns and offline replay, 74/74. |
| Python and source checks | PASS. Discovery ran 280 tests with 59 skips; the compiled fixture and configured CTest registration checks run separately. The matrix check passed 10/10. Doxygen checked 296 pages and two negative fixtures. The reference verifier checked 29 payloads, 27 extracts and 2,441 local links. Exact ledger, version and whitespace checks passed. |
| Portable and package consumers | PASS. Two tests in `build/p61-consumer` and one DP83825I exported-package test in `build/p61-export/out` passed with library tests disabled. The 29 exported inputs were refreshed and compared byte-for-byte to the working source. |
| SDK link | PASS. The three esp32p4 images below linked with the pinned SDK. Each primary application binary was hashed after the final build. |
| Remote CI | NOT RUN. Prompt 71 owns first-execution remote CI. |
| Physical | NOT RUN. Q15 requires an identified DP83825I fixture, clock source, partner and authorization. No board, serial port or instrumentation was operated. |

| ESP-IDF binary | Bytes | SHA-256 |
| --- | ---: | --- |
| `dp8382x_adapter_link.bin` | 411760 | `a96e54377b1f7cbcfaf051d2b2329d222928faff9e30710b49476fec687a6200` |
| `dp8382x_p4_ethernet.bin` | 720656 | `ab9d5bdfa8e592a7e630da4bd0645d1cdadab7b805aaf6eda1c31889a8f33cd5` |
| `dp8382x_p4_owner.bin` | 598880 | `593eb2f261f19403feac13cc25ab457ea84277e19c4e4a744280b81b37e8d212` |

## Phase (c) independent review

**IN PROGRESS.** The wrapper verified main and origin/main at
`094310d7452faf5d8ed82b4d8a34c123c448ea9a`. The index and worktree were clean.
Two independent read-only reviewers used the session's high-reasoning model
for the silicon sequences and ownership/deadline analysis. They reviewed the
phase (b) diff from `594e8c1` in parallel and compiled probes against its
Release library. The lead reviewed the event, pin, wake, adapter, CLI, runner
and field-matrix paths, integrated the fixes and registered the regressions
in the existing executable suites. The sibling revisions and paths in
phase (a) were rechecked. TMP1x2 supplied the bus-silent variant rejection
convention. MB85RC supplied the separate identity, field validity and
restoration checks. The shared MDIO collector owns destructive PHY evidence.

| Finding on the committed implementation | Correction and regression |
| --- | --- |
| With an owned AN profile, DEFAULT, EXTENDED_REACH and EXTENDED_REACH_EEE admitted HARD with only 18, 75 and 163 total frames. Those budgets exhausted after reset, even with immediate command completion. | The owner includes the link, PCS clock, final EEE proof and owned recovery passes before HARD. In the reviewed follower fixture the three complete passes use 42, 99 and 195 frames. `p25_functional_modes` accepts the exact allowance and withholds HARD at one frame below it. Additional hardware polling uses the same deadline. |
| A raw read of asserted PCS reset during link configuration allowed six more target writes and a successful restoration verdict. A scheduling jump immediately before bilateral LPI entry let the force write execute after the complete procedure could no longer fit. | The accepted MMD3:0000 observation terminates the pending dependent operation, even on a later release failure. The vendor mode receipt survives. Negotiated and bilateral LPI entry and exit reserve the remaining writes and verification at the callback boundary. The tests check strict deadline equality, output validity and cleanup debt. |
| An independently owned equalizer bit changed table 6-7's 030F from 0400 to 0480 with both owners reporting success. Configuring EEE before LED2 IRQ rejected the route. The reverse order could reset the PHY and then fail IRQ restoration. | The long mode rejects any owned DSP group in its script, including the equalizer, before a callback in both orders. LED2 IRQ uses 0305 and can coexist with EEE. Both roles exercise each order and HARD replay. |
| SNLS638C tables 7-60 and 7-62 document 0302[6] and its 0308[0] status. Phase (b) rejected both clock choices. A contradictory SOR1 observation could also leave dependent pin 2 delivery marked verified. | The interface owner applies both clock codes with masked writes and independent status readback. The leader requires approval to disable the clock. IRQ, wake and pin proof follow the observed role. The LED0 test also exercises a clock jump at the first callback and preserves the caller's previous result. `p25_outputs` and the compiled CLI exercise these paths. Physical pin behavior remains Q15. |
| Plain Magic Packet configuration, status reads and restoration included the three password offsets. SNLA261A gives those otherwise unlisted words a Secure-ON purpose. | On DP83825I, only a Secure-ON profile accesses 04A5..04A7. The other wake profiles use three MAC words and a 21-frame observation. Secure-ON uses six words and 33 frames. A secure-to-plain rekey, status, HARD replay and both-role offline campaigns check the resulting validity mask. |
| The register-only collision test could clear newly asserted isolation or power-down between preflight and its BMCR write. A failed collection whose first valid word indicated link down could leave an earlier MSE prerequisite current. | A same-lock BMCR predicate protects collision entry and stop, including retry after an uncertain clear. A completed contrary link or disruptive power observation invalidates MSE eligibility. `p25_diagnostics` checks untouched BMCR, retained cleanup, the read-clear prefix and zero MSE frames after the contradiction. |

The original probes remain under the ignored `build/p61-c-mode` and
`build/p61-c-diag` directories. The permanent regressions are in
`test/p25_modes.inc`, `test/p25_profiles.inc`, `test/p25_diagnostics.inc`
and `test/cli/pin_transcript.py`. Exact-part source inputs remain the archived
SNLS638C, SDAA081, SNLA328 and SNLA261A cited in phase (a). The README and
CHANGELOG already describe feature availability. The API, CLI, wake guide,
field matrix and public clock/wake documentation now specify the corrected
ownership and observation rules.

### Phase (c) verification

The first combined Release run passed 1,015 of 1,017 registrations. Its
link-transition test caught an extra stale transition while a coherent read
was still acquiring its own frames. Comparing the completed observation
with the state at entry preserves one UP-to-DOWN change. A failed pass
keeps the earlier consumed evidence. The injected serial finalization test
also reached its six-second watchdog during overlapping checks. The final
run used the original watchdog and link assertions. An initial test build
caught a narrowing conversion in a new literal vector. The final vector
uses explicit 16-bit values.

Use the phase (b) Windows GNU 15.1.0, Ninja, CMake 4.0.1 and Python
3.12.10 configurations. Host builds with pinned SDK sources require the
process-local safe directory for `C:/esp/v6.1/esp-idf` during the build.
For the P4 links, use the same process-local clangd, ccache and Git paths
as the [prompt 53 build](53-regression-transaction-ownership.md).

```powershell
cmake --build build/p60-tests --parallel 4
ctest --test-dir build/p60-tests --output-on-failure --no-tests=error -j 1
cmake --build build/p55-ubsan --target dp8382x_interface_tests dp8382x_link_tests --parallel 3
ctest --test-dir build/p55-ubsan -R '^p25_(events|outputs|core|profiles|diagnostics|functional_modes|link_.*)$' --output-on-failure --no-tests=error
cmake --build build/p61-consumer
ctest --test-dir build/p61-consumer --output-on-failure --no-tests=error
cmake --build build/p61-export/out
ctest --test-dir build/p61-export/out --output-on-failure --no-tests=error
scripts/idf.cmd -C test/esp_idf/adapter_link -B C:/esp/build/dp8382x-loop/56l build
scripts/idf.cmd -C examples/esp_idf/basic -B C:/esp/build/dp8382x-loop/56b build
scripts/idf.cmd -C examples/esp_idf/owner -B C:/esp/build/dp8382x-loop/56o build
```

The exported consumer uses the phase (b) 29-file source package. Refresh
each packaged file from its repository-relative path and compare all 29
byte-for-byte before the out-of-tree build. Both consumer configurations
have library tests disabled.

| ESP-IDF v6.1 fff9895c, esp32p4 | Bytes | SHA-256 |
| --- | ---: | --- |
| DP83825I adapter link | 412544 | `1e12488598e1e63d7664fdcd6feb9891578e651dded39f799f0f32376a4ccb98` |
| Basic Ethernet example | 721600 | `077efc569c66edb4369574ae51f36b78d7eda7e9ec728746af9fe35ed3fea415` |
| Owner example | 599808 | `e5283bf6253d540ca1bb5bf531f7bf906a049735aedcdd3de6ec7df7ec376c13` |

**Host PASS.** The final combined Release run passed 1,017/1,017 in
244.83 seconds. This includes the original transition and serial watchdog
regressions, and 37 DP83825I fixture cases on each RMII role with offline
replay (74/74). The four reviewer probes were recompiled against the final
Release library. The first admitted HARD budgets equalled the first successful
budgets of 42, 99 and 195 frames. The clock-jump LPI entry issued zero frames,
PCS reset prevented further link writes, collision preserved isolation and
the collector's down-link sample blocked MSE. The reference verifier passed
29 vendor payloads, 27 extracts and 2,442 local links. Whitespace passed.

All three SDK links passed using the reviewed working tree based on
`094310d`. Portable consumer 2/2, exported consumer 1/1 and focused
UBSan 15/15 passed. Python discovery ran 280 tests with 59 skips. The
original-PDF comparison passed all four ledger tests. The matrix passed
10/10, Doxygen checked 296 pages and rejected both negative fixtures.
The ledger and version checks passed.

Remote CI and physical evidence are NOT RUN. Prompt 71 owns remote CI.
Q15 still requires an identified board, clock/partner evidence, wake
recognition and electrical measurements. The unknown Register_12/MISR
latch relationship, CDCR calibration completion and independent DSP
tuning limits retain their separate source dispositions. Phase (d) must
reconcile those software rows before setting the status for prompt 61.
