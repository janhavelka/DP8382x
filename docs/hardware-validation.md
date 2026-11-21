# DP83826I observation, configuration and link validation

The campaign runner implements the cases below through the shared CLI.
Prompt 50 exercises them using the compiled host fixture, offline replay and
an injected serial backend. Use an identified, approved fixture for physical
execution in prompts 72 and 73. A transcript proves the assertions recorded
for that run. The board, wiring, power sequence, cable and peer identifiers
are supplied declarations. The compiled fixture's simulated link and reset
registers are offline execution evidence.

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
inapplicable combinations such as DP83826I RGMII remain outside the case
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
