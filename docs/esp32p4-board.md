# ESP32-P4 starting board

This is a firmware context snapshot, not a hardware re-audit or a library default.
The primary fitted PHY is **DP83826IRHBR**, paired with ESP32-P4NRW32X.

The local source is `Projects/EasyEDA/ESP32P4X-EasyEDA-PCB/docs/ethernet-mcu.md`,
read on 2026-10-24. Its 2026-10-16 Ethernet control assignments and 2026-10-20
interface table supersede older pin assignments in historical audit files.
Recheck the final schematic/netlist before writing or flashing board firmware.

| Setting | Starting board context |
| --- | --- |
| Interface | RMII, DP83826 enhanced mode |
| Reference clock | 25 MHz oscillator into PHY; PHY supplies 50 MHz to P4 GPIO44 |
| MDC / MDIO / PHY reset | GPIO49 / GPIO43 / GPIO54 |
| RMII transmit signals | GPIO40/41/42; use the final netlist for signal-to-pin mapping |
| RMII receive signals | GPIO51/52/53; use the final netlist for signal-to-pin mapping |
| PHY interrupt | Not connected; plan link polling |
| PHY management address | Expected address 0 from existing board audit; verify straps and MDIO response during bring-up |

Keep strap-related MCU pins in the required state during PHY reset. GPIO and
clock setup, reset timing and post-reset register initialization must follow the
actual chip datasheet and final board wiring.

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
