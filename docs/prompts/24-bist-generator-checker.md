# 24 - Implement complete BIST generator and checker sessions

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

Prerequisites: [23](23-loopback-modes.md), [20](20-event-counter-acquisition.md), [4](4-silicon-diagnostic-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Use the [BIST fact table](../reference/library-ti-features.md#loopback-and-bist-anchors)
and re-open SNLS647I tables 8-40/45/46. BISCR `[13:12]=0/1/2/3`
means off/single constant-data generator/checker-only/generator+checker;
bit 14 changes counter wrap policy, not packet-generation mode. BICSR1
`0x001B[7:0]` is IPG in four-byte units (`0x7D=500` bytes);
bit 15 snapshots/clears the error counter, whose sample is `[15:8]`.
BICSR2 `0x001C[10:0]` is packet bytes (`0x05EE=1518` default).
Test `0x807D` snapshot composition without corrupting IPG, saturated/wrapped
counts and BISCR lock/activity evidence. Reject the troubleshooting guide's
RCSR lock address and analog `0x3108 -> 0x3B04` expectation; one second in
its procedure is not a guaranteed completion bound.

Deliver all documented BIST/PRBS generator/checker options as bounded diagnostic
sessions with trustworthy measurements and full stop/restore. Extend existing
diagnostics and operation/result types; do not implement an independent test
framework or use a single analog-loopback smoke case as feature completion.

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
setup before deciding the result. Zero errors without valid active traffic or
valid counter acquisition is inconclusive. Preserve raw counters and partial
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
