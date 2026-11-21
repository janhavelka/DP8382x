# Open questions

These questions cover how to use the DP83826I Ethernet chip in a product
and what to check on the bench. The [design decisions](design-decisions.md)
record the accepted library behavior. The [feature coverage](feature-coverage.md)
and [implementation progress](implementation-progress.md) show which checks
have been run and which still need evidence.

## When should Ethernet recovery require a board restart?

**Example.** If the drive cable is unplugged, close the active EtherCAT
session. When the cable is back, the application can use the same installed
Ethernet driver after the required recovery and link checks. A different kind
of failure may require removing and reinstalling the driver. With the selected
version of ESP-IDF, a background task that receives Ethernet data may still
be using the old driver after Ethernet has stopped. Restarting the board is
the supported way to recover from that situation.

| Option | Pros | Cons |
| --- | --- | --- |
| Keep the installed driver for ordinary link recovery. Restart the board if recovery requires reinstalling the driver. | Uses the existing recovery paths. A cable reconnect can be handled while other services keep running. | The board restart interrupts other services. The machine needs a defined restart and resume procedure. |
| Validate an ESP-IDF version that can stop, remove and reinstall the Ethernet driver safely while the board keeps running | Can restore Ethernet without interrupting other services when the installed driver must be replaced. | Needs a supported SDK method that proves the receive task has finished using the old driver, plus tests under traffic and failure conditions. The pinned SDK does not establish this. |

**Suggested starting point.** Keep the existing driver when recovery can
verify it still works. Check that the link came back at the expected speed
in full duplex and that the Ethernet controller is running before opening a
new session.
Consider testing driver reinstallation without rebooting if the machine's
other services need to keep running through a driver failure.
[Native lifecycle decision](design-decisions.md#6-public-lifecycle-route-for-the-pinned-sdk) ·
[EtherCAT integration](ethercat-integration.md)

## How quickly must the machine detect a failed EtherCAT exchange?

**Example.** While the master exchanges drive commands and feedback every
1 ms, someone unplugs the drive cable. The unplug has the same physical
effect whenever it happens. The repeating exchange matters because the
master knows when the next reply is due. A drive can also stop replying
while the Ethernet link still reports up.

- **EtherCAT process data.** The master exchanges commands and feedback on
  a regular schedule. A process data object (PDO) groups the fields sent
  in one direction, such as target speed sent to a drive or actual speed
  sent back. The idea is the same as a CANopen PDO, and EtherCAT carries
  the fields in Ethernet frames sent by the master. For a 1 ms cycle, the
  master sets a reply deadline inside that cycle and checks the working
  counter in each datagram, the part of the frame that requests a read,
  a write, or both. The devices update the counter when they carry out the
  operation. A frame can return on time with a counter that differs from
  the expected value, so a reply alone does not establish a valid exchange.
- **PHY status polling.** The firmware reads the Ethernet chip's link
  status on a schedule. A bounded poll has a set time or amount of work
  it may spend before returning an error or giving control back. The gap
  between polls and the maximum age of a link reading are separate
  settings. The owner demo polls PHY link status every 1,000 ms. That
  setting belongs to the demo's link checks. The EtherCAT master sets its
  own cycle and reply deadline.

The DP83826I can signal an event to the processor through a hardware
interrupt. In Enhanced mode it can be configured to pull pin 21
(`PWRDN/INT`) low when an interrupt occurs. In the saved ESP32P4X schematic,
the pin reaches the `ETHPDN` test point and a pull-up resistor that holds it
high at idle. It has no connection to an ESP32 input (GPIO). The interrupt
can help report a physical link event sooner. A 1 ms EtherCAT exchange and
the master's reply deadline do not require this pin.
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

## Which test setup should we use first?

**Example.** A known-good desktop Ethernet port connects to the product
board at 100 Mbit/s full duplex, which means both ends can send and receive
at the same time. This can test power-up and link recovery. Separate tests
need a 10 Mbit/s partner for that speed, a wired interrupt for measuring
interrupt timing, or a drive for testing its replies to repeating commands.

| Option | Pros | Cons |
| --- | --- | --- |
| Start with the intended P4 board and a known 100 Mbit/s full-duplex Ethernet partner once the board is ready | Tests the actual product port, including the clock for the chip-to-processor interface (RMII), reset, hardware startup settings (straps) and register access (MDIO). | Needs the final board wiring record and independent measurements of power, clock and reset. This setup covers the features the board provides. |
| Use an evaluation board with the same DP83826I to test the chip | Can test other Ethernet partners, chip modes or a wired interrupt without modifying the product board. | The evaluation board can have different wiring or a different processor. Its results apply to that setup. |

**Suggested starting point.** Record the exact board and chip, device at
the other end of the cable, instruments and approved steps to recover the
setup after a fault. Test the intended 100 Mbit/s product link first when
the board is ready. Use a second setup for additional features if needed.
[Fixture and case requirements](hardware-validation.md#common-fixture-and-owner-admission)

## Deep power-down for later library support

**Owner direction, 2026-11-22.** Keep deep power-down in the reusable
library's design and implement it when the full entry and exit behavior
can be verified. The first EtherCAT application does not need it and is
expected to keep the PHY in normal operation.

**Example.** A product puts the Ethernet chip into a low-power mode while
idle and later asks it to wake without rebooting. The Ethernet controller
(MAC) needs the chip's reference clock again. The processor may be able to
read a chip register before the clock or Ethernet link is ready, so test
those separately.

| Option | Pros | Cons |
| --- | --- | --- |
| Keep the PHY in normal operation during idle | Has the fewest power changes to test and keeps the existing clock and link path available in idle. | Idle current may exceed the product budget. |
| Test the implemented active-sleep mode on the actual board | Can reduce idle power using the existing driver, with a separate check that wake-up worked. | Measure wake time, register access and the clock supplied to the Ethernet controller under the expected conditions at the other end of the cable. |
| Test deep power-down if measured idle power still misses the requirement | Can address a tighter standby-current target if wake-up can be verified. | Before enabling the mode, verify how this exact chip enters and leaves it, when its clock is usable, and what configuration has to be restored. |

**What remains open.** Establish the complete deep-power-down exit and
recovery sequence for this exact chip, including register access, the
clock supplied to the processor and restored settings. Keep entry disabled
until that evidence is available. If a future product has a standby-current
requirement, compare the supported power modes against that requirement.
[Power policy and source gate](power.md)
