# Prompt 26 - Cable autorun and shared result acquisition

Date: 2026-11-15. Branch: `main`. Clean, fetched, synchronized entry checkpoint:
`1aedc22238027120da7eb9ea389fb11074a6d27f`. Final SHA and upstream equality are
recorded in the handoff without a self-referencing commit.

## Rules, scope and selected contracts

Reopened root/parent AGENTS, COMMON, CONVENTIONS, EVIDENCE, FULL_COVERAGE,
ENGINEERING, VERIFICATION, API OP-1 through OP-17, README, architecture,
variants and reference index. Prerequisites are stage-25 cable controls/results,
shared reset/restoration, stage-20 collector and stage-4 diagnostic ledger.
Owned ledger procedure: `cable_autorun`. Shared `cable_capture` is reused.

Lead owns existing public header, cable/lifecycle integration, CMake, ledger,
guides, progress and Git. One application owner serializes calls and all complete
controller transactions. No ownership spans waits. Starts/cancel/cached views
are bus/clock silent. No SDK, allocation, worker, second collector or generic
job engine was added. Existing enum ordinals are preserved by appending values.

The shipped policy is exactly one enable flag with explicit maintenance admission.
Desired intent becomes authoritative at admission. Observed value/validity/time
and trust remain independent, including failed/late writes. Successful policy
apply is not a physical trigger. Failed mutation uses OP-9 and later explicit
initialization, with no cleanup callback or retry exception.

Shared initialization restores explicitly owned policy last, after all temporary
saved-field tails. Verification/link configuration checks it without writing.
Unowned policy contributes no replay frames. Manual handoff is explicit disable
and owned readback; engine quiescence remains unknown and manual live preflight
still runs. Raw autorun acquisition uses the existing cable phases once, never
waits for an environmental trigger, and retains partial nondestructive words.
Read-plan completion does not establish freshness/coherence or SNLA428 context.

## Fresh exact-part and sibling evidence

Original archived SNLS647I (August 2025) and the current official
[DP83826I datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) agree.
Reviewed section 8.3.16.1 p43, table 8-28 pp60-61, table 8-47 p73 and
tables 8-54 through 71 pp76-81. Fresh exact-part TI support searches found no
stronger generation, result-latch, quiescence or completion-maximum evidence.
Shadow controls suppress peak artifacts; they do not provide atomic snapshots.
Vendor payloads, hashes and licenses remain unchanged; no source code was copied.

Independent literals: enable 00A2 ->01A2, disable 03F3 ->02F3, preserve FEFF and
compare 0100. Initial CDCR 0102 already has done set. Only result banks 0180/185/18A
are admitted; reserved banks do not become peaks. Raw bookends can detect a
change but cannot prove a single silicon generation, even with identical data.
PCS/restart retain vendor state; HARD defaults it. Explicit desired policy is
replayed through the existing owner, not an automatic reset/recovery loop.

Read-only sibling worktrees were clean at these pins:

| Source / revision | Actual material and adopted/adapted/rejected principle |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | AGENTS; `include/TMP1x2/TMP1x2.h` result/snapshot/lifetime; `src/TMP1x2Operations.cpp` retained finish/cancel/take; `test/jobs/test_jobs.cpp` cancellation/late publication and registrations; `examples/common/Tmp1x2CliDiagnostics.cpp` result/restore display; `tools/hil_tmp1x2_runner.py:1215` fixture verdict; `tools/test_hil_tmp1x2_runner.py:829,839,1429,1450` parser/failure tests and CMake jobs/hil_runner registration. Adopt fixed retained values and explicit validity; reject sensor timing and automatic restore/rebind as PHY rules. |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | AGENTS; public observed/retained-result contracts; `src/PCA9555.cpp` cooperative cleanup; `test/test_basic.cpp:1019,1745,2120,2495,2543,2580` deferred observation, cancellation, deadline and detach tests with RUN_TEST registration and native platform configuration. Adopt primary/restoration separation and finite ownership; reject I2C ACK/pointer cleanup semantics. |

