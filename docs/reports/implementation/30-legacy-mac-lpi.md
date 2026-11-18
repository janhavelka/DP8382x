# Prompt 30: forced TX LPI control

Implemented ledger procedure `legacy_lpi` as direct bounded `startLpi`/`stopLpi`
calls. [The guide](../../lpi.md) owns public use, failure, cancellation and physical
qualification details; [literal vectors](../../register-vectors.md) remain
independent of production code. No operation slot or controller lock spans dwell.

## Rules, ownership and checkpoint

Started on clean fetched `main`, equal to upstream, at
`356320ac645f7fad405ff35b8ca3e7c853423ea4`. Reopened full prompt 30, COMMON,
CONVENTIONS/EVIDENCE/FULL_COVERAGE, API OP rules, parent/root agent guidance,
architecture, variants, reference index and current prerequisite implementation.
Stages 29 EEE recipe/status, 28 power observation and 22 shared HARD restoration
supply concrete admission and recovery behavior. The lead owns shared headers,
source integration, guides/progress and Git. Source work extends the existing
EEE implementation; no new job engine, collector or transaction mechanism.
One application MDIO owner serializes callbacks, snapshots and all bus clients.
Siblings and vendor payloads remain unchanged. No workflow exists yet; automatic
CI and remote status checks remain prompt 71. No SDK or fixture action was taken.

## Fresh source and sibling audit

The independent source reviewer reopened original SNLS647I pp15,26,27, including
the actual NOM column. A fresh official TI PDF download was byte-identical to
`docs/reference/source/TI-DP83826I-E-SNLS647I.pdf`, RevI Aug2025, SHA256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Section 8.3.4 establishes bit 12 only; 008B->108B->008B preserves EFFF. No inspected
current exact-part clarification established bits 14:13 or a readiness event.
100M and no-automatic-sleep admission are conservative library policy. The
existing stage 29 source-implied EEE mapping/recipe remains subject to physical
Q1; Q2 line/timing/traffic/energy remains NOT RUN. Source material was compared,
not copied; vendor bytes/licenses are unchanged.

All sibling worktrees were clean and read-only; current paths/revisions:

| Repository | Revision | Reopened evidence |
| --- | --- | --- |
| MB85RC | `3ace168eb01d687ab60e9fe88cc72322e5cae260` | `include/MB85RC/MB85RC.h` SleepState/enterSleep/wake; `src/MB85RC.cpp`402-490; `test/test_basic.cpp`5350 wake_transport_failure_marks_power_state_unknown_without_retry,6070 wrap case and RUN_TEST6537/6554; CLI signed-status/cached output, `tools/test_hil_runner.py`, AGENTS. |
| PCA9555 | `8b461b2afe9910b5b15fe32cc049d52e3b498970` | Public operation/cleanup declarations; `src/PCA9555.cpp` _i2cWriteCleanupTracked/_invalidateShadowPair/_readInputRegisters; registered primary/cleanup/input-retention tests in `test/test_basic.cpp`; CLI main.cpp 307/485 status/cached settings, `tools/test_run_i2c_hil_parser.py`, AGENTS. |
| TMP1x2 | `afd50889312f6e218a6a34eca687e3e8547d8b40` | Public OperationResult/Snapshot; `src/TMP1x2Operations.cpp`97 finishJob,239 cancel,245 takeResult; `test/jobs/test_jobs.cpp`335 cancellationAndTeardown/registration 667; CMake jobs 37-38; actual diagnostic restoration `examples/common/Tmp1x2CliDiagnostics.cpp`150 restoreProfile; `tools/test_hil_tmp1x2_runner.py`, AGENTS. |

Adopt retained uncertain transitions, separate primary/restoration outcomes,
independent configuration trust and passive rendering. Reject EEPROM ACK/tREC,
sensor conversion waits and I2C pointer cleanup as PHY sequencing evidence.

## Implementation and review

The lead implements the facade/shared hooks and the existing EEE source.
Three inherited capable-model agents were used because the work benefits from
independent ownership/source reasoning and literal tests: `lpi30_sources` reviewed
original PDF/arithmetic/siblings read-only; `lpi30_tests` owned only
`test/lpi.cpp` with an independent Release build; `lpi30_review` inspected
OP-9/deadline/invalidation and final source read-only. No agent edited siblings
or Git. The lead inspected the actual changes and all reported findings.

Entry validates owner/MAC admission, recipe/control trust and current cached
EEE/link/power. Whole-call admission reserves 9 frames/2 callbacks. Each callback
rechecks its remaining allowance. RMW owns one complete transaction; readback
reselects fully. No retry or additional cleanup follows failure. Target effects
remain distinct from selector effects and managed health. Successful stop compares
bit 12 only. A previous result copy remains immutable while the later stop has its
own budget/result. The cached session controls OP-9 debt with the slot IDLE.

The direct synchronous API has no in-flight cancellation token. Before invocation
the owner can skip the call; after return it cancels dwell with explicit stop.
Backend aborts use terminal transport errors and retain effect evidence; illegal
CANCELLED/nonterminal statuses normalize without pretending the write was undone.
This is the documented adaptation of prompt 30's cancellation boundaries to its
explicit two-bounded-call requirement.

Confirmed review fixes and regressions:

- Recognize the owned temporary bit during EEE observation without dirtying
  persistent bypass trust merely because LPI is active.
- Preserve any actual bypass contradiction from either read prefix, including
  failure before the target write. Successful stop cannot erase unrelated debt.
- Mark the EEE aggregate stale when partial LPI acquisition replaces its control
  sample, preserving the old timestamp rather than presenting mixed-age evidence.
- Count only independently valid attempted/completed receipt prefixes; a broken
  earlier completion cannot authorize trusted later data or inflated counts.
- Keep uncertainty after failed teardown and explicit abandonment; successful
  local end retains historical LPI evidence, not a hardware-clear claim.

Final maintainability review kept one small helper for the two mirror calls,
fixed-size values and existing shared reset restoration. No new allocation,
scheduler, public raw override or hidden physical timing assumption was added.

## Documentation and verification

Updated public Doxygen, API/architecture/EEE/LPI/initialization/health/behavior
and feature guides, independent vectors, ledger, README/CHANGELOG, test guide and
[owner example](../../../examples/portable/Lpi.h). The tests-OFF consumer compiles
and links the owner calls and exercises rejected entry/exit. Host tests execute
normal/failed exit and retained primary results. Variant admission, version and
SDK/tool commands need no changes; no family support is broadened.

Windows, GNU 15.1, Ninja, C++17 Release with active checks and strict warnings:

- PASS: configured/built `build/host`;178 registered CTests, including six LPI
  suites and its intentional Release failure-path test.
- PASS: separately configured/built tests-OFF `build/lpi30-consumer`; both CTests.
- PASS: 103 Python tests; ledger 97 registers/52 procedures/35 features; reference and
  local-link checks (1714 links); Doxygen 215 generated pages and two negative fixtures;
  deterministic version check; `git diff --check`.
- NOT RUN: SDK/controller and installed-package qualification; portable linking
  is the tested consumer layer here.
- NOT RUN: remote CI (prompt 71 only); physical Q1/Q2 including actual LPI line
  state, partner wake, traffic loss, nominal timing distributions and energy.

All stage 30 software outputs/local checks are complete. Commit/upstream equality
is recorded in the handoff to avoid a self-referencing commit loop. Existing
unrelated source gates remain open; host success never qualifies physical timing.
