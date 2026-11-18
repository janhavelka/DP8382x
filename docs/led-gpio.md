# LED and GPIO profiles

`configurePins` applies one complete `PinSettings` profile through bounded direct
calls. It returns a `PinResult` containing partial evidence even when an admitted
application fails. `pinsSnapshot` copies retained intent, raw observations and
trust without I/O or clock sampling. The application serializes these calls and
every controller participant honors each complete indirect/RMW transaction.

The exact DP83826I definitions are SNLS647I section 8.3.18 and tables 8-42/43,
8-49, 8-73 through 8-78, 8-85 and 8-89. The [ledger](register-map.md) is the factual
register map; [independent vectors](register-vectors.md) supply literal field
tests. Names and positions from DP83826AI/AE or neighboring parts do not extend
these permissions.

## Admitted profiles

Every request requires `maintenanceAdmitted`, `macStopped` and
`outputsQuiesced`. These are owner attestations that disruptive changes are
permitted, not silicon evidence of a glitch-free transition. `availableMask`
declares the selected board pins available; bits correspond to LED0 through
LED3 rather than package pin numbers. An unconnected LED remains a usable
software feature. Requests are validated as a whole before mutation or replacing
desired intent.

`PinFunction::UNOWNED` leaves a pin outside this profile. Removing ownership does
not disconnect its old physical mux. Source, polarity and drive options apply to
LED functions; non-LED functions do not provide alternate IRQ/WoL setters.

| Pin | BASIC | ENHANCED RMII | ENHANCED MII |
| --- | --- | --- | --- |
| LED0, package pin 30 | LED source and force; fixed-low polarity | LED source, optional polarity/swap and force | Same as ENHANCED RMII |
| LED1, package pin 31 | LED source and force; fixed-low polarity; existing WAKE/INTERRUPT route | LED, existing WAKE/INTERRUPT route, subject to CLKOUT release | Same, with declared clock consumer released |
| LED2, package pin 28 | Unavailable | LED, existing WAKE/INTERRUPT route, HIGH | LED/WAKE/INTERRUPT/HIGH require unused COL; COLLISION3/5/6 retain documented COL routing |
| LED3, package pin 29 | Unavailable | LED source only; no programmable force/polarity | Rejected: source prose promises LED routing, but the mux field is read-only |

LED0 and LED3 mux fields are read-only, so no alternate GPIO function is written
on those pins. LED1's documented writable LED, WoL and interrupt selections are
admitted. Its reserved encodings and TX_ERROR/CLOCK choices remain rejected:
figure 8-10 and table 8-75 disagree about the latter functions. LED2 supports three
documented COL encodings plus constant HIGH; its reserved clock encoding is
rejected. No TX_ER encoding exists in the pin 28 mux table. No general input or
tri-state setting is invented.

The clock selector fields on LED0/1 name PLL and recovered clocks, but their
activation mux is read-only or reserved. Clock selector fields on LED2/3 are
reserved. Those programmable routes remain `pin_fields`/Q12 gates; a diagram
alone cannot authorize their writes. PHYCR's undocumented LED Configuration bit
is excluded, including recipes that label it differently without consistent
field semantics.

## Sources, drive and common controls

Each admitted LED independently accepts all fifteen `LedSource` values below.
The reserved next encoding is rejected. MLED routing remains enabled for LED0;
the reserved low nibble of LEDCFG and reserved LEDCFG2 fields are preserved.

| Source | Indicated condition |
| --- | --- |
| LINK | Link OK |
| RX_TX | Receive or transmit activity |
| TX | Transmit activity |
| RX | Receive activity |
| COLLISION | Collision |
| SPEED100 | Speed indication, high for 100BASE-TX |
| SPEED10 | Speed indication, high for 10BASE-T |
| FULL_DUPLEX | Full-duplex mode |
| LINK_ACTIVITY | Link OK with activity blinking |
| STRETCH | Internal active-stretch signal |
| AN100_FULL | 100 Mbit/s full-duplex link established through negotiation |
| LPI | Energy Efficient Ethernet LPI indication |
| MII_ERROR | Transmit/receive MII error |
| LINK_LOST | Latched link-loss indication; BMSR read clears it |
| PRBS_ERROR | PRBS-error indication; error-counter clearing affects it |

`LedPolarity::PRESERVE` leaves the observed/reset-loaded polarity untouched.
ACTIVE_LOW and ACTIVE_HIGH are admitted only for ENHANCED LED0/1/2. BASIC's
fixed-low prose conflicts with the register override offering, so even a BASIC
ACTIVE_LOW override is rejected rather than silently written. LED3 polarity is
strap-derived and observable through the existing strap snapshot; reserved
LEDCFG2 fields do not become an override.

`LedDrive::NORMAL` uses the selected source. LOW and HIGH select the documented
force value and enable for LED0/1/2. They are physical-level requests, not proof
that an attached LED is lit. LED3 has no documented force control and admits
NORMAL only. `led0SwapOwned` independently owns the MLED polarity-swap control
in ENHANCED mode; its interaction with wiring and other polarity controls remains
a physical Q12 check.

