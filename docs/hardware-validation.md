# DP83826I and DP83826E hardware validation

DP83825I Q15 needs an identified board for each measured role. Record the
actual package, ID words, SOR1[3], RCSR[7], XI and pin 2 rate and direction,
plus RESET_N, first idle MDC cycle, first access and negotiated link. Capture
the 25 µs reset pulse, 2 ms reset-to-SMI and 50 ms POR intervals, including
the stable-clock margin when XI arrives late. Measure pin 2 under each
approved isolation/mux/power change and follower setup timing with the paired
RCSR/0042 settings. Managed repeater tests require simultaneous external
reset of both identified PHYs. These physical observations remain pending.

For each of DP83826AI and DP83826AE, retain the BOM grade, ID1/2, revision,
mode, address, SOR1/SOR2 and live RCSR before testing. Record POR/RESET_N,
the reference clock and first allowed SMI edge. Measure 10/100 link outcomes,
both RMII clock roles, pin 30 PLL/recovered clocks with a known partner and
isolation effects on dependent clocks. For MII FLD capture Strap11, CR3,
0117, 0131, the single collected FLDS word, fault stimulus and independently
timed link loss. Qualify baseline-wander and EMC conditions separately for
each identified board. AI and AE physical results are pending. [Procedure
vectors](registers/dp83826a-procedures.json)

The campaign runner implements the cases below through the shared CLI.
Prompt 50 exercises them using the compiled host fixture, offline replay and
an injected serial backend. Use an identified, approved fixture for physical
execution in prompts 72 and 73. A transcript proves the assertions recorded
for that run. The board, wiring, power sequence, cable and peer identifiers
are supplied declarations. The compiled fixture's simulated link and reset
registers are offline execution evidence.