Fixture labels and scripted disconnect instructions remain distinct from measured
hardware evidence. No sibling or vendor file was edited. Framework/SDK/package
decisions and version remain unchanged; no later variant support is inferred.

## Delegation, review and corrections

Three inherited capable-model agents handled disjoint source reasoning, peer/
lifecycle review and independent executing tests. `autorun26_sources` was
read-only. `autorun26_peers` reviewed actual integration and wrote only
`examples/portable/CableAutorun.h` and the cable consumer extension.
`autorun26_tests` owned only `test/autorun.cpp`, using `build/autorun26-tests`.
The peer consumer used `build/autorun26-peer-consumer`. Lead reviewed actual
source, contributions, tests and final diff; only lead performs Git operations.

Confirmed review corrections:

- Append new operation enum values; retain existing cable ordinals.
- Keep initialization/restoration incomplete until final owned autorun replay
  verifies. Failure, cancellation, release error and late readback retain earlier
  effects and valid raw observations without a false initialized state.
- Keep known calibration recovery owed across ordinary raw zero reads. A later
  clear bookend cannot prove the required HARD occurred. Mutation starts and
  initialization retain the gate; raw reads remain available. Only an owning
  manual stop that observes clear and still owes HARD, or external reset/POR
  notification, can release that gate before prompt 35's explicit recovery.
- Correct ledger language that implied automatic cleanup, hardware generation
  tracking or proven engine quiescence.
- Mark cached tuning context invalid for autorun. Default manual control values
  cannot masquerade as observed settings for a raw hardware-triggered result.

Final maintainability review retained one cable phase switch and one persistent
restore path. Policy application owns only one mask; raw acquisition reuses
the three result reads/bookend. Fixed values expose uncertainty instead of a
second measurement abstraction. Cached rendering is passive and sources remain
independent test oracles.

## Checks and remaining evidence

Environment: Windows PowerShell, GNU C++15.1, Ninja, C++17, Release with executing
checks and registered failure probes.

| Layer | Result |
| --- | --- |
| Host | PASS: 144 registered Release CTests. Seven autorun suites plus failure probe cover literal masks, all frame/release failures, update positions, old/equal done, partial samples, cancellation, replay, verification, handoff, calibration, clocks and cross-PHY contention. |
| Portable | PASS: two tests-OFF consumer CTests. Cable consumer executes explicit enable, passive views, later raw capture, explicit disable and manual/HARD flow. |
| Python / source checks | PASS: 103 Python tests; ledger, vendor/reference/local links, Doxygen with negative fixtures, deterministic version and diff checks. |
| SDK/package | NOT RUN: no native controller or release package changed; portable linkage is not SDK evidence. |
| Remote CI | NOT RUN: first-execution checkpoint remains prompt 71. |
| Physical | NOT RUN: actual link-down trigger timing, engine quiescence, result coherence and Q7 distance accuracy require identified admitted measurements. |

Commands: configure/build `build/host` with Release and tests ON, then
`ctest --test-dir build/host --output-on-failure --no-tests=error`.
Configure/build `test/consumer` separately in `build/autorun26-consumer` with
Release and tests OFF; run its CTest. Also run
`python -m unittest discover -s test -p "test_*.py" -v`,
`python tools/verify_ledger.py`, `python tools/verify_references.py`,
`python tools/check_doxygen.py`, `python scripts/generate_version.py check`
and `git diff --check`.

Public Doxygen, README/CHANGELOG, cable/API/architecture/initialization/
reconciliation/health/device guides, vectors, ledger/coverage, audit dispositions,
test guide and progress are updated. The [cable guide](../../cable.md) separates
manual/autorun ownership, fixed work bounds, retained raw validity and planned
fixture scenarios. `tdr_freshness_abort`, `tdr_recipe`, `tdr_calibration` and Q7
remain explicit; calibration recovery belongs to prompt 35. Later power features
extend the shared replay rather than assuming they are already implemented.
