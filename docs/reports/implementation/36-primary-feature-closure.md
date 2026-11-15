# Prompt 36: bounded primary-chip integration review

The bounded review and focused corrections are delivered. **Full primary-chip
closure remains BLOCKED** by required source-gated branches, particularly
`power_deep`. This stage does not reconcile every field or replace prompt 55's
row closure. The maintained [feature matrix](../../feature-coverage.md) remains
the single implementation matrix; no new feature catalogue was created.

The later [independent audit](36-independent-audit.md) records additional
quarantined-link and own-batch reset corrections against this delivered stage.
The verification below remains the original stage checkpoint's evidence.

## Entry, ownership and method

Started from clean, fetched `main` at
`2f64d3018179e86a11e9e747eecb1d13e75989aa`, equal to `origin/main`.
Read the complete attachment and repository prompt, COMMON, conventions,
engineering, evidence, coverage, API/OP contracts and repository/parent rules.
Prerequisites supply the ledger, one collector, terminal batch transport and
working typed procedures from prompts 11-35. No ledger procedure assigns
`implementationStage == 36`; this stage owns review and identified corrections.

The lead owns source/header integration, tests, the report, shared guides and Git.
Names remain namespace/class `DP8382x`, existing `readClause22`, `readMmd`,
`readEeeStatus` and current snapshots. One application owner serializes all
instance access and complete controller transactions. No new lock, clock,
collector, framework or retry path was introduced. Vendor bytes and read-only
siblings were preserved. No workflow definitions exist yet; remote CI remains
prompt 71 only.

Three independent read-only agents used the current capable model, splitting
difficult domains rather than repeating the whole inventory:

- `closure36_transport`: transport/MMD/alias admission, bounds and partial
  collector/IRQ evidence; independent reproductions in its own ignored build.
- `closure36_restore`: shared persistent replay, temporary overlays, ownership,
  result lifetime and pin conflicts; independent combined-owner probes.
- `closure36_silicon`: original-PDF arithmetic/access, critical procedures,
  public declarations, literal tests and physical/source claim boundaries.

The lead verified both production findings against actual code and traces and
made all tracked edits. The transport reviewer then reread the final source and
test diff. Original-source literals, not production masks, supply the new reset
vectors. Existing registered tests were inspected before adding a single suite.

## Source and sibling evidence

Original SNLS647I August 2025 retains SHA256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Reviewed feature text and detailed tables through the original PDF alongside
the [TI catalogue](../../reference/library-ti-features.md). Samples include
tables 8-19/28..32/36..40/50/52/53/72/73..78/85/86/89..97 and EEE sections
8.3.3/4. SNLA266B April 2025 retains SHA256
`74af41cba6912b8734097332b7e766105feaa8fb015321f47d345b7596aa13d5`;
applicable compliance sequences remain distinct from neighboring-part recipes.
No numeric correction, source-gate closure or vendor-code import was justified.

Fresh sibling source, agent rules, registrations and cached/live CLI inspections
used clean revisions and these paths relative to Projects:

- TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40`:
  `include/TMP1x2/Config.h`, `Status.h`, public operation/snapshot declarations,
  `src/TMP1x2Operations.cpp`, `src/TMP1x2Configuration.cpp`,
  `test/jobs/test_jobs.cpp`, common CLI/diagnostic source and
  `tools/test_hil_tmp1x2_runner.py`.
- PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970`:
  public Config/transport declarations, `src/PCA9555.cpp` transport mapping and
  cleanup tracking, registered ambiguity/deadline/cleanup cases in
  `test/test_basic.cpp`, `examples/common/HealthDiag.h` and
  `tools/test_run_i2c_hil_parser.py`.
- INA228 `4de84f2564eb6382835ca9a72d8688377657dcb4`:
  public DiagAlert declarations, `src/INA228.cpp` readDiagAlert/capture,
  `test/test_basic.cpp`, cached/live diagnostics in
  `examples/esp_idf/basic/main/main.cpp` and parser tests.

