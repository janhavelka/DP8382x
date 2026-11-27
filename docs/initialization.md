# Initialization and reset operations

DP83822I/H/IF/HF copper initialization checks revision 0, captures SOR1/SOR2
under one controller lock, reads CR2[14], and checks the board declaration.
NONE also reads BMCR, PHYRCR and MMD 3 offset 0000 for active resets.
With an empty owned profile this takes 17 frames and writes only MMD selectors.
VERIFY checks all three reset registers. HARD writes C22 1F = 8000, enforces
SMI silence, then reacquires identity, straps and live medium. C22 1F = 4000
restarts digital logic while retaining registers. The mapping to the prose
"soft reset" in section 7.4.4.2 is inferred. BMCR[15] resets PCS. MMD 3
0000[15] resets MMD 3, MMD 7 and PCS. Both retain vendor registers and
startup rechecks the copper configuration before replay. Cancelled or
uncertain commands retain their cleanup obligation. On the native adapter
allow 200 ms from power-on, at least 10 microseconds of RESET_N assertion, and 2 ms
from release to SMI readiness plus any longer declared idle MDC period.
The application owns clock stability, preamble and board evidence.
[SNLS505H timing and reset tables](reference/source/TI-DP83822-SNLS505H.pdf)

DP83825I checks the declared ID followed by vendor 31:0467 and direct
RCSR (17h). `strapSnapshot()` and the operation result retain SOR1 and RCSR
with separate validity flags. NONE, PCS, HARD and RESTART use this part's
default functional mode. No VOD programming or verification is required.
After each reset the owner reacquires identity and clock role before replaying
persistent settings. VERIFY checks both role sources even with no profile.
If recapture stops partway, the operation result retains the latest returned
word for each register, marks the pair stale and withdraws its timestamp.
The live strap snapshot shows which words returned during recapture. A
returned word that contradicts the declared role sets the result's mismatch
even when the terminal status reports a transfer or controller-release failure.
A HARD reset retains the controller-wide quiet period. For a physical reset,
the application supplies stable XI, RESET_N timing and the idle MDC cycle.
[Clock and interface procedure](interface-clock.md)

For either A grade, SNLS783A sections 7.5, 7.6 and 8.4.1 require the
same `030E=4A40` startup recipe. HARD restores the POR value `8400`;
PCS preserves vendor words and RESTART preserves settings. Each requested
initialization rechecks identity, SOR1/SOR2 and the VOD readback. Use
`strapSnapshot()` for current-epoch validity per word, including a valid
SOR1 if the second read fails. On A, `strap11High()` reports pin 19 when
SOR1 is valid. ENHANCED Strap1 and Strap6 are POR-only. An independently
observed PHYRCR[14] restart preserves these strap words for profile replay.
An observed HARD reset invalidates the capture and requires a fresh pair.

