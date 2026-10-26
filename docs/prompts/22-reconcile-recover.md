# 22 - Implement explicit reconciliation and bounded recovery

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [21](21-passive-health.md), [16](16-hard-reset-restart.md),
[18](18-forced-crossover-settings.md). Verify the concrete inputs below; retain
unrelated open gates under COMMON.

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
when the current hardware image is untrusted. Verify documented stable readable
owned fields independently by readback. For non-readable recipe state, require
source-defined completion evidence; unprovable restoration remains untrusted.
Keep retries and desired-profile changes explicit application decisions.

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
retry policy. New feature tasks must extend this single restoration ledger. Give each stable
field one configuration owner and a reset/power dependency. Temporary diagnostics
override that owner only for the admitted session; restore the persistent profile
without recursively invoking reset-bearing setup or restarting prior test traffic.
Test an overlapping-field restore and an interrupted reset/reapply sequence.

## Concrete TI source inputs

Use the [TI reset/configuration failure audit](../reference/library-ti-core.md#initialization-reset-and-recovery-procedure-audit)
and current exact datasheet to define reconciliation inputs. A failed selector
write invalidates addressing certainty; reconstruct all MMD selectors on the
next explicit access instead of using a later data read as proof of the target.
PHYRCR hard reset `0x1F/0x8000`, restart `0x1F/0x4000` and BMCR PCS reset
`0x00/0x8000` require distinct trust/restoration treatment. Reapply the currently
owned `0x030E=0x4A40` setup where reset scope requires it, and do not clear an
earlier partial-configuration error because one later read succeeded. Preserve
source conflicts and hardware gates through recovery; compare admitted real
reset/restore traces without inventing a fallback sequence or a successful fixture.

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
