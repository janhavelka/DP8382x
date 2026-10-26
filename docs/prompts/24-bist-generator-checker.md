# 24 - Implement complete BIST generator and checker sessions

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [23](23-loopback-modes.md), [20](20-event-counter-acquisition.md),
[4](4-silicon-diagnostic-ledger.md). Verify the concrete inputs below; retain unrelated
open gates under COMMON.

## Scope, current evidence and work

Deliver all documented BIST/PRBS generator/checker options as bounded diagnostic
sessions with trustworthy measurements and full stop/restore. Extend existing
diagnostics and operation/result types; do not implement an independent test
framework or use a single analog-loopback smoke case as feature completion.

Use the [BIST fact table](../reference/library-ti-features.md#loopback-and-bist-anchors)
and re-open SNLS647I tables 8-40/45/46. BISCR `[13:12]=0/1/2/3`
means off/single constant-data generator/checker-only/generator+checker;
bit 14 changes counter wrap policy, not packet-generation mode. BICSR1
`0x001B[7:0]` is IPG in four-byte units (`0x7D=500` bytes);
bit 15 snapshots/clears the error counter, whose sample is `[15:8]`.
BICSR2 `0x001C[10:0]` is packet bytes (`0x05EE=1518` default).
Test `0x807D` snapshot composition without corrupting IPG, saturated/wrapped
counts and BISCR lock/activity evidence at register `0x16`. Derive the analog
mode and result vectors independently from table 8-40. Select a finite owner
observation interval separately from any claimed silicon completion bound.

Reopen PCA9555 primary/cleanup evidence tests, TMP1x2
`Tmp1x2CliDiagnostics.cpp` result consumption and
`test_hil_tmp1x2_runner.py` terminal/cleanup cases. Inspect exact
SNLS647I BIST/PRBS/counter tables and applicable TI troubleshooting procedures.
Keep fixture requirements separate from evidence that traffic actually occurred.

Enumerate generator/checker selection, single/continuous behavior, packet length,
inter-packet gap, stimulus options, lock/synchronization, counter start/reset,
freeze/acquire/clear and supported loopback interactions. Give every legal
encoding and prohibited combination a typed setting or coherent configuration
helper. Reuse task 23's speed-correct loopback procedures rather than duplicate
their register programming.

Validate all settings, capability, maintenance admission and finite duration/count
before I/O. A hardware continuous mode still has a finite owner session and
explicit stop. Capture only needed owned state, handle untrusted starting settings,
execute ordered start/acquire/stop/restore through the same operation slot and
observe aggregate primary and finite cleanup budgets. No lock spans a wait.

Define activity, lock, measurement validity, counter width/overflow and expected
setup before deciding the result. Zero errors without evidence that the selected mode generated or checked its
required traffic, or without valid counter acquisition, is inconclusive. Select
mode-specific evidence: a single-packet generator may have gone inactive before
its first poll, while checker-only operation relies on admitted external stimulus.
Do not require generator-active and checker-lock for modes that lack one of them. Preserve raw counters and partial
evidence through the single collector; formatting cannot reread destructive
registers. Separate test outcome from measurement validity, primary transport/
deadline failure and cleanup status. A successful test with failed restore keeps
its measurements but is not a successful overall session.

Use independent literal traces for every generator/checker option and packet/IPG
boundary, plus lock gained/lost, absent activity, no counter sample, overflow and
stale data. Fail every callback boundary, accepted write, stop and restore phase;
cancel at each mutation. Verify exact maximum transfers, no retries, preserved
primary/cleanup details and untrusted configuration after unverifiable restore.
Consume and repeat a session to prove old counters/tokens cannot satisfy it.

Compile a bounded owner example and document setup, inconclusive outcomes,
counter semantics and internal-path versus physical-frame limitations. Do not
claim measured signal integrity or delivered application frames from simulated
counters. Future CLI/runner mappings remain planned until implemented.

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
