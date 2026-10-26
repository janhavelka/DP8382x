# 22 - Implement explicit reconciliation and bounded recovery

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

Prerequisites: [21](21-passive-health.md), [16](16-hard-reset-restart.md), [18](18-forced-crossover-settings.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver stable-owned-bit reconciliation and `startRecover` through the
existing reset/configuration verification procedures. Complete raw/external
invalidation semantics for all implemented fields. Do not create a recovery
manager or duplicate the apply/reset state machine.

Reopen TMP1x2 `invalidateDeviceState`, configuration dirty/verify/apply
source and operation declarations; compare PCA9555 `ObservedState`,
shadow establishment/invalidation and cleanup-failure tests. Inspect FieldCore's
lifecycle/presence/health separation as a read-only consumer precedent, not a
new library dependency.

Define a current evidence table for desired settings, observed stable fields,
identity/mode, configuration trust and remaining hardware effects. Raw write,
external writer/reset and ambiguous managed writes invalidate affected knowledge
without automatically classifying transport failure. Bus-silent
`invalidateDeviceState` records this fact; successful unrelated MDIO/probe
cannot restore trust. End/reset/cancel cannot erase unresolved accepted effects.

Implement explicit reconciliation reads and comparisons against validated owned
desired settings. Compare only stable source-supported masks, not commands,
latches or unowned fields. Publish verified trust only after complete proof;
retain mismatch evidence if a later read fails. Partial observation is not
atomic silicon state and cannot validate an entire profile.

Admit recovery with the existing token, one-operation slot, maintenance/lifetime
rules and aggregate deadline. Select required verify/reapply/reset steps from
source/current evidence; reuse the exact procedures including 10BASE-Te and
forced/crossover dependencies. Valid desired settings can support recovery even
when the current hardware image is untrusted. Restoration claims require
readback proof; no retries, guessed rollback or quiet desired-profile mutation.

Cancellation retains accepted effects. Deferred cleanup has its explicit finite
allowance and independent status; successful cleanup cannot replace the primary
failure or falsely recover managed health. Abandoned/denied cleanup leaves
remaining effects and untrusted configuration observable. Cable reconnect does
not start recovery or change speed policy. The application decides scheduling,
backoff and whether disruption is allowed.

Test recovery after raw writes, partial apply, external reset and changed mode.
Inject failure at each verify/apply/reset/readback/cleanup boundary, including
accepted writes followed by timeout and mismatch then read failure. Test zero
budget, cancellation, stale token, pending result and contention; exact call
bounds and whole MMD atomicity must hold. Assert invalidation/cached getters
perform no I/O and healthy reads alone cannot revalidate a profile.

Add a compiling bounded recovery owner example and a guide showing unchanged
desired intent, partial observation, failed restoration and application-owned
retry policy. New feature tasks must extend this single restoration ledger.

## Concrete TI source inputs

Use the [TI reset/configuration failure audit](../reference/library-ti-core.md#initialization-reset-and-recovery-procedure-audit)
and current exact datasheet to define reconciliation inputs. A failed selector
write followed by a successful data read in TI's unchecked RMW does not prove
the intended target; reconstruct all MMD selectors on the next explicit access.
PHYRCR hard reset `0x1F/0x8000`, restart `0x1F/0x4000` and BMCR PCS reset
`0x00/0x8000` require distinct trust/restoration treatment. Reapply the currently
owned `0x030E=0x4A40` setup where reset scope requires it, and do not clear an
earlier partial-configuration error because one later read succeeded. Preserve
source conflicts and hardware gates through recovery; compare admitted real
reset/restore traces without inventing a fallback sequence or a successful fixture.

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
