# 25 - Implement complete manual cable capture

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

Prerequisites: [22](22-reconcile-recover.md), [20](20-event-counter-acquisition.md), [4](4-silicon-diagnostic-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Start the independent vectors from the
[cable field/segment table](../reference/library-ti-features.md#cable-controls-and-remaining-interpretation-gates),
then verify the current exact datasheet and procedure. CDCR `0x001E`
uses start/done/fail masks `0x8000/0x0002/0x0001`; reset `0x0102`
already contains done. CDSCR `0x0170[10:8]` codes 0-6 mean 1-64
averages; code 7 is reserved. Segment codes 1-5 have explicit duration, skip,
shadow and pulse settings; test segment 1 skip `0x07` versus reset `0x04`.
Documented first-peak fields are `0180[7:0],0185[6:0],018A[11,5]`;
reserved result banks do not establish five usable peaks. Maximum completion
time, fault conversion and distance calibration remain separate evidence gates;
do not invent them from another part's TDR routine.

Deliver the exact DP83826I manual cable diagnostic procedure, complete controls,
raw results, cancellation and restoration. Keep start/acquire/stop/restore in
this task; no half-feature diagnostic scaffold. Extend existing diagnostics and
the one operation slot. No TDR service or additional polling owner.

Reopen TMP1x2 `src/TMP1x2Operations.cpp::poll`, `finishJob`,
`takeResult`, PCA9555 timeout/cancel/cleanup assertions, TMP1x2
`tools/hil_tmp1x2_runner.py` fixture verdicts and 24Cxx
`tools/test_hil_identity.py`. Host configuration/fixture labels cannot
substitute for observed device evidence. Reconcile the original SNLS647I cable
tables and feature text; SNLA253 DP83822 and SNLA330A DP83825I sequences do not
become DP83826I programming recipes by family name.

Implement documented manual start, A/B transmit/capture, same/cross-pair capture,
segments, averaging, DC removal and threshold/pulse/shadow controls with all legal
settings. Derive link/mode/peer prerequisites, source waits, done/fail evidence
and clear/freeze order. Unresolved distance calibration cannot excuse omitting
documented raw controls/results. Hardware autorun integration belongs to task 26.

Validate request values, declared/cached capability and maintenance admission
before I/O. When link/peer conditions need live evidence, use bounded observation
reads before any diagnostic mutation; an external peer/fixture claim is not a
fact the PHY can necessarily verify. Capture required owned state, execute finite owner-driven
start/wait/acquire/stop/verify-restore and never test an active EtherCAT session.
The transaction lock is released between waits. Cancellation itself performs no
MDIO; subsequent admitted cleanup has a finite allowance and cannot replace the
primary result. Denied/failed restoration leaves uncertain effects visible.

Retain raw peak position, amplitude, polarity and cross-pair evidence with exact
validity. Derived open/short/normal/distance requires authoritative interpretation,
units, range, quantization and calibration evidence. Unknown/saturated/missing data
cannot become zero metres or good cable. Gate unresolved interpretations individually,
not all raw acquisition; unsupported requires exact-part absence evidence.
Coordinate destructive reads with the existing collector and preserve consumed
partial samples if a later read fails.

Test independent literal sequences and raw vectors including every documented
capture combination, legal extrema, invalid settings, done/fail/never-done,
missing/corrupt/saturated peaks and stale results. Inject failure/cancellation
at every acquisition/stop/restore boundary; assert sentinel outputs, per-source
validity, primary/cleanup status, exact bounds, collector contention and
exactly-once result consumption.

Write an actionable known-good/open/short/length fixture recipe with termination,
peer disconnects, manual interventions and calibration limits. Compile a raw
capture owner example. Physical distance accuracy remains NOT RUN/BLOCKED until
known-cable measurements; a planned fixture neither exists nor operates by prose.

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
