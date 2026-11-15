# Clock, interface and repeater configuration

`configureInterface` applies one complete `InterfaceSettings` profile with a
finite `IoBudget`. `interfaceSnapshot` copies retained desired settings, observed
words and configuration trust without MDIO or clock sampling. The application
serializes these calls and every controller participant honors each complete
indirect or read/modify/write transaction.

The exact DP83826I sources are SNLS647I sections 8.3.7–10 and tables 8-19, 8-28/29,
8-35, 8-41, 8-73 through 8-78 and 8-84. The [ledger](register-map.md) owns field facts;
[independent vectors](register-vectors.md) and host tests establish literal
composition. Neither related-family registers nor vendor example code extend
the accepted exact-part permissions.

## Admission and interface matrix

The profile's `interface` must match the binding's application-declared MII or
RMII wiring. It does not change that declaration or configure a MAC. UNKNOWN,
RGMII and a mismatched declaration reject before I/O. A hardware strap may load
a different register value; applying an admitted profile establishes the requested
software setting while preserving observed mode separately.

Every request requires `maintenanceAdmitted`, `macStopped`, `pinsAvailable`,
`referenceClockStable`, `managementClockAvailable` and `clockChangeApproved`.
These are explicit application attestations, not measured properties. Stop the
affected MAC and clock consumers, establish the required reference and ensure
that the MDIO controller remains usable before calling. The same admission is
required for both isolating and reconnecting an interface. No automatic source
switch, clock wait, reset-pin operation or MAC restart is hidden in the call.

| Declared interface | Reference and direction | Admitted settings |
| --- | --- | --- |
| MII, BASIC or ENHANCED | Application supplies the declared 25 MHz reference; PHY supplies the MII interface clocks | Fast RX_DV, odd-nibble policy, COL in full duplex, minimum IPG and optional isolation. RMII-only settings must retain their dormant defaults. |
| RMII leader, BASIC or ENHANCED | 25 MHz crystal or CMOS oscillator reference; PHY supplies 50 MHz to the MAC or peer | RMII revision, elasticity, CRS_DV/RX_DV selection, compatible receive/IPG settings and optional isolation. TX clock shift rejects. |
| RMII follower, BASIC or ENHANCED | Shared 50 MHz CMOS reference from the MAC or another admitted source; never a second driver on the same clock net | Leader settings plus the documented transmit-clock shift. |

`referenceClockMHz` must be 25 for MII/leader and 50 for follower. Selecting
`RmiiClock` is a register intent, not proof that the reference is present. A live
change that lacks the required stopped-consumer declarations rejects even when
the silicon field itself is programmable.

BASIC and ENHANCED describe different pin maps and hardware bootstraps.
Table 4-3's absence of a BASIC repeater bootstrap does not remove the documented
runtime RX_DV control. BASIC odd-nibble detection is disabled by default; its
detailed register field remains programmable. ENHANCED Strap 1/Strap 6 are
POR-only observations; runtime writes do not resample them. Other families and
interfaces require their later exact-map qualification.

## RMII and compatibility controls

`revision10=false` selects RMII 1.2, where CRS_DV toggles at the end of a packet
to indicate carrier deassertion. `true` selects RMII 1.0, where CRS_DV remains
asserted through the final data without that toggle. `txClockShift` is accepted
only with the follower reference.

Elasticity encoding is deliberately not ordered by buffer depth:

| `elasticityCode` | Tolerance in bits | Maximum packet bytes at the documented ±50 ppm case | At ±100 ppm |
| --- | --- | --- | --- |
| 0 | 14 | 16800 | 8400 |
| 1 | 2 | 2400 | 1200 |
| 2 | 6 | 7200 | 3600 |
| 3 | 10 | 12000 | 6000 |

`maxFrameBytes` must be positive and within the selected source-backed bound;
`clockTolerancePpm` accepts the documented 50 or 100 ppm arithmetic cases. This
buffer-capacity validation does not certify clock electrical compliance, jitter
or the board's actual frequency error. The exact electrical specifications and
identified measurements still apply. MII does not use the RMII capacity fields.

`rxDv` selects RX_DV instead of CRS_DV on the RMII output. Repeater intent
requires RX_DV and the explicit `repeaterWiringAdmitted` declaration. There is no
additional repeater-enable register or autonomous forwarding/recovery policy.

The profile also owns these independent compatibility settings:

| Setting | Meaning |
| --- | --- |
| `fastRxDv` | Assert RX_DV on the first /J/ symbol instead of waiting for /JK/; a missing following /K/ produces RX_ER. |
| `oddNibbleDisable` | True disables odd-nibble transmit-error detection. False enables the documented extra TX_CLK cycle/error behavior. |
| `collisionFullDuplex` | Enable the COL signal in full duplex as well as half duplex. Admitted only for MII with a compatible COL pin owner. |
| `shortIpg` | Select 120 ns rather than 200 ns minimum IPG. This does not set application transmit scheduling. |

CR2's separate RX_ER-during-IDLE control is preserved; it is not the minimum-IPG
setting. Other owners' robust crossover, autorun, extended duplex, power,
loopback and IRQ fields are preserved. No setting establishes protocol or
EtherCAT conformance from register readback alone.

## Clock output and pin ownership

`ClockOutput::UNOWNED` preserves the current clock/LED1 route. ENHANCED mode
additionally admits the documented IO_CFG1 selection:

- `CLKOUT25` selects CLKOUT availability. The enum follows the field's name;
  section 8.3.8 describes buffered XI output, and actual output frequency and
  waveform still depend on the reference and physical configuration.
- `LED1` selects LED1/GPIO availability after the application releases its
  clock consumer. The pin/source owner remains responsible for the LED or
  interrupt/wake mux and source; clock selection does not enable those features.