The core implements exact-address DP83826I/E and DP83826AI/AE initialization with `ResetKind::NONE`,
`PCS`, `HARD` or `RESTART`. Every path accepts identity, captures raw straps,
checks BMCR and PHYRCR for active commands, then writes and verifies mandatory
VOD settings. Initialization
can succeed with cable down. It proves this implemented procedure completed;
link-profile replay is implemented in stages 17–18 and
[passive health](health.md) in stage 21. Initialization callbacks count as
managed transactions even before initialization completes, and
hardware qualification requires measured fixtures. The
[reset-effects table](device-behavior.md#reset-domains-and-readiness) distinguishes
the settings, identity and strap domains of every trigger.

[Verification and reconciliation](reconciliation.md) use the same slot and
stable comparisons. startVerify does not write VOD or clear a cleanup obligation;
restoration explicitly uses startInitialize with the stored desired intent.

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
2. Capture vendor SOR1. On DP83825I, collect direct RCSR next. On DP83826,
   read SOR2 in a second complete indirect batch.
3. Read BMCR and PHYRCR separately. Wait out any active command, withdrawing
   identity/configuration knowledge when its reset domain requires it. NONE also
   refreshes identity/straps after an observed configuration reset. This prevents
   a restoration from proceeding while an earlier failed HARD is still active.
4. Issue the selected single literal command: PCS `W(00,8000)`, HARD
   `W(1F,8000)` or RESTART `W(1F,4000)`. There is no sampled-word RMW.
   After the appropriate wait, PCS polls BMCR bit 8000; HARD/RESTART poll PHYRCR
   bits C000 so neither PHYRCR command may remain active. Refresh identity and
   straps at the configured address. An already-active configuration reset also
   requires fresh identity acceptance before issuing the requested command.
5. For DP83826 I/E or AI/AE, issue the mandatory VOD write and separate verification read.
   PCS retains vendor registers; it cannot substitute for this write. Readback
   must match every owned stable bit. A mismatch retains expected, mask and raw
   observed value in `ConfigurationMismatch`.

On DP83826 with no owned link or optional-feature settings, clear initial commands
and a clear first completion read, NONE uses 20 frames and PCS/HARD/RESTART use 32.
Owned desired settings add their shared replay and verification steps in the
finite restoration order. Interface replay may update stable RCSR fields while
retaining its destructive samples through the collector; an unowned interface
adds no RCSR mutation. CDCR calibration recovery is a separate explicit route.
Raw LED/IRQ/WoL maintenance does not create desired intent.

An accepted or uncertain PCS or HARD reset invalidates initialized, identity and configuration
knowledge even when a later read fails. `commandAccepted` records command frame
completion; `protocolComplete` requires the later in-budget clear observation.
An independently returned active PCS/HARD indication also invalidates that
knowledge before a later release, deadline or clock error terminates the poll.
The error stays primary; observing reset does not claim a driver-issued mutation.
An observed RESTART retains initialization and settings, but invalidates link/history.
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

Stage 18 expands that replay to all owned robust/routing/mirror/extension fields
and forced mode. Forced desired intent never accesses ANAR; it preserves any
inactive advertisement and neutralizes AN commands in BMCR. Every reset kind
restores the complete admitted profile after mandatory VOD, with fresh readback.
No forced profile is invented when the caller has never admitted one.

[Loopback sessions](loopback.md) now retain a temporary barrier across result
consumption. Only owning stop or HARD initialization is admitted while it is set;
PCS/NONE/RESTART cannot promise vendor stimulus removal. HARD reuses the saved
ordinary-field restore tail and clears temporary knowledge only after full
verification, including isolation that BASIC straps may reload.

Stage 24 extends this same HARD-restoration tail with saved BIST packet/IPG.
Those values are captured before any loopback companion mutation. A failed BIST
start/acquire requires owning stop or HARD; no earlier measurement is erased or
converted into a restoration success. See [BIST](bist.md).

Manual cable sessions also extend this restore path. Their stop always admits
HARD, then current persistent replay and saved documented cable controls. Every
new HARD attempt replays the entire saved tail even after partial restoration.
Known CDCR calibration blocks initialization: clear-then-HARD belongs to prompt
35, not an incidental reset. The [cable guide](cable.md) separates RAW ownership,
SNLA428 companions, retained primary evidence and physical quiescence limits.

Stage 26 adds explicitly owned autorun policy to the shared restore order,
after every persistent profile and temporary saved-field tail. Unowned CR1
policy is untouched; owned enable or disable adds one RMW/readback. Failed or
cancelled final replay keeps initialization incomplete and prior effects visible.
`startVerify` checks this policy without changing it. Future power transitions
extend the same path; no enable operation authorizes automatic reset or recovery.

Prompt 27 adds explicitly owned interrupt policy as the final shared replay tail,
after diagnostic saved state and autorun. Unowned IRQ policy adds no frames.
BASIC preserves PHYSCR OE; ENHANCED validates the declared pin 21 role. Replay
never restores transient test 1. Readback or identity failure leaves initialization
incomplete and the original effects/result intact. Alternate pin muxes are
validated, not implicitly programmed; prompt 32 extends the same dependency order.
See [interrupt ownership](interrupts.md).

## Owned power replay

Stage 28 adds [power preparation and replay](power.md). With explicit owned power
policy, non-HARD initialization reads live BMCR/PHYSCR and clears/verifies PLL
disable before resetting retained vendor state; HARD reloads it itself. Mandatory
VOD and other persistent owners run before final power replay. Power failure
retains earlier initialization effects and OP-9 debt; no retry or silent wake.

With explicitly owned [EEE policy](eee.md), initialization selects one effective
HARD even for NONE/PCS/RESTART requests. The result reports HARD. This source-backed
reset/reapply prefix precedes VOD/profile and ordinary replay, and cannot recurse.
Without owned EEE, all earlier reset-kind semantics remain unchanged.

A retained forced-TX-LPI session requires explicit HARD restoration or its
bounded [stop call](lpi.md). Shared initialization never replays temporary LPI;
only completed persistent restoration withdraws uncertain session evidence.


Owned [WoL settings](wake-on-lan.md) extend the same replay: detector disable,
needed route/match update and verification, final enable, then IRQ source/masks
and final power state. Unowned initialization never invents a WoL-off write.
Acknowledgement is transient and is never restored. Clock/pin incompatibility
retains failed restoration and cleanup debt.


Owned [pin profiles](led-gpio.md) now replay through the shared tail before
WoL/IRQ activation and final power policy. Source/value preparation precedes
clock release, mux and final force enable. IRQ/WoL source owners restore their
own routes; the pin profile is a checked dependency, not a second mux writer.
NONE/PCS/HARD/RESTART and existing power/diagnostic recovery retain this same
sequence. Read-only VERIFY detects changed owned fields without repair.


Owned [interface/clock settings](interface-clock.md) now replay after mandatory
VOD/link work and before pin, WoL, IRQ and final power policy. NONE/PCS/HARD/RESTART
share the same procedure; reset-loaded defaults never replace admitted desired
intent. RCSR replay/readback consumes RC samples through the collector. Optional
isolation uses the independent power request; application clock admission also
covers reassertion after reset. Non-HARD owned PLL shutdown is cleared by existing
power preparation before interface mutation; VERIFY makes no such writes.
Readiness remains false through mutating interface and pin restoration, including
failure/cancellation after mandatory VOD verification. Only full successful
restoration republishes it. RESTART's command alone still preserves readiness;
its subsequent mandatory setup and replay must complete independently.

Owned quality profiles now replay after interface restoration and before pins,
WoL/IRQ and final power policy. The same finite pass restores detector thresholds
before causes and preserves unrelated settings. Read-only verification performs
no detector disable or threshold write. [Quality controls](quality.md) retain
partial effects and require a separately admitted restoration after failure.

Stage 35 analog replay follows mandatory VOD setup, interface and quality replay.
Explicit termination tuning may then replace the owned baseline fields; every
initialization still writes/verifies the baseline first. `startVerify` compares
the admitted desired override. Compliance HARD stop and calibration recovery
leave readiness false and cleanup owed until a separate initialization completes
all persistent tails. See [electrical controls](electrical.md).

The [prompts 34-35 audit](reports/implementation/34-35-independent-audit.md)
verifies analog handoff to each later owner and their combined profile. A
finished analog phase no longer remains selected while pin/WoL/IRQ/power replay
is pending. External reset/POR notifications clear current electrical session
and calibration HARD obligations while preserving frozen historical results.
