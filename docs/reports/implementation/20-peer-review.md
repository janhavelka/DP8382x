# Prompt 20 sibling and collector review

Reviewed 2026-11-10 on Windows/PowerShell. Scope: read-only sibling conventions
and the prompt-entry collector; this report is the only owned writable file.
The lead owns headers, implementation, integration, progress and Git. The current
capable model was retained for the bounded source/API review; no model substitution
was needed. This is source review, not execution or physical evidence.

Entry: branch `promptset/dp8382x-implementation-20261028`, commit
`0d176b555c7fb35c1836041d9b7499ac5e9d5304`; working and staged trees were clean
when inspected. Parent `Projects/AGENTS.md`, repository `AGENTS.md`, COMMON,
prompt 20, CONVENTIONS, README, architecture, variants, reference README and
relevant API contracts were reopened. The first-execution remote-CI exception
still assigns remote dispatch/status/wait to prompt 71. No sibling was fetched,
modified, built, committed or pushed by this reviewer.

## Current sibling identities

All paths below are relative to `C:/Users/Honza/Documents/Projects`.
`git rev-parse HEAD` and `git status --short` were read during this review.

| Sibling | Current commit | Worktree |
| --- | --- | --- |
| INA228 | `73f954838d3fac8cce747d54d0ad738940d4c625` | Clean |
| TMP1x2 | `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb` | Clean |
| PCA9555 | `414e0f66b13082e974ec27a4a6e86985a65d83fd` | Clean |

All three `AGENTS.md` files were inspected. Applicable shared principles are
external serialization and transport ownership, bounded terminal callbacks,
passive cached views, explicit partial-state evidence, and framework-independent
C++17. Sibling health/admission policies and hardware effects are not transferred
as PHY facts.

## INA228 destructive evidence

Inspected `INA228/include/INA228/INA228.h` (`DiagAlert`, `DiagAlertSnapshot`,
`readDiagAlert`, `readDiagAlertRaw`, acknowledgement), `INA228/src/INA228.cpp`
(`readDiagAlert`, `_readDiagAlertTracked`, `_captureDiagAlert`), and
`INA228/test/test_basic.cpp`, including the named cases
`test_public_read_diag_alert_is_destructive_and_preserved`,
`test_get_settings_is_bus_silent_and_does_not_consume_diag`,
`test_tick_preserves_diag_alert_evidence_when_polling_cnvrf` and
`test_diagnostic_acknowledgement_clears_matching_snapshot_evidence`.
These tests appear in that file's `RUN_TEST` registrations.

The typed live read parses a fresh word only after a successful transport read.
One internal capture route preserves diagnostic bits even when readiness polling
caused the read. Cached diagnostic history ORs event evidence, while latest
configuration/MEMSTAT bits are refreshed. DiagnosticEvents separately retains
latest raw, sticky events and first-observed timestamps for newly retained bits.
Acknowledgement is an explicit software history action; it is not another
hardware acquisition. Reset/recovery and accumulator-reset retention differ
according to documented chip semantics.

The Arduino `INA228/examples/01_basic_bringup_cli/main.cpp` and native
`INA228/examples/esp_idf/basic/main/main.cpp` both implement `printDiagSnapshot`
through `getDiagAlertSnapshot` and explicitly label it cache-only. Their live
diagnostic routes call `readDiagAlert` separately. `INA228/platformio.ini` registers
the native Unity environment, and `.github/workflows/ci.yml` runs
`pio test -e native`. Inspected Python paths are
`INA228/tools/run_i2c_hil.py` and `INA228/tools/test_run_i2c_hil_parser.py`:
finite commands and explicit missing/NOT RUN results are useful precedents;
their sensor-specific restore defaults and protocols are not PHY policy.

Adopt central capture and separate cached/live routes. Adapt sticky event
retention with explicit coalescing and possible lost-history flags. Reject
representing the DP83826 latest raw word as an OR of different acquisitions:
latest raw and historical event union need separate fields. Also reject copying
INA228's timestamp ambiguity: DP8382x already has explicit per-source validity
and a conservative completion bound. INA228 does not establish the DP83826
counter width, overflow behavior, read-clear masks or reset effects.

## TMP1x2 staged observations

Inspected `TMP1x2/include/TMP1x2/TMP1x2.h` (`RegisterSnapshot`,
`SettingsSnapshot`, `readSnapshot`),
`TMP1x2/src/TMP1x2Configuration.cpp:readSnapshot`, and
`TMP1x2/test/helpers/test_helpers.cpp:snapshotFailureEvidence`.
The implementation fills a local snapshot through five reads, then publishes
only after successful CONFIG bookends. Tests fail each of the five reads and
retain `0xDEAD`/`0xBEEF` caller sentinels. Independently learned dirty/format
evidence survives a later read failure. Raw/decoded/trusted temperature and
timestamp validity are separate. The header explicitly states that bookends do
not make an atomic device snapshot or detect unseen intervening changes.

