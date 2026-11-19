# ESP32-P4 ordinary Ethernet example

This is a native ESP-IDF v6.1 application using the real ESP32-P4 MAC, the
DP8382x native PHY owner and `esp_netif` DHCP. It advertises only 100/full,
reports the resolved link separately from DHCP, and makes one explicit
stop/start cycle. It runs for about 30 seconds, then stops and retains installed
resources until restart. The shared diagnostic CLI attaches in prompt 47;
this example needs no console input, FieldCore service or Internet connection.

## Board gate and wiring

The saved `ESP32P4X-EasyEDA-PCB` netlist at revision
`ee16c0ed32356cd87e8efec2d6fd5aa5988d2096`,
`current/2026-11-04g-i2c-finish/final/erc-and-netlist.json`, agrees with the
2026-10-27 allocation table: MDC 40, MDIO 39, 50 MHz RMII REF_CLK input 44,
TX_EN 49, TXD0 41, TXD1 42, CRS_DV 51, RXD0 52, RXD1 53 and RESET_N 54.
P4 IO_MUX restricts these data/clock pads. The PHY receives a 25 MHz oscillator;
the selected PHY address **0 is an expected strap value**, not a readback.
`BoardConfig.h` sets every relevant SDK field and validates them at startup.
The `1000 ms` link poll is application policy, not a PHY timing guarantee.

**Do not flash or enable the board gate yet.** The latest saved netlist's
`3V3` net shows only L1.2 and L3.1, with no demonstrated supply source for the
PHY input. Resolve and measure that rail, then recheck the final board netlist,
address straps, 25/50 MHz clocks, R21/C24 reset pulse, all power rails and the
PHY's electrical limits. The 1 ms requested reset assertion does not prove the
actual RC waveform. `Phy::prepareBoard` enforces the 2 ms post-release and
50 ms power-on SMI silence after application-attested stable clock; the SDK's
100 us default assertion alone does not. These are source-backed software
conditions; real timing is unmeasured.

The committed `sdkconfig.defaults` leaves `DP8382X_BOARD_READY` off. With the
gate off, the image reports build/board identity and returns before touching
GPIO, MAC, MDIO or the network. After the physical checks above, an authorized
board owner may use `menuconfig` to enable the gate in a local ignored
`sdkconfig`, rebuild and inspect the exact configuration before flashing.

## Build and gated physical use

From the repository root on Windows:

```bat
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 set-target esp32p4 build
```

Only after resolving the board gate and identifying an authorized fixture/port:

```bat
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 menuconfig
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 build
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 -p <PORT> flash monitor
```

The final command is an instruction for a future qualified fixture, not a run
performed by this stage. Confirm the supply, reset, clocks and netlist before
enabling the gate. Connect a local DHCP-capable peer/switch and record PHY
identity, negotiated profile, assigned IP and a separately requested local-peer
probe. This application does not probe a peer on its own; `reachability` prints
`not-requested`. DHCP failure is IP evidence even if MDIO and link are healthy.
A valid 10/half link does not satisfy this application's 100/full profile.

## Ownership and limits

`app_main` owns the static callback context, netif, shared MDIO mutex, actual
MAC, native PHY and Ethernet driver. It initializes the shared lwIP facility,
creates or borrows the default event loop, creates a netif, MAC and PHY, prepares
board reset/clock readiness, installs the driver, creates glue, attaches netif,
registers application handlers, initializes/configures/reconciles the PHY,
then starts the framework. The SDK's own periodic timer is the only PHY link
poller. DHCP belongs to `esp_netif`; no static address or remote probe is
implicitly configured.

Callbacks only copy bounded link/IP facts or request shutdown; they perform no
console output, blocking cleanup or PHY I/O. The owner task prints cached
management health, configuration trust, resolved link/profile and IP state.
Its finite service loop does not wait for console input and stops/restarts once.
An unexpected STOP callback requests owner shutdown. Task 47 can route shared
CLI requests through this same owner loop without adding a second poller.

Before successful driver installation, cleanup releases created MAC/PHY/mutex,
netif and the owned default loop in dependency order. `esp_netif_init` is
process-wide and IDF v6.1 cannot deinitialize its lwIP thread; the example does
not claim to own or delete it. After installation, `native_rx_quiescence` keeps
driver, glue, netif, MAC, PHY, handlers and static context alive until system
restart, including failures after attachment. `esp_netif_attach` can publish
the netif RX pointer before reporting failure. `Phy::retire` stops/fences and
closes admission; a failure keeps everything alive for inspection. The first
primary result, first cleanup error and native lifecycle journal remain
separate. No event handler waits for its own unregister, and no installed glue
is deleted. Glue deletion has no independent deadline: unregister can wait on
the default-loop mutex for the longest handler. Handlers here only set atomics.

The example's transport and callback bounds are **conditional admission
values**, inherited from the linked fixture, not measured controller or RTOS
wall-time guarantees. Arbitrary task starvation cannot be bounded by them.
The source Git commit and clean/dirty state refresh into a build-directory
header on each build; archives report `unknown`. Semantic library version is
separate. Physical link, DHCP, peer reachability, waveform and usable board
power remain **NOT RUN**.
