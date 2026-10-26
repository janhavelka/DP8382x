# 21 - Implement passive driver health and exact accounting

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

Prerequisites: [20](20-event-counter-acquisition.md), [11](11-clause22-transport.md), [15](15-initialize-pcs-reset.md), [7](7-value-transport-contracts.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver four-state passive health over the transport/effect accounting already
implemented. Own health projection and its tests; do not move frame accounting
away from the actual callback boundary or add an offline admission policy.

Reopen TMP1x2 `src/TMP1x2.cpp::track`, `healthSnapshot` and
public `HealthSnapshot`; compare PCA9555 `_updateHealth`,
`_i2cWriteCleanupTracked` and
`test_input_read_failures_survive_successful_cleanup_in_health_and_validity`.
Inspect INA228's actual `HealthPolicy` branches and 24Cxx busy-poll
accounting. Their differences need explicit adopted/adapted/rejected decisions;
busy EEPROM NACKs have no MDIO analogue.

Use `DriverState::{UNINIT, READY, DEGRADED, OFFLINE}`,
`driverState`, `healthSnapshot` and the frozen field widths/names.
Document initialized separately from health; READY does not prove link, IP,
verified configuration or application safety. OFFLINE reports repeated managed
MDIO failure and never silently blocks the owner's next attempt.

Implement the complete transition/accounting table for binding, initialization,
managed success/failure, local rejection, raw/probe diagnostics, cleanup,
cancellation, reset, invalidation and end. Count logical transactions separately
from constituent frames and chip counters. Successful early frames followed by
one failed MMD frame cannot produce a successful logical outcome. Local BUSY,
IN_PROGRESS, invalid requests, no link and pending negotiation are not physical
transport failures; actual cable-down reads still count as successful activity.

Successful cleanup must not hide the primary failure or clear its consecutive
failure evidence. Cleanup activity/errors remain visible in their appropriate
domains. Preserve full last-error detail across later success and saturate all
selected counters. Zero timestamps are valid when flagged; unavailable time
has explicit invalidity. If `resetStatistics` is part of the frozen contract,
it clears only documented cumulative counts, never trust, pending results or
active failure evidence.

Use table-driven tests for every transition/threshold and actual counter width,
independent literal frame/transaction totals, saturation, last-error retention,
timestamp zero/missing/wrap and two instances. Test primary failure plus both
successful and failed cleanup, raw/probe traffic, no-I/O cached views, cable
disconnect and application-policy mismatch. Prove OFFLINE still admits an
otherwise valid explicit operation and unrelated reads cannot establish
configuration recovery.

Document the transition table and an application mapping example that reads
cached health without I/O. No retry timer, recovery manager, background task
or automatic power cycle belongs in this task.

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