The pass verifies the owned control and the read-only clock/LED availability
status. BASIC rejects these runtime clock-output requests because the detailed
override is ENHANCED-only. `PLL` and `RECOVERED` requests remain explicitly
source-gated: selectors are named, but the detailed mux tables do not provide
an unambiguous usable activation route. No input or tri-state clock-disable
encoding is invented from prose.

The interface profile and `PinSettings::led1ClockOverride` cannot both own the
clock-release field. Explicitly release the old owner before assigning the new
one. CLKOUT selection conflicts with a retained LED1 pin, IRQ or WoL role.
An owned COL request similarly conflicts with LED2 GPIO/IRQ/WoL use. Reverse
setters honor the same ownership. Enabled EEE's clock recipe excludes explicit
clock-route ownership and isolation. Application wiring is not an exception to
these source and ownership checks.

## Isolation and power

`isolationOwned` makes `isolate` authoritative; otherwise isolation remains
outside this profile. Isolation is distinct from the power owner's software
power-down request. Its BMCR update neutralizes reset/restart commands and uses
the existing software power intent instead of echoing a hardware-OR power-down
observation into a new software request.

Isolation disconnects the MAC interface while preserving serial management
access. In RMII leader mode it also disables the 50 MHz output. Releasing isolation
can reconnect that clock and the MAC pins, so `clockChangeApproved` is required
on both paths. The caller controls when downstream consumers and MAC traffic may
resume. A successful readback proves only the requested control bit.

The finite live preflight rejects an active reset/restart, an unowned interrupt
test, an unowned power-down observation or a disabled PLL before mutation. Owned
power intent remains separately visible. Shared initialization performs its
existing power preparation and restores final power policy after interface and
dependent pin/source settings.

## Ordered application, evidence and restoration

The whole request and complete direct allowance are validated before desired
intent changes. The pass observes BMCR/PHYSCR prerequisites, prepares compatible
receive/COL/IPG controls, applies the IO route, configures RCSR and applies owned
isolation at the final dependency point. Each owned control has one atomic
read/modify/write and one masked readback. An owned clock route additionally
checks the read-only availability status.

The ordinary RMII profile takes 32 frames / 13 callbacks; a profile owning both the
clock route and isolation takes at most 39 frames / 16 callbacks. Admission computes
the exact smaller plan for the requested fields. Callbacks receive the remaining
aggregate allowance and hold at most one complete transaction. No lock spans
owner scheduling, no wait is invented and no failed phase retries.

Every RCSR read, including RMW and verification, enters the existing collector.
Its successfully consumed RC samples survive later failures. A read that may
have cleared hardware without valid data records possible history loss; it does
not become a zero-fault sample. Writes preserve reserved bit 6 and do not replay
the RC status bits. The `rmii_rc` conflict keeps raw samples available without
asserting overflow/underflow polarity or rearm semantics.

`InterfaceResult` retains primary and release status, exact attempted/completed
frames, target-effect evidence and the first mismatch. `InterfaceSnapshot`
retains desired intent separately from the RCSR-observed interface and per-word
validity. Failed requested words lose current validity; historical words/time
remain explicitly stale. A successful later word cannot erase an earlier
contradiction. Cached rendering performs no I/O or second destructive read.
LED1 pin, interrupt and wake views also require a verified owned clock route.
This applies equally to a pin-owned clock override and an interface-owned route.
An uncertain route makes those views stale even when their individual controls
were verified earlier. Reading wake detector status cannot repair clock routing.

After an accepted or possible mutation fails, OP-9 retains restoration debt.
There is no guessed rollback when a clock write makes readback unavailable.
Cancellation of a cooperative restoration freezes its own result; direct calls
have no intermediate token cancellation point. Local end cannot certify restored
clocks or stopped hardware. Separately admitted `startInitialize` restores the
same persistent profile with a new budget/result, before pin/IRQ/WoL and final
power completion. NONE, PCS, HARD, RESTART and existing power/diagnostic recovery
reuse this path with their distinct reset effects. `startVerify` uses the same
owned masks without writes. Reset never invents an unrequested interface profile.
Initialization does not publish readiness between mandatory setup and unfinished
pin/interface replay. Failed or cancelled restoration retains uninitialized state.

## Two-PHY repeater ownership and qualification

The application configures and verifies each identified PHY independently. It
first admits both board profiles and finite allowances, declares crossed
RX_D[1:0] to peer TX_D[1:0] and RX_DV to peer TX_EN, and establishes either one
leader plus one follower or two followers sharing one external 50 MHz source.
It must never connect two clock outputs. Matching traffic/speed requirements,
MAC replacement, scheduling and cable paths remain application responsibilities.

If the second PHY fails, preserve the first PHY's successful result and the
second PHY's partial evidence. Restore each owner separately as needed; success
on one side does not imply a working repeater, and software must not drain or
retry until the environment happens to work.

Physical qualification remains NOT RUN. Record exact part/revision, mode/straps,
board wiring, clock sources and consumers, firmware identity and instruments.
Measure reference/output frequency, jitter, interface edges, isolation/removal
and reconnect behavior, packet limits and repeater traffic in both topologies.
`pin_fields`/Q12 retains unresolved clock/mux permissions; `rmii_rc`/Q17 needs
controlled clock-offset/fault-injection and read-clear traces. Host tests prove
bounded composition, ownership and retained effects, not a working MAC clock,
repeater link or compliant electrical timing.

[Quality profiles](quality.md) own CR2 RX_ER-during-IDLE separately from this
profile's odd-nibble-disable bit and GENCFG IPG. Quality nonzero Fast Link Drop
requires declared and observed MII; a zero disable remains legal on RMII.
Both profiles preserve one another's masks and share restoration.
