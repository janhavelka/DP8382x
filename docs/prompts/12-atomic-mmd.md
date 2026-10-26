# 12 - Implement complete atomic MMD transactions

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

Prerequisites: [11](11-clause22-transport.md), [3](3-silicon-core-ledger.md), [6](6-framework-ownership.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver `readMmd` and `writeMmd` through the exact DP83826I Clause
22 indirect mapping, including safe stable-field read/modify/write. Keep one
register procedure implementation; no native Clause 45 claim or alternate
transaction mechanism.

Reopen PCA9555 transport/cleanup wrappers and failure tests,
MB85RC callback completion accounting, and TMP1x2 timing contracts/source.
Inspect SNLS647I REGCR/ADDAR tables and independently derive literal read/write
traces; production masks are not their own test oracle.

Validate supported device selectors, address holes, operation and masks before
I/O. A manufacturer procedure-only address authorizes its named recipe, not
arbitrary raw writes. One application-controlled ownership scope covers selector,
address and data frames and the full stable-field RMW. Every participant,
including other PHY addresses and future SDK/CLI paths, must honor it. Distinguish
acquiring ownership from execution inside an existing transaction so mediator
callbacks cannot recursively lock.

Pass the remaining aggregate allowance to each terminal callback. A cooperative
budget either admits the entire transaction or yields before starting; it never
releases an indirect sequence halfway through. Release before any later scheduled
wait. Keep frame attempts, logical outcome, selector effects and target mutation
distinct. Partial failure leaves selector uncertainty; re-establish the complete
address sequence on the next explicit operation. A later read cannot retrospectively
validate a previous ambiguous target write.

Use fixed storage and staged ordinary read output. Preserve primary/release
statuses and accepted effects. Generic RMW is limited to stable readable writable
fields; mixed/destructive access remains task 13's source-specific procedure.
No automatic retry, nested lock, per-PHY lock registry or retained caller buffer.

Inject failure at every selector/address/data phase, including writes accepted
before error, then prove a subsequent call re-addresses fully. Attempt competing
access between every frame and across two PHY addresses; demonstrate exclusion
for both MMD and RMW. Test zero budget, minimum remaining allowance, deadline
between frames, invalid statuses/counts, sentinel output, all exit releases and
exact counts. An admitted indivisible sequence must never be divided by a poll
budget. Add a complete trace and portable consumer example to the transport guide;
keep actual SDK/controller timing separate from host protocol evidence.

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
