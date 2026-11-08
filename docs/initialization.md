# Initialization and reset operations

Stages 15–16 implement exact-address DP83826I initialization with `ResetKind::NONE`,
`PCS`, `HARD` or `RESTART`. Every path accepts identity, captures raw straps,
checks BMCR and PHYRCR for active commands, then writes and verifies mandatory
VOD settings. Initialization
can succeed with cable down. It proves this implemented procedure completed;
link-profile application arrives in stages 17–18, managed health in 21, and
hardware qualification requires measured fixtures. The
[reset-effects table](device-behavior.md#reset-domains-and-readiness) distinguishes
the settings, identity and strap domains of every trigger.

The [public header](../include/DP8382x/DP8382x.h) owns declarations and Doxygen;
[OP-1 through OP-17](api-contracts.md#one-cooperative-operation-and-retained-result)
own admission/lifetime rules. The [ledger](registers/dp83826i.json) procedures
`pcs_reset`, `hard_reset`, `restart`, `external_reset` and `initialize_10base`
own numeric/source facts. The
[independent vectors](register-vectors.md) and [executing tests](../test/operations.cpp)
check the procedure without deriving expected traces from production constants.

## Board readiness and ownership

Before first probing, the application waits at least 50 ms after both supplies
reach final level. If the clock is absent during ramp, SNLS647I Figure 7-1 (p18)
recommends holding RESET_N low and releasing at least 100 us after clock
stabilization. Alternatively, §7.6 footnote 2 (p17) requires an additional RESET_N
after POR completion, initiated at least 100 us after both clock stabilization
and POR completion. The application establishes one of these paths before
probing. SMI is inactive for 2 ms after
release. These external readiness conditions are not a PCS reset maximum.

The application establishes board/clock/address history, quiesces affected
traffic, serializes every instance call and supplies a truthful bounded terminal
backend. One controller owner excludes every PHY and SDK participant for each
complete batch. No controller lock or selector session survives a poll. Core
does not own pins, schedule work, retry, recover automatically or busy-spin.

## Procedure and results

`startInitialize` validates/copies the request without clock or MDIO calls and
returns IN_PROGRESS with a nonzero object-lifetime token. A rejected start leaves
the caller token and state unchanged. Each `pollOperation` runs at most one batch:

1. Read both identity words in one batch and accept the declared exact part.
2. Capture SOR1 and SOR2 in separate complete indirect batches.
3. Read BMCR and PHYRCR separately. Wait out any active command, withdrawing
   identity/configuration knowledge when its reset domain requires it. NONE also
   refreshes identity/straps after an observed configuration reset. This prevents
   a restoration from proceeding while an earlier failed HARD is still active.
4. Issue the selected single literal command: PCS `W(00,8000)`, HARD
   `W(1F,8000)` or RESTART `W(1F,4000)`. There is no sampled-word RMW.
   After the appropriate wait, PCS polls BMCR bit8000; HARD/RESTART poll PHYRCR
   bitsC000 so neither PHYRCR command may remain active. Refresh identity and
   straps at the configured address. An already-active configuration reset also
   requires fresh identity acceptance before issuing the requested command.
5. Always issue the complete mandatory VOD write and separate verification read.
   PCS retains vendor registers; it cannot substitute for this write. Readback
   must match every owned stable bit. A mismatch retains expected, mask and raw
   observed value in `ConfigurationMismatch`.

With clear initial commands and first completion read clear, NONE uses 20 frames;
PCS/HARD/RESTART use 32. No RCSR mutation or CDCR read is admitted. Only mandatory VOD is
currently persistent owned state: raw LED/IRQ/WoL maintenance does not create
a desired profile. Later feature procedures extend this same finite replay path.

An accepted or uncertain PCS or HARD reset invalidates initialized, identity and configuration
knowledge even when a later read fails. `commandAccepted` records command frame
completion; `protocolComplete` requires the later in-budget clear observation.
Selectors do not count as target mutation. A failed/late target write remains
possible hardware state; no read retroactively turns its earlier failure into
success. Partial ID/strap words survive with explicit validity/staleness; a later
release failure cannot erase a successfully captured strap word.

`isInitialized` becomes true only after complete verification. NONE reapply keeps
it until identity/verification fails or a mandatory target effect is possible and
unverified. `EvidenceSnapshot.configurationDirty` is a conservative historical
mutation/uncertainty latch, including internal writes; it is not the current
initialized flag and a successful initialization does not clear old event history.

## Bounds, cancellation and restoration

The absolute deadline and total frame count are immutable allowances except for
consumed frames. The per-poll `maxFrames` is a slice: a short or zero slice yields
before a whole batch. Exhausted total frames, exclusive raw deadline, inability
to fit a truthful batch bound, or `maxStalledPolls` end the operation. A frozen
clock cannot make repeated waits infinite. A regressing/overflowing clock latches
CLOCK_CONTRACT and refuses further timed work until repaired and rebound.

Reset admission uses OP-6's strict formula with command/read bounds, four clock
lags and the policy interval. It is checked again at the actual pre-callback
sample. Waits begin at the post-callback raw time plus clock lag plus interval;
a timestamp captured before the write is insufficient. Additions are checked or
bounded in uint64 arithmetic. Raw time at the deadline is expired. Quantized
clocks need enough total allowance; nominal silicon timing is not a timeout.

Cancel freezes CANCELLED immediately without I/O. Take delivers a result once;
pending poll/cancel never overwrite it. If an unsuccessful operation completed
or possibly applied a semantic write, `cleanupOwed` survives take. Admit a new
`startInitialize` with a new token/budget to restore; its success clears the
barrier but cannot replace the original result. Failed restoration retains its
own result and the obligation. A reset alone does not start a temporary stimulus.

End/rebind refuse a running operation, pending result or cleanup obligation.
If restoration is impossible, the owner establishes external reset, isolation,
power removal or explicit transfer of responsibility, then calls bus-silent
`abandonCleanup`. Clock failure follows take, abandon if owed, end, external clock
repair/readiness and bind. Abandon does not release controller quarantine or
prove hardware restoration. Tokens do not restart on end/bind.

## Direct access and examples

HARD adds a shared-bus silence floor at the post-callback upper timestamp plus
`max(2000, pollIntervalUs)` us. This applies to any attempted or untrustworthily
reported command, even on failure. `busQuietUntilUs`/`busQuietValid` survive
cancel, take, abandon, invalidation and later operation starts. Cached snapshots
do not expire them; a timed call does so only after observing the endpoint.
`nextPollAtUs` never advertises an earlier legal reset read. The owner closes
admission to all PHY addresses before the command and copies the floor immediately
after the poll, without retaining the transaction lock during silence.

If the post-callback clock regresses or the mandatory floor cannot fit the
nonwrapping timeline, the anchor is invalid and timed work is latched off. Preserve
the primary transport error; otherwise floor-addition overflow returns
BOUND_ADMISSION. Owner disposition and a trustworthy external time source are
required before rebinding; no zero/invalid anchor proves an elapsed wait.
End/bind may discard the local floor under OP-12, so the application carries
the physical responsibility across driver lifetimes. PCS/RESTART do not create
this floor, but cannot bypass one still owed by an earlier HARD.

RESTART preserves register/identity and initialized knowledge while invalidating
live event/link observations. Its initialization still refreshes identity/straps
and applies mandatory VOD; a failed VOD mutation cannot preserve readiness.
External reset and POR notifications are distinct owner attestations, already
represented by `InvalidationReason`: both perform zero callbacks and make old
knowledge historical. POR also samples ENHANCED Strap1/6, which pin reset retains.
Neither notification clears
a previously recorded hard-reset floor or performs implicit reinitialization.

Raw writes reject running work, pending results and cleanup barriers. Safe raw
reads and the collector remain available between polls and with pending results
or cleanup, except during reset WAIT or a persistent hard-reset floor. External invalidation freezes a running
operation; explicit external reset/power-on clears current cleanup by owner
attestation. Already frozen results remain historical and immutable. An admitted
live identity read that fails or contradicts the operation invalidates that
running operation before it can write using obsolete identity.

[Initialize.h](../examples/portable/Initialize.h) compiles against the real core
and shows one-poll owner scheduling, retained results, separately admitted
restoration and explicit abandon. The [portable consumer](../test/consumer/main.cpp)
executes it with a clearly simulated controller. The [test guide](../test/README.md)
records local commands. Backend/SDK timing and physical reset/link behavior remain
NOT RUN until the later adapter and admitted fixture campaigns.

## Negotiated replay after stage 17

When an admitted desired negotiated profile exists, every initialization kind
continues from mandatory VOD verification into the shared advertisement/BMCR
apply and stable readback path. It always restores NP=0 and restarts AN; transient
page progress and pages are never replayed. Initialization can finish with cable
down and does not implicitly wait for negotiation. Only complete owned replay
clears the restoration barrier. See [configuration](negotiation.md).
