# Prompt 35: electrical controls and finite compliance procedures

Implemented source-admitted `analog_configure`, `collision_test`,
`compliance_session` and `calibration_recovery`. See the
[electrical guide](../../electrical.md) for public ownership, failure and fixture
contracts. Source-blocked branches remain explicit, not silently omitted.

## Entry, rules and ownership

Started on clean fetched `main` at
`a4ee2d4f18b12dc577ed52e75387ab936b8bfc3d`, equal to `origin/main`.
Read the full attachment/repository prompt, COMMON, shared conventions,
engineering/evidence/coverage/operation contracts, architecture/variants,
reference index and parent/root agent rules. Prerequisites 23/24/33/22 supply
the operation slot, diagnostics, shared forced profile, collector and restoration.
One application owner serializes every instance access and complete MDIO batch;
controller locks never span waits or application dwell. Siblings stayed read-only,
no vendor bytes changed, and no remote CI was dispatched or inspected.

The lead owns the public header, shared integration, ledger/guides, progress and
Git. Three parallel agents used the available capable model: `analog35_impl`
owned only analog source/tests; `compliance35_impl` owned electrical source/tests
and the portable example; `electrical35_sources` independently reviewed original
PDF arithmetic/access, siblings and final code, read-only. Disjoint writes and
independent build directories avoided shared-file conflicts. The source reviewer
used independent literals, not generated production masks, and the lead checked
contributions against actual code/diff before integration.

## Source and sibling audit

Original SNLS647I August 2025, SHA256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`,
tables 8-19/47/48/50/73/76/79..83/86 and section 8.3.11.6 establish the exact
fields, commands and access limits. SNLA266B April 2025, SHA256
`74af41cba6912b8734097332b7e766105feaa8fb015321f47d345b7596aa13d5`,
page 13 tables 5-1/2/4/5 supplies applicable DP83826 scripts. DP83822-only 5-3 is
inapplicable. Current targeted TI clarification search found no evidence closing
`ioctrl_example`, `analog_rx_gain` or `compt_reserved`; no vendor implementation
was copied and existing per-file source licensing/provenance remains unchanged.

Fresh sibling paths relative to Projects and clean revisions:

- TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40`:
  `src/TMP1x2Operations.cpp` finish/cancel/take semantics;
  `examples/common/Tmp1x2CliDiagnostics.cpp` diagnostic restoration and retained
  status; `test/jobs/test_jobs.cpp`, `test/cli/test_cli.cpp`,
  `tools/test_hil_tmp1x2_runner.py` and CMake registrations.
- PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970`:
  `src/PCA9555.cpp` primary/cleanup and ambiguous-write handling;
  `test/test_basic.cpp` registered cancellation, detach-refusal, shadow versus
  observation and uncertain apply cases; `examples/common/HealthDiag.h`
  cached rendering and fixture/parser checks. Relevant agent rules were read.

Adopt retained effects, token-owned results, distinct primary/restoration status,
explicit fixture verdicts and passive rendering. Reject I2C pointer cleanup,
byte/ACK assumptions, sensor delays and automatic diagnostic restoration as
PHY procedure authority. SDK/controller timing remains a separate layer.

## Implemented behavior and acceptance

`DP8382xAnalog.cpp` validates complete owned profiles, snapshots current words,
uses the legal low 8 bridge for split MDIX negative amplitude and verifies every
write before continuation. All 289 legal current/target endpoint pairs retain
legal intermediate codes, including accepted-error prefixes and restoration.
It exposes all legal negative/positive/zero DAC, termination/enables and slew
encodings; explicit gain requests and reserved examples reject before I/O.
The full analog pass is at most 70 frames/18 callbacks, read-only 22/6. Restoration
reobserves all owned words instead of trusting pre-reset reserved-bit snapshots.
Mandatory initialization 4A40 remains first; admitted termination overrides follow,
and VERIFY compares the owned desired value.

`DP8382xElectrical.cpp` uses the existing operation switch, HARD/restart waits,
shared forced profile and collector. It implements both manual pairs for 100M,
10M NLP and 10M PRBS scripts, all 14 COMPT10M patterns, explicit MII COL test and
conditional CDCR recovery. COUNT1000..31, including E/F, is representable but
rejects before 0428 access. Forced routing owns C000, preserves reserved/strap
fields and uses CR1 robust; BISCR uses 701F=7001 without importing script RO bits.
Live and desired power/isolation prerequisites prevent accidental entry.

Start/stop each have finite independent budgets and retained results. HARD stop
proves stopped/reset state but leaves persistent initialization owed. Collision
stop can discharge its own barrier only with unchanged unrelated proof. CDCR is
read once; high 14 admits adjacent clear/HARD writes in one batch. First-frame
failure stops the batch; accepted/possible clear retains a specific HARD debt,
and any attempted HARD retains the bus-wide quiet floor. Calibration accuracy
or completion is never inferred from reset completion.

The compiling `examples/portable/Electrical.h` demonstrates application-owned
entry, polling, finite dwell, explicit stop and later initialization. Documentation
covers termination/probes/reference conditions, source exceptions and failed exit.
No fixture was operated and no instrument result was fabricated.

## Independent review and corrections

Final independent read-only review found no remaining confirmed defect after
these integration corrections and their regression tests:

1. Retain accepted calibration HARD even when release/deadline subsequently fails.
2. Scope electrical effect tracking to its own kinds; clear no-effect cancelled
   sessions so later ordinary configuration cannot inherit false stimulus state.
3. Require the requested HARD to complete before stop finishes; an already-active
   PCS/restart cannot substitute for it.
4. Retain reset observations from all new prerequisite positions before aggregate
   failure, invalidating initialization and capabilities immediately.
5. Prevent NONE/PCS/RESTART from clearing calibration HARD-required debt.
6. Retain contradictory BMCR mode evidence instead of preserving old VERIFIED
   link settings during analog/collision work.
7. Compare interleaved observed analog words against the proved snapshot/bridge/
   final phase, stopping dependent writes after detected drift. Keep shared raw
   VOD evidence current after explicit tuning.

The ledger's old automatic post-failure HARD continuation was reconciled with
OP-9 and the frozen stop-on-first-failure batch contract. Its analog cleanup
bound now accounts for re-snapshotting every owned word. These are contract
corrections; unresolved silicon facts remain unresolved.

## Verification and maintainability

Windows, GNU C++15.1.0, Ninja, C++17 Release, warnings-as-errors:

- PASS: configure/build `build/host`, all 229 registered CTests, including active
  assertion failure probes and complete previous-stage regression suites.
- PASS: separate `build/electrical35-ubsan`, all 229 CTests with undefined-behavior
  sanitizer and trap-on-error.
- PASS: tests-OFF `build/electrical35-consumer`, both linked registered consumers;
  the maintenance example compiles without SDK dependencies.
- PASS:103 Python unit tests, `verify_ledger.py`, `verify_references.py`,
  deterministic version check and `git diff --check`.
- PASS: Doxygen 264 generated HTML pages, local assets/links and two deliberately
  rejected negative fixtures.
- NOT RUN: SDK/controller link and physical timing/compliance/calibration/EMC.
  No SDK code is introduced; later adapter/package stages retain those checks.
- NOT RUN: remote CI, reserved for prompt 71 under the first-execution rule.

Tests inject every primary/stop/replay frame failure, accepted-write timeout,
release failure, malformed receipt, readback mismatch, cancellation boundaries,
contention, short/zero slice, frozen clock, overflow, regression and never-clearing
HARD. They assert literal words, adjacency, exact counts, unchanged ownership,
retained samples/effects, no retry and bus-silent cached/local rejection paths.

The final design adds two cohesive source responsibilities and fixed values to
the existing facade. It introduces no allocation, generic register/job framework,
SDK headers, clock, logger, per-register classes or parallel restore engine.
README/CHANGELOG, API/lifecycle/coverage/source guides, vectors and progress were
updated. Version/manifests/licenses need no change: this is unreleased feature
work with no imported code. Commit and upstream-equality evidence is recorded
in the final handoff to avoid a self-referencing report commit.

The later [independent prompts 34-35 audit](34-35-independent-audit.md) corrects
combined-owner replay, external-reset reconciliation, reset-only session
classification and interleaved reset handling. It records separate verification
without changing this original checkpoint's historical evidence.
