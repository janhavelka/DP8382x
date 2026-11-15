# Interrupt configuration and bounded service

`startConfigureInterrupts(settings, budget, token)` owns complete interrupt
policy. It is opt-in: ordinary initialization adds no IRQ writes until this
binding explicitly accepts `InterruptSettings`. Start copies intent without
clock or MDIO calls. Polls use the existing operation slot and controller-wide
transaction owner; no lock spans polls or application observation.

The [portable owner example](../examples/portable/Interrupts.h) is compiled and
executed by the tests-OFF consumer. It does not assume the initial board has a
connected IRQ. A platform ISR only posts a bounded application signal. The
serialized owner performs configuration, service, result delivery and any later
notifications. Neither ISR work nor the core allocates, logs or schedules recovery.

## Policy, pin roles and application admission

Settings provide both source-enable bytes, global delivery, output enable,
polarity, shared polarity/WoL source and selected pin route. Named source masks
and access provenance remain in the [register catalog](../include/DP8382x/CommandTable.h)
and [ledger](registers/dp83826i.json); no generic raw selector session is exposed.
All eight sources in each MISR are supported, including disabled-source and
global-disabled combinations. A source enable is not a current condition or proof
of a physical edge.

Maintenance and pin ownership must be explicitly declared before any callback.
`pinAvailable` authorizes both the selected route and pin21's electrical role:
selecting an alternate LED mux does not disconnect the dedicated interrupt pin.

| Route/mode | Admission and live proof |
| --- | --- |
| BASIC pin21 | Observed BASIC identity means strap-selected INT. PHYSCR OE is preserved and has no power-down meaning. |
| ENHANCED pin21 | OE enables open-drain IRQ output. Requesting OE clear always requires `powerDownInputSafe`; low on the resulting input asserts power-down. |
| BASIC LED1 | Owner declares availability; live LED1 mux must already select IRQ. |
| ENHANCED LED1 | Also requires `led1ClockUnused` and live GPIO availability from the clock/LED status register. |
| ENHANCED LED2 | Live IRQ mux required; MII also requires `colUnused`. RMII has no MII COL consumer. |
| BASIC LED2, LED0/LED3 mux mutation | BASIC LED2 rejects. No LED0/LED3 mux write API exists; their tables mark mux read-only. |

Prompt 32 owns LED1/LED2 mux programming and must respect this same retained
pin policy. This stage reads and validates existing routing only. Pin electrical
levels, pulls, MCU input voltage and board wiring remain application facts and
physical Q9 work. A board declaration cannot prove that routing was applied;
the bounded live check rejects mismatches before IRQ mutation.

A HARD reset can remove an externally established alternate mux. Until the pin
owner's replay is integrated at prompt32, that case deliberately fails the live
route check and retains restoration debt instead of reporting a usable IRQ.
The dedicated pin21 route has complete replay at this stage.

Configuration rechecks exact identity, pin prerequisites and an already-active
unowned test stimulus. It disables global delivery, collects prior shared events,
writes the whole RXFS selector word, reads it through the collector, and consumes
crossing MISR2 history as ambiguous. It then replaces both enable bytes, applies
polarity/output, enables global delivery last, and verifies the owned stable
fields. MISR RMW reads retain events immediately and never echo high status bits
as writable controls. PHYSCR unrelated power and loopback fields survive.

For pin21 this is 26 frames in 14 callbacks. LED1/LED2 checks add one four-frame
callback; ENHANCED LED1 adds another. A short poll slice yields before a whole
batch. No selector or dependent RMW is split. Aggregate deadlines, frame counts,
clock contracts and stalled-poll bounds follow the shared operation contract.

Desired intent becomes authoritative at admission. Independent raw validity,
readback time and `ConfigurationTrust` do not become valid merely because a
write was accepted. Failure/cancellation freezes the result and issues no cleanup
frames. Accepted/possible effects retain OP-9; later `startInitialize` replays
the admitted policy with its own token, budget and result. It does not replace
the original error or manufacture rollback.

## One collector and selector attribution

All MISR reads use the existing collector, including service, polling,
configuration readback, raw aliases and mixed-register maintenance. The latest
MISR2 `EvidenceSample.sharedSource` records the context at consumption. Sticky
`polaritySeen`, `wakeSeen` and `sharedAmbiguous` retain separate histories. They
are occurrence evidence, not counts; later settings cannot relabel old events.

