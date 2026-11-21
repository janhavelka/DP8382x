# Open questions

These are product and bench choices for the DP83826I integration. The
[design decisions](design-decisions.md) record the accepted library behavior.
The [feature coverage](feature-coverage.md) and
[implementation progress](implementation-progress.md) carry detailed gates
and actual evidence.

## Is a system restart acceptable after native Ethernet retirement?

**Example.** A machine withdraws an EtherCAT session after a port failure.
Maintenance stops the native Ethernet path. The product either stays out of
service until it can be restarted or must recreate the installed Ethernet
objects while the rest of the machine stays up.

| Option | Pros | Cons |
| --- | --- | --- |
| Accept a controlled system restart after terminal retirement | Uses the current supported route. Resources stay alive for any SDK RX work still referencing them. | Restart interrupts other services. The application must own when to restart and how to resume. |
| Require recovery without a system restart | Lets other services stay up when Ethernet is replaced. | Needs a public SDK lifecycle route that proves the old RX task has relinquished every reference. Until then reusable installed teardown stays blocked. |

**Suggested starting point.** Use restart for terminal retirement if the
product can tolerate it. Link loss by itself has a separate maintenance and
readmission route. If in-process replacement is a product requirement, put
that requirement into the consumer before selecting an SDK change. An
application callback count of zero cannot close the RX lifetime gap.
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
| Qualify an interrupt on a separate fixture or select it for a later board | Gives a hardware event path to measure if polling is insufficient. | The recorded first board has no verified PHY INT wiring. A wired fixture adds pin-mux, polarity and timing tests and still needs an application fallback. |

**Suggested starting point.** Use the existing polling path and ask the
consumer for its withdrawal budget. Measure from an independent event marker
through session withdrawal under realistic cyclic load. Revisit wired INT
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
| Leave deep power-down out of the product profile and qualify existing power options as needed | Preserves the current source gate. Active sleep, IEEE power-down or normal operation can be compared on the actual board. | May fail a product's standby-current or wake-time target. Any selected power option still needs its clock and board conditions checked. |
| Require deep power-down and obtain exact-part exit evidence | Can address a concrete power requirement if the full transition is supported. | Needs the complete entry, exit, clock-readiness and restore contract before the gated operation can be enabled. The current library refuses it. |

**Suggested starting point.** Write down the product standby-current and
wake-time goals before depending on deep power-down. Keep the existing
library investigation open for exact-part evidence regardless of the first
product's choice. [Power policy and source gate](power.md)
