# DP83826I core silicon behavior

Stages 15–16 execute the [initialization/reset procedure](initialization.md) with
NONE/PCS/HARD/RESTART, raw strap capture and unconditional mandatory VOD verification. Its
retained effects and separate restoration result do not establish link or physical
reset timing. Neither RCSR replay nor CDCR rescal is silently included.

This is the prompt-3 research contract for **DP83826I / SNLS647I**, including
DP83826IRHBR. No driver is implemented or physically qualified. The
[ledger](registers/dp83826i.json) owns numeric definitions, ordered procedures,
source citations, ownership and gates. [Independent vectors](register-vectors.md)
are the arithmetic oracle; [coverage](feature-coverage.md) assigns later work.
The [source audit](reports/implementation/3-source-audit.md) records actual
manufacturer/upstream and sibling revisions and licenses.

## Identity and admission

Retain the declared orderable part, both raw ID words, OUI, model, revision,
operating mode and accepted capabilities separately. BASIC and ENHANCED are
observable model distinctions. Shared IDs do not establish an I/E temperature
suffix, A-generation support or interchangeable packages. Revision 1 is the
documented original-part baseline; another revision needs an explicit evidence
and acceptance decision, not a wildcard mask copied from Linux.

Both ID reads must succeed before publishing a new identity. Preserve previous
output on a failed second read, record transport failure and make previous
evidence stale. All-zero/all-one samples fail plausibility; they do not prove
that hardware is absent. Reject invalid variants and unsupported interfaces
before I/O from the declaration or valid cached capabilities. Live identity
validation is a separate bounded acquisition. PHY protocol address range and
the three physical strap address bits are different constraints.

## Register and observation ownership

One application owner serializes every call, including cached snapshots. The
controller lock spans each complete indirect or RMW transaction across all bus
participants, including SDK access and other PHYs. REGCR/ADDAR cannot be used by
an independent raw client. Reject invalid Clause 22 register numbers before any
callback; a reset mask is never a register address. Indirect addressing does
not imply native Clause 45 support.

Every selector phase must succeed before the next phase. A failed selector write
can leave the old target selected; do not continue to a data operation. The next
explicit transaction re-establishes all selectors. No selector cleanup write is
needed. A failed or late data write may already have applied; preserve uncertainty
and invalidate affected configuration trust. Do not retry automatically.

Ordinary read results are staged until success. Destructive samples are retained
immediately, even if the aggregate subsequently fails. Failed destructive reads
can lose unreturned history and must say so. One collector owns BMSR latched
link/fault, ANER page events and the related PHYSTS mirrors. PHYSTS read-clear
labels do not mean every flag clears on a PHYSTS read: its field descriptions
name RECR, 10BTSCR, FCSCR, ANER, MISR and BMSR as different clearing owners.
Signal/descrambler RC observations are retained too. A diagnostic dump uses
cached destructive evidence unless explicit collection is admitted.

Mixed-register writes preserve reserved stable state and compose every command's
neutral value. Reset/restart W1S fields are zero except the requested command.
The schema also represents W0C/W1C for the later event/wake audit; W0C neutral
is one, not zero. No current core field is invented as W1C merely to exercise
that vocabulary. ANAR remote fault is prohibited despite R/W notation; partner
remote-fault observations remain legitimate.

## Reset domains and readiness

