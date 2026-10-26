# 19 - Implement live and cached link observations

Read [COMMON](COMMON.md), [ENGINEERING](ENGINEERING.md),
[CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Use [SOURCE_MAP](SOURCE_MAP.md) for current
source locations; historical reports do not replace a fresh audit.

At entry record owned paths, applicable rules, current source revisions/dirty
state, selected names, one MDIO owner, local checks and the recorded execution
branch. Resume that branch. Use disjoint implementation/test work or independent
read-only review when useful; choose available models/effort by task complexity
and record the rationale. The lead owns shared contracts, integration and Git.
Siblings remain read-only.

Prerequisites: [18](18-forced-crossover-settings.md), [17](17-negotiated-link-settings.md), [13](13-side-effect-register-access.md), [4](4-silicon-diagnostic-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver `readLinkStatus`, passive `linkSnapshot` and retained change
evidence usable by a later adapter. Extend the existing collector/driver or one
cohesive status source. No notification scheduler, SDK callbacks or rendering
belongs in core.

Reopen TMP1x2 `readSnapshot` and `RegisterSnapshot`/
`SettingsSnapshot`, INA228 `SettingsSnapshot` and actual
`readDiagAlert`, plus FieldCore cached `copyStatus`. Inspect their
tests and CLI rendering. Staged software publication is useful; sensor freshness
and CONFIG bookends cannot prove simultaneous PHY sampling.

Specify the exact read plan from SNLS647I with latch-low/current/destructive
semantics, ordering and validity dependencies. Keep current link separate from
consumed historical loss evidence through task 13's single collector.
Distinguish down, negotiating, resolved, forced, stale/unknown observation and
MDIO failure. Numeric defaults never imply valid speed/duplex. Old values after
failure keep old time with explicit invalidity/staleness.

Resolve speed/duplex from exact mode evidence. PHYSTS forced-mode ambiguity
remains a named gate if stronger official evidence does not resolve it; retain
documented raw evidence without inventing resolution. Derive RX/TX pause from
both advertisements and documented ANAR/ANLPAR/PHYCR semantics, valid only under
the required full-duplex negotiation conditions. Forced, half-duplex or incomplete
negotiation cannot imply resolved pause.

Define coherence if link changes between reads. Use a fixed maximum for any
resample or publish uncertainty; never retry until stable. Preserve each consumed
sample immediately even when aggregate acquisition later fails. Task 20 expands
the event/counter acquisition surface; this task must already honor shared
collector ownership and history loss.

Define first-valid, changed, repeated, stale-failure and regained-validity
evidence without duplicate event consumption. Portable applications drive
sampling; the later IDF adapter follows the selected single polling/notification
owner. Cached getters and rendering do no I/O. A valid 10/half link may fail
application 100/full policy without becoming MDIO failure. DHCP/IP/EtherCAT and
machine readiness are not PHY snapshot fields.

Test literal latch-low/current-up/down and negotiation transitions, forced
resolution/gates and independent pause truth-table vectors. Change link between
every read; fail every phase and verify no falsely current aggregate output.
Test zero/missing/wrapped timestamps, age, repeated readers, reset invalidation,
change evidence and cable-down successful transport accounting. Add a compiling
cached-status example and a guide table separating link, freshness, transport
and application policy.

## Documentation, verification and checkpoint

Ship accurate public Doxygen and procedure citations with this behavior: units,
legal states, ownership, output validity, I/O effects, deadlines and cleanup.
Update API contracts, affected guides, restoration/feature evidence and the stage
report/progress. Review README, CHANGELOG and relevant example/test/tool guides;
update affected claims and record why others need no change. Future APIs and
physical qualification remain explicitly unimplemented/unverified.

Run registered focused host tests with active Release checks, relevant portable
consumer/source-documentation/link checks, the reference checker and
`git diff --check`. Record actual commands and separate PASS/FAIL/NOT RUN/BLOCKED
for host, SDK, packages and hardware where relevant. Missing source evidence
blocks the affected requirement; it cannot become unsupported silicon or close
a required gate. Continue independent eligible work.

Review the final diff for simple control flow, stale comments, duplicated
procedures and undocumented errors. Commit and sync checked tests/docs/code on
the same execution branch after this task and each coherent verified unit.
The lead fetches, safely integrates, pushes and verifies upstream equality;
report exact sync failures. First-execution remote CI dispatch, polling and
waiting belong only to checkpoint **71**, never this task.
