# Open questions

These are product and bench choices for the DP83826I integration. The
[design decisions](design-decisions.md) record the accepted library behavior.
The [feature coverage](feature-coverage.md) and
[implementation progress](implementation-progress.md) carry detailed gates
and actual evidence.

## When should Ethernet recovery require a board restart?

**Example.** If the drive cable is unplugged, close the active EtherCAT
session. When the cable is back, the application can use the same installed
Ethernet driver after the required recovery and link checks. A different kind
of failure may require removing and reinstalling the driver. With the pinned
ESP-IDF, the receive task may still be using driver memory after Ethernet has
stopped. Restarting the board is the supported way to recover from that
situation.

| Option | Pros | Cons |
| --- | --- | --- |
| Keep the installed driver for ordinary link recovery. Restart the board if recovery requires reinstalling the driver. | Uses the existing recovery paths. A cable reconnect can be handled while other services keep running. | The board restart interrupts other services. The machine needs a defined restart and resume procedure. |
| Validate an ESP-IDF version that can stop, remove and reinstall the Ethernet driver safely while the board keeps running | Can restore Ethernet without interrupting other services when the installed driver must be replaced. | Needs a supported SDK method that proves the receive task has finished using the old driver, plus tests under traffic and failure conditions. The pinned SDK does not establish this. |

**Suggested starting point.** Keep the existing driver when recovery can
verify it still works. Check the link again and open a new session before
using it. Consider qualifying driver reinstallation without rebooting if
the machine needs its other services to stay up through a driver failure.
[Native lifecycle decision](design-decisions.md#6-public-lifecycle-route-for-the-pinned-sdk) ·
[EtherCAT integration](ethercat-integration.md)

## How quickly must the machine detect a failed EtherCAT exchange?

**Example.** While the master exchanges drive commands and feedback every
1 ms, someone unplugs the drive cable. The unplug has the same physical
effect whenever it happens. The repeating exchange matters because the
master knows when the next reply is due. A drive can also stop replying
while the Ethernet link still reports up.

- **EtherCAT process data.** The master exchanges commands and feedback on
  a regular schedule. The drive fields are mapped as PDOs, the same kind of
  process data as CANopen PDOs. EtherCAT carries them in Ethernet frames
  sent by the master. For a 1 ms cycle, the master sets a receive deadline
  inside each cycle and checks the working counter for each datagram.
- **PHY status polling.** The firmware reads the Ethernet chip's link
  status on a schedule. A bounded poll has a set time or amount of work
  it may spend before returning an error or giving control back. The gap
  between polls and the maximum age of a link reading are separate
  settings. The owner demo polls PHY link status every 1,000 ms. That
  setting belongs to the demo's link checks. The EtherCAT master sets its
  own cycle and reply deadline.

The DP83826I supports a hardware interrupt. In Enhanced mode its
`PWRDN/INT` pin is pin 21 and can be configured as an active-low interrupt
output. The checked ESP32P4X netlist connects it to the `ETHPDN` test point
and a pull-up, with no ESP32 GPIO connection. The interrupt can help report
a physical link event sooner. A 1 ms EtherCAT exchange and the master's
reply deadline do not require this pin.
[Board wiring](esp32p4-board.md#phy-interrupt-wiring) ·
[TI datasheet, table 5-1](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) ·
[EtherCAT process data](https://www.ethercat.org/en/technology.html) ·
[Demo polling settings](../examples/esp_idf/owner/README.md)

| Option | Pros | Cons |
| --- | --- | --- |
| Check each EtherCAT exchange and poll PHY link status for diagnostics and recovery | Fits the current board. Detects late or invalid drive traffic even when the Ethernet link stays up. | The master needs a tested deadline and a policy for invalid exchanges. A timeout alone does not identify the physical cause. |
| Use the same EtherCAT checks and add a PHY interrupt if the product needs faster physical-link reporting | Can shorten the time between a PHY link event and the firmware checking its cause. | Needs wiring and measured interrupt handling. The master still needs its reply deadline and working-counter check. |

**Suggested starting point.** Keep the present wiring. On the first
MotorControl-EC firmware, measure the 1 ms exchange under the intended
load and check both a cable pull and a drive that stops replying with the
link up. Measure the time until the master marks the exchange invalid and
the time until the machine responds as separate results. Use the actual
drive modes and PDO mapping in those tests.
[Timing evidence](esp-idf.md#timing-declarations)

For the A6-EC, the drive manual names CSV for velocity, CSP for position
and CST for torque. Confirm what `CPT` means for the intended drive before
choosing its PDO mapping.
[A6-EC user guide](https://omc-stepperonline.com/download/A6-EC_series_servo_drive_manual.pdf)

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

## Deep power-down for later library support

**Owner direction, 2026-11-22.** Keep deep power-down in the reusable
library's design and implement it when the full entry and exit behavior
can be verified. The first EtherCAT application does not need it and is
expected to keep the PHY in normal operation.

**Example.** A product powers down the PHY while idle and later requests
normal operation without rebooting. Its MAC expects a usable reference clock
again. A successful management read alone cannot establish clock or link
readiness after the transition.

| Option | Pros | Cons |
| --- | --- | --- |
| Keep the PHY in normal operation during idle | Has the fewest transitions to qualify and keeps the existing clock and link path available in idle. | Idle current may exceed the product budget. |
| Qualify the implemented active-sleep mode on the actual board | Can reduce idle power with the existing driver and retains an explicit wake check. | Wake time, management access and the PHY-supplied MAC clock need measurement under the actual peer conditions. |
| Qualify deep power-down if measured idle power still misses the requirement | Can address a tighter standby-current target if the full transition is supported. | Needs exact-part evidence for entry, exit, clock readiness and restoration before the library can enable it. |

**What remains open.** Establish the complete deep-power-down exit and
recovery sequence for this exact chip, including management access, the
PHY-supplied clock and restored configuration. Keep entry disabled until
that evidence is available. If a future product has a standby-current
requirement, compare the supported power modes against that requirement.
[Power policy and source gate](power.md)