Adopted explicit completion/effect evidence, separate primary/restoration results
and passive rendering. Rejected I2C byte/ACK/pointer cleanup, sensor conversion
timing/scaling and automatic CLI restoration as PHY procedure authority.

## Findings and focused fixes

1. **Reset observations were feature-dependent.** `_retainEvidence` only applied
   active-reset invalidation while quality, analog or electrical work was active.
   A successful raw BMCR `9000` in IDLE left readiness/capabilities valid. During
   IRQ configuration, the same observation allowed six subsequent semantic writes
   and a successful result. Common retention now handles every valid direct or
   vendor-alias BMCR/PHYRCR observation. Interleaved reads freeze the running
   operation before dependent work; own reset polls keep their bounded phases.
   RESTART retains configuration, while PCS/HARD invalidate it. A raw observation
   does not claim command acceptance, create a software HARD floor or clear debt.
   Writes to retained result identity/strap fields are limited to RUNNING, so
   RESULT_PENDING remains historical.
2. **EEE quarantine rejection left apparently fresh data.** A malformed raw
   receipt could quarantine the controller after a valid EEE sample; the rejected
   next refresh retained `stale=false` and OK status. The existing failure path
   now retains BUSY, marks stale and withdraws negotiated validity, preserving
   raw words/time and leaving caller output unchanged without I/O.
3. **Coverage narrative was obsolete.** The matrix introduction still described
   post-23 procedures as PENDING despite its 24-35 implementation entries. It now
   distinguishes admitted implementations from required gated branches. The
   power row and progress explicitly retain incomplete DPD ownership. Progress
   rows 32/33 also incorrectly said NOT STARTED despite their implementation,
   reports and tests; corrected them to source-admitted completion with gates.

Registered `verify_raw_reset_observation` covers 48 combinations of direct/MMD,
IDLE/RESULT_PENDING, PCS/HARD/RESTART and successful/release/late/clock-regression
outcomes. It checks output sentinels, current trust, retained desired intent,
frozen identity/strap results and exact frames. `quality_ownership` adds IRQ
interleaving before/after effects through both aliases. `eee_acquisition_failures`
adds quarantined refresh, old timestamp/raw retention and zero additional I/O.

## Representative integration dispositions

These are review findings against existing matrix IDs, not another coverage map:

- `access`, `reset`, `interrupts`, `counters`: reserved-zero REGCR and complete
  MMD re-addressing, indivisible RMW, aggregate admission, release evidence,
  finite tokens/results, immediate partial MISR retention and pending half-full
  service requests agree with registered tests. Raw aliases share access and
  invalidation. RECR remains raw; FCSCR saturation is lower-bound evidence.
- `initialization`, `negotiation`, `next_page`, `crossover`, `link`: mandatory
  VOD and replay, all forced speeds/duplexes, robust/manual coupling, owner-opted
  extended duplex, prohibited local ANAR fault, finite page service and transient
  NP ownership remain explicit. Forced resolution uses BMCR with the extension
  uncertainty rule; PHYSTS remains raw rather than proof of partner duplex.
- `loopback`, `bist`, `cable`, `eee`, `lpi`, `compliance`, `calibration`: separate
  finite starts/stops and retained debt reuse shared restoration. Temporary
  overlays do not become persistent intent. Supplemental probes passed MII,
  digital and reverse loopback with analog/pin/WoL/IRQ/power ownership, and
  analog plus enabled EEE replay for all four reset requests, exactly one HARD
  each. Existing registered suites cover failure boundaries. No recursive EEE
  reset recipe or replayed stimulus was found. CDCR recovery does not prove
  calibration; D3 derived cable output remains recipe/unit-qualified.
- `power`, `wol`, `led`, `pin_mux`, `clock`, `mii`, `rmii`, `compatibility`:
  isolation and software power intent remain independent; active-sleep policy
  differs from state. Bidirectional pin conflicts and optional activation remain
  guarded. Original LED field placement/selector width, reserved protection,
  odd-nibble polarity, RXFCFG1 W0C and MAC ordering match literal tests.
