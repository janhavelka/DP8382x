# Passive driver health

`driverState()` and `healthSnapshot()` copy local evidence without MDIO, clock
callbacks, retries or recovery. The application serializes these calls with all
other instance access. `DriverState` is a projection of current initialization
and managed callback history. It does not decide whether another operation is
admitted. In particular, OFFLINE never blocks an otherwise legal explicit attempt.

| Condition | State | Meaning |
| --- | --- | --- |
| Initialization not established | UNINIT | Counters/errors may still describe earlier managed attempts. |
| Initialized, failure streak zero | READY | Managed transport evidence, not link or settings verification. |
| Initialized, streak below threshold | DEGRADED | One or more consecutive managed failures. |
| Initialized, streak at/above threshold | OFFLINE | Repeated failure; the owner still chooses the next action. |

`Config::offlineThreshold` defaults to 3; legal values are 1 through UINT32_MAX.
This is application policy, not a TI timing or silicon limit. A valid 10/half
link can fail an application's 100/full requirement while health stays READY.
Cable-down and negotiation-in-progress observations can also have healthy MDIO.

## One callback, one transaction

Initialization, persistent configuration, explicit verification and live link
acquisition are managed.
Each invoked batch callback contributes at most one health outcome. The first
three successful selector/address frames of an MMD batch cannot turn a failed
fourth target frame into a success. A complete link pass has two managed
transactions (eight and two reads), not ten health successes or one public-call
success. A failure of its second batch retains the first batch's success and
counts one failure; its aggregate output remains unpublished.

Raw Clause 22/MMD access, `readIdentity`, `acquireEvidence`, `collectEvents`,
`modifyInterruptEnables` and `modifyWakeControl` are diagnostic/maintenance
traffic. They update transport and collector evidence, not managed health. A raw
call made while an operation is running does not inherit that operation's
managed classification. Future typed persistent features extend the existing
internal classification; temporary session stimulus/stop remains transport-only.

| Callback result | Health accounting |
| --- | --- |
| No callback: local validation, short slice, expired budget, cancellation, cached view | No count or streak change. |
| Valid zero-attempt BUSY/OPERATION_TIMEOUT, successful release, reusable controller | Excluded; no count. |
| Valid zero-attempt BUSY/CONTROLLER_QUARANTINED with successful release and nonreusable controller | Excluded; existing quarantine is not a new failure. |
| Complete valid transport/release | One success, clear failure streak. |
| Primary MDIO_ERROR/MDIO_TIMEOUT, including lock timeout with no frame | One failure; preserve normalized primary and full signed detail. |
| Release failure, newly unsafe controller, or malformed report | One failure, even if the public primary status is BUSY. Primary MDIO error wins; otherwise record release/controller/report cause. |
| OPERATION_TIMEOUT after a valid attempted prefix | One failure; retain public timeout and record incomplete managed batch as MDIO_ERROR with the same detail, without claiming a physical wire defect. |
| Clean transport but callback returns late or post-callback clock fails | One success; public deadline/clock failure remains separate and can prevent publication. |
| Complete transport followed by wrong identity, configuration mismatch, link down or incoherent link bookends | Transport success remains counted; semantic outcome is separate. |

Malformed reports count only independently valid frame prefixes in
`transportSnapshot()`. That snapshot retains the raw receipt, normalized
violation and late/clock information. Health adds no physical-frame accounting.
For example, public BUSY/OWNER_ADMISSION with release MDIO_ERROR/-37 records
one health failure with lastError MDIO_ERROR/-37. It cannot become a successful
transaction merely because no target frame ran.

## Lifecycle and retained evidence

| Action | Health and other evidence |
| --- | --- |
| Successful bind/rebind or local end | New local health evidence, UNINIT; refused teardown/rebind preserves everything. |
| Start/poll initialization before completion | Callbacks count, but state remains UNINIT while initialization is false. |
| Successful complete initialization | Current initialization permits state projection; no link claim. |
| Accepted reset or external invalidation | PCS/HARD attempts or possible effects and external invalidation withdraw initialization, projecting UNINIT. RESTART and a NONE reapply retain prior initialization under OP-13. Counters/error history survive. |
| Managed failure | Increment failure total and streak; retain cause and timestamp validity. |
| Later managed success, including separately admitted restoration | Increment success total, clear streak; preserve lastError and both earlier operation results. |
| Raw/probe/session-stop success or release alone | Cannot reset managed streak or repair configuration trust. |
| Cancel, takeResult, abandonCleanup | No fabricated callback or health recovery. Existing initialization/effect/barrier rules still apply. |
| resetStatistics | Clear only totalSuccess/totalFailures; no I/O and no other state changes. |

All three health counters saturate at UINT32_MAX. `resetStatistics()` preserves
the streak, threshold, initialization, last error, both timestamps and their
validity, transport totals, chip counters, collector history, desired settings,
configuration trust and retained results. It cannot manufacture recovery.

The error survives later success, including successful restoration; the owner's
earlier failed operation result is a separate immutable value. Restoration has
its own token, budget and result. A subsequent successful read cannot retroactively
prove that a prior uncertain write was applied correctly.

## Time and application use

Health timestamps are raw post-callback clock samples in microseconds, with the
binding's `clockLagUs` uncertainty. They are not exact wire completion instants.
Zero is valid when its flag is true. A new outcome with an invalid post-callback
clock stores zero/false for that outcome's timestamp. The opposite outcome's
historical timestamp remains unchanged. Clock regression or uint64 overflow
latches the existing clock fault; historical stamps cannot be used to calculate
age in a broken timeline. Cached queries never read a clock to expire evidence.

[Health.h](../examples/portable/Health.h) demonstrates copying under the owner
and mapping the value to application advice. Its tests-OFF portable consumer
links the real core. Displaying two copies does not sample the PHY twice.
For link/application freshness, use [link observations](link-observation.md);
for silicon counters, use [event acquisition](event-counters.md).

The transport interpretation follows SNLS647I 8.3.11.1-.5, tables 8-31/32,
and PHYSTS table 8-34. These sources define transactions and observations, not
software health policy. Host tests establish accounting and lifecycle behavior;
controller timing and physical fault rates remain NOT RUN.

Stage 23 classifies loopback start and stop, including stop's initialization
and desired-profile replay, as RAW. The separate explicit `startInitialize(HARD)`
escape remains managed. Temporary-state barriers never follow health state.

BIST start/acquire/stop are RAW, including shared stop replay. Counter errors
and inconclusive test outcomes are distinct from MDIO and managed health.

Explicit autorun policy configuration is MANAGED traffic. Its single-run raw
acquisition is RAW; policy enable is not a physical trigger, and uncertain cable
freshness is not an MDIO failure. An owning diagnostic stop remains RAW through
its final persistent autorun replay, while explicit initialization stays managed.

Prompt27 classifies persistent interrupt configuration and its initialization/
verification tails as managed batches. Deferred service and test stimulus
start/stop remain RAW diagnostic traffic. Counter requests and IRQ events are
collector evidence; neither a pin test nor an empty event pass proves physical
interrupt delivery or managed recovery.
