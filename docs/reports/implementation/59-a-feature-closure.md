# Prompt 59 · DP83826AI and DP83826AE diagnostic and optional controls

## Phase (a) assessment and implementation brief

**IN PROGRESS.** Prompt 58 completed at `36e7ae3adaa4e8d8e5545b222b21eff9126721ab`.
The wrapper verified that commit on `origin/main` on 2026-11-25. The index
and worktree were clean on entry, with zero commits ahead or behind. Both
grades already bind revision 4 in BASIC and ENHANCED. Their current feature
gates and the primary I/E implementations are the starting point. [Prompt 58
verification](58-a-core-link.md#phase-d-final-verification) · [Prompt 59](../../prompts/59-a-feature-closure.md)

The following 31 procedure IDs have owner 59. The A ledger also assigns 147
fields, 18 features (including the absence disposition), 56 exceptional write
actions, 18 gates and five timing records here. AI and AE share these facts.
Each declared grade needs its own executing tests and physical evidence.
[A field ledger](../../registers/dp83826a.json) · [Procedure assignments, gates
and test contract](../../registers/dp83826a-procedures.json)

| Existing owners | Assigned procedure IDs |
| --- | --- |
| Collector and interrupts | `collect_event_counters`, `interrupt_configuration`, `interrupt_test` |
| Pins and wake | `led_configure`, `pin_configure`, `wol_configure`, `wol_acknowledge` |
| Loopback and PRBS | `loopback_near`, `loopback_digital`, `loopback_reverse`, `loopback_external`, `bist_run`, `bist_capture` |
| Cable | `cable_configure`, `cable_manual`, `cable_autorun`, `cable_capture` |
| Power and EEE | `power_ieee`, `power_active_sleep`, `power_pll_control`, `power_deep`, `eee_initialize`, `eee_negotiation_status`, `legacy_lpi` |
| Quality, electrical and recovery | `signal_quality_sample`, `tx100_configure`, `tx10_configure`, `analog_configure`, `collision_test`, `compliance_session`, `calibration_recovery` |

### Silicon decisions and executing acceptance cases for phase (b)

1. **Establish the pin contract first.** In `src/DP8382xPins.cpp` the source
   shift `i*4`, the two LEDCFG2 banks and the old IO_CFG1 clock override are
   I/E conventions. For A at MMD 31 offset 0460, assign LED3/2/1 sources
   A/4/2 from reset 5565 and expect 5A42. Change one pin at a time with other
   nibbles asymmetric. At 0469 exercise LED3 polarity/value/enable at bits
   10/9/8 separately. Use MLEDCR/LEDCR for LED0. Preserve PHYCR[5] and
   IO_CFG1[6] as reserved and reject the original override. Observe the
   strap-loaded 0304/0305 values. BASIC exposes LED0/1 with active-low
   polarity. Cover all eight mux codes at each available pin, including
   explicit reserved and conflicted dispositions. In ENHANCED MII, require
   release of COL on pin 28 or CRS on pin 29 for alternate outputs. Extend
   the typed roles and board permissions to express these choices. Pin 30
   clock selection belongs to the interface owner at 0303. Check both orders
   of a conflicting clock, LED, IRQ or wake claim. Pin 31 code 1 is reserved,
   and figure 8-10 and table 8-76 disagree on codes 5/6. Retain the ENHANCED
   POR observation and gate software activation for the disputed codes.
   Derive frame/callback costs from the selected preflight, apply, verify and
   replay rows, counting dependent owners once. [SNLS783A §8.3.18 and tables
   8-74–79/86/90](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf)
2. **Route IRQ and wake through their existing owners.** Trace
   `src/DP8382x{,Interrupts,Wake,Interface}.cpp`, including admission,
   per-pin readback, raw-write invalidation and restoration. Extend IRQ/WoL
   routes to LED0/3 where available. Exercise all 16 MISR enables, mask
   delivery during setup and enable last. On MISR2 failure retain the
   consumed MISR1 upper byte in the collector. Keep saturated FCSCR as a
   lower bound and RECR raw. Before RXFCFG1 writes harvest RXFS/MISR2 and
   establish the detector and traffic prerequisites for rekeying. Observe
   bits 7 and 0 across modes and straps, preserve bit 6 and compose bit 11
   W0C. For the same settings expect ordinary/ack words 1981/1181 and
   SecureON 19A1/11A1. Map the first transmitted octet to bits 7:0 at the
   first word. MAC 02:12:34:56:78:9A gives 1202/5634/9A78 and password
   A1:B2:C3:D4:E5:F6 gives B2A1/D4C3/F6E5. Cut at each credential write
   and at acknowledgement. Preserve uncertainty and independently observed
   line state. TI Linux corroborates packing and enable/pending overlap.
   Its unchecked intermediate results, write-one clearing and 822-specific
   filtering do not establish an A procedure. [Pinned TI Linux driver,
   GPL-2.0](https://github.com/TexasInstruments/ti-linux-kernel/blob/97becb13b81c314c1d17fcf2226aac9a32086f3d/drivers/net/phy/dp83822.c)
   · [Family IRQ/wake source comparison](../../reference/library-family.md#interrupt-and-wake-facts-from-ti-linux)
   · [SNLS783A tables 8-36–40/91–98](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf)
3. **Select the EEE sequence and its budget together.**
   `src/DP8382xEee.cpp` assumes 18 rows and a terminal BMCR write.
   `_admitEeeStep` in `src/DP8382x.cpp` assumes 66 recipe frames. On A,
   25 addressed writes and two direct writes cost 102 frames. Including
   capability, advertisement readback and the first HARD poll gives 111
   frames in 30 callbacks from capability entry for that portion. Budget
   identity, subsequent polling, clock lag, completion-anchored quiet time,
   VOD, tail and owned replay according to the actual path. Check exact
   budget and one below before mutation. Require whole-word 0002 first at
   3:0014, then at 7:003C after HARD and the advertisement write. Preserve
   the 27-row order across the page break, including 0308=0000 as a script
   exception, 0416=1830 and 04F5=2864 at rows 7 and 25. Exclude 04D3.
   Row 26 is direct 00=3300, row 27 is 1F:0466=FD00. Cut each selector
   and data frame, distinguish an uncertain restart from an accepted restart
   followed by a final-row failure, and preserve both outcomes through end.
   Reconcile mandatory 030E=4A40 before an owned analog override and replay
   persistent owners once. In `readEeeStatus` the current bookends read BMSR
   before PHYSTS. Apply the A ordering from prompt 58 and check partial
   evidence and both bookend decodes. Validate the five D2 tuples, reject
   concurrent standard next-page exchange, exercise 04D1 008B→108B→008B
   under drained MAC ownership and retain LPI history separately from current
   LPI. The 36 µs wake figure is nominal. [SNLS783A §§7.6, 8.3.3–4](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf)
4. **Exercise inherited diagnostics on each grade.** In
   `src/DP8382xDiagnostics.cpp` digital setup writes BISCR=0104 and existing
   clear/restore/BIST preserve masks include hardware-updated bits.
   `src/DP8382xElectrical.cpp` also composes BISCR. Seed RH bits 11/9/8
   and assert the actual write frames. Exercise speed, mode, forced-link
   prerequisites, external partner and bounded stop for each loop. Use the
   100M digital restart recipe and keep possible opaque effects. In the guide
   0110 selects reverse, 5000 disables the checker and 3B08 is an analog
   lock/activity sample. One second of dwell supplies no fresh checker
   evidence. Test four BIST modes and freeze/capture 807D→A57D, including
   loss after a successful freeze. For `src/DP8382xCable.cpp` distinguish
   the D3 six-segment TX-A recipe from ordinary CDCR composition with RH
   bits 1:0. Retain GENCFG bit 3, require fresh progress/done, handle a
   second drop during autorun, and retain partial raw captures. Calibration
   clear and HARD must be consecutive under one bus reservation, followed by
   quiet time. [Troubleshooting guide pp. 18–20](../../reference/source/TI-DP83826A-Troubleshooting-SNLA491.pdf)
   · [Exact-part D3 selection and source gates](../../registers/dp83826a-procedures.json)
5. **Reconcile the remaining controls at their existing entry points.**
   Open the TX10/TX100 portions of `src/DP8382xQuality.cpp` with FLD still
   subject to its prompt 58 prerequisites. Sweep squelch 0–8, reject 9–F,
   test all four FIFO depths and preserve polarity samples. Read raw MSE at
   0218. SNLA491's 0225 conversion lacks a demonstrated mapping. In
   `src/DP8382xAnalog.cpp` verify each legal split-VOD transition in both
   directions. Keep 0461=0400 and undemonstrated receiver-gain settings
   fenced. For collision and 10M compliance, require fixture ownership and
   finite stop and cover pattern codes 0–D. In `src/DP8382xPower.cpp` test
   asserted external power-down after clearing software intent, sleep
   observation and PLL prerequisites. The 1.4 s NLP interval gives no wake
   deadline. Reversible deep power and the 100M compliance session depend
   on safe 0428 access. Preserve their zero-I/O rejection and the reason.
   [Exact-part source and test case for each procedure](../../registers/dp83826a-procedures.json)

### Integration and verification

The lead owns the declarations in `include/DP8382x/`, capabilities, shared
collector, reset/replay, architecture, coverage matrix and integration.
Preserve existing public enum values, C++17 conventions and Doxygen. Use
per-procedure admission with exact identity. A rejected declaration or local
mode/role conflict costs zero callbacks. A live mismatch retains its observed
prefix. Keep indirect and RMW batches atomic across bus users, release the
lock before waits and preserve primary, release and separately budgeted OP-9
restoration results. Inject failures at every transfer position, including
applied-before-error, expired budget, cancel and end. Assert zero traffic
from caches. Keep transient stimuli out of replay. [API contracts](../../api-contracts.md)
· [Verification contract](../../prompts/VERIFICATION.md)

- **Coverage.** Append exact-A rows to the existing `docs/feature-coverage.md`
  matrix. Join all 52 procedures and 35 features, including prompt 58 evidence,
  to the 79 detailed register entries, 285 fields, recipe exceptions and
  concrete typed/CLI routes, records and registered tests. Preserve the named
  gate for every blocked mode. `test/test_cli_coverage.py` currently indexes
  by bare ID against the I ledger. Make its checks generation-aware and reject
  missing A rows, original masks on A and duplicate IDs within a generation.
  Keep essential unresolved software rows OPEN. For a new requirement,
  record its source and repair the assignment before counting closure.
  Record AI and AE physical results separately.
- **Consumers.** Update the shared CLI command tables and pin decoder in
  `examples/common/`, including discoverable per-map raw LEDCR policy. Verify
  actual getters and owner grants, BASIC/ENHANCED results and P4 admission
  with and without feature attestation. The compiled CLI fixture initializes
  several control words from original defaults. Give it independently derived
  A reset and strap vectors. In `tools/dp8382x_hil_campaign.py`,
  `dp8382x_hil_controls.py` and `dp8382x_hil_runner.py`, replace the broad
  A gates and single-command baseline assumption per eligible case. Cover
  multi-command setup, pre-nonce interruption, wrong declared grade and
  offline replay with corrupted fields. Retain existing I/E artifact coverage
  and stock board permissions. [CLI](../../cli.md) · [Hardware cases](../../hardware-validation.md)
- **Checks.** Extend the registered core suites using
  `test/support/PartSelection.h`, root `CMakeLists.txt` and
  `test/esp_idf/CMakeLists.txt`. Run both A grades, I/E regressions, Release,
  focused UBSan, tests-OFF portable consumers, compiled CLI/Python execution
  and replay, Doxygen, version, ledger and references. Check clean consumer
  use of changed exports. Link `test/esp_idf/adapter_link` for each A grade
  plus the basic and owner examples using `scripts/idf.cmd`, ESP32-P4 and
  v6.1 `fff9895c` in separate directories under
  `C:/esp/build/dp8382x-loop`. The prompt 58 report contains the baseline
  commands. Update README, CHANGELOG, variants, API, feature, adapter and
  fixture guidance alongside delivered behavior. Physical qualification for
  each grade includes Strap11 EMC when FLD is used. It requires identified
  fixtures and corresponding measurements. [Rollover guide §3](../../reference/source/TI-DP83826-to-DP83826A-SDAA067A.pdf)

### Peer and source audit

Two read-only agents used the session's high-reasoning model in parallel for
the silicon/transaction audit and the peer/consumer audit. The lead checked
the assignments, entry code and registration. All three sibling checkouts
were clean on `main` tracking `origin/main`. Their `AGENTS.md` files were
reopened. This repository has no tracked CI workflow at this checkpoint.

| Peer revision | Paths reopened | Decision |
| --- | --- | --- |
| TMP1x2 `0791429f8fce1170e2082f1b011456fc461318b8` | `include/TMP1x2/Config.h`, `src/TMP1x2.cpp`, `src/TMP1x2Configuration.cpp`, `src/TMP1x2Operations.cpp`, `test/variants/test_variants.cpp`, `test/jobs/test_jobs.cpp`, `examples/common/Tmp1x2CliDiagnostics.cpp`, `tools/hil_tmp1x2_runner.py`, `CMakeLists.txt` | Reuse exact-part legality, preflight rejection and retained ambiguous effects across cancel/teardown. |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `include/MB85RC/CommandTable.h`, `src/MB85RC.cpp`, `test/test_basic.cpp`, `examples/common/DiagnosticCore.h`, `tools/hil_runner.py`, `platformio.ini` | Reuse explicit-variant and mismatch tests. AI/AE share observable IDs, so the application retains grade selection. |
| TUSB32x `6989c77fad3bc4651403942a4c9f7ca3940f859b` | `test/test_diagnostics.cpp`, `test/cli/test_cli_diagnostics.cpp`, `test/CMakeLists.txt`, `tools/tusb32x_hil_plans.py` | Adopt independent scripted order, partial evidence and cleanup checks. Derive MDIO costs from actual PHY frames. |

### Phase (a) receipts

Windows 11 AMD64, CPython 3.12.10. **Host source/ledger PASS.** The first
local PDF comparison covered 79 tables and 427 rows per generation. The
second checked all 27 A EEE rows and the six-run TDR words. The maintained
ledger verifier passed and all 100 ledger tests passed. **Host CLI baseline
PASS.** One compiled regression ran both grade selections against the existing
prompt 58 fixture. All 22 simulated results matched replay, including the
current EEE and digital loopback denials for each grade. Documentation
references and whitespace passed. This establishes the starting behavior
for phase (b).

```powershell
python -B build/p57-final-sources/crosscheck.py
python -B build/p57-procedures/check.py
python -B tools/verify_ledger.py
python -B -m unittest discover -s test -p 'test_*ledger.py' -q
python -B test/test_hil_features.py --fixture build/p53-baseline/test/cli/dp8382x_cli_fixture.exe --work-dir build/p59a-runner CompiledFeatureTests.test_a_feature_cases_require_their_generation_profiles
python -B tools/verify_references.py
git diff --check
```

New prompt 59 host behavior, portable consumer, exported package consumer,
SDK link, remote CI and physical evidence are **NOT RUN** in phase (a).
Prompt 71 owns remote CI. Phase (b) starts with the shared pin/route contract
and grade-specific vectors above. Software closure remains open.