| Trigger / source terminology | Wait / completion evidence | Settings | Identity and mode evidence | Strap resampling | Restore owner |
| --- | --- | --- | --- | --- | --- |
| `startInitialize({PCS})`, BMCR PCS reset (table 8-19) | Caller interval, no fixed silicon maximum; BMCR reset bit clear | PCS reset; vendor retained. Exact full domain remains `pcs_reset_extent`. | Withdraw old permissions, reacquire IDs at configured address | Reload stored strap values | Core verifies mandatory VOD now; future owned PCS profiles extend same path |
| `startInitialize({HARD})`, PHYRCR hard reset, pin-equivalent wording (table 8-48) | Persistent frame-free floor: completion upper + max(2000 us, interval); PHYRCR hard bit and other command clear | All configuration may be lost, including VOD; old observations invalid | Withdraw permissions; reacquire IDs/address reachability and captured straps, without scanning | Eligible straps **possibly** resample; software-reset wording remains arguable (`hard_reset_straps`, Q5). ENHANCED Strap1/6 exceptions retained. | Same core path: identity/readiness, mandatory VOD, then future validated desired profiles |
| `startInitialize({RESTART})`, PHYRCR circuitry restart (table 8-48) | Caller interval; no HARD floor or fixed maximum; PHYRCR restart bit and other command clear | Registers retained; live link/counter epoch stale; temporary vendor stimuli not proved stopped | Preserve initialized/identity until later verification or VOD mutation requires invalidation; refresh IDs before publication | No new physical resampling established | Same mandatory initialization/reverification path; no transient commands or temporary settings replayed |
| Application RESET_N + `invalidateDeviceState(EXTERNAL_RESET)` | Low at least 25 us without debounce capacitors; wait 2 ms after actual release; application establishes readiness | Configuration lost; notification makes observations historical, clears current cleanup by attestation | Withdraw initialized/capabilities; reacquire at declared address after readiness | Eligible straps; ENHANCED Strap1/6 are not re-latched | Application owns GPIO/readiness; later explicit initialization owns restoration |
| Application POR + `invalidateDeviceState(EXTERNAL_POWER_ON)` | Wait 50 ms from final supplies; delayed-clock rule below; application establishes readiness | Configuration lost; no implicit recovery or additional reset | Withdraw initialized/capabilities; declared ordering suffix remains a declaration | All straps, including ENHANCED Strap1/6 | Application owns supplies/clock; later explicit initialization owns restoration |

Do not generalize ENHANCED POR-only exceptions into an unsupported BASIC reset
claim. BASIC speed/isolation straps have their own tables. The BASIC/ENHANCED
pinout choice is a hardware configuration, not a software mode setter. Read SOR
as captured straps/loaded configuration, not as a mirror of later software
changes. Its printed defaults are not invariant values on every board. PHYSTS's
published reset value is also not a universal post-strap observation: its forced
speed/duplex descriptions conflict with those default bits. Retain the published
value as source evidence, never as a persistent configuration readback assertion.

Software reset completion is the relevant self-clearing command becoming zero
at its actual register. Another active reset command still prevents readiness.
AN restart becoming zero means initiation, not completed negotiation or link.
No inspected software-reset field gives a maximum completion time. Use an
application aggregate deadline and a finite poll count, separate from the
specified external-pin/power timing. Nominal signaling times are not maxima.

The application establishes valid power/clock/reset history before management
access. The ledger records minimum pin pulse, power-up and deassert-to-SMI waits
with their source qualifiers. Anchor any minimum post-action wait at observed
completion or a conservative later timestamp. Release the controller lock before
waiting. A timeout, cancellation or teardown never retroactively undoes an
accepted reset. Retain the primary result, pending effects and cleanup result.

The [prompt-6 ownership design](design-decisions.md) assigns external power-up
and hardware-reset SMI silence to the board/controller coordinator across the
entire shared bus, including other PHY addresses. Close admission before the
transition and drain active frames; release the mutex during the readiness wait
while logical admission remains closed. HARD also enforces the conservative
OP-14 floor described above. Its expiry is not proof of the internal reset-release
time or completion; Q5 must measure those separately. PCS/RESTART do not inherit
that floor. RESET_N/POR table values are 2 ms/50 ms MAX readiness limits used as
waiting windows, whereas 25 us is a MIN pulse and FLP 1500 ms is NOM.

If clock arrives after the ramp, Figure 7-1 recommends holding RESET_N low and
releasing at least 100 us after clock stabilization. Alternatively §7.6 footnote2
requires an additional reset after POR, initiated at least 100 us after both
clock stabilization and POR completion. Apply the 2 ms release-to-SMI wait too.
Notification has zero callbacks and cannot establish these physical conditions.

## Initialization and link configuration

Every `startInitialize`, for every reset kind, writes and verifies the exact
VOD_CFG3 recipe. This unconditional library policy covers later 10BASE-Te use,
including advertisement. POR, pin and PHYRCR hard reset lose the setting; PCS
reset and circuitry restart retain it, but initialization repeats the write.
Readback and trust must
still reflect external/raw mutations. Table 8-81 marks both termination-value
fields and their enables read/write, consistent with the required initialization.
Reserved bits still cannot be tuned arbitrarily. Other analog-control procedures
remain assigned to prompts 5 and 35; there is no read-only-field conflict here.