Inspected CLI files `TMP1x2/examples/common/Tmp1x2Cli.cpp`,
`Tmp1x2CliDiagnostics.cpp` and `Tmp1x2CliOutput.cpp`: cached settings/sample
rendering is separate from explicit live diagnostics. `TMP1x2/CMakeLists.txt`
registers `helpers`, `cli`, the other executing native suites and `hil_runner`
with the native CLI fixture. `.github/workflows/ci.yml` actually configures,
builds and executes CTest, separately from SDK links. Inspected
`TMP1x2/tools/hil_tmp1x2_runner.py` (classification, restoration and bounded
transcript retention) and
`TMP1x2/tools/test_hil_tmp1x2_runner.py`: missing/unverified restoration,
transcript failure, bounded artifact capacity and JSON finalization are separate
failure cases, not silently successful cleanup.

Adopt successful-only aggregate publication and unchanged error outputs.
Adapt retained dirty evidence into per-source retained destructive samples.
Reject adding destructive read bookends merely to imitate snapshot consistency:
a second PHY read can clear newly arrived events. Keep the fixed acquisition
order and describe observation times rather than claiming simultaneous hardware
state. Keep protocol counters separate from transport and managed health.

## PCA9555 primary and cleanup evidence

Inspected `PCA9555/include/PCA9555/PCA9555.h:OperationResult`,
`PCA9555/src/PCA9555.cpp` input-read and pointer-park phases, and
`PCA9555/test/test_basic.cpp`, particularly
`test_failed_cooperative_input_read_preserves_primary_and_cleanup_evidence` and
`test_input_read_failures_survive_successful_cleanup_in_health_and_validity`.
Those tests are registered with `RUN_TEST`. The first read's timeout remains the
primary result when pointer cleanup also fails; explicit cleanup status and
requirement remain observable. A proven no-attempt read does not create cleanup,
whereas a possibly committed command does. Successful cleanup cannot erase the
failed read from health or establish input validity.

Inspected `PCA9555/examples/common/HealthDiag.h`: rendering accepts an existing
SettingsSnapshot. `PCA9555/platformio.ini` defines native Unity tests and
`.github/workflows/ci.yml` runs them. Inspected
`PCA9555/tools/run_i2c_hil.py` and `PCA9555/tools/test_run_i2c_hil_parser.py`:
positive finite command timeouts, explicit unsafe-command admission, hard errors
dominating expected rejections, and cached snapshot invariants are concrete
tested runner principles. Address scan response is explicitly not identity proof.

Adopt the independent primary/cleanup evidence and no-attempt versus possible
effect distinction. Do not copy the PCA9555 pointer-park operation or its
after-deadline cleanup scheduling: DP8382x has its own terminal batch release
status and OP-9 separately admitted restoration contract. PHY counter reads
cannot undo a destructive effect by rereading or successfully releasing the lock.

## Entry collector gaps and integration recommendations

At the entry commit, `include/DP8382x/DP8382x.h` already defines EvidenceSource,
EvidenceSample and EvidenceSnapshot. `src/DP8382x.cpp:acquireEvidence` constructs
fixed batches and publishes caller output only after success.
`_retainEvidence` preserves validated prefix words before aggregate failure,
keeps raw/one-union/zero-union separate, and records ambiguous consumption in
historyLost. `src/DP8382xLink.cpp` uses this same collector. Preserve those owners.

1. Add typed source interpretation over the existing retained samples, with
   MISR high-byte history separated from low-byte enables. Known event fields
   should use independent literal tests; ambiguous MISR2 polarity/WoL remains
   raw unless owned selector provenance permits interpretation. Do not add an
   RXFS acquisition merely to make this decoder appear definite.
2. FCSCR currently sums low bytes but lacks a hardware-saturation lower-bound
   flag. Keep arithmetic saturation separate from a saturated chip sample and
   from historyLost. A successful zero after saturation cannot repair old count
   exactness. RECR raw acquisition is useful; its entry raw-word sum cannot
   become a qualified receive-error total while width/saturation is unresolved.
3. Make software retention policy explicit. A bounded sticky union coalesces
   occurrences and cannot count interrupt multiplicity. Either explicit masked
   acknowledgement or explicitly non-consuming lifetime retention is coherent;
   ordinary cached readers must not consume it. If acknowledgement is exposed,
   it must be bus-silent and must not reset counters or heal historyLost.
