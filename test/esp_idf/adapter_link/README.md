# ESP32-P4 adapter link fixture

Build from the repository root with the pinned side-by-side SDK:

```bat
scripts\idf.cmd -C test\esp_idf\adapter_link -B C:\esp\build\dp8382x-adapter set-target esp32p4 build
```

The final firmware links the actual ESP32-P4 EMAC, native PHY factory, framework
installer, typed AN/forced configuration, SDK reconciliation, correctly typed
flow control, complete MMD access, cached getters and bounded start/stop. A
volatile application guard defaults to false, retaining those references while
booting only an identity/status message. This is linked-image evidence; it does
not establish MDIO, clocks, timing, link, physical RX scheduling or Q13 results.
The application refreshes its Git commit/tree identity on every build, independently
of the release version read from `library.json`. Archives report `unknown`.

No flashing or board operation is part of prompts 37-38. Before deliberately enabling
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

The SDK runs periodic link observation on the shared `esp_timer` task. The P4
fixture configuration used `CONFIG_ESP_TIMER_TASK_STACK_SIZE=3584` bytes.
For the November 18 second-round correction, P4 GCC 15.2.0 `-fstack-usage` reported 304
bytes for `getLink`, 512 for `readLinkStatus`, 416 for the core batch call
and 160 for the native MDIO backend. Those four nested frames account for
1392 bytes before entering the SDK MAC. The separate verified-settings check uses
448 bytes; it is not nested inside the MDIO chain. The timer dispatcher, MAC, event posting and error logging also use
this stack. The pinned SDK adds `TASK_EXTRA_STACK_SIZE` to the configured
size in `esp_system/include/esp_task.h`. Qualification requires high-water
measurements under the application's release build, enabled logging and
worst-case link/error traffic, with headroom for its callbacks. The fixture
has no physical stack measurement.

Explicit board pins are MDC40, MDIO39, RESET54, REF_CLK44, TX_EN49, TXD0=41,
TXD1=42, CRS_DV51, RXD0=52 and RXD1=53. These reflect the maintained board record
and admitted pinned P4 IO_MUX choices. They are not reusable component defaults.

The optional route prepares readiness before install, performs NONE initialization
and verified 100/full autonegotiation configuration, reconciles SDK AN state,
disables MAC flow control with the already-clear PHY pause advertisement, and
reads MMD 31 offset `0x0018` through one owner batch. It checks the SDK AN cache,
starts, waits 100 ms and stops. It then configures forced 10/half with manual
MDI routing, reconciles SDK state, checks cached AN is disabled, starts, waits
100 ms and retires. The flow route avoids the pinned SDK's defective shared
`bool*` / `uint32_t*` ioctl argument. Speed/duplex getter output is explicitly
the SDK publication cache, which can be default or stale, not immediate
desired-setting readback. These short
intervals do not qualify completed negotiation, partner compatibility or link.
Each operation and lifecycle attempt has an aggregate deadline and finite polling
cap. A static RX callback drops/frees every received frame. After successful
install, the driver, MAC, PHY, bus mutex and all contexts remain alive until system
restart, including initialization or start failure. Reusable destruction remains
BLOCKED by `native_rx_quiescence`; there is no uninstall path in this fixture.
