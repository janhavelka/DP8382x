# 12 - Implement supported loopback and BIST

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-11.

Read [CONVENTIONS](CONVENTIONS.md) and restate the applicable stage rules before
coding. Re-audit live sibling source; use simple engineering language, source
Doxygen, affected guides and local checks. Keep the same execution branch and
coherent-unit/per-prompt commit+sync; first-set remote CI belongs only to 29.

## Fresh audit and owned work

Inspect explicit mutating selftests, finite diagnostics and restoration in
PCA9555/MB85RC/TMP1x2; keep their pin/memory/sensor effects out of PHY procedures.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Concrete source pass and implementation boundaries

Read [CONVENTIONS](CONVENTIONS.md). Reopen PCA9555 owner results and cleanup,
especially `test_failed_cooperative_input_read_preserves_primary_and_cleanup_evidence`
in `test/test_basic.cpp`. Read
`TMP1x2/examples/common/Tmp1x2CliDiagnostics.cpp` result consumption/restoration
and `tools/test_hil_tmp1x2_runner.py` terminal/cleanup cases. Adapt bounded work
and truthful restoration; no sibling supplies PHY loopback sequences or proves
that BIST passed. Review the actual assertions, not only matching test names.

Extend the existing operation engine and diagnostic values. A cohesive
`src/DP8382xDiagnostics.cpp` can own loopback/BIST and later cable procedures
if separation is warranted; no production worker or generic test framework.
Use stage-04 typed configuration/results. If a required procedure lacks a frozen
declaration, settle one clear `start...` operation there before coding and retain
`pollOperation`, `cancel`, `takeResult`. Persistent configured loopback and a
finite diagnostic session need distinct semantics, not synonymous setters.

## Decisions and independent test oracles

Resolve the precise speed-specific MII-loopback instruction: SNLS647I section
8.3.14.2 gives a general BMCR.14/BISCR.2 recipe, while table 8-19 specifies
`BISCR[4:0]=00100` at 100BASE-TX and `00001` at 10BASE-Te. Use the detailed
speed-specific definition absent stronger exact-part evidence, record the
discrepancy, and test independent literal vectors for both speeds. Do not copy
the prose recipe unchanged into the 10-Mbit path or block all loopback modes.

1. Expand the ledger to a mode matrix: MII, PCS input/output, digital, analog,
   reverse/far-end and documented generator/checker options; exact supported
   speed/interface, companion writes, termination/peer, counters and restore.
   Resolve SNLS647I/troubleshooting conflicts. Procedure-only addresses allow
   exact source-backed recipes, not arbitrary writes into undocumented space.
2. Validate the full request before I/O: capability, maintenance admission, mode,
   finite duration/count, packet/IPG bounds and fixture assumptions. Hardware
   continuous mode still needs a finite owner session and explicit stop path.
3. Capture only needed owned state; explicitly handle or reject an untrusted
   starting profile. Start, wait/acquire, stop and verify-restore use the existing
   one-operation slot. Cancel/timeout retains accepted effects; later restoration
   uses the separately defined finite cleanup allowance.
4. Define counter start/reset, active/lock/sync evidence, latch/freeze/clear order,
   widths and overflow. Zero errors without activity and valid acquisition is
   inconclusive. Result formatting cannot reread destructive counters. Poll/IRQ
   and requested diagnostics share the existing collector.
5. Separate primary test outcome, measurement validity and cleanup Status. A
   successful test with failed restore is not a successful overall session;
   retain measurements and uncertain effects. Restore only captured owned state
   and verify stable source-supported bits, not every observed status register.
6. State internal PHY path coverage versus MAC loopback, external peer/cable and
   application frames. No implicit activation during bind, initialize, passive
   status or recovery. No allocation, background loop or platform delay in core.

Use independently derived literal traces for every mode and generator/checker
option, length/IPG boundaries, lock gained/lost, absent activity/sample and
counter overflow. Test zero-I/O mode rejection. Fail/cancel after every mutation,
including stop and each restore phase; assert bounds, counts and primary/cleanup
precedence. Repeat after consumption and prove no stale counters/token satisfy
the next operation. Simulated counters are host protocol evidence, not measured
signal integrity or successful physical frame delivery.

## Documentation shipped in this stage

Doxygen configuration/results with mode combinations, units, counter validity,
disruption, bounds and cleanup. Extend the diagnostics guide with the complete
mode matrix, compiling owner example, fixture requirements and restoration-failure
results. Ledger entries map to future CLI/runner stages without claiming those
are implemented. Update README, CHANGELOG, API contracts and validation; run
Doxygen generation/warning/link checks. Keep one execution branch and coherent-
unit/per-prompt sync. First-set remote CI runs/status checking remain at 29.

## Work

Enumerate and implement all documented loopback and BIST/PRBS operating options,
including generator/checker selection, single/continuous behavior, packet/IPG
controls, lock/synchronization, counter freeze/clear and applicable stimulus
controls. A single analog-loopback smoke test does not complete this feature
family. Resolve exact register/recipe interactions and fixture limitations.

Implement only exact DP83826I loopback/BIST modes whose register sequences and
prerequisites are documented. Make capability, link/interface restrictions,
duration/count bounds and result meaning explicit. Distinguish internal PHY test, MAC
loopback and actual cable/frame tests; do not imply a PHY BIST validates the application
frame path.

Require owner admission into maintenance before disturbing an active link. Capture owned
configuration, run bounded steps through the operation mechanism, collect counters once,
and restore/verify the applicable state. Stop/cancel/deadline failure records both
primary outcome and cleanup outcome, with configuration left untrusted if restoration
cannot be proved.

Document BIST counter widths, reset/clear/overflow rules, expected test setup and which
outcomes are inconclusive without a peer. Never label lack of errors during an unstarted
test as PASS. No hidden test activation during initialization or passive status.

Keep live/destructive diagnostic reads behind the single collector. Surface results for
CLI/runner use without adding rendering code to core. Cite exact source sections and
retain unresolved field interpretations as blocked implementation rows and open
completion gates. Use unsupported only where exact silicon/mode evidence proves
the feature is absent; missing evidence cannot close a required capability.

## Acceptance and handoff

Test mode gating before I/O, exact start/stop sequences, deadline/cancel at every
side-effect boundary, no-counter-data cases, wrap/saturation per source and restoration
errors. Independently verify literal sequences. Host tests establish state/protocol
behavior only; physical signal/path checks remain stage 30.

Expected output: Bounded loopback/BIST with precise result and cleanup semantics.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
