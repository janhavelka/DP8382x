# Interrupt configuration and bounded service

The [typed control CLI](cli.md#typed-controls) exposes explicit service, cached evidence, mask configuration and separately admitted IRQ test and stop.

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
`pinAvailable` authorizes both the selected route and pin 21's electrical role:
selecting an alternate LED mux does not disconnect the dedicated interrupt pin.

The following table describes the original I/E map. On revision-4 AI/AE,
LED0 can also route IRQ in both modes and LED3 can route IRQ in ENHANCED.
In MII, LED2 needs the COL release and LED3 needs the CRS release. Each route
uses its own mux field and preserves adjacent bits. The interface owner must
release any conflicting pin 30 clock selection before an IRQ claim.
See the [AI/AE coverage matrix](feature-coverage.md).

| Route/mode | Admission and live proof |
| --- | --- |
| BASIC pin 21 | Observed BASIC identity means strap-selected INT. PHYSCR OE is preserved and has no power-down meaning. |
| ENHANCED pin 21 | OE enables open-drain IRQ output. Requesting OE clear always requires `powerDownInputSafe`; low on the resulting input asserts power-down. |
| BASIC LED1 | Owner declares availability; source-owned mux setup and readback select IRQ. |
| ENHANCED LED1 | Also requires `led1ClockUnused` and live GPIO availability from the clock/LED status register. |
| ENHANCED LED2 | Source-owned mux setup/readback; MII also requires `colUnused`. RMII has no MII COL consumer. |
| BASIC LED2, LED0/LED3 mux mutation | BASIC LED2 rejects. No LED0/LED3 mux write API exists; their tables mark mux read-only. |

Stage 32 integrates [pin dependencies](led-gpio.md). The IRQ source owner now
configures/restores its low mux field after disabling global delivery, with one
masked readback. A matching pin-profile entry never writes the route twice.
Configuration rejects a different retained pin owner, and cannot move away from
a still-declared INTERRUPT dependency. VERIFY and stimulus operations require
the route already established and never repair it. Pin electrical levels, pulls,
MCU input voltage and board wiring remain application facts and physical Q9 work.

A HARD reset can remove the selected mux. Shared restoration now replays it;
lost clock availability still rejects unless the explicitly owned pin profile
first restores its runtime clock-release override. No POR-only strap is resampled
by assertion.

Configuration rechecks exact identity, pin prerequisites and an already-active
unowned test stimulus. It disables global delivery, applies/verifies a needed
route change, collects prior shared events,
writes the whole RXFS selector word, reads it through the collector, and consumes
crossing MISR2 history as ambiguous. It then replaces both enable bytes, applies
polarity/output, enables global delivery last, and verifies the owned stable
fields. MISR RMW reads retain events immediately and never echo high status bits
as writable controls. PHYSCR unrelated power and loopback fields survive.

For pin 21 this is 26 frames in 14 callbacks. LED1/LED2 checks add one four-frame
callback; ENHANCED LED1 adds another. A changed mux adds 9 frames/2 callbacks for
the complete RMW and readback. A short poll slice yields before a whole
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
This includes owned power and autorun policy, not only link settings. Standalone
IRQ configuration checks those other owners at admission and completion.

Every valid PHYSCR control read is retained, including the read prefix of a test
RMW whose write or release fails. A stable-control contradiction dirties trust
and remains in the result alongside the primary transport error. A contradictory
RXFS observation between selector verification and crossing invalidates the
pending selector proof even while attribution is UNKNOWN. An independent
collector call freezes that operation immediately; its next poll issues no more
frames. Valid reset or identity-change observations likewise prevent a pending
IRQ operation from restoring obsolete proof. Cached readers remain bus-silent.

Shared initialization replays owned policy after profile/diagnostic and autorun
restoration; it never replays test=1. NONE/PCS/HARD/RESTART share this path. A
retained temporary test requires owning stop or HARD under OP-9. Read-only
`startVerify` and ordinary link configuration verify policy without enabling it
or changing its selector. A mode change during the final identity check rejects
the operation before further IRQ writes; stale earlier proof cannot become
initialization success. External reset/POR notification is bus-silent and keeps
desired policy for later explicit replay.

## Qualification boundaries

LED2 route acquisition appends one RCSR read to its four-frame mux read in the
same owned batch. Current MII/RMII mode must match declared wiring before any
route write; declaration alone cannot waive COL admission. RC samples remain in
the single collector. A later contradictory mode observation withdraws route
trust and stops an active pass before another mutation.
On AI and AE the ENHANCED LED1 route also requires a clock-release sample
at 0308. A later contradictory sample stops setup even if the mux still
needs repair.

The host [tests](../test/interrupts.cpp) cover every source, simultaneous events,
pin/mode admission, transitions, bounds, partial read effects, all apply/stop/
restore failure phases and cross-PHY exclusion. They do not prove that an IRQ
reached the MCU. Q9 requires an identified board, safe pin role/pulls and logic
analyzer/MCU timestamp capture: measure active polarity, assertion/deassertion,
level versus edge behavior, events during service, test start/clear and reset.
Record physical fixture identity and admitted maintenance; do not drive a pin
still owned by clock, COL or power-down circuitry. Q9 is NOT RUN.
