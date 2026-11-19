# Prompt 40: P4 board and pin source review

Reviewed 2026-11-19, read-only. The EasyEDA project at
`Projects/EasyEDA/ESP32P4X-EasyEDA-PCB` was clean on `main` at
`ee16c0ed32356cd87e8efec2d6fd5aa5988d2096`. Its latest saved native
netlist capture is
`current/2026-11-04g-i2c-finish/final/erc-and-netlist.json`.
The current allocation in `docs/ethernet-mcu.md` lines 140-154 supersedes its
older pin table at lines 273-284. The repository's
[board snapshot](../../esp32p4-board.md) agrees with that current allocation.
No EasyEDA files were changed.

| Interface | Current board evidence | Application setting |
| --- | --- | --- |
| Management | Netlist U1.81 `MDC`, U1.80 `MDIO`; U5.12 through R12 and U5.11 | MDC GPIO40, MDIO GPIO39 |
| RMII clock | X2 is the selected 25 MHz oscillator; U5.19 through R27 to `RX_CLK` at U1.86 | External 50 MHz RMII input on GPIO44; clock presence remains unmeasured |
| Transmit | U1.92 `TX_EN`, U1.82 `TX_D0`, U1.83 `TX_D1` | GPIO49/41/42 |
| Receive | U1.94 `CRS_DV`, U1.95 `RX_D0`, U1.97 `RX_D1` | GPIO51/52/53 |
| Reset | U1.98, U5.32, R21.1 and C24.2 share `RST_N`; R21.2 is on `PHY_VDDIO_3V3` | GPIO54; firmware must enforce power, clock, pulse and SMI-ready timing |

The pinned ESP-IDF v6.1 source is clean at
`fff9895c82d744c7237be8847347bdd1b07c6643`. Its
`components/esp_hal_emac/esp32p4/emac_periph.c` permits this IO_MUX set:
clock input GPIO44 (line 60), TX_EN49 (96), TXD0/1 41/42 (110/124),
RX_DV51 (142), RXD0/1 52/53 (160/178). These choices are also present in
the board project's archived `datasheets/espressif/esp-idf-v6.1-emac_periph.c`.
MDC/MDIO use separate management routing. SDK pin legality and schematic
connectivity do not establish electrical readiness or physical link operation.

**Hardware gate:** the latest netlist places only L1.2 and L3.1 on net `3V3`.
L1.1 feeds `PHY_VDDIO_3V3`, including U5.17 and X2.4, but `3V3` has no source
connection to the separate `3V` rail. The board project's
`docs/open-questions.md` item 1 explicitly calls this an unfixed PHY power
fault. The MCU I/O banks U1.21/.62/.85/.96 are now connected to `3V` in the
same current capture; older prose saying they are unwired is historical.
Do not flash or interpret link/DHCP results until final board wiring and
supplies are corrected and checked.

PHY management address 0 is an **expected configuration input**, not a
measured address or observed chip identity. The intended 100/full application
profile is likewise policy, not observed resolution. The fitted reset R21/C24
means a software 100 us assertion alone does not prove the actual low pulse.
The exact-part reset requirements include a 2 ms post-release SMI-silent
interval, a 50 ms cold-power-up interval, and stable-clock prerequisites;
elapsed checks and the final board power sequence must establish them.

Physical PHY, DHCP and local-peer evidence: **NOT RUN**. This review supplied
board and SDK source facts for the example's configuration and no firmware
image or hardware result.
