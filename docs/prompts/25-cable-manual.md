# 25 - Implement complete manual cable capture

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [22](22-reconcile-recover.md), [20](20-event-counter-acquisition.md),
[4](4-silicon-diagnostic-ledger.md). Verify the concrete inputs below; retain unrelated
open gates under COMMON.

## Scope, current evidence and work

Deliver the exact DP83826I manual cable diagnostic procedure, complete controls,
raw results, cancellation and restoration. Keep start/acquire/stop/restore in
this task; no half-feature diagnostic scaffold. Extend existing diagnostics and
the one operation slot. No TDR service or additional polling owner.

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

Reopen TMP1x2 `src/TMP1x2Operations.cpp::poll`, `finishJob`,
`takeResult`, PCA9555 timeout/cancel/cleanup assertions, TMP1x2
`tools/hil_tmp1x2_runner.py` fixture verdicts and 24Cxx
`tools/test_hil_identity.py`. Host configuration/fixture labels cannot
substitute for observed device evidence. Reconcile the original SNLS647I cable
tables and feature text; SNLA253 DP83822 and SNLA330A DP83825I sequences do not
become DP83826I programming recipes by family name.

Prove which observations establish completion of this request: an already-set
CDCR done bit is insufficient. Account for a start that completes between write
and first poll without requiring an unobservable busy transition; retain an
explicit freshness gate if the exact silicon provides no adequate distinction.

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

Update public Doxygen, procedure citations, affected API/user/test/tool guides,
coverage, stage report and progress with this behavior. Review README/CHANGELOG
and record relevant no-change reasons. Run the focused registered host tests
above with active Release checks, affected portable-consumer and Doxygen checks,
`python tools/verify_references.py` and `git diff --check`.

Apply COMMON's final maintainability review and evidence reporting. Keep source
gaps, SDK/package results and physical qualification separate. The lead commits
and syncs this complete task and coherent verified units on the same execution
branch, verifies upstream equality, and reports any sync failure. First-set
remote CI runs and status checks belong only to prompt 71.
