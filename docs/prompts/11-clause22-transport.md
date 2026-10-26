# 11 - Implement bounded Clause 22 transport

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

Prerequisites: [6](6-framework-ownership.md), [7](7-value-transport-contracts.md), [8](8-operation-contracts.md), [9](9-portable-source-foundation.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver a usable portable driver binding and direct Clause 22 reads/writes.
Own the existing Config/driver/register headers, corresponding source and focused
tests; add a source file only for a real responsibility.

Reopen PCA9555 `include/PCA9555/Config.h`,
`src/PCA9555.cpp::_mapTransportResult`, `_i2cWriteReadRaw` and
`_i2cWriteTracked`; compare MB85RC `src/MB85RC.cpp::_mapTransportResult`
and TMP1x2 `src/TMP1x2.cpp::read`, `write`, `track`. Inspect their
ambiguous-write, short-success and deadline tests and registration. Record which
byte/ACK rules are rejected for MDIO.

Implement the chosen terminal callback/transaction mechanism, `bind`,
`isBound`, local lifetime guards, `readClause22` and `writeClause22`.
Validate PHY/register addresses, readable/writable access and inputs before any
callback. Publish ordinary outputs only on complete terminal success. Borrowed
context/buffers cannot outlive callbacks or be retained/re-entered. Binding and
cached access are bus-silent; reject teardown that would abandon live users.

Account for lock acquisition and controller execution within the aggregate
allowance; measuring an overrun after return cannot prove bounded execution.
Normalize illegal/nonterminal callback statuses and impossible completion counts,
preserving full signed backend detail. Count actual attempted frames/transactions
at this boundary, separately from managed health. Failed or late writes retain
possible hardware effects without retry or cache rollback. Release ownership on
every exit; document primary versus release-error precedence.

Restrict this unit to source-proved safe Clause 22 access. Destructive reads and
mixed command/status mutations needing the collector are rejected until task 13;
do not ship placeholder MMD calls or advertise their support. This is a complete
direct-access subset, not a successful shell for later operations.

Test every address bound, local zero-I/O rejection, terminal-status normalization,
short completion, unchanged sentinel outputs, accepted-write-then-error,
deadline-before-lock, contention and release failure. Prove no automatic retries,
exact frame counts, two-instance isolation and no SDK dependency with a linked
portable consumer. Document the callback contract and a compiling direct-read
example that checks Status before using output.

## Concrete TI source inputs

Use the [TI core transport audit](../reference/library-ti-core.md#transport-and-mmd-procedure-audit)
and [timing table](../reference/library-ti-core.md#timing-silicon-limits-versus-library-policy).
TI `GenericPhy_readReg/writeReg` merely forward callbacks; they do not prove
address validation, deadlines or safe output publication. Clause 22 register
numbers are five bits and data is 16 bits: reject register `0x8000` rather than
truncating it to BMCR. Keep protocol PHY-address range distinct from the part's
three strap bits. SNLS647I 7.6/8.3.11 and table 8-20 give MDC 2.5 MHz nominal,
24 MHz maximum, a 500 ns minimum suppressed-preamble transaction gap plus the
specified MDC/MDIO idle edge, and preamble/readiness requirements. Carry these
into backend evidence; host callbacks cannot prove physical bus timing.
Recheck current source/datasheet and correct discrepancies before coding; retain
NOT RUN hardware timing evidence until admitted measured traces exist.

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
