# ESP32-P4 starting board

This is a firmware context snapshot, not a hardware re-audit or a library default.
The primary fitted PHY is **DP83826IRHBR**, paired with ESP32-P4NRW32X.

The GPIO assignments in the example came from
`Projects/EasyEDA/ESP32P4X-EasyEDA-PCB/docs/ethernet-mcu.md`, read on
2026-11-07 at revision
`ee16c0ed32356cd87e8efec2d6fd5aa5988d2096`. Its adopted allocation table
(lines 140–154) supersedes the older detailed pin table still present lower in
that document. The source links its 2026-10-27 current-board checkpoints.
The newer board design has changed some of those GPIOs. Recheck the final
schematic and the firmware configuration before flashing.

| Setting | Older wiring snapshot used by the example |
| --- | --- |
| Interface | RMII, DP83826 enhanced mode |
| Reference clock | 25 MHz oscillator into PHY; PHY supplies 50 MHz to P4 GPIO44 |
| MDC / MDIO / PHY reset | GPIO40 / GPIO39 / GPIO54 |
| RMII transmit signals | TX_EN GPIO49, TXD0 GPIO41, TXD1 GPIO42; recheck final netlist |
| RMII receive signals | CRS_DV GPIO51, RXD0 GPIO52, RXD1 GPIO53; recheck final netlist |
| PHY interrupt | No ESP32 GPIO connection. See the saved netlist check below. |
| PHY management address | Expected address 0 from existing board audit; verify straps and MDIO response during bring-up |

ESP32-P4 EMAC IO_MUX restricts the pads available for each RMII data/clock
signal; they are not arbitrary GPIO-matrix choices. The selected mapping is
listed in the pinned v6.1
[emac_periph.c](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_hal_emac/esp32p4/emac_periph.c).
MDC and MDIO use the separate management-signal routing. A legal SDK mapping
still needs confirmation against the final board netlist.

Keep strap-related MCU pins in the required state during PHY reset. GPIO and
clock setup, reset timing and post-reset register initialization must follow the
actual chip datasheet and final board wiring.

The source retains R21/C24 on RESET_N. The datasheet's 25 us reset pulse minimum
is explicitly without debounce capacitors; qualify the actual board pulse and
release timestamp. Enforce the 2 ms reset-release and 50 ms power-up SMI-silent
windows with elapsed-time checks and the documented clock prerequisites.
ESP-IDF GPIO5, address autodetection, no post-reset delay and 2000 ms link polling
are generic defaults, not the selected wiring or EtherCAT timing guarantees.
The source also retains unfinished supply/net qualifications, so this snapshot
does not authorize an assumed ready-to-flash board.

## PHY interrupt wiring

Texas Instruments names pin 21 `PWRDN/INT` in Enhanced mode. It starts as
a power-down input. Firmware can configure it as an active-low open-drain
interrupt output. [TI datasheet, table 5-1](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)

In the saved ESP32P4X schematic export
`current/2026-11-16f-mcu-gnd-dsi-usb/final/erc-and-netlist.json` in
`ESP32P4X-EasyEDA-PCB` at `1084131`, U5 pin 21 connects to the `ETHPDN`
test point and R19, a 10 kΩ pull-up to `PHY_VDDIO_3V3`. That net has no
ESP32 GPIO connection. This is a check of the saved schematic wiring.

The later Ethernet pin note in that repository also lists REF_CLK on
GPIO50, MDC on GPIO54, MDIO on GPIO0 and PHY reset on GPIO39. The table
above describes the older example pinout. Use the final design record
for the board being brought up.

## Link policy

The user intends this board to run at **100 Mbit/s**, with the planned EtherCAT
drive port using **full duplex**. Preserve autonegotiation, advertise 100/full,
and check that the resolved link meets the application requirement.

The reusable library should still expose 10 Mbit/s functionality for general
use. A board-specific restriction must not remove that capability from the
general interface. Hardware tests of that capability can use an appropriate
separate configuration; it is not a fallback policy for the drive port.

EtherCAT master software and its timing qualification belong to the final
application. Ordinary Ethernet networking is a separate initial test use of
the same MAC/PHY hardware.
