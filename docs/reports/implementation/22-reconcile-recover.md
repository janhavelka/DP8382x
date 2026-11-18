# Prompt 22 - explicit reconciliation and bounded recovery

Date: 2026-11-15. Branch: `main`. Clean, fetched, synchronized entry checkpoint:
`fe539237f3a2e71078491c81c9ac2678ffad2f2a`. Final commit and upstream equality
are recorded in the handoff, avoiding a self-referencing commit.

## Rules, scope and actual prerequisites

Applied root/parent AGENTS, COMMON, prompt 22, CONVENTIONS, ENGINEERING,
EVIDENCE, FULL_COVERAGE, SOURCE_MAP, VERIFICATION and API OP-1 through OP-17.
Reviewed current README/architecture/variants/reference index and audit
G2-15/R3/R17 dispositions. The concrete inputs are passive health (21), complete
NONE/PCS/HARD/RESTART initialization (15/16), shared negotiated/forced profile
application (17/18) and the single collector/invalidation path (13/20).
No ledger procedure has implementationStage 22; verification is a software
operation over existing owned-field procedures, not a new silicon recipe.

The lead owns existing driver/configuration source, public header, operation
integration, CMake, example/consumer, guides/report/progress and Git. One owner
serializes all instance calls and each complete controller transaction. Starts,
cancel, invalidation and cached getters remain bus/clock silent. No locks span
polls/waits. Siblings/vendor/SDK sources remain read-only. Current no-workflow
first-execution policy is unchanged; remote CI belongs to prompt 71.

## Delivered behavior and maintainability

Added startVerify(OperationBudget, OperationToken) and OperationKind::VERIFY in
the same slot. It reads identity, a complete four-frame VOD transaction, then
reuses the existing mode-specific stable comparison steps. Successful plans are
6 frames/2 batches without desired profile, 12/8 for AN, and 11/7 for forced.
Forced never reads ANAR; no semantic write, command, negotiation wait, reset,
retry or escalation occurs. Only MMD selectors are written, in one ownership
scope. Every next explicit MMD operation re-addresses after selector failure.

Shared _beginOperation removes duplicated slot setup for the three actual
starts; it performs no admission or hardware action. The existing initialization
and configuration state switches remain the only procedure implementations.
Explicit startInitialize always rewrites and verifies VOD then replays validated
stored desired intent; NONE issues no reset. No separate recovery alias/engine.

SettingsSnapshot gains rawVod/vodValid, with the same historical capture meaning
as other raw fields. First mismatch and valid raw words survive later transport,
release, clock or deadline failure. The later failure is terminal; a complete
mismatch can publish a decodable observed profile without promoting trust.
Without desired settings a successful identity/VOD check cannot invent a
verified profile. A changed accepted mode withdraws old initialization context
and returns INVALID_CONFIG/DEVICE_INVALIDATED; declared identity mismatch keeps
DEVICE_MISMATCH. VOD mismatch withdraws mandatory initialization evidence.

Successful VERIFY can report all current stable fields proved, but never clears
cleanupOwed or changes its owner. No verification read failure creates a semantic
mutation obligation. Existing raw/external invalidation conservatively retains
desired/raw evidence, withdraws initialized/current identity/configuration
knowledge, and does not manufacture a managed MDIO failure. Unrelated successful
reads cannot restore trust. Abandon retains effects/history while invalidating
trust; successful external reset attestation follows existing OP-15.

Independent review found a valid-read edge: active BMCR reset must invalidate
readiness before returning a later release/deadline/clock error. This was fixed
in the shared valid-word capture block and tested. If transport succeeds,
VERIFY terminates BUSY without waiting; stable comparison ignores AN restart.
No physical simultaneity or external-writer detection is claimed.

## Fresh source and peer review

Three inherited capable agents handled source/peer audit, independent contract
review, and test implementation. Their scopes were disjoint; reviewers were
read-only and the test agent wrote only test/verify.cpp. Existing exact masks and
reset procedures were independently checked against original PDF text, with no
new source conflict requiring a changed silicon interpretation. The lead reviewed
all contributions. The test agent used build/verify22-tests; the final reviewer
used a separate build/verify22-review. No subagent edited siblings or Git.

All relevant sibling worktrees were clean:

| Repository / revision | Paths and adopted/adapted/rejected principles |
| --- | --- |
| TMP1x2 afd50889312f6e218a6a34eca687e3e8547d8b40 | include/TMP1x2/TMP1x2.h; src/TMP1x2.cpp invalidateDeviceState/dirty state; src/TMP1x2Configuration.cpp verify/apply; src/TMP1x2Operations.cpp; core/jobs/helpers/CLI tests and CMake registration. Adopt desired/observed separation and retained first mismatch despite later failed read. Adapt active invalidation to DP OP-15; reject sensor timing and duplicate recovery vocabulary. |
| PCA9555 8b461b2afe9910b5b15fe32cc049d52e3b498970 | include/PCA9555/PCA9555.h ObservedState; src/PCA9555.cpp shadow establishment/invalidation/verification/cleanup; registered ambiguous-write/verification/detach tests. Adopt staged observations, primary/cleanup retention. Reject using test_matching_verify_reconciles_an_ambiguous_full_commit_without_reapply to clear DP opaque-recipe obligations. |
| FieldCore-node c40ea8341edf8d665460b902cafefbe857a7f73f | include/TunnelMonitor/contracts/Health.h and DeviceMeasurement.h; src/devices/i2c/Ina228Module.h/.cpp; native INA228 module/CLI tests, suite_main.cpp and PlatformIO registration. Adopt lifecycle/presence/health separation and cached copyStatus without extra clock calls. Reject application absence thresholds as PHY identity proof; no dependency added. |

