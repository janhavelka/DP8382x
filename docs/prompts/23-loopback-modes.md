# 23 - Implement all supported loopback modes

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

Prerequisites: [22](22-reconcile-recover.md), [4](4-silicon-diagnostic-ledger.md), [20](20-event-counter-acquisition.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver complete exact-part loopback configuration and finite owner-driven
sessions, including stop and verified restoration. Persistent configured
loopback and a finite diagnostic session need distinct semantics. Extend the
existing operation/result mechanism and cohesive diagnostic source; no new
scheduler or autonomous maintenance worker.

Reopen PCA9555 owner cleanup/result types and
`test_failed_cooperative_input_read_preserves_primary_and_cleanup_evidence`.
Inspect TMP1x2 `examples/common/Tmp1x2CliDiagnostics.cpp` restoration/
result handling and `tools/test_hil_tmp1x2_runner.py` cleanup cases.
These supply ownership precedents, not PHY loopback recipes.

Complete the source matrix for MII, PCS input/output, digital, analog,
reverse/far-end and every other documented loopback mode, with speed/interface,
companion writes, termination/peer prerequisites and restore effects. Procedure-only
addresses permit exact sourced recipes, never arbitrary reserved writes.
Generator/checker sessions are task 24; loopback itself must work completely now.

Resolve the SNLS647I speed-specific discrepancy: section 8.3.14.2 gives the
general BMCR.14/BISCR.2 recipe, while table 8-19 specifies
`BISCR[4:0]=00100` at 100BASE-TX and `00001` at 10BASE-Te.
Use the detailed speed-specific definition absent stronger exact-part evidence,
record the conflict and test both independent literal vectors. Do not copy the
100-Mbit recipe into the 10-Mbit path or block all loopback modes.

Validate capability, mode, interface, maintenance admission and finite-session
bounds before I/O. Capture only required owned state and explicitly handle/reject
an untrusted starting profile. Start, wait/acquire where applicable, stop and
verify-restore use one operation slot and aggregate deadline; release the bus
lock between polls. Persistent configuration extends reset/recover ownership.
No implicit loopback activation during initialization, passive status or recovery
except replay of explicitly requested persistent settings.

Cancel/timeout preserves accepted effects. Cleanup runs through later owner
execution under its finite allowance; primary and cleanup statuses remain
separate. A completed diagnostic with failed restore is not overall success.
Report what internal PHY path was selected without claiming MAC-frame, cable or
peer validation from a register write.

Test every supported mode/speed/interface with independent exact sequences,
unsupported zero-I/O rejection and invalid prior state. Fail/cancel after every
mutation including stop and each restore/readback. Assert transaction bounds,
stale-result protection, two sessions after consumption and unchanged unrelated
state. Compile a maintenance-owner example and document the complete mode matrix,
external fixture prerequisites and failed-restoration evidence.

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
