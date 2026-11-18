# Verification and explicit restoration

`startVerify(budget, token)` checks the current identity, mandatory VOD setup
and any validated desired link profile through the existing operation slot.
It requires initialization and fresh accepted identity. Admission is bus/clock
silent; each `pollOperation` executes at most one complete admitted batch.
`takeResult` delivers its immutable result once. The application owns scheduling,
serialization, retry and any decision to restore or reset.

The [native owner](esp-idf.md#configuration-public-reconciliation-and-raw-entry)
adds a separate public SDK cache step after successful core configuration or
restoration. `reconcileSdkState` acknowledges verified settings through stopped
SDK controls; it does not rerun this chip procedure or rewrite its retained
result. Raw/external mutation withdraws both hardware proof and SDK agreement.

Verification changes no semantic register or command. Its VOD read writes only
the three indirect selector/address frames, under the same controller ownership
as the data read. It does not write VOD, restart AN, wait for negotiation, clear
events, issue reset or escalate after a mismatch. Forced profiles never read
ANAR. This finite pass is evidence collected at different times, not an atomic
silicon snapshot: the owner must exclude other writers and report external
reset/power changes through `invalidateDeviceState`.

## Exact bounded read plan

The plan is identity (two C22 reads), VOD_CFG3 (one complete four-frame DEVAD 31
read), then ANAR/CR1/PHYCR/CR3/CR2/BMCR for negotiated intent or
CR1/PHYCR/CR3/CR2/BMCR for forced intent. There are no hidden retries or resamples.

| Owned desired profile | Successful callbacks | Physical frames |
| --- | --- | --- |
| None | 2 | 6 |
| Negotiated | 8 | 12 |
| Forced | 7 | 11 |

The original aggregate deadline and remaining frame allowance govern every
callback. A poll slice smaller than the whole next batch yields before starting
it. Exhausting the aggregate budget or the frozen-clock stall bound terminates
the operation. Ownership never spans polls or scheduled waits. Even after a
selector failure, the next explicit MMD access establishes all three selector
frames again; a later data word cannot certify a previous ambiguous write.

Comparisons reuse the existing configuration masks and composition functions.
Only stable, active owned fields participate. AN intent includes the IEEE
selector, legal technologies/pause, prohibited local fault clear and persistent
NP clear. Forced intent ignores inactive advertisement. Automatic crossover
does not compare the preserved manual-routing bit. BMCR restart and unowned
fields are excluded. VOD compares `FFC0` with `4A40`, or the explicitly owned stage 35 termination override; reserved low bits remain
unknown/unowned. The ledger and [independent vectors](register-vectors.md) own
the field catalogue; this guide does not create another register map.

An observed active BMCR reset is a validity failure, not a stable-bit mismatch.
It withdraws initialization and returns BUSY/OPERATION_ACTIVE without waiting.
If release or timing also fails, that primary error wins while the consumed
reset evidence still invalidates readiness. Accepted identity changing between
BASIC/ENHANCED withdraws the old initialization context and returns
INVALID_CONFIG/DEVICE_INVALIDATED; the fresh accepted identity remains visible.
An actual declared-part/mode/revision mismatch retains DEVICE_MISMATCH instead.

The shared initialization path also retains independently returned active
BMCR/PHYRCR reset evidence before any release/timing error. PCS/HARD withdraw
readiness and settings trust; RESTART retains them and interrupts link/history.
The original error and any earlier driver-issued effects remain separate.

## Current evidence and ownership

| Evidence | Establishment | Invalidation/retention |
| --- | --- | --- |
| Desired profile | Complete validated `startConfigure` admission | Raw/external changes, cancellation and failed restoration never replace it. No implicit fallback profile. |
| Observed decoded profile | Complete successful stable read pass, including a decodable mismatch | Partial/failing passes keep the old tuple/time as stale. A complete mismatch can publish a fresh observed tuple without promoting trust; undecodable active policy remains stale. |
| Raw stable words | Each independently returned word, including new `rawVod`/`vodValid` | Retained on subsequent failure. Per-word validity is historical capture, not proof that all words share one epoch; aggregate trust/staleness governs the profile. |
| Identity/mode | Exact-address accepted two-word read | Reset, external mutation and conservative raw-write invalidation withdraw current permissions. Changed/rejected identity invalidates prior settings context. A raw reprobe cannot reinitialize or verify settings. |
| Configuration trust | Complete matching identity/VOD/desired-field pass | Verification admission marks an existing desired profile DIRTY/stale. Partial, cancelled or mismatched passes cannot restore trust. Without desired settings, a successful mandatory-only check leaves profile trust unverified. |
| Initialization | Complete existing initialization procedure | PCS/HARD effects, external invalidation, abandon, changed identity or observed bad VOD/reset withdraw it. Ordinary link-profile mismatch is distinct. RESTART/NONE retain prior initialization only under OP-13. |
| Remaining effects | Operation results and `cleanupOwed` | Successful verification never clears the obligation or changes its owner. Earlier failed results survive a later restoration as separate values. |
| Historical mutation/consumption | Collector `configurationDirty`, samples and history-loss flags | Historical latches survive verification. They are not the current `SettingsSnapshot::trust` or health state. |

A forced request that observes AN enabled has no fresh ANAR: preserve the older
observed tuple/time as stale, rather than completing it from historical ANAR.
A non-IEEE ANAR selector likewise cannot establish decoded IEEE technologies.

The first mismatch is retained while the finite remaining stable reads execute.
A later MDIO/release/deadline/clock failure is the terminal status, with that
earlier mismatch still present in `OperationResult`. CONFIG_MISMATCH becomes
terminal only after the planned pass completes. Valid VOD data and mismatch
survive even a failing release/deadline on that same batch. A mismatching VOD
withdraws mandatory initialization evidence; later matching fields cannot undo it.

Raw LED/MISR/WoL maintenance uses the existing conservative invalidation:
possible target mutation marks configuration/link/collector evidence stale and
withdraws initialization/current identity context. Selector-only failure does
not claim target mutation. Bus-silent external invalidation retains desired/raw
evidence, freezes running work, and does not manufacture an MDIO health failure.
Healthy unrelated reads and cable reconnection cannot revalidate a profile.
See [access semantics](side-effect-access.md) and [health](health.md).

## One restoration procedure

`startInitialize({NONE}, ...)` explicitly restores the same stored desired
intent without reset. PCS, HARD and RESTART use their existing distinct command,
wait and identity procedures; the driver never chooses a stronger reset itself.
Every initialization writes and verifies mandatory VOD, even if the reset domain
retained it, then replays the same coupled configuration procedure in source
order. There is no second reset/apply implementation or implicit retry.

An outstanding cleanup obligation permits verification when no temporary
stimulus is active, but even matching stable bits cannot prove opaque recipe
restoration. `persistentRestored` on a successful VERIFY records all currently
owned stable comparisons passing; its `cleanupOwed` remains independently true
when already owed. A new successful initialization (or eligible CONFIGURE retry)
clears the obligation. A failed restoration has its own token, budget and result.
End/rebind refuse unresolved obligations or retained results. `abandonCleanup`
requires owner-established external disposition, preserves evidence and withdraws
initialization/trust; it is not a claim that hardware was restored.

[Reconcile.h](../examples/portable/Reconcile.h) demonstrates finite owner-driven
verification and references the same initialization helper for restoration.
The separately linked portable consumer executes partial observation, a failed
restoration and an explicitly admitted retry with unchanged forced intent,
retaining all three outcomes. Loopback temporary overrides and their separate stops are implemented in
prompt 23; later diagnostics retain their own feature owners.

Sources: SNLS647I 8.3.11.1-.5 and tables 8-19/21/22/23/28/29/30/43/48/81.
Reset strap ambiguity (Q5), forced observation/routing qualification (Q11),
physical MDIO/restore traces and SDK integration remain separate open evidence.
Host verification does not close those gates.

Stage 23 now supplies the [loopback session owner](loopback.md). VERIFY rejects
while temporary state may remain. An owning stop or explicit HARD uses the same
initialization/profile restore plus saved ordinary-field verification; it does
not replay prior diagnostic traffic or replace an earlier failed result.

Explicitly owned cable autorun is part of stage-26 reconciliation. Verify reads
its owned CR1 bit once and retains mismatch/partial readback without writes.
Initialization replays it last after diagnostic restoration. Policy trust is
separate in `cableAutorunSnapshot`; a raw capture does not verify policy or clear
cleanup. Disabled policy never supplies engine-quiescence evidence.

Prompt 27 extends the same verification with owned IRQ policy and pin prerequisites.
It consumes MISR/RXFS evidence through the collector, reads without selector or
control writes, and keeps crossing attribution unknown until a bounded drain.
A contradictory RXFS word withdraws prior selector context even if release later
fails. Restoration remains explicit initialization; test stimulus has a separate
owned stop and is never replayed. See [interrupts](interrupts.md).

Owned [power controls](power.md) extend VERIFY with BMCR/PHYSCR/BISCR reads only.
Restoration uses startInitialize and the same power procedure, including live
non-HARD PLL preparation. Verification cannot clear prior cleanup debt. No
readback claims measured PLL lock, external-pin release or partner readiness.