4. Define reset epochs separately from history consumption. Preserve old raw
   samples with stale/epoch provenance, or begin a documented new observation
   epoch without silently mixing earlier sums with newly reset hardware. Normal
   configuration writes need not invent a new hardware counter epoch. An
   external reset notification cannot establish that missing counter history
   was zero. A fresh valid sample can refresh current raw evidence while history
   remains incomplete.
5. Keep per-source timestamp validity and complete-acquisition result separate.
   Last successful aggregate output may contain historical samples outside the
   selected mask; callers must not treat it as an atomic snapshot. Per-source
   words already consumed remain readable after a later read or release fails.
   Preserve the first failure and separately exposed controller release status.
6. Exercise MISR1 success then MISR2 failure; each phase failing/no-attempt;
   after-effect timeout; events arriving between reads; repeated acquisitions;
   zero after nonzero; FCSCR 0/0x7F/0x80/0xFE/0xFF; RECR raw 0x0100/0xFFFF;
   reset staleness; repeated cached readers; MMD-selector contention; timestamps
   and aggregate sentinels. Bounds must count actual MDIO frames, including four
   frames for one indirect source. No retry/drain loop or lock across waits.

These recommendations describe entry gaps and acceptance conditions, not a
second proposed API owner or a claim that final integration remains deficient.
Exact-part silicon conclusions and original PDF table inspection belong to the
parallel source review; peer code supplies conventions only.

Verification for this contribution: source review PASS; Markdown whitespace
check PASS. Host behavior, portable/package consumer, SDK link, remote CI and
physical measurements NOT RUN by this reviewer. The lead records integrated
verification and final commit/upstream synchronization.

## Integrated code review

Reopened the prompt-20 working diff after implementation, specifically
`include/DP8382x/DP8382x.h`, `src/DP8382x.cpp`, `test/events.cpp`,
`test/access.cpp`, `test/consumer/main.cpp`, `examples/portable/Events.h` and
CTest registration in `CMakeLists.txt`. This review found no blocking
correctness issue in the supplied implementation.

`collectEvents` delegates to `acquireEvidence`; it introduces a typed complete
aggregate rather than another bus-reading or retention owner. The six-source
order is fixed and fits one batch. New aggregate output is assigned only on
complete success, while validated prefix samples remain available even after
an error, release failure, clock fault or deadline. Cached copies and formatting
perform no bus or clock calls. A later collector read makes the previous complete
aggregate historical; reset/invalidation also preserves it with explicit stale
status. Original clock-domain timestamp validity is distinct from freshness.

`clearEvidence` is a bus-silent explicit host-history discard. It refuses callback
reentry, polling and a running operation, and preserves configuration trust,
identity, link, transport and pending operation results. The next hardware read
can include events from before this local clear, as the public contract states.
Discontinuous retained history and stale raw values provide the reset policy;
there is no false claim of a hardware counter clear or globally numbered epoch.

RECR remains a raw returned word and its old generic sum no longer advances.
FCSCR distinguishes silicon saturation, arithmetic saturation, possible missing
history and discontinuity. These reasons cannot be healed by a later zero sample;
the typed total remains a lower bound where necessary. Event history excludes
MISR enable bits, retains bounded unions and labels unavailable multiplicity.

The five new executing suites plus their expected-failure assertion check are
registered. Read-through confirms literal event-bit and FCSCR boundary vectors,
all six failure positions, after-effect loss, sentinel outputs, prior aggregate
retention, release-versus-primary results, competing MMD admission, callback
reentry, two cached renderers, local clear/reset, late completion and malformed
timestamps. The lead reported 84 Release tests passing; this reviewer did not
independently execute that build and does not substitute this report for its log.

One focused coverage suggestion was sent to the lead: directly assert the new
ANER, 10BTSCR and RXFS `eventBits` projections with literal words. Existing access
tests exercise those sources and raw retention, while the new event-vector loop
directly covers MISR1, MISR2 and FLDS. This is a small test improvement, not a
request for another collector or an identified behavior defect. Arithmetic
UINT64_MAX/sample-count exhaustion is not claimed to have been reached by these
finite acquisition tests. Silicon counter and physical error-rate qualification
remain separate gates.

Integration follow-up: the test owner added direct ANER/10BTSCR/RXFS projection
cases and raw-only RCSR checks. The lead added literal UINT32/UINT64 boundary
tests against the same private arithmetic helpers used by the collector.
The earlier coverage observations describe the reviewed intermediate version;
final command results are recorded in the stage report.