Advertise any legal nonempty subset of 10/100 half/full capabilities, subject to
the application policy. The initial board's 100/full-only choice is not a core
restriction. Validate the entire technology selector, local remote-fault
prohibition, pause and next-page settings before mutation. Apply advertisement
and companion settings before enabling/restarting negotiation. Keep extended
full-duplex compatibility opt-in and coordinated with the partner; it can change
the ordinary IEEE result with a forced 100M partner.

Forced 10/100 half/full configuration uses BMCR with AN disabled. CR1 requires
robust crossover in forced operation. PHYCR chooses automatic versus manual
pair selection; CR3 pair and polarity swap are separate controls, with both used
for port mirroring. Apply the complete coupled settings under operation admission,
verify stable masks and retain partial effects if any write fails.

Coherent AN-disabled BMCR and current link-up establish the configured line mode
under decision D4: speed and duplex come from BMCR, while PHYSTS forced bits stay
raw. Forced 100-half with extended full duplex not known disabled retains speed
100 but duplex UNKNOWN and invalid mode. The adapter rejects a requested
100-half plus extended policy. Forced operation never establishes negotiated pause.
The physical `forced_physts` question remains Q11, without blocking this selected
local interpretation.

Next-page support includes local message/unformatted code, more-pages and ACK2,
hardware-controlled toggle, partner ACK/toggle/page fields and consumed page
evidence. Reserved message codes cannot be transmitted. Each unformatted-page
series requires a preceding message page defining its use; referenced pages must
follow immediately in the defined order. Bound page count and time,
including null pages while the peer continues. Preserve a consumed event with missing
payload if the subsequent partner read fails. Protocol message interpretation
belongs to the relevant IEEE/application contract; receiving an unrecognized
raw code must not silently turn it into a supported operation. EEE page/address
research retains explicit wire/control gates. IEEE's restart state clears the next-page
loaded flag: prepare desired data locally before restart, but load ANNPTR after
fresh base-page receipt. Distinguish BASE and NEXT events, retain received payload
before advancing, and finish only when both sides are done and BMSR reports AN-complete. The selected IEEE sequence remains physically unqualified under Q10. Next-page
intent belongs only to the exchange epoch: success clears it without restart;
failure/cancel retains cleanupOwed, and separately admitted initialization restores
ordinary advertisement with next-page clear and restarts. A peer declining next pages is not a transport failure.

Pause RX and TX resolve separately, only after AN selects a full-duplex common
technology. The ledger records the independently reviewed complete truth table.
Half duplex, forced mode, incomplete negotiation and stale link evidence do not
establish negotiated pause. The SDK's Boolean projection is a separate adapter
concern; it does not replace the portable two-direction result.

## Finite replay and failure handling

The accepted procedures in the ledger include prerequisites, ordered steps,
owned masks, waits, timeout policy, partial effects, cleanup and restoration.
Their planned API/CLI names express ownership, not already implemented signatures.

After full reset, first establish identity/address/mode and readiness; then apply
required initialization and persistent interface/clock/power dependencies, coupled
crossover/compatibility, advertisement/pages, AN or forced activation, and event
enables. EEE initialization owns its reset-bearing sequence; replay other settings
acyclically without recursively resetting EEE again. Do not replay temporary
diagnostic settings, sampled commands or consumed events as desired state.

Each frame is bounded and terminal. The aggregate budget includes admission,
lock acquisition and controller execution; a finite frame/poll limit still applies
if the clock freezes. Zero work budget performs no I/O. Cancellation/cleanup uses
its separately admitted finite allowance and does not overwrite the primary
failure. Incomplete replay is not configuration trust; successful register access
is not link or application readiness. Cached snapshots perform no I/O.

## Diagnostic sessions and retained evidence

Prompt 4 researches the full loopback, BIST, cable, event/counter and quality
families. The ledger's ordered procedures own their detailed settings; the
[diagnostic source review](reports/implementation/4-diagnostic-source-review.md)
and [event review](reports/implementation/4-event-source-review.md) identify
original tables and independent arithmetic. These are future operation contracts,
not executable PHY support or physical qualification.

