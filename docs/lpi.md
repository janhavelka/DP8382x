# Forced TX LPI owner calls

The [typed control CLI](cli.md#typed-controls) exposes legacy LPI start, stop and cached status. The application separately confirms TX drain.

`startLpi(options, result, budget)` and `stopLpi(result, budget)` implement
SNLS647I section 8.3.4 on the declared DP83826I. Both return synchronously after
at most nine frames in two terminal callbacks: one complete five-frame MMD RMW,
then one fully addressed four-frame readback. The aggregate `IoBudget` must admit
both batches before the first frame; every callback still receives the remaining
allowance. There are no waits, retries, scheduler, allocation or retained buffers.
The application serializes the driver and every shared-bus participant.

Configure [EEE](eee.md) explicitly first. Then acquire current link, power and EEE
observations. Entry requires initialized/accepted identity, all owned persistent
controls verified, the established opaque EEE recipe, nonstale negotiated EEE100,
both bypass controls disabled, a resolved current 100M link, and normal power with
PLL-disable/automatic-sleep policies clear. Entry's100M and no-automatic-sleep
restrictions are conservative library policies, not additional silicon claims.
Unowned normal power can be established by observation; no unnecessary persistent
power configuration is required. Cached validity has no implicit maximum age:
the application chooses sampling cadence and admits the remaining physical risk.

The MAC owner must stop and drain transmission, then explicitly set both
`LpiOptions::macTxQuiesced` and `maintenanceAdmitted`. The PHY does not buffer
MAC frames. EEE advertisement alone never starts this disruptive procedure.
Entry sets only EEECFG3 bit 12, preserving every other bit. The ordinary recipe
word becomes 108B from 008B. No DP83825 bits 14:13 are imported. Masked readback
publishes REQUESTED, which means request-bit evidence, never PHY or partner
readiness.

The operation slot stays IDLE throughout. OP-9 cleanup/temporary barriers remain
set during application-owned dwell and reject unrelated mutations, teardown and
new operations. OP-14 admitted reads and link acquisition continue through the
same collector. Link publishes its current link bit while mode/pause remain
unresolved for the temporary session. EEE observation preserves stable-control
trust when it sees our owned temporary bit, but cannot publish negotiated validity
during the session. Raw 04D1 remains procedure-only; use typed EEE observation.

Stop has its own budget and result. It works after failed entry, link loss,
stale caches or explicit abandonment while the binding/session remains available.
It clears bit 12 and compares only that bit. Any valid contradictory bits 0/3
observed during either call dirty persistent EEE trust immediately, even if a
later frame fails. A successful stop then reports OFF but retains unrelated
cleanup debt. It never repairs bypass controls or silently reapplies EEE.
After a normal stop, acquire fresh observations before another entry.

| Evidence | Meaning |
| --- | --- |
| OFF | No owned request, verified request clear, or reset reconciliation |
| REQUESTED | Set/readback completed; no readiness event or dwell limit is documented |
| UNCERTAIN | Forced LPI may remain active; frames sent now can be lost |
| `result.status` | Normalized call outcome, distinct from physical readiness |
| `result.primary` / `release` | Exact backend statuses of the final/failing batch, preserving signed detail |
| `result.readbackValid` | Returned final control word; late/release failure can still make the call fail |

Local preflight rejection leaves the caller result and last-call snapshot unchanged.
Once a callback is invoked, `LpiResult` is diagnostic output even on failure.
Each result copy remains independent: stop never overwrites the caller's saved
entry result. `lpiSnapshot()` is bus/clock silent and holds the last admitted call
plus current request evidence. `mutationAttempted/Completed/Possible` distinguish
target effects from selector frames. Counts cover valid receipt prefixes, with
raw malformed reports available in `transportSnapshot()`. Temporary start/stop
are RAW traffic: transport counts change; managed health does not.

Any attempted/uncertain target mutation retains the barrier. An admitted failed
exit leaves UNCERTAIN with the warning:

> forced LPI may remain active; frames sent now can be lost

No extra cleanup frame follows failure. A valid backend release failure retains
its independent status; the shared controller owner must quarantine all clients
until ownership is repaired. Malformed reports additionally stop the binding.
A later explicit stop is a new attempt, never an automatic retry. Alternatively,
explicit HARD initialization runs the existing shared persistent restoration;
only its complete success clears LPI uncertainty. External reset/POR notification
is bus-silent and relinquishes the temporary session. Other invalidation retains
UNCERTAIN. `abandonCleanup` authorizes local teardown, not hardware clearance;
end retains historical LPI uncertainty, and a new bind starts new local evidence.
Save prior results before rebinding.

There is no cancellable operation token for these synchronous calls. Cancellation
before invocation means not starting; after the bounded return, cancel dwell with
a separately budgeted stop. Callback reentry is forbidden. A controller abort
must use the frozen terminal transport contract and retain possible effects;
CANCELLED/IN_PROGRESS backend reports normalize to transport errors, never a false
successful exit. This is the direct-call adaptation of OP-9 cancellation.

[The compiling owner example](../examples/portable/Lpi.h) retains separate entry
and exit results and keeps the MAC stopped after failure. Its readiness decision
requires an explicit application post-exit condition in addition to successful
stop and cleared cleanup debt. At 100M, sleep 210us, quiet 20ms, wake 36us and
refresh 200us are nominal values in SNLS647I 7.6, not deadline guarantees. Dwell,
post-exit delay, MAC queueing, partner readiness and energy policy stay outside
core.

Q2 remains NOT RUN. An admitted physical campaign must record exact PHY/revision,
straps/interface, firmware identity, partner/EEE negotiation, full 04D1 traces,
MAC drain/traffic timestamps, line entry/exit, packet loss and energy. Exercise
008B/108B/008B on the line and preserve the neighboring-part difference if it
fails. Measure actual sleep/wake/quiet/refresh and traffic restart distributions;
do not turn the nominal table into pass deadlines. Q1 wire/EEE interpretation,
SDK/controller timing and package qualification remain separate evidence.
