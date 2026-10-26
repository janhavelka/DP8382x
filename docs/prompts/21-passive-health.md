# 21 - Implement passive driver health and exact accounting

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [20](20-event-counter-acquisition.md), [11](11-clause22-transport.md),
[15](15-initialize-pcs-reset.md), [7](7-value-transport-contracts.md). Verify the
concrete inputs below; retain unrelated open gates under COMMON.

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

## Concrete TI source inputs

Recheck the [TI transport/error audit](../reference/library-ti-core.md#transport-and-mmd-procedure-audit)
and [link audit](../reference/library-ti-core.md#negotiation-forced-modes-and-observations)
against current source. A vendor MMD read/write has four frames; its first three
successful selector/address writes do not establish target success. The source
RMW comparison supplies fail-at-every-frame cases. Count attempted frames,
logical operation status and uncertain target effects independently.
Successful PHYSTS with link bit0 clear is link-down with healthy MDIO; stale
caller state cannot become fresh link evidence.
Use literal failure traces and prove health getters perform zero I/O. Record
actual backend/physical evidence separately; never infer wire health from mocks.

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