Admit disruptive tests only after exact identity, mode, speed, interface and
fixture prerequisites are satisfied and the application has coordinated MAC/
stack ownership. Save persistent owned settings before the first mutation.
Validate locally before I/O; budget any necessary live prerequisite reads as a
separate phase. Every session has a finite primary duration and transaction/poll
limit supplied at admission, and a separate finite cleanup allowance. These are
application policy, not undocumented silicon completion times. Never hold the
controller lock while waiting for hardware or printing results.

Near-end loopback disables negotiation except for the documented external-loop
exception. MII loopback's speed-dependent companion selection follows the detailed
BMCR table, retaining its disagreement with the all-speed feature sentence.
PCS input is 10-only, digital is 100-only, analog requires the documented
termination, and reverse loopback ignores MAC-originated data. The digital
example touches three recipe-only offsets and restarts circuitry; it cannot be
reduced to a freely writable BISCR flag. A successful internal loop proves only
the exercised path, not a cable, partner, MAC or application outside that path.

BIST uses a PRBS15 generator/checker with packet and interpacket controls.
Error count, checker lock, received evidence, observation interval and saturation/
wrap uncertainty belong in the finite result. Merely enabling the generator,
receiving traffic or reading zero errors before checker lock is not a pass.
BICSR1's read counter overlaps its write freeze/clear action: use the explicit
write action while preserving IPG, never echo a sampled counter into a write.
Retain the frozen sample before another freeze/clear. Continuous counter wrap
means an observed zero alone does not prove zero errors over an unbounded run.

TDR requires a disconnected or quiet partner, or the documented link-down
autorun condition. Its reset-time done indication is not a new measurement.
Associate completion and every result word with an admitted trigger/epoch,
configuration and segment; if freshness cannot be established, report uncertainty
instead of successful measurement. Distinguish stopping autorun from stopping a
manual operation. Do not read reserved result banks as additional peaks. The
documented raw location, amplitude and sign evidence remains separately available.
Decision D3 selects the six-run SNLA428 transmit-A recipe and its source-implied
derived helper; the datasheet tuning remains a distinct raw capability. Do not
mix recipes or infer pair-B/whole-cable health. Retain per-run validity, NOT_RUN
for unperformed rows and NO_PEAK when no threshold qualifies. The matching-row
location and whole peak word drive interpretation, never a guessed sign bit.
Caller propagation delay and Q7 fixture evidence remain explicit. Opaque tuning
requires hard-reset exit and later initialization, not invented saved-word replay.

Cancellation, failure and timeout stop new primary phases and preserve accepted
or uncertain hardware effects. Cleanup disables temporary stimulus, restores
only saved persistent owned settings in dependency order and verifies stable
bits. Recipe-only offsets without documented readback/restoration need the named
reset/reinitialization recovery contract; arbitrary saved-word replay is not
proved safe. A failed cleanup leaves configuration trust invalid and the pending
effect visible. Successful cleanup cannot erase the primary error. Full reset
invalidates diagnostic results and reapplies required 10BASE-Te initialization;
restart and PCS reset retain their distinct core contracts and uncertainty.

## Events, counters and signal quality

One collector owns MISR reads, counter reads and reads that clear related latches.
Read effects depend on the clearing register, not just the displayed access
abbreviation. PHYSTS duplicates several events whose clear source is BMSR, ANER,
MISR, RECR, FCSCR or 10BTSCR. Retain each successful sample immediately with its
source, epoch, timestamp and validity; a later failed read does not erase it.
An ambiguous failed destructive read records possible history loss. Aggregate
snapshots retain their prior value with explicit stale/incomplete state; ordinary
cached views and safe dumps perform no destructive acquisition.

MISR masks and global interrupt controls share this collector path. Mask updates
can themselves consume events if implemented with a read; route that sample to
the collector. ISR work requests owner service and does not perform MDIO.
Finite drain limits prevent an asserted interrupt or continuous traffic from
creating an unbounded service loop. Current link state, latched events, counter
intervals and transport health are separate observations.