An owned selector transition first consumes pending MISR2 using its prior
context. The selector write immediately withdraws attribution, including an
ambiguous or failed write. A verified selector followed by a successful crossing
drain establishes the context for future samples. Events between the earlier
drain and selector write remain ambiguous. Reading RXFS alone never proves an
earlier event's epoch. A contradictory RXFS read withdraws known attribution
even if a later release or deadline check fails.

Raw/external mutation, reset uncertainty and WoL control changes withdraw this
context. WoL can select wake automatically; its stage-31 owner must use the same
invalidation/drain path. DP83822 default/W1S rules are not imported into DP83826I.
The RXFS SFD interpretation gate remains explicit; raw status stays available.

## Finite deferred service and partial evidence

`serviceInterrupts(output, budget)` always reads PHYSTS first, then MISR1 and
MISR2 in one three-frame callback. It never uses PHYSTS's summary bit to skip a
MISR. Consumed half-full events request FCSCR and/or RECR in one additional
callback. Maximum work is two transactions and five samples/physical frames.
`InterruptServiceBudget` independently bounds transactions, samples, frames and
the absolute deadline. There is no drain-until-empty loop.

`morePossiblyPending` is true for either nonzero event byte or an exhausted
allowance, including an incomplete pass. The application schedules another
bounded pass or periodic poll rather than relying on a new edge. Use an
appropriate level-sensitive input or explicit pin sampling. Events arriving
after a particular source read remain for a later read; software cannot promise
an atomic silicon snapshot or lossless multiplicity.

If the counter allowance is unavailable after consuming a half-full event,
`EvidenceSnapshot.pendingInterruptCounters` retains that request. Later service
reads the requested counter even when both MISRs now return zero. Any valid
read through the single collector satisfies that request; a failed destructive
read keeps the request and sticky history loss. `collectEvents` shares this
mechanism, and `clearEvidence` discards these host requests without clearing
hardware. Reset preserves history as discontinuous, not as a new exact count.

Ordinary output is unchanged on any error. `interruptServiceSnapshot()` keeps
the previous aggregate stale with the new status and possible-pending flag.
`evidenceSnapshot()` independently exposes every successfully consumed prefix,
its timestamp/validity, pending requests and possible history loss. A later read
cannot recover a lost event. FCSCR saturation/lower-bound rules remain distinct
from source-gated RECR arithmetic and from MDIO/managed-health counters.

For example, MISR1 success followed by MISR2 read-after-effect timeout retains
MISR1 and its counter requests; MISR2 is not a fabricated zero. Two cached
readers consume neither hardware nor one another's history. The owner emits
notifications once per acquisition, not once per cached rendering. Collector
sample counts and saturation expose the limits of retained identity/history.

## Software test and restoration

`startInterruptTest` requires verified owned policy and an initially clear test
bit. It sets and verifies only the persistent software-test stimulus through a
bounded operation. Success leaves `testMayBeActive`, temporary state and cleanup
owed. Application observation time is separate: an empty MISR pair while test
is asserted is not an error and does not cause a loop.

After taking that result, `stopInterruptTest` is a new operation that rechecks
identity, clears the test bit and verifies stable policy plus clear stimulus.
Cancel is bus-silent. A failed or late stop keeps uncertainty and the original
result remains unchanged. Stable-policy mismatch withdraws policy trust; a
failure solely to set/clear the test bit does not invent a stable-policy mismatch.
If persistent state is dirty, clearing test alone cannot claim full restoration.

Shared initialization replays owned policy after profile/diagnostic and autorun
restoration; it never replays test=1. NONE/PCS/HARD/RESTART share this path. A
retained temporary test requires owning stop or HARD under OP-9. Read-only
`startVerify` and ordinary link configuration verify policy without enabling it
or changing its selector. A mode change during the final identity check rejects
the operation before further IRQ writes; stale earlier proof cannot become
initialization success. External reset/POR notification is bus-silent and keeps
desired policy for later explicit replay.

## Qualification boundaries

The host [tests](../test/interrupts.cpp) cover every source, simultaneous events,
pin/mode admission, transitions, bounds, partial read effects, all apply/stop/
restore failure phases and cross-PHY exclusion. They do not prove that an IRQ
reached the MCU. Q9 requires an identified board, safe pin role/pulls and logic
analyzer/MCU timestamp capture: measure active polarity, assertion/deassertion,
level versus edge behavior, events during service, test start/clear and reset.
Record physical fixture identity and admitted maintenance; do not drive a pin
still owned by clock, COL or power-down circuitry. Q9 is NOT RUN.
