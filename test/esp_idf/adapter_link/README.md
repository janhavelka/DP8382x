# ESP32-P4 adapter link fixture

Build from the repository root with the pinned side-by-side SDK:

```bat
scripts\idf.cmd -C test\esp_idf\adapter_link -B C:\esp\build\dp8382x-adapter set-target esp32p4 build
```

The final firmware links the actual ESP32-P4 EMAC, native PHY factory, framework
installer, typed initialization/configuration and bounded start/stop route. A
volatile application guard defaults to false, retaining those references while
booting only an identity/status message. This is linked-image evidence; it does
not establish MDIO, clocks, timing, link, physical RX scheduling or Q13 results.
The application refreshes its Git commit/tree identity on every build, independently
of the release version read from `library.json`. Archives report `unknown`.

No flashing or board operation is part of prompt 37. Before deliberately enabling
`runBoardFixture`, review the final board/netlist, authorize the identified fixture,
verify DP83826I enhanced mode/address 0, continuous valid clocks, reset circuitry,
and measure or replace the conditional transport bounds. The source's
`{fixedUs=2000, frameUs=1500, clockLagUs=1}` values are **unmeasured conditional
application assumptions**, as are `notificationUs=101000` and the aggregate
`callbackBudgetUs=1000000`; none is an SDK or silicon timing guarantee. The SDK nominal
1000 us MDIO polling allowance excludes scheduler delays and cannot prove them.
The 1000 us RESET_N assertion is application policy; R21/C24 requires actual
waveform qualification. The clock timestamp attests continuously valid PHY/MAC
clock conditions at the observed time; it cannot detect a missing oscillator.

Explicit board pins are MDC40, MDIO39, RESET54, REF_CLK44, TX_EN49, TXD0=41,
TXD1=42, CRS_DV51, RXD0=52 and RXD1=53. These reflect the maintained board record
and admitted pinned P4 IO_MUX choices. They are not reusable component defaults.

The optional route prepares readiness before install, performs NONE initialization
and verified 100/full autonegotiation configuration, starts, waits 100 ms, then
retires. That short interval does not qualify completed negotiation or link.
Each operation and lifecycle attempt has an aggregate deadline and finite polling
cap. A static RX callback drops/frees every received frame. After successful
install, the driver, MAC, PHY, bus mutex and all contexts remain alive until system
restart, including initialization or start failure. Reusable destruction remains
BLOCKED by `native_rx_quiescence`; there is no uninstall path in this fixture.