RECR's documented width and saturation prose disagree; keep raw words and
ambiguity rather than silently selecting an eight- or sixteen-bit accumulator.
RCSR fault polarity and read-clear/reset interaction also remain explicit. MSE
uses the datasheet address for raw acquisition; the troubleshooting address and
SNR calculation do not establish an alias or a dB API. SNLA495 corroborates the
datasheet address, but its system-specific noise threshold is not a universal
signal-quality classification. PHYSTS forced-mode conflicts remain coordinated
with core link and future SDK behavior.

Fast Link Drop is MII-only. Its physical response limits and enabled-detector
conditions do not define an MDIO observation timeout or apply to the P4 RMII
configuration. The MLT3 prescribed configuration and the prose threshold/window
are inconsistent; retain both with a named gate. EtherCAT ESC error counters,
PDI access and F28P65 reset workarounds are application-specific comparisons,
not PHY or ESP32-P4 master procedures.

## Power, EEE and wake

IEEE power-down, PLL disable, deep power-down and active-sleep policy are separate
operations. Enabling active-sleep policy can succeed while a partner keeps the
PHY awake; observe state independently, without a wait-until-sleep loop.
An asserted ENHANCED power-down pin can override the software request.
Deep-power entry has a documented order, but its complete exit/readiness and
shared opaque-register composition remain source-gated.

EEE keeps capability, requested advertisement, partner and LPI evidence separate.
D2 selects five source-implied wire tuples from the recorded cross-part evidence;
the ledger distinguishes their printed addresses, safe reads and collector reads.
There is no digit-stripping rule or vendor alias. Initialization checks the whole
capability word before any write, then verifies the whole advertisement word after
hard reset before continuing the opaque recipe. Failures stop the sequence and
retain its reset/accepted effects. Q1 is still physically unqualified. Conflicting
clock/LED ownership rejects before I/O; the sourced clock recipe row itself is
eligible. Disabling EEE requires hard reset and reapply, with no invented inverse.

Forced TX LPI start and stop are separate bounded operations. Each modifies and
verifies the documented request bit, then returns; the application owns dwell,
MAC drain and post-exit coordination. REQUESTED/UNCERTAIN block other mutations
but permit safe observation. No hardware session deadline or readiness event is
invented. All four EEE timing values remain nominal; the PHY buffers no MAC frames.

WoL configuration quiesces detection, retains pending collector evidence, writes
and verifies credentials, configures source/routing and enables the detector last.
TI MAC byte numbering runs from the explicit most-significant byte 5; API MAC
octet 0 is the first conventional octet. Password numbering is separately defined
by its own tables. Ordinary RXFCFG1 writes use W0C neutral one; explicit level
acknowledgement writes zero while preserving stable settings. Pulse indication
and MCU wake are separate physical outcomes. The named mask flag supplies no
custom-pattern engine, and the sources establish no lossless atomic live update.

## Pins, interfaces and exceptional stimuli

Board wiring, clock consumers and POR straps constrain programmable overrides.
Use a per-pin legal-function matrix, including source, direction, polarity,
drive, blink/stretch and interface conflicts. Read-only LED0/3 mux fields,
reserved LED2/3 clock selectors and the contradictory LED1 clock route remain
source gates. Documented source and drive controls are independently eligible.
Interrupt and wake sources belong to their owners; configuring a pin includes
the finite budgets of every delegated source operation without recursive calls.

MII/RMII, elasticity, repeater, isolation, odd-nibble detection, IPG and transmit
controls require their exact mode prerequisites and stable verification.
Odd-nibble detection is disabled by setting its control bit. Next-page exchange
belongs to prompt 17, extended duplex/crossover to 18, isolation to 33 and
collision testing to 35. RMII repeaters coordinate both PHYs and reserve both
operations' budgets; success on one does not imply atomic configuration.

Analog amplitude settings include a cross-register code; validate the complete
request and establish the current code before writing. Every intermediate code
must also be documented. When its high field changes, stage the low field through
the shared legal encoding, verify each write before continuing, and use the same
path for restoration. MAC quiescence does not disable analog output or establish
glitch-free updates. Failed or uncertain transfers stop dependent writes;
cleanup must re-observe the current code before choosing a restoration path.
Retain partial application and cleanupOwed state when it cannot be established. Raw DAC
fields do not prove physical units, and the receiver-gain range lacks an exact
code map. All-reserved analog entries remain unwritable. IOCTRL examples touching
reserved bits are not accepted field settings.