The original DP83826I and DP83826E use the same revision-1 BASIC and
ENHANCED IDs. Record the ordering grade from the fitted part or assembly
record and require the same declaration in the console handshake. Each
grade needs its own identified fixture for physical qualification. The
two ESP32-P4 examples declare the fitted I. [I/E source record](reports/implementation/56-e-support.md#shared-silicon-record)

## Common fixture and owner admission

Record the fitted part number, board/netlist revision, silicon ID and mode,
MII/RMII wiring, address, reference clock, firmware revision, console and
authorization. Verify supplies, magnetics, differential pairs, RESET_N,
MDC/MDIO and straps against the board schematic and the
[initialization waits](initialization.md#board-readiness-and-ownership).
Supply an unmodified mandatory VOD initialization profile for the VOD cases.
Qualify console open/close and exclusive attachment separately. The saved
P4 netlist's PHY supply and the board's reset/clock/strap checks remain open.

Use `--suite campaign --case ID` with `--operating-mode basic|enhanced`,
`--polling exclusive|concurrent`, `--board`, `--authorization`, verified build
identity, clock and console settings. If a peer is required, supply its
identifier, exact `--peer-profile` from the table, and the cable identifier.
For `open`, record the deliberately disconnected topology under the same
fixture authorization. Keep peer settings, bench wiring and authorization
records with the run. The cited sources define the accepted exact-part
IDs and the distinction between advertisement, partner ability and observed
resolution. [Identity](identity.md) · [Link observation](link-observation.md)

For the basic example, stop the application and confirm maintenance, no
traffic and no shutdown or retirement. For the owner example, release any
raw session, then stop and check the maintenance phase and fence. Use a
fixture record that establishes exclusive polling for the whole-bus delta.
The campaign blocks this delta assertion with concurrent polling because
the current CLI has whole-owner counters. A cold-start observation may use
concurrent polling and leaves whole-bus delta unasserted. Source-proved
inapplicable combinations such as DP83826I/E RGMII remain outside the case
matrix. Missing compatible equipment yields NOT RUN. Missing INT wiring
or a missing supported owner/evidence route yields BLOCKED. Poll-based rows
remain eligible without INT wiring.

## Case table

Every row starts with the correlated session and the common cached checks
below. `A N P` denotes `configuration apply an N P auto 0`. `F S D`
denotes `configuration apply forced S D auto 1`, with robust crossover.
The table gives measured/asserted outcomes separately from the required
fixture declaration. Every mutating row independently verifies and restores
the entry profile using the procedure below.

| Case ID | Minimum fixture / peer profile | Commands and evidence beyond the common checks |
| --- | --- | --- |
| `observe` | Declared board, exclusive stopped owner, qualified console | Bracket the cached group, perform `registers write c22 24 65536`, require exact range rejection and replay the whole cached group. Require equal state and unchanged, unsaturated frame totals. |
| `cold-boot` | Authorized power and boot record, known prior boot cookie and qualified console after board waits | Declare `--power-record` and `--previous-boot`. Require a distinct boot, first request, ready initialized health, accepted identity and equal VERIFIED desired/observed profile. If the application is already stopped and in maintenance, exclusive mode adds `registers read mmd 31 0x030E`. A running application requires concurrent mode, which uses cached evidence. Supply separate instrument records for rails, clock, RESET_N and POR timing. |
| `int-pin` | Wired INT_N and identified edge instrument | First board has no verified INT_N wiring. Record BLOCKED. Physical pin stimulus and instrument procedures belong to diagnostic qualification. |
| `owned-apply` | Stopped board with an owned, verified profile | `A 9 symmetric`; verify and read back all target profile fields, then reapply and verify the saved profile. |
| `pcs` | Same as owned-apply | `recovery reset pcs`; require requested/effective PCS, required/accepted/complete reset and replay, then verify the unchanged entry profile. |
| `hard` | Same as owned-apply, board qualified for the reset route | `recovery reset hard`; require requested/effective HARD and its own acceptance, completion and replay, then verify the saved profile. |
| `restart` | Same as owned-apply | `recovery restart`; require circuitry RESTART acceptance, completion and replay, then verify the saved profile. |
| `10base-te` | `an-10-half`, 10 Mbit/s copper peer and cable | `A 1 none`, `link partner 1`, then `registers read mmd 31 0x030E` must return DEVAD 31, offset 782 and 19008 (0x4A40). Electrical waveform claims require suitable instruments. |
| `100-full-only` | `an-100-full-asym`, restricted 100/full AN with asymmetric pause only | `A 8 both`, `link partner 8`; enforce exactly local mask 8, qualified peer mask 8, 100/full and local RX pause enabled/TX pause disabled. |
| `an-10-half` | `an-10-half`, restricted 10/half AN | `A 1 none`, `link partner 1`; require mask 1 on both sides and resolved 10/half without qualified pause. |
| `an-10-full` | `an-10-full`, restricted 10/full AN | `A 2 none`, `link partner 2`; require mask 2 on both sides and resolved 10/full with both pause directions off and validity present. |
| `an-100-half` | `an-100-half`, restricted 100/half AN | `A 4 none`, `link partner 4`; require mask 4 on both sides and resolved 100/half without qualified pause. |
| `an-100-full` | `an-100-full`, restricted 100/full AN without pause | `A 8 none`, `link partner 8`; require mask 8 on both sides and resolved 100/full with both pause directions off and validity present. |
| `forced-10-half` | `forced-10-half`, matched 10/half forced | `F 10 half`, `link read 1`; require current 10/half with AN disabled and no qualified AN advertisement, partner or pause. |
| `forced-10-full` | `forced-10-full`, matched 10/full forced | `F 10 full`, `link read 2`; require current 10/full with AN disabled and the same withheld AN evidence. |
| `forced-100-half` | `forced-100-half`, matched 100/half forced, extension disabled | `F 100 half`, `link read 4`; require current 100/half and the same withheld AN evidence. |
| `forced-100-full` | `forced-100-full`, matched 100/full forced | `F 100 full`, `link read 8`; require current 100/full and the same withheld AN evidence. |
| `absent-peer` | `open`, confirmed open connection | `A 8 none`, `link read 8`; require down and successful management, with no resolved or qualified peer/pause data. |
| `wrong-policy` | `an-10-half`, restricted 10/half AN peer | `A 9 none`, `link read 8`; require current 10/half, policy mask 8 and `wrong_policy`. |
| `no-common` | `an-10-half`, no compatibility or fallback mode | `A 8 none`, `link read 8`; require down or pending AN and successful management. Lack of common ability comes from the declared peer restriction and observed local mask. |
| `unplug-replug` | `an-100-full`, removable copper cable | `A 8 none`, `link read 8` up, then identified `unplug`, down, `replug` and bounded fresh up observations. |
| `peer-reset` | `an-100-full`, independently controllable peer with reset hold/release | Same profile and observations as unplug/replug, with `hold-peer-reset` and `release-peer-reset` on the peer. |
| `bounded-recovery` | `an-100-full`, removable cable and peer with observable reacquisition | Same bounded unplug/replug sequence. The compiled fixture delays relink to exercise another fresh acquisition attempt. |

The common sequence is `session`, `cached transport`, `local board`,
`capabilities`, `connection identity`, `cached health`, `cached operation`,
`cached settings`, `cached link`, `application status`, `cached transport`.
The runner correlates both identity records to the expected build/board
context and checks literal accepted active-mode ID fields, copper
capabilities, initialized management, idle slot and reusable controller.
The compiled host fixture reports that application status is unsupported.
Mode/fault cases bracket live observations with health snapshots and require
successful management with unchanged failure totals. An unresolved or
physically down link cannot on its own invalidate transport health.

## Bounds, stopping and restoration

Each command is sequential and uses the host exchange bound and its CLI
budget. The campaign issues at most 64 commands and accepts at most four
typed external-action acknowledgments. Each live link wait allows at most
eight attempts and three seconds of device time from the first terminal.
A correlated `BUSY/OBSERVATION_CHANGED` with no usable sample counts as an
attempt. A down sample may still report completed AN while withholding
current resolution, partner ability and pause. Serial retries space live
polls by 250 ms, limited by the aggregate budget.
An intervention acknowledgment must echo the action and the current prompt
code before `--intervention-timeout-ms` expires (default 10000, maximum
20000). The remaining stage allowance can shorten that deadline.
Responses declare completed actions. Timeout,
decline, application denial and failed observations stay visible.

Before mutation, read `configuration desired` and `configuration observed`,
then run `configuration verify` and re-read both. Require complete, matching,
replayable VERIFIED profiles and an idle slot without debt or temporary
state. For profile cases retain target apply admission, terminal response,
verification and readback before any restore. For reset cases keep the exact
reset receipt and independently verify the unchanged entry profile.

On primary failure stop the main sequence. If a peer intervention may have
changed starting state, recover the declared entry cable/peer condition
and require fresh resolved evidence for peer recovery. A known reusable
owner can still restore and verify the saved profile if peer recovery fails.
The case retains the peer failure. If operation framing or owner continuity
is lost, retain the available evidence and stop issuing restoration commands.
An incomplete restoration stays unproved. Under the separate finite cleanup
allowance a known reusable owner may reapply the saved entry profile,
verify it and read it back. Cleanup debt requires the separately authorized
`--reinitialize` option and a current desired profile matching the saved or
selected target. The cleanup route is `connection initialize none`, followed
by explicit saved-profile apply, verify, and readback. Check the final
slot, application state and management health.
An initialization replay of changed intent alone cannot prove restoration.
The application was admitted stopped and remains in maintenance. An
application owner chooses any later start after checking current readiness.

For a repeated campaign, the fixture authorization must cover the named case,
the requested repetition count and the whole maintenance interval. One boot
and one serial attachment cover the run. Review each iteration's saved-state
readback, cleanup verdict and counter snapshots. Loss of the session after
a write can leave the PHY or peer changed. Record the unconfirmed command
and the last proved restoration step before deciding on a separately
authorized recovery. The remaining iterations stay unattempted.

On primary expiry at a clean boundary, inspect the separate restore verdict.
The saved profile or held stimulus can have a verified exit even though the
iteration stopped early. If an operator prompt stalls, its action remains
uncertain. Subsequent prompts on that same console cannot attest recovery.

To exercise these paths offline, run the two-iteration compiled example and
replay its exclusive artifact pair. The interruption tests inject loss during
setup, admitted operations, peer actions, restore/readback and publication.
The firmware fixture also services a held TDR through a stalled output queue
and stops watch/stress with the actual partial count.
[Finite soak commands and limits](hil-runner.md#finite-soak)

## Feature fixtures

Select a single diagnostic or control case using `--suite campaign --case ID`.
The executable cases below currently use the compiled `sim-mdio` fixture. The
runner requires documented rev 1, the selected BASIC/ENHANCED ID and MII/RMII
binding, an initialized owner and exclusive polling. It checks a clean slot
and matches independently observed link settings to the owned entry profile
before stimulus. The compiled fixture supplies explicit maintenance and
feature grants. A stock P4 image has no such grants for these cases, and the
serial campaign returns BLOCKED before stimulus. A board name, option or
operator response supplies provenance only.

For physical work, qualify the fixture under prompt 73. Record the schematic
revision and tested points, instrument ID/calibration/settings, load and probe
loading, selected exact-part source limits, elapsed times, raw captures and
operator actions. Synchronize each relevant capture with the operation token
or observed edge. Compare actual samples against the selected source/fixture
limits. Missing wiring or a compatible peer leaves a measurement NOT RUN.
Unavailable owner admission or unresolved source behavior leaves it BLOCKED.
The feature ledger retains source-proved absent modes, such as RGMII, separately.

The fixture profile must identify every changed owned setting and the typed
command that restores it. Capture entry, target and restored readback. Preflight
any clock, power, isolation or mux change with a qualified path that keeps
management and console available and an independently authorized fallback.
Changing the MDIO address has no automated route here. A partial write, reboot,
clock fault, unexpected fixture effect or loss of continuity stops stimulus.
Stop external traffic or stimulus at its own source. With a known usable owner,
treat the typed stop, supported replay and independent readback as separate
receipts. A failed primary remains failed after cleanup. If the path is lost,
record the unresolved effect and leave recovery to the identified fixture owner.
Keep the MAC fenced through verification and release it only on an explicit
application decision.

An unchanged link profile alone cannot prove restoration. The compiled cases
also compare fresh ordinary link state before and after stimulus. Keep the
original peer and cable in place except for an explicitly planned disturbance.

### Diagnostic cases

| Case selection | Typed stimulus and return | Physical connection, acquisition and acceptance |
| --- | --- | --- |
| `loopback-mii`, `loopback-mii-line`, `loopback-pcs-in`, `loopback-pcs-out` | `diagnostics loopback-start` with `mii 100`, `mii 100 1`, `pcs-in 10` or `pcs-out 100`. Require verified mode/speed, retained session token in `loopback-status` and released stimulus after `loopback-stop`. | Inject identified frames on the MAC-side path at the selected rate and compare sequence and payload at the return endpoint. For line transmit, also capture the loaded differential MDI trace. Verify the ordinary path after stop. |
| `loopback-digital`, `loopback-analog`, `loopback-reverse`, `loopback-external` | `diagnostics loopback-start` with `digital 100 16`, `analog 100 2`, `reverse 100 4 1` or `external 100 8`. Retain status and stop. Digital must report its HARD exit and persistent replay. | Digital uses MAC stimulus/capture. Analog uses the documented 100-ohm MDI termination and MAC return. Reverse uses a live compatible 100/full peer as stimulus and observer. External uses a compatible loop fixture and MAC capture. Preserve loss, duplication and erroneous frames. Remove external stimulus before exit and prove the original link path. See [loopback](loopback.md). |
| `bist-off`, `bist-single`, `bist-checker`, `bist-both`, `bist-digital`, `bist-analog`, `bist-external` | `diagnostics bist-start MODE COUNTER BYTES IPG OBSERVE-US SPEED PATH [FLAGS]`, `bist-read [single\|stimulus]`, `bist-status`, `bist-stop`. Retain the before/counter/after words and all validity, activity, lock, sync, history, saturation, wrap and interval flags. | Use an identified 100/full generator/checker and capture trigger with the chosen line, digital, analog or external path. Confirm disabled behavior, a correlated single packet, or independently known checker stimulus as appropriate. The simulated error injection proves the software reporting path. Physical zero-error and rate claims require a qualified window and counter semantics. See [BIST](bist.md). |
| `cable-manual`, `cable-raw` | `diagnostics cable-start 63 3` or `diagnostics cable-raw aa 1 4 1 0x0d07 0x24 0x1f7a 3`. Then use `cable-read`, `cable-status`, `cable-interpret 5000` (simulated propagation input), then `cable-stop`. Check each selected row's freshness, completion and raw validity. The raw path rejects interpretation. | Use quiet peer conditions, known good/open/short cable ends and measured lengths. Record independent propagation calibration and uncertainty, all selected raw rows and fault classification. Compare interpreted distance with independently measured length and the selected tolerance. Preserve incomplete captures and require verified HARD stop/replay. See [cable procedures](cable.md). |
| `cable-autorun` | Verify typed entry policy off and its CR1 readback, enable and verify it, observe known link loss, `diagnostics cable-autorun-read`, then restore off and re-observe the entry peer. | Use a defined cable disturbance and an independent edge/time capture. Raw stale data can prove acquisition but cannot attribute the retained words to the induced loss or supply distance. Accept event attribution only with independent provenance. |
| `compliance-tx100`, `compliance-nlp10`, `compliance-prbs10`, `compliance-pattern10` | `diagnostics compliance-start` with `tx100 mdi 0 3`, `nlp10 mdi 0 3`, `prbs10 mdix 0 3` or `pattern10 mdix 3 3`. Observe `electrical-status`, stop with `compliance-stop`, then use separately admitted `recovery reinitialize none` and readback. Replay requires `--reinitialize`. | Use the selected MDI/MDIX path, calibrated differential scope and prescribed compliance load. Record the exact pattern and rate, waveform amplitude/mask, timing and pulse observations relevant to that mode, and compare each with the selected SNLA266B and applicable electrical limits. Cover alternate supported paths and PATTERN10 codes when qualifying them. COUNT100 remains source-gated. |
| `collision-10`, `collision-100`, `calibration-recovery` | `diagnostics collision-start 10 3` or `100 3`, `electrical-status` and `collision-stop`. Calibration uses `calibration-recover 1` with a documented CDCR condition, separate HARD completion and `recovery reinitialize none`. | Wire MII TX_EN and the explicitly routed COL to a calibrated logic analyzer, select a compatible 10/100 peer and compare assert/deassert timing to the applicable source bounds. For CDCR, capture the conditional flag, HARD completion and restored configuration. Prove calibration accuracy separately with the identified resistor and instrument setup. See [electrical controls](electrical.md). |

For collision, require COL to assert within 512 bit times of TX_EN assertion
and deassert within four bit times of TX_EN deassertion. The respective bounds
are 51.2 µs and 400 ns at 10 Mbit/s, and 5.12 µs and 40 ns at 100 Mbit/s.
[SNLS647I Table 8-19](reference/source/TI-DP83826I-E-SNLS647I.pdf)

### Control cases

| Case selection | Typed target and restoration evidence | Physical connection, acquisition and acceptance |
| --- | --- | --- |
| `irq` | `irq configure`, `irq test-start`, `irq service`, `irq cached-service`, `irq test-stop` and `irq status`. Use a single destructive collector and compare raw status, masks, requested-counter validity and restored route. | Wire the selected INT/LED alternate to an analyzer with the documented pull and polarity. Establish the ENHANCED INT pin's power-down input role and external levels before changing output enable. Capture assert/deassert edges and cause, compare to the service receipt and prove stimulus clears. First-board INT remains BLOCKED. Polling evidence can be acquired in a separately eligible case. |
| `pins`, `pins-basic` | `controls pins` and `controls pins-status` validate complete typed saved and target roles and stable owned words. | Probe LED0/LED1 and only routed ENHANCED LED2/LED3 under the specified load, with prior pin consumers released. Exercise each admitted LED source, force, polarity, blink/stretch, swap and alternate role against known traffic/link clocks. Measure edges, level and timing at the pin, then verify original owner settings. Source-gated pin mux paths remain BLOCKED. |
| `interface-rmii25`, `interface-rmii50`, `interface-mii`, `repeater-leader`, `repeater-follower` | `controls interface` and `controls interface-status` validate mode, clock/mux, compatibility, elasticity/isolation and the selected per-owner repeater control. Restore each admitted owner's complete profile. | Wire the selected MII or RMII arrangement and probe actual clock frequency/direction and signal timing. For repeater, cross RX_D to TX_D and RX_DV to TX_EN with one RMII leader or two followers on one qualified common 50 MHz source. Avoid two driven clock outputs. Capture matched traffic in both directions and save each owner's independent setup and restoration receipts. |
| `power-ieee`, `power-sleep`, `power-pll`, `power-deep` | `power configure`, `power read` and `power status` distinguish requested fields from independently valid BMCR, PHYSCR and BISCR. Restore PLL before clearing IEEE down. Deep-power entry is source-gated by the unresolved exit. | Measure current, clocks and management continuity before, during and after transition. To qualify active sleep/wake, control peer presence and correlate observed sleep and wake with the traces. Prove original power policy, clocks and management return before release. See [power](power.md). |
| `eee`, `lpi` | `eee configure`, `eee read`, fresh `link read`, and independently verified entry replay. `lpi start` and `lpi stop` require separately qualified 100/full EEE and TX drain. | Use an identified EEE-capable 100/full AN partner and link analyzer. Capture advertised/local/partner ability and actual LPI entry and exit on the link; confirm traffic resumes only on owner release. The register receipt alone proves the request and observation path. See [EEE](eee.md) and [LPI](lpi.md). |
| `wol-magic`, `wol-secure`, `wol-pulse` | `wol configure`, `wol read`, `wol acknowledge` for level output, `wol status` and known typed restore. The compiled fixture injects RXFS and MISR2 history and models a pending level latch only for level mode. | Supply an identified packet source and matching/nonmatching packets with the selected CRC policy. Capture wake at the admitted status/IRQ/LED route or application wake endpoint. Measure level through W0C acknowledgment, or pulse width and polarity, and require the expected response only for admitted packets. Expect 256 ns nominal for pulse code 2. Keep live Secure-ON values outside generic artifacts. Custom wake patterns lack exact-part support. Unresolved LED routes stay source-gated. See [WoL](wake-on-lan.md). |
| `quality`, `mse`, `analog` | `controls quality` and `quality-status` verify FLD, 100M/10M and recovery settings. `controls mse` and `mse-status` preserve raw DEVAD 31 offset 0x0218 and prior link context. `controls analog` and `analog-status` verify the admitted amplitude, termination and slew fields. | Use MII for enabled FLD and calibrated 10/100 fault/pattern controls. Compare drop/recovery edges versus the injected energy/symbol/descrambler event, and retain MSE raw without deriving SNR. Probe MDI using the prescribed load and MAC I/O with calibrated bandwidth/impedance. Compare actual amplitude, termination and slew with selected limits. Verify the original settings and route. Receiver gain remains source-gated. See [quality](quality.md) and [electrical controls](electrical.md). |

Run individual submodes and physical measurements only under the fixture
authorization. The [feature coverage matrix](feature-coverage.md) links the
ledger procedures to their typed command and physical scope. Physical evidence
for these feature cases is NOT RUN.

The compiled IRQ target is `irq configure 32 2 wake intr 2` and the entry
policy is `irq configure 0 0 polarity intr 0`. Capture with `irq service 2 5 5`
while stimulus is owned. The IEEE target is `power configure down 0 0`, sleep
policy is `power configure normal 1 0` and PLL target is
`power configure down 0 1`. For PLL, separately prove IEEE down before
`power configure down 0 0`, then verify the original
`power configure normal 0 0`. The EEE target and baseline are
`eee configure on 2 0` and `eee configure off 0 0`. Complete replay and
a fresh 100/full AN and EEE observation precede any LPI start. The complete
literal baseline and target tuples for pins, interface, quality and analog,
and the synthetic WoL vectors are in the
[control case definitions](../tools/dp8382x_hil_controls.py).
The [diagnostic case definitions](../tools/dp8382x_hil_diagnostics.py) list
the full BIST mode, counter, length, IPG and dwell tuples.

AI and AE each need an identified revision-4 fixture and an independent
qualification record. Match the declared grade, mode, interface and board
permissions. Measure pin 30 clock and LED3/CRS on the selected routes,
observe IRQ and wake line levels independently, and correlate EEE/LPI with
the partner. Strap11 EMC and analog amplitude need the specified instruments
and conditions. Both grades have physical status **NOT RUN**.
[Per-feature cases and gates](feature-coverage.md#dp83826ai-and-dp83826ae)