`blinkOwned` selects the single common blink period. Codes 0/1/2/3 mean
50/100/200/500 ms respectively; the library does not create per-pin timers.
`stretchOwned` controls the common stretching bypass. Neither setting establishes
measured timing, pulse visibility or IRQ delivery. Unowned global fields remain
preserved.

## Clock and source ownership

ENHANCED LED1 shares CLKOUT. The owner must declare `led1ClockUnused`; a pin
availability flag does not prove that a clock consumer can tolerate its removal.
Strap6 selects CLKOUT versus LED1 at POR and is not re-latched by ordinary reset.
`led1ClockOverride` owns the documented IO_CFG1 override that releases CLKOUT
for LED1. The procedure verifies the override and the read-only availability
status before using the mux. Without an owned override, the live availability
check must already admit LED1. No generic write to the status register or guessed
input-mode recipe is used.

ENHANCED MII LED2 requires `colUnused` for a non-COL function. Selecting its LED
function explicitly writes and verifies the mux even if the sampled reset code
is already zero: the pin table says the MII default is COL, so reset code alone
is not routing proof. The application must stop and reconfigure affected MAC
consumers before admitting this transition.

Any profile owning LED2 or LED3 first reads RCSR through the shared collector
and checks that the current MII/RMII bit matches declared wiring. A declaration
or read-only mux reset value alone cannot prove pin availability. A mismatch
rejects before target mutation. The read still consumes RC status, whose raw
sample or possible history loss remains in the collector. Later contradictory
RCSR observations withdraw pin trust; the pin API never changes interface mode.

WAKE and INTERRUPT profile entries require matching, already owned source routes.
The WoL and IRQ owners retain source activation, masks, status and acknowledgement.
The source owner configures/restores its own mux after quiescing delivery; a
matching pin entry performs no duplicate mux write. The pin profile never enables
either feature implicitly and cannot steal a pin
from them. Conversely, later source setters must honor the pin profile. Disabled
WoL still owns its selected route. EEE's clock recipe cannot coexist with an
owned conflicting LED1 route or clock-release override. Resolve conflicting
intent explicitly before applying a new profile; there is no hidden rollback or
recursive pin/source setup.

## Evidence and restoration

Each stable field update uses the existing complete MDIO transaction mechanism.
Source and force values are prepared before final drive/mux activation where
applicable. No documented silicon wait is added. The aggregate allowance covers
prerequisite reads, target updates and masked readback. The maximum all-control
profile takes 85 frames in 24 callbacks; admission counts the exact smaller plan.
Each callback holds at most one complete five-frame RMW; no controller lock spans
owner scheduling or output. A finite failure stops the current pass without an
automatic retry, reset or compensating write.

Desired intent becomes authoritative only on admission and survives partial
application. `PinSnapshot::raw` retains per-word observations; `validMask`,
`observedStale` and timestamp validity distinguish successful observations from
historical or missing words. A readback mismatch or ambiguous accepted write
withdraws trust. Raw/external mutation invalidates affected proof rather than
pretending the old settings were restored.

IRQ/WoL LED1 snapshots require verified clock routing whether the override is
owned by the pin profile or interface profile. Reading detector status cannot
repair an uncertain clock route. Dependent views retain their own timestamps.

Pin mutation is persistent. Cancellation or local teardown cannot undo a write.
An incomplete application with accepted or possible mutations leaves OP-9
restoration debt; a separately admitted
`startInitialize` restores the owned profile through the common path with its
own budget/result. NONE, PCS, HARD and RESTART retain their exact reset semantics;
the profile does not claim to resample POR-only straps. Shared replay restores
pin prerequisites before dependent source/power completion. `startVerify` checks
owned fields without mutation. A successful restoration does not replace the
original failed result.
Initialization readiness is published only after all owned restoration tails
complete; failure or cancellation in a mutating pin/interface tail leaves it false.

The existing collector owns BMSR and BIST/error evidence. Reading BMSR can clear
the LINK_LOST LED latch; the BIST counter-clear procedure can change PRBS_ERROR
indication. Cached LED views do not consume either source. A configured source
and retained collector sample do not prove the instantaneous voltage or visible
light at a pin.

## Physical qualification

Q12 remains NOT RUN. Record exact part/revision, BASIC/ENHANCED straps, MII/RMII,
board loads/pull resistors and firmware identity. Exercise every source and legal
force/polarity combination; measure blink periods, stretching, output levels and
transition behavior with a scope. Correlate link-loss and PRBS indications with
the single collector's actual acknowledgements. Verify LED1 clock-consumer
release and reset behavior, MII LED2/COL handoff, and IRQ/WoL pin coexistence.

Resolve the named mux/polarity/clock contradictions with exact-part TI evidence
or identified qualification before widening programmable permissions. Host tests
prove composition, rejection, exclusion and retained failure semantics; they do
not prove visible LEDs, signal integrity, clock quality or an awakened host.
