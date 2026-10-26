# 30 - Implement bounded legacy-MAC LPI sessions

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

Prerequisites: [29](29-eee-negotiation.md), [28](28-power-states.md), [22](22-reconcile-recover.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver exact documented legacy-MAC EEE/TX LPI coordination as complete finite
owner-admitted sessions, including entry, status acquisition, exit and verified
restoration. Reuse EEE configuration and the existing operation engine; no
independent LPI task or implicit background control.

Reopen MB85RC timed wake and uncertain transition source/tests, PCA9555
primary-versus-cleanup evidence and TMP1x2 operation/result and diagnostic
restoration code. Inspect source and registered tests, not just wrappers.
Reopen the exact DP83826I legacy-MAC EEE recipe, LPI control/status and wait
definitions alongside the mapping/procedure evidence settled in task 29.

Specify application/MAC prerequisites, explicit transmit coordination, finite
session bounds and what the PHY does not buffer. A PHY LPI control cannot
guarantee the MAC stopped sending or retain frames on its behalf. Require the
selected owner's maintenance/admission contract and validate request values and
available cached EEE/link/power evidence before transfers. Any required live
precondition uses bounded observation reads before mutation; do not demand
zero-I/O proof of current hardware state. External MAC/frame scheduling remains outside
portable core.

Implement ordered entry, source-defined waits, status/evidence, exit and restore
using one operation slot and aggregate deadline. Capture only necessary owned
state, preserve exact command-neutral values and never split indirect sequences.
The bus lock is released before scheduled waits. Distinguish request accepted,
verified LPI state, link/partner evidence and successful exit; a local callback
return cannot prove an environmental condition.

Any accepted TX-inhibit/LPI mutation remains visible until source-supported exit/
restoration proves otherwise. Cancellation is bus-silent; subsequent cleanup has
its separate finite allowance. Primary test/transport/deadline failure and cleanup
failure remain independently reported. If cleanup is denied, times out or cannot
restore, explicitly report whether TX may remain inhibited and leave relevant
configuration untrusted. No automatic reset/retry or apparent success after
an uncertain exit.

Test independent literal entry/exit traces and legal setting combinations,
missing MAC admission and invalid prerequisites with zero I/O. Inject every
mutation/acquisition/exit/restore failure, including error after TX inhibit took
effect, cancellation at each effect boundary and never-valid status. Assert
maximum calls, no lock across waits, no stale result reuse, primary/cleanup
precedence and retained inhibited-state evidence after teardown attempts.

Compile an owner sequence demonstrating application coordination and both normal
and failed-exit handling. Document bounded software work separately from physical
partner wake/link timing and energy measurements. Opt-in LPI stays disabled unless
explicitly requested; EEE advertisement alone must not silently initiate this
disruptive legacy-MAC procedure.

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
