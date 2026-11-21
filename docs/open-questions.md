# Open questions

These are product and bench choices for the DP83826I integration. The
[design decisions](design-decisions.md) record the accepted library behavior.
The [feature coverage](feature-coverage.md) and
[implementation progress](implementation-progress.md) carry detailed gates
and actual evidence.

## Is a system restart acceptable after native Ethernet retirement?

**Example.** A machine withdraws an EtherCAT session after the cable is
unplugged. Once the cable is back, verified maintenance and a new session
can recover using the same installed objects. A failure that instead
requires retiring those objects raises a separate question about product
downtime.

| Option | Pros | Cons |
| --- | --- | --- |
| Recover in place when maintenance can verify the existing installation. Restart after terminal retirement. | Uses the existing routes. Ordinary link recovery keeps other services up, and a terminal failure retains resources until restart. | A terminal failure interrupts other services during restart. The product needs a defined restart and resume procedure. |
| Qualify stop, teardown and reinstallation on an SDK baseline with a proven public RX shutdown boundary | Can restore Ethernet after terminal retirement while other services stay up. | The pinned SDK has an unresolved RX lifetime gap. This route needs source review and start/stop/failure tests on a suitable baseline before it can be selected. |

**Suggested starting point.** Use verified in-place recovery whenever it
applies and decide how much terminal-outage downtime the product can accept.
Recovery must establish a fresh resolved link and admit a new session before
using it. Pursue the SDK route only when restarting would violate a real
availability requirement. A callback count of zero alone leaves the SDK RX
lifetime unresolved.
[Native lifecycle decision](design-decisions.md#6-public-lifecycle-route-for-the-pinned-sdk) ·
[EtherCAT integration](ethercat-integration.md)

## What link-loss response does the first application require?

**Example.** The drive-port cable is unplugged during cyclic traffic. The
application needs to decide when to stop admitting frames and invalidate the
old session. Specify a maximum acceptable interval from the observed cable
event or loss of usable traffic to session withdrawal. Define how the machine
handles that interval independently of the PHY.

| Option | Pros | Cons |
| --- | --- | --- |
| Use bounded PHY polling with an application observation-age limit | Fits the recorded first-board wiring and can be measured on the intended RMII path. | Poll interval, scheduler load and PHY transition time limit detection. Shorter polls consume more owner time. |
| Combine PHY polling with the application's cyclic traffic timeout | The master can reject a stale session after missing cyclic traffic even when link status has yet to change. | Requires a tested application timeout policy. Missing traffic can have several causes and does not identify a physical cable event. |
| Use a wired PHY interrupt as a service hint, then confirm the link through the owner | Can reduce the wait before checking a physical link event. | The recorded first board has no verified PHY INT wiring. A wired fixture adds pin-mux, polarity and timing tests and still needs fallback checks. |

**Suggested starting point.** Use the existing polling path and ask the
consumer for its withdrawal budget. Measure from an independent event marker
through session withdrawal under realistic cyclic load. Also check a peer
that stops exchanging usable traffic while the electrical link stays up.
Use the existing cyclic timeout if PHY polling cannot cover that case.
Revisit wired INT
only if the application requires it or the measured polling path misses the
budget. Board readiness, a resolved 100/full link and machine-level response
remain separate checks. [First-board wiring and link policy](esp32p4-board.md) ·
[Timing evidence](esp-idf.md#timing-declarations)

## Which fixture leads physical qualification?

**Example.** A known-good desktop peer establishes 100/full on the product
board. That can exercise board power-up and link recovery. It cannot prove
10 Mbit/s behavior, interrupt edges or a drive's cyclic response without the
applicable peer, wiring or observer.

| Option | Pros | Cons |
| --- | --- | --- |
| Start with the intended P4 board and a known 100/full peer once the board is ready | Collects evidence for the actual RMII clock, reset, straps, MDIO and application port first. | Needs the final board/netlist and independent power, clock and reset evidence. Cannot cover features absent from that board. |
| Use a verified exact-part development or evaluation fixture for PHY-only cases | Can exercise additional peers, PHY modes or wired INT without modifying the product board. | Its wiring or host may differ. Results apply to that fixture and do not qualify the product board. |

**Suggested starting point.** Record the first available exact board/PHY
combination, peer, cable, instruments and approved recovery route. Prioritize
the intended 100/full product path when its board is ready. Use a second
fixture for additional supported features where needed.
[Fixture and case requirements](hardware-validation.md#common-fixture-and-owner-admission)

## Does a consumer need deep power-down?

**Example.** A product powers down the PHY while idle and later requests
normal operation without rebooting. Its MAC expects a usable reference clock
again. A successful management read alone cannot establish clock or link
readiness after the transition.

| Option | Pros | Cons |
| --- | --- | --- |
| Keep the PHY in normal operation during idle | Has the fewest transitions to qualify and keeps the existing clock and link path available in idle. | Idle current may exceed the product budget. |
| Qualify the implemented active-sleep mode on the actual board | Can reduce idle power with the existing driver and retains an explicit wake check. | Wake time, management access and the PHY-supplied MAC clock need measurement under the actual peer conditions. |
| Qualify deep power-down if measured idle power still misses the requirement | Can address a tighter standby-current target if the full transition is supported. | Needs exact-part evidence for entry, exit, clock readiness and restoration before the library can enable it. |

**Suggested starting point.** Measure normal operation against the product's
standby-current target. If it misses, compare active sleep using the same
fixture and check elapsed time to a usable fresh link. Treat deep power-down
as a candidate only if a simpler mode misses the target and the required
exit evidence is available. [Power policy and source gate](power.md)