- `quality`, `analog`: raw MSE has no invented SNR, analog transitions remain
  legal, and unknown GPIO/IOCTRL interpretations stay gated. Original EEE timing
  columns are NOM; applicable FLD figures are MAX with conditions. Neither is a
  software timeout or an executed physical measurement.

All newly persistent groups were checked in the existing shared restoration
order: EEE recipe prefix, mandatory VOD/link profile, saved diagnostic fields,
autorun, interface, quality, analog, pins, WoL, IRQ and power. Related fields use
their established owner composition/collector. Unrelated successful reads do
not establish trust. No second restoration ledger or register procedure was added.

## Required remaining acceptance and downstream use

`power_deep` returns to prompt 28: obtain exact-part safe 0428 composition and
preservation rules, complete exit order and SMI/clock readiness. Then implement
and test finite entry, exit, cancellation/uncertainty and separately retained
restoration across every write/clock-loss boundary. A zero-I/O rejection is a
correct guard, not completed support. The owning prompt now records this return.

Existing source-blocked branches also remain with their owners: prompts 32/33
need exact per-pin/mode corrections and activation evidence for `pin_fields`;
prompt 35 needs IOCTRL example reconciliation, legal receiver-gain encoding and
100M 0428 access/composition/stop evidence. Their exact next-evidence records
remain in the ledger. After clarification, owners must deliver the whole legal
procedure and failure/restoration tests before prompt 55 can close those rows.
MSE, RECR, TDR, reset straps and timing/physical questions retain their named gates.

Reviewed future mappings: native lifecycle/transport/state in 37-42, typed CLI
and cached rendering in 43-47, artifact/protocol/fixture runners in 48-52, and
physical campaigns in 72-73. Existing examples/portable consumers do not establish
those SDK/CLI/runner surfaces. Stage 37 can use the implemented transport,
initialization/reset/configuration/link subset independently of DPD and other
unrelated gated branches. This single-stage request does not execute stage 37.

## Verification, documentation and checkpoint

Windows, GNU C++ 15.1.0, Ninja, C++17 Release with warnings-as-errors. The shared
retention fix affects every terminal batch, so all existing C++ suites were run
once after the final edits; unchanged Python suites were not repeated.

- PASS: `cmake --build build/host`; all 230 registered CTests via
  `ctest --test-dir build/host --output-on-failure --no-tests=error`, including
  active Release assertion probes and the new cross-feature suite.
- PASS: separate `build/electrical35-ubsan` build and all 230 CTests with
  undefined-behavior sanitizer/trap mode.
- PASS: tests-OFF `build/electrical35-consumer` build and both registered linked
  portable consumers. Reused directory names do not imply an older binary result.
- PASS: `python tools/check_doxygen.py`, 264 generated pages, local links/assets
  and two rejected negative fixtures; public observation comments were updated.
- PASS: `python tools/verify_ledger.py` (97 registers, 52 procedures, 35 families),
  version check, reference/hash/local-link checks and final diff checks.
- NOT RUN: unchanged Python test suites, installed/exported package checks
  (69-70), SDK/controller link (37-42), remote CI (71) and all physical campaigns.
  All 35 feature records still have hardware qualification NOT RUN; no measured
  trace/capture was available or fabricated.

Reviewed README/CHANGELOG, public/API contracts, direct/EEE/test guides, feature
matrix, owning prompt and progress; updated affected descriptions. Architecture,
numeric ledger/vectors, variants, version/manifests and source licensing need no
change: this adds no new feature/variant/encoding or dependency. The native example
plan now names the selected unmodified-SDK public lifecycle route explicitly.
Final maintainability review retained small shared corrections and existing test
fixtures, with no speculative refactoring or alternate execution path. The final
handoff records the commit and verified upstream equality without a self-reference.
