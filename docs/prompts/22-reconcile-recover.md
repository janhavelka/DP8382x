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

Deliver `startVerify` and reconciliation through existing `startInitialize`
procedures. Complete raw/external invalidation for implemented fields without
adding a recovery manager or a second apply/reset state machine.

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

Options select steps; the driver never escalates. `startVerify` performs no
semantic/command mutation; `startInitialize({NONE})` performs no reset. Verify
VOD_CFG3 under `FFC0` without writing; every initialization rewrites and verifies
`4A40`, including reset kinds that retained it. Insufficient evidence leaves
trust unverified. Reuse the same forced/crossover and reset procedures, with
explicit owner retry and profile changes.

Cancellation freezes its result and effects under OP-9. Restoration is a new
operation with its own token, result and budget. Successful `startVerify` never
clears cleanupOwed; stable readback cannot prove opaque recipe restoration.
Abandon preserves evidence and invalidates trust. Cable reconnect triggers no
recovery or policy change.

Test recovery after raw writes, partial apply, external reset and changed mode.
Inject failure at each verify/apply/reset/readback/cleanup boundary, including
accepted writes followed by timeout and mismatch then read failure. Test zero
budget, cancellation, stale token, pending result and contention; exact call
bounds and whole MMD atomicity must hold. Assert invalidation/cached getters
perform no I/O and healthy reads alone cannot revalidate a profile.

Compile an owner example showing unchanged intent, partial observations,
failed restoration and an explicitly admitted retry. Keep one restoration ledger
and one owner per stable field. Test interrupted reset/reapply here; temporary
diagnostic overrides and overlapping-field restoration belong to prompt 23.

## Concrete TI source inputs

Use the [TI reset/configuration failure audit](../reference/library-ti-core.md#initialization-reset-and-recovery-procedure-audit)
and current exact datasheet to define reconciliation inputs. A failed selector
write invalidates addressing certainty; reconstruct all MMD selectors on the
next explicit access instead of using a later data read as proof of the target.
PHYRCR hard reset `0x1F/0x8000`, restart `0x1F/0x4000` and BMCR PCS reset
`0x00/0x8000` require distinct trust/restoration treatment. Reapply the currently
owned `0x030E=0x4A40` setup on every initialization, even when reset retained it, and do not clear an
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
