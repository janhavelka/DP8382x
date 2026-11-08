# Initialization and PCS reset

Stage 15 implements exact-address DP83826I initialization with `ResetKind::NONE`
or `PCS`. Both paths accept identity, capture raw straps, check for an already
active BMCR reset, then write and verify mandatory VOD settings. Initialization
can succeed with cable down. It proves this implemented procedure completed;
link-profile application arrives in stages 17–18, managed health in 21, and
hardware qualification requires measured fixtures. HARD/RESTART arrive in 16.

The [public header](../include/DP8382x/DP8382x.h) owns declarations and Doxygen;
[OP-1 through OP-17](api-contracts.md#one-cooperative-operation-and-retained-result)
own admission/lifetime rules. The [ledger](registers/dp83826i.json) procedures
`pcs_reset` and `initialize_10base` own numeric/source facts. The
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
3. Read BMCR separately. If reset is already active, invalidate old knowledge
   and wait for a clear bit; NONE also refreshes identity/straps after that reset.
4. For PCS, issue the single literal `W(00,8000)`, then schedule completion reads.
   There is no BMCR RMW. After clear, refresh identity and straps.
5. Always issue the complete mandatory VOD write and separate verification read.
   PCS retains vendor registers; it cannot substitute for this write. Readback
   must match every owned stable bit. A mismatch retains expected, mask and raw
   observed value in `ConfigurationMismatch`.

With clear initial BMCR and first completion read clear, NONE uses 19 frames;
PCS uses 31. No RCSR mutation or CDCR read is admitted. Only mandatory VOD is
currently persistent owned state: raw LED/IRQ/WoL maintenance does not create
a desired profile. Later feature procedures extend this same finite replay path.

An accepted or uncertain reset invalidates initialized, identity and configuration
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

Raw writes reject running work, pending results and cleanup barriers. Safe raw
reads and the collector remain available between polls and with pending results
or cleanup, except during reset WAIT. External invalidation freezes a running
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
