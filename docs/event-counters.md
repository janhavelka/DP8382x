# Event and counter acquisition

`collectEvents(output, budget)` reads PHYSTS, FLDS, MISR1, MISR2, FCSCR and RECR
once, in that order, using one six-frame terminal batch through the existing
collector. `counterSnapshot()` copies the last complete aggregate without I/O.
`evidenceSnapshot()` exposes independently retained partial samples. The
[portable example](../examples/portable/Events.h) acquires once and formats any
number of copied views, checking buffer truncation without accessing the driver.

The serialized application owner admits exact-part diagnostics and shares the
controller lock with all PHYs, MMD diagnostics, link reads and later IRQ service.
The same OP-14 reset WAIT and hard-reset floor apply. No lock spans a wait; this
call has no waits, drain loop or retries. Short frame allowance or an inadequate
remaining callback bound rejects before MDIO. The budget includes acquisition,
execution and release. Source-backed host behavior does not establish physical
bus timing or error rates.

## Sources and ordering

The [ledger](registers/dp83826i.json) and [named register fields](../include/DP8382x/CommandTable.h)
own numeric masks and per-field access. The six-source default supplies typed
current-pass event masks and raw words. Explicit `acquireEvidence` requests can
also select BMSR, ANER, RCSR, 10BTSCR and RXFS through that same collector.
PHYSTS comes first when requested, before reads that clear its mirrors. Live
link observation has its own bounded consistency plan using this retention path.
ANER acquisition is rejected while the typed next-page operation owns its events.
Requesting all eleven explicit sources is bounded to fourteen physical frames
in two callbacks (eight plus six); RXFS contributes a complete four-frame MMD
read. The typed six-source pass adds no implicit RXFS or ANER acquisition.

| Source | Meaning of returned data | Retention/limitation |
| --- | --- | --- |
| MISR1/MISR2 | High-byte events; low-byte enables | Reading consumes events. Retained event union excludes enables; distinct events use their register's named masks. |
| FLDS | Read-clear fast-link-drop causes | Raw word and cause union; not a resolved link result or safe-dump read. |
| FCSCR | Low-byte saturating false-carrier counter | Reading clears it. A saturated sample is a lower bound, not a wrap delta. |
| RECR | Full raw read-clear word | Printed field width and saturation disagree. No qualified numeric sum, width, modulo delta or rate. Q16 remains open. |
| PHYSTS/BMSR | Current bits mixed with latched history/mirrors | Preserve raw/one/zero evidence; event collection alone does not resolve current link. |
| ANER/10BTSCR | Page/polarity history with clearing dependencies | Captured during all admitted reads and future source-specific RMW/readback. |
| RCSR | Raw RMII word including conflicting RC fields | No fault-polarity/rearm interpretation; Q17 remains open. |
| RXFS | Raw filter/wake status and source selector | SFD configuration-reference gate remains. An observed selector does not establish its earlier event epoch. |

MISR2's shared polarity-or-WoL bit stays unclassified here. No extra RXFS read is
added merely to label it. DP83826 reset selects WoL, but generic history may span
earlier state and unknown writes. Prompt27 owns qualified selection and its
whole-word writes; WoL enable/reset invalidates that future selection knowledge.
DP83822's default and W1S semantics cannot be imported into this primary-part API.

## Partial evidence versus complete output

Suppose MISR1 returns an event, then MISR2 times out after its hardware latch has
cleared. The ordinary CounterSnapshot remains unchanged. Its cached predecessor
becomes stale with the returned error. The MISR1 sample immediately retains its
new word, event union, timestamp validity and sample count. MISR2 retains old
history with `historyLost=true`; it does not report a new zero. Any other valid
prefix survives later release, deadline or clock failure too. The raw transport
receipt preserves primary and release statuses separately.

Each per-source `valid`, `stale`, `timeValid` and `historyLost` flag is independent.
Missing optional frame timestamps use the conservative completion upper bound.
Malformed timestamps or clock failure cannot manufacture a valid new time. Zero
is a legal timestamp. Complete aggregate publication requires success for the
whole bounded acquisition; it does not claim simultaneous silicon sampling.
An event arriving after one source's read remains for a later acquisition, while
an event arriving before another source's read can appear in this pass.

Any subsequent collector read makes the prior complete counter aggregate
historical until another complete `collectEvents` succeeds. Reset, external or
configuration uncertainty marks samples and aggregate stale. Clock faults also
withdraw aggregate use without changing its historical values/time. Cached
getters and safe-dump formatting never consume evidence a second time.

## Retained history and counter domains

The fixed per-source window starts at binding or explicit `clearEvidence()`.
`raw` is the latest exact returned word. `observedOnes`/`observedZeros` retain
raw unions, including latch-low evidence. `eventBits` retains only sourced event
bits for MISRs, FLDS, ANER, 10BTSCR and RXFS. `coalesced` explicitly means event
occurrence is known but multiplicity is unavailable, even after one hardware
latch sample. This is not an allocated queue or a lossless record of flaps.

`samples` counts successful consumed words; `samplesSaturated` reports attempts
to advance past the retained limit. FCSCR `sampledSum` accumulates its low byte,
with separate `counterSaturated` (hardware), `sumSaturated` (host arithmetic),
and sticky `counterLowerBound` flags. Ambiguous lost reads and history
discontinuity also force the lower-bound flag. A later zero cannot repair them.
Without those flags the sum is exact for returned samples, not automatically
the PHY lifetime or any wall-time interval. RECR `sampledSum` stays zero because
arithmetic is gated, not because the device had no receive errors.

Reset/restart does not discard host history. `discontinuous` records that the
retained unions/sums cross uncertain hardware history. New latest words can be
fresh while these unions/sums remain mixed history. There is no per-reset epoch
archive or attribution of old bits to a later reset. Sample validity/time and
discontinuity must be inspected together.

`clearEvidence()` is an explicit, bus/clock-silent discard of **host** collector
history and the counter aggregate. Copy any needed evidence first. It refuses
callbacks, polls and running operations, and preserves link, configuration trust,
identity, transport counters and retained operation results. It never clears
hardware: the next returned sample may contain events predating the discard.
Binding/end also begin a new local history. Multiple cached readers do not clear
or acknowledge one another's evidence.

Chip receive/false-carrier samples are distinct from actual MDIO frame counts in
`transportSnapshot()`, and from managed health introduced in prompt21. Cable
traffic errors do not imply an MDIO failure; successful MDIO cannot prove clean
Ethernet traffic or application readiness.

See [independent literals](register-vectors.md#prompt-20-event-and-counter-vectors),
[source audit](reports/implementation/20-source-review.md) and
[implementation verification](reports/implementation/20-event-counter-acquisition.md).
Physical event/counter, Q16 and Q17 measurements remain NOT RUN.