Agent guidance and actual rendering/runners were reopened: TMP CLI output and
tools/hil_tmp1x2_runner.py; PCA examples/common/HealthDiag.h and
tools/run_i2c_hil.py; FieldCore cached copyStatus and explicit refreshStatus.
Recovery decisions belong to the application. I2C pointer parking, sensor waits,
EEPROM-style polling and sibling CI policies are not PHY procedure authority.

Current [TI RTOS sources](https://github.com/TexasInstruments/ethernet-rtos-drivers/tree/94bf4343004732983b0b8e1eec123d88ff70453b)
remain at main commit 94bf4343004732983b0b8e1eec123d88ff70453b.
Reopened rtos_drivers/src/generic_phy.c and dp83826.c, both BSD-3-Clause;
no implementation was copied. Ignored intermediate errors, sampled reset RMW
and reset address/mask confusion remain rejected comparison behavior.

The current [official PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
is byte-identical SNLS647I August 2025, 3,366,322 bytes, SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Original tables 8-19 (pp54-55), 8-23 (57-58), 8-28/29/30 (60-62), 8-43 (71)
and 8-81 (85-86) establish the independently checked masks. VOD arithmetic is
F000|0800|0780|0040=FFC0; value 4/enable/value 4/enable gives
4000|0800|0200|0040=4A40. Thus 4A7F passes, while 8400 and 4A00 fail.
Independent tests use literal BMCR 3100, ANAR ADFF, CR1/CR2 0020, CR3 0060 and
PHYCR 8000/C000; no production register constants form their oracle.

## Acceptance, documentation and verification

[Reconciliation guide](../../reconciliation.md) records exact plans, current
intent/observation/identity/trust/effect table, admission, readback limits and
one restoration owner. Public Doxygen, API, architecture, health/access/init
and feature guides, README/CHANGELOG, test guide and progress are updated.
Version remains 0.1.0; supported variants, package/tool/SDK routes and source
ledger facts are unchanged. No new restoration ledger is introduced.

The portable consumer compiles/executes Reconcile.h: unchanged forced intent,
partial observations, failed NONE restoration, separate explicit retry and all
retained outcomes. Ten focused suites cover independent traces, every verification
frame/release boundary, reserved/unowned fields, mismatch followed by failure,
changed mode, reset evidence with later errors, zero budget, clock failures,
stale/pending tokens, cancellation, raw/external invalidation, cleanup owner,
contention and every reset/reapply frame failure followed by explicit recovery.
The test owner initially omitted the persistent hard-reset floor during retry;
its fixture scheduler was corrected to honor busQuietUntilUs, not the driver.

Final verification used Windows, GNU C++ 15.1, Ninja and C++17 Release, with
active CHECK assertions under NDEBUG and strict conversion warnings.

| Evidence layer / command | Result |
| --- | --- |
| `cmake -S . -B build/verify22 -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`, build, CTest | PASS: 108/108, including 13 intentional negative assertion probes and standalone public headers |
| Independent contract reviewer, separate `build/verify22-review` Release build | PASS: 108/108; final source, API, example and test review found no remaining issue |
| Ten focused verification suites, separate `build/verify22-tests` | PASS: 97,541 checks plus the intentional assertion probe |
| `cmake -S test/consumer -B build/verify22-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release`, build, CTest | PASS: 1/1, linked tests-OFF portable consumer including reconciliation and failed-restoration example |
| `python -m unittest discover -s test -p "test_*.py" -v` | PASS: 103 tests |
| `python tools/check_doxygen.py` | PASS: 159 pages and two negative fixtures |
| `python tools/verify_ledger.py` | PASS: 97 registers, 52 procedures, 35 features |
| `python tools/verify_references.py` | PASS: vendor payload hashes, extracts and local document links |
| `python scripts/generate_version.py check` | PASS: deterministic 0.1.0 metadata |
| `git diff --check` | PASS |
| SDK link / installed package consumer | NOT RUN: no SDK/package implementation changed |
| Remote CI | NOT RUN: first-execution prompt 71 owns it |
| Physical reset, restore, routing and timing | NOT RUN: no admitted fixture or measured trace |

The original PDF remains unchanged. Q5 hard-reset strap behavior and Q11 forced
routing/partner behavior remain physical gates; no host result closes them.
Verification proves the returned stable fields within its bounded read pass,
not simultaneous silicon state, link completion or absence of an external writer.
The final maintainability review confirms one operation slot, one owned-field
comparison path and the existing initialization/replay path, with fixed storage
and no new recovery manager, allocation, SDK dependency or automatic retry.
