# 16 - Implement hard reset, restart and external-reset reconciliation

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

Prerequisites: [15](15-initialize-pcs-reset.md), [3](3-silicon-core-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Extend the existing lifecycle with complete PHYRCR hard-reset and restart
procedures and bus-silent external-reset/POR notification. This task owns each
kind's full completion, restoration and failure behavior; hardware GPIO/power
operation stays with the application.

Reopen TMP1x2 reset/recovery operation phases, PCA9555 primary/cleanup and detach
tests, TCA9548A bus-silent `end`, and their public lifecycle contracts.
Recheck SNLS647I BMCR/PHYRCR/strap/reset sections and independent reset vectors.
The vendor source's reset bit mask is not a register address.

Maintain a reset-effects table with trigger, source-defined terminology, minimum
wait, completion evidence, retained/invalidated settings, identity/mode effects,
strap resampling and restore ownership. Distinguish BMCR PCS reset, PHYRCR hard
reset, PHYRCR restart, external reset and POR, including POR-only straps. No
ambiguous Boolean reset option or inferred interchangeable behavior.

Implement each newly supported software kind through task 15's operation slot,
deadlines/tokens/results and shared register procedures. Validate before I/O,
issue the correct command-neutral composition once, wait cooperatively and verify
completion under one aggregate primary deadline. Release transaction ownership
before waits. Re-read/reselect identity when effects require it; stale mode-derived
capabilities cannot survive a reset that invalidated them.

Restore validated desired owned state in exact reset-specific order even if the
current hardware image was untrusted. Verification compares stable owned fields,
not self-clearing commands/status. New configuration/features later extend this
same restore path rather than creating alternate reset implementations. Explicit
external notification records uncertainty and changed knowledge without touching
pins, resetting hardware again or performing implicit recovery.

Cancellation before/after each command and incomplete restoration preserve
effects and independent primary/cleanup statuses. Cleanup has the existing finite
allowance; denied/abandoned cleanup remains visible. Local teardown cannot claim
hardware stopped. A callback/adapter user cannot outlive its borrowed resources.
Neither a successful management read nor a reset clears unrelated prior failures
or proves the desired profile restored.

Use independent literal traces per reset kind. Test every trigger, completion,
restore and readback failure, accepted-write timeout, exact waits, wrap, frozen
clock, never-completing command and zero budget. Assert strap/identity/configuration
differences, external notification's zero callbacks, cancellation/end/rebind
evidence and contention without lock retention across waits. Compile the owner
examples and extend the lifecycle/reset-effects guide with exact application
responsibilities and currently supported restoration scope.

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