Compliance is a finite, fixture-admitted stimulus session. The exact sourced
fourteen 10-Mbit pattern actions are narrow mask exceptions; upper reserved bits
remain protected and codes E/F are not 10-Mbit patterns. The 100-Mbit recipe's opaque preservation read
and composition remain blocked pending exact evidence. Stop emission or complete
the authorized hard reset before restoring COMPT or a persistent profile.
Failure to prove stop retains possible ongoing emission and cleanupOwed
state. Restoration never replays the temporary stimulus.

## Gates and future public documentation

The ledger names every conflict, selected conservative behavior, owning stage
and next evidence. A research record can be complete with such a gate; the affected
implementation/qualification remains open. Physical qualification is NOT RUN.
No fabricated measurement or passing schema test closes a silicon conflict.

From the first C++ declaration, Doxygen must cite the exact ledger/source table
for non-obvious access or sequence rules and document units, legal states,
ownership, side effects, output validity, failure uncertainty and bounds.
Power/WoL/LED declarations must additionally cite mixed-command neutrality,
API-versus-source octet numbering, per-pin legal encodings, nominal timing
qualifiers and recipe-only exceptions. Name any source gate that rejects an
operation before I/O. Future tests must exercise each transfer/cancellation
boundary, finite frozen-clock limits and stop-before-restoration ordering.
No placeholder C++ is needed to record this research. Vendor bytes/notices remain
unchanged; factual GPL comparisons do not authorize importing GPL implementation.

## Prerequisite audit refinements (WP3)

Any attempted PHYRCR hard-reset command imposes a bus-wide frame-free 2 ms floor
from its completion upper bound, including ambiguous failure; the owner also
honors a longer poll interval. Other PHY addresses share that silence. This is a
library policy pending Q5 measurement, not host proof of controller timing. Treat
eligible straps as potentially resampled, while ENHANCED straps 1/6 remain POR-only.

Analog and reverse loopback use the sourced digital restart; MII/PCS do not gain
one by analogy. The analog troubleshooting tuning word conflicts with the legal
field range and is rejected. Restore robust crossover and pair policy after
near-end testing. External cable loopback remains a separate physical fixture.

Read PHYSTS before dependent clearers when mirrored history matters. Service both
MISRs regardless of PHYSTS's interrupt mirror, preserving possible further pending
work at a drain boundary. RXFS selects the shared polarity/WoL event; cached or
unknown selection cannot justify two event labels. Its configuration is a sourced
whole-word action with collector verification, never destructive RMW.

BMCR power-down observation is the software request OR the external pin. A clear
request with an observed set bit is WAKE_BLOCKED_OR_UNCONFIRMED after one write,
not a retry loop. Requested state and BISCR active-sleep observation stay separate.
WoL reset is not off. Before RXFCFG1 writes, retain RXFS and MISR2 evidence; skip
unchanged stable policy. W0C follows the exact-part table pending Q3 physical
comparison with contradictory cross-part/shared-driver intent. DP83822 MAC word
values match DP83826; their table byte labels differ, while acknowledgement differs.

BASIC exposes LED0/1 and fixed active-low polarity. ENHANCED RMII exposes LED0-3;
ENHANCED MII LED2 requires free COL and explicit verified mux programming, and LED3
has no writable route. ENHANCED LED1 also depends on CLKOUT selection. Conflicting
TX_ER pin codes and PHYCR LED-configuration semantics remain Q12 gates. Fast Link
Drop allows disabling all causes on RMII, but no nonzero cause policy; printed
response maxima for modes 2/3/5 require RX-error or energy-loss detection enabled.

Stage 17 executes [these negotiated procedures](negotiation.md) with finite
caller budgets. Pending negotiation is explicit, including a new AN cycle with
NP enabled and no page loaded. No page-loading retry or hidden NP cleanup occurs
on failure/cancel; a new restoration operation owns those effects. Supported
local message semantics are null, remote fault, OUI and PHY identifier with their
ordered unformatted pages. Other technology/extended-only/reserved message codes
are rejected, and received raw codes never activate features implicitly.
