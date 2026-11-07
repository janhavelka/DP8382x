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

Use SNLA428 table 2-1, the exact-part TDR note named by SNLS647I 8.3.16.1,
as the selected recipe under decision D3 and source conflict `tdr_recipe`.
Keep datasheet tables 8-56/57 as separate independent vectors and retain typed
controls capable of either setting set. Do not copy the note's copyrighted code.
Run 1A, 1B, 2, 3, 4, 5 in order, or an explicit caller subset; skipped runs
are NOT RUN, never zero-filled. Write addresses 0170, 0173, 0175, 0178, 0416,
0411, 0456, 001E in that order. Independent literal words are:

| Address | 1A | 1B | 2 | 3 | 4 | 5 |
| --- | --- | --- | --- | --- | --- | --- |
| 0170 | 5C12 | 5C12 | 5C22 | 5E32 | 5E42 | 5E52 |
| 0173 | 0D00 | 0D0C | 0D13 | 1A20 | 343A | 8F6E |
| 0175 | 1007 | 1007 | 1007 | 1007 | 100A | 100D |
| 0178 | 0002 | 0002 | 0002 | 0002 | 0002 | 0006 |
| 0416 | 1FA0 | 1FA0 | 1FA0 | 1FA0 | 1FA0 | 1F90 |
| 0411 | 0813 | 0813 | 0813 | 0815 | 0816 | 0816 |
| 0456 | 0608 | 0608 | 0608 | 0608 | 0608 | 0608 |
| 001E | 8102 | 8102 | 8102 | 8102 | 8102 | 8102 |

All words are hexadecimal. GENCFG 0456 is the explicit exception: RMW mask0600
preserves prompt 33's bit 3; with bit 3 clear the composed word is `0x0600`, unprinted
by TI and part of Q7. Save/restore GENCFG[10:9]. Procedure-only `0x0411`/`0x0416` have no
read/reset proof, so exit requires HARD and full initialization replay, including
EEE; restart retains them and cannot prove restoration. Trust stays unverified
until the reset/replay completes.

Reopen TMP1x2 `src/TMP1x2Operations.cpp::poll`, `finishJob`,
`takeResult`, PCA9555 timeout/cancel/cleanup assertions, TMP1x2
`tools/hil_tmp1x2_runner.py` fixture verdicts and 24Cxx
`tools/test_hil_identity.py`. Host configuration/fixture labels cannot
substitute for observed device evidence. Reconcile the original SNLS647I cable
tables and feature text; SNLA253 DP83822 and SNLA330A DP83825I sequences do not
become DP83826I programming recipes by family name.

Read CR1[8] before mutation and reject when autorun is enabled; prompt 26 adds
the disable/restore handoff. Read live CDCR and retain its word. If bit 14 is set,
end with an explicit error and no cable write; prompt 35 owns the required
clear-then-HARD contingency. A run completes only after start bit 15 reads clear
and done bit 1 is set under the freshness rule; reset and previous runs leave
done set. Retain fail bit 0 and then read0180/0185/018A. Keep unproved maximum
completion time/freshness claims explicit; use a finite owner deadline.

Implement documented A/B transmit/capture, same/cross-pair, averaging, DC,
threshold/pulse/shadow controls with legal encodings; averaging codes 0..6 mean
1..64 and7 is reserved. The selected six-run recipe is transmitA/same-channel
only. No derived result describes pairB or the whole cable. B/cross-pair raw
settings remain available without derived distance. Autorun belongs to26.

Validate request values, declared/cached capability and maintenance admission
before I/O. When link/peer conditions need live evidence, use bounded observation
reads before any diagnostic mutation; an external peer/fixture claim is not a
fact the PHY can necessarily verify. Capture required owned state, execute finite owner-driven
start/wait/acquire/stop/verify-restore and never test an active EtherCAT session.
The transaction lock is released between waits. Cancellation itself performs no
MDIO; subsequent admitted cleanup has a finite allowance and cannot replace the
primary result. Denied/failed restoration leaves uncertain effects visible.

Retain each non-destructive CDCR/0180/0185/018A read with per-word validity;
a failed read means invalid data, not history loss, and bounded re-read is legal.
Only prerequisite BMSR/PHYSTS/MISR reads use the collector. Reserved result
banks are not decoded peaks. Provide a pure SNLA428-only helper over retained
samples: search performed runs2,3,4,5,1B,1A; take the first amplitude0185&7F
above decimal10 (segments2..5) or24 (segment1). Use that same row's index0180&FF
and018A. Metres=abs((index-5)*8/(2*prop_dly)), with caller propagation delay
(ns/m): Cat5/5e5.35, Cat6 5, Cat7 4.6. Zero018A means open; nonzero means short.
Keep raw bits 11/5 separately without inventing bit 11 polarity. No qualifying
peak is NO_PEAK, never good cable or zero metres. Q7 qualifies physical accuracy.
Test the complete table 2-5 sample: rows 2..4 zero;5=(78,11,0),
1B=(0C,06,800),1A=(06,44,0) gives run5/open/100m within1e-9.
The note's code has indexing/undefined-variable inconsistencies; derive the
helper independently from the selected recipe and declared interpretation.

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
