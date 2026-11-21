# Open questions

These questions cover how to use the DP83826I Ethernet chip in a product
and what to check on the bench. The [design decisions](design-decisions.md)
record the accepted library behavior. The [feature coverage](feature-coverage.md)
and [implementation progress](implementation-progress.md) show which checks
have been run and which still need evidence.

## What happens when the EtherCAT cable is unplugged and plugged back in?

**A cable unplug does not require an MCU reboot.** If a drive cable is
unplugged, the master marks the exchange invalid and closes the old
session. When the cable is plugged back in, the board can use the same
Ethernet driver. The application checks that the link is back at the
expected speed and duplex, completes its recovery checks, and opens a new
EtherCAT session. If the Ethernet controller needs to be stopped and
started during these checks, it can reuse the same driver. The machine
decides when motion is allowed to resume.
[EtherCAT integration](ethercat-integration.md)

### Separate SDK question: replacing the Ethernet driver's running state

The Ethernet driver is code already in the firmware. When the MCU starts
Ethernet, that code sets up memory and callbacks for the Ethernet controller.
*Reinstalling the driver* means freeing and rebuilding that running state
without rebooting the MCU. A cable unplug leaves the running driver available
for reuse. This SDK question only matters if the system needs to discard
the driver itself and build it again.

With the current ESP-IDF, its Ethernet receive task may still use the old
driver memory after the Ethernet controller has stopped. Until the SDK can
prove it has finished using that memory, keep it intact. A board restart
rebuilds the driver from scratch if that is required.

| Option | Pros | Cons |
| --- | --- | --- |
| Keep the driver in memory for link recovery. Restart the board only if its Ethernet driver state must be rebuilt. | A cable reconnect can be handled while other services keep running. Uses the current SDK safely. | A board restart briefly interrupts the other services if the driver state has to be rebuilt. |
| Qualify an ESP-IDF version that can safely rebuild the driver state while the MCU keeps running | The rest of the board can keep running even when the Ethernet driver has to be rebuilt. | Requires an SDK method that proves the receive task has finished using the old memory, plus tests under traffic and failure conditions. |

The current cable-recovery work uses the first route. Revisit rebuilding
the driver while the board runs only if the product needs that capability.
[Native lifecycle decision](design-decisions.md#6-public-lifecycle-route-for-the-pinned-sdk) ·
[EtherCAT recovery example](../examples/esp_idf/owner/README.md)

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

Pin 21 on the DP83826I is called `PWRDN/INT` in Enhanced mode. It has
two possible roles:

- **Power-down input at startup.** Pulling the pin low requests IEEE
  power-down. The chip's automatic Active Sleep and Deep Power-Down modes
  have separate settings.
- **Interrupt output after firmware configures it.** The PHY can pull
  the pin low to tell the processor an enabled event happened, such as
  link going up or down. The processor then reads the PHY's status to
  find out what happened.

In the saved ESP32P4X schematic, the pin reaches the `ETHPDN` test point
and a pull-up resistor that holds it high at idle. It has no connection
to an ESP32 GPIO. A wired interrupt could report a physical link change
sooner. The master's 1 ms EtherCAT exchange and reply deadline work
independently of the pin.
[Board wiring](esp32p4-board.md#phy-interrupt-wiring) ·
[TI datasheet, table 5-1 and section 8.3.6](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) ·
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
