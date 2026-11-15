# BIST generator and checker sessions

`startBist`, `startBistAcquire` and `stopBist` use the existing operation slot,
tokens and finite `OperationBudget`. Admission calls neither clock nor MDIO.
Consume each result before admitting the next operation. The application owns
dwell, normal-traffic quiescence and external stimulus; the controller lock covers
one batch and is released before waits. `bistSnapshot()` is passive.

Start requires fresh exact DP83826I identity, initialization, a VERIFIED desired
profile and `options.loopback.maintenanceAdmitted`. Persistent desired settings
remain unchanged. Normal line operation uses `useLoopback=false`: it does not
pretend an external-loop fixture exists. Optional digital, analog and external
loopback reuse [the existing loopback procedure](loopback.md). SNLS647I 8.3.15
does not establish BIST routing for the same PHY's MII/PCS/reverse loops; those
combinations reject before I/O. Digital requires admitted HARD exit, analog its
100-ohm termination, and external loopback an actual fixture.

## Settings and source limits

| Setting | Meaning and admission |
| --- | --- |
| OFF | Neither generator nor checker; observation reports DISABLED |
| SINGLE_PACKET | One constant-data packet, checker disabled |
| CHECKER | Checker only; actual external stimulus must be correlated |
| GENERATOR_CHECKER | Continuous pseudo-random generator and checker |
| SATURATE / WRAP | Independent error-counter policy, never packet-generation selection |
| packetBytes | All documented 11-bit codes, 0 through 2047; raw encodings do not promise valid Ethernet frame sizes |
| interPacketGapBytes | Multiples of four from 0 through 1020; default 500 |
| observationUs | Nonzero minimum spacing selected by the owner, not a maximum silicon lock/completion time |

The [ledger](register-map.md) owns the numeric catalog. Source definitions are
SNLS647I 8.3.15, table 8-40 (BISCR), table 8-45 (BICSR1) and table 8-46 (BICSR2).
Mode, packet and gap options have no additional guessed dependency on each other.
There is no source-backed independent continuous-generator-only encoding.

Before any temporary mutation start saves packet/IPG, then invokes the shared
loopback procedure or its ordinary line path. It programs and verifies packet/IPG,
captures/clears a baseline, reads PHYSTS through the collector, enables the selected
PRBS mode and verifies controls. Digital/analog wait for link after their restart,
within the setup budget. No rejected `030B=3380` analog gain write is added.
Lack of lock remains inconclusive under `loopback_recipe`, not a reason to guess
another gain word. Initial BISCR must show both inactive controls and generator.

## Capture and outcome

Each acquire uses four frames in three callbacks: BISCR before, an indivisible
BICSR1 snapshot-write/read pair, then BISCR after. Small poll slices yield before
the whole pair. There is no drain loop, automatic repeat or retry. The minimum
observation spacing is anchored after activation verification and after each
complete acquisition, using the clock's conservative upper bound. Overflow,
deadline and frozen clocks terminate under the existing operation rules.

Writing BICSR1 bit 15 captures the error count and clears the live counter; the
write preserves the configured IPG, with other high bits zero. The subsequent
read's bit 15 is a counter MSB, never a command-completion flag. The library does
not poll it clear or invent a write-zero unlatch protocol. Baseline raw data is
retained by the collector but cannot satisfy the current typed acquisition.

`OperationResult::bist` retains each successfully returned status/counter word
even if a later frame, release or timestamp check fails. `measurementValid` means
a complete successful raw acquisition in this session. It is separate from
`counterValid`, time validity, operation Status, test outcome and restoration.
A snapshot/clear that may have occurred without valid returned data sets possible
history loss. It never becomes zero errors. New sessions reset typed measurement
validity and tokens; collector history keeps its own historical validity.

| Evidence | Result |
| --- | --- |
| Complete zero-error capture | INCONCLUSIVE, even with activity/lock, while the source gate below is open |
| Nonzero error byte with required checker lock/traffic and no observed sync-loss | ERRORS_OBSERVED; no exact rate or interval total |
| Single generator, with actual owner-correlated packet observation | SINGLE_PACKET_OBSERVED; no checker claim |
| Single generator inactive before first sample, no correlated packet | INCONCLUSIVE, not proof of failure |
| Checker-only without actual correlated stimulus, missing lock, or sync loss | INCONCLUSIVE; raw words remain available |
| Saturated FF sample | Lower bound; not an exact count of all errors |
| WRAP mode, including zero | Possible unobserved wraps; no exact total |
| Acquisition error/cancel | Invalid complete measurement, retained partial evidence and primary detail |

`bist_result_semantics` remains open. The datasheet does not fully specify repeat
latch release/overwrite or a reliable sync-loss history interval. RH means
hardware set/clear, not read-clear. Two locked observations cannot prove continuous
lock. Every raw measurement exposes `intervalUncertain`; no PASS enum or derived
error rate is offered. Repeated explicit captures are usable raw observations,
not proof that an undocumented latch lifecycle has been qualified. Source or
identified fixture evidence is required before adding that pass policy.

## Stop, failures and retained state

The first attempted session mutation retains `temporaryMayBeActive` and
`cleanupOwed`. Failure/cancel freezes immediately and performs no cleanup I/O.
An acquire or stop has its own budget/token/result. A failed start still admits
its owning stop; a failed acquire cannot replace the earlier setup result.

Stop disables PRBS, verifies the controls and waits boundedly for generator-active
to clear. Only then does it invoke shared loopback stop, initialization/profile
replay, saved loopback fields and finally saved IPG/packet writes/readbacks.
Digital uses HARD; saved values were acquired before any companion mutation so
early failures can still restore them. Explicit HARD initialization uses the same
tail. No prior PRBS mode is replayed. Failed restoration keeps trust DIRTY and the
barrier; earlier measurements remain separate historical evidence.

Plain `stopLoopback` cannot bypass BIST's restoration owner. Other feature starts
and raw mutations remain excluded. Abandon does not claim a stopped generator.
External reset/POR invalidation is bus-silent and retires the session, preserving
historical evidence; a fresh bind starts new local history. Diagnostic start,
acquire and stop, including stop replay, are RAW transport activity. Separately
requested initialization remains managed health activity.

With immediately ready hardware and a persistent AN profile: normal-line start
uses 25 frames/19 callbacks; analog 47/36; digital 59/40. Stop uses 65/39, or
79/45 for digital. Forced profile stop uses four fewer frames/three fewer callbacks.
Each additional link/restart/generator-off poll consumes one frame and callback
within the supplied budget. These are host protocol bounds, not latency promises.

[Bist.h](../examples/portable/Bist.h) compiles in the separate tests-OFF consumer.
It reuses the common owner loop and shared HARD-reset floor. The consumer acquires
once, copies two cached views without I/O, then stops with a separate result.
CLI/runner mappings remain planned. Physical counters, lock behavior, packets,
termination and signal integrity are NOT RUN; no simulated sample qualifies them.
