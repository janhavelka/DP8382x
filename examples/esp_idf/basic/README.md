# ESP32-P4 ordinary Ethernet example

This is a native ESP-IDF v6.1 application using the real ESP32-P4 MAC, the
DP8382x native PHY owner and `esp_netif` DHCP. It starts with a 100/full-only
advertisement, reports the resolved link separately from DHCP, and makes one
explicit stop/start cycle. The default cycle runs for 10 seconds, stops for
1 second, runs for 20 seconds, then stops in maintenance. The shared CLI
continues to run and the installed resources remain available. An explicit
`application stop` supersedes the automatic cycle. The cycle needs no console
input, FieldCore service or Internet connection.

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
actual RC waveform. The native owner budgets 1 ms for each GPIO callback
by default, separately from the required low pulse. Qualify both on the board.
`Phy::prepareBoard` enforces the 2 ms post-release and
50 ms power-on SMI silence after application-attested stable clock; the SDK's
100 us default assertion alone does not. These are source-backed software
conditions; real timing is unmeasured.

The committed `sdkconfig.defaults` leaves `DP8382X_BOARD_READY` off. With the
gate off, the UART console provides build and board identity, help and local
status. The application leaves the PHY GPIO, MAC, MDIO and network inactive.
The gate is read through a `volatile` value,
so the default image still links the complete MAC, PHY, glue and service path;
a gate-closed build therefore proves that path links, not only the banner.
After the physical checks above, an authorized
board owner may use `menuconfig` to enable the gate in a local ignored
`sdkconfig`, rebuild and inspect the exact configuration before flashing.

## Build and gated physical use

From the repository root on Windows:

```bat
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p47-basic set-target esp32p4 build
```

Only after resolving the board gate and identifying an authorized fixture/port:

```bat
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p47-basic menuconfig
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p47-basic build
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p47-basic -p <PORT> flash monitor
```

The final command is an instruction for a future qualified fixture, not a run
performed by this stage. Confirm the supply, reset, clocks and netlist before
enabling the gate. Connect a local DHCP-capable peer/switch and record PHY
identity, negotiated profile, assigned IP and a separately requested local-peer
probe. This application does not probe a peer on its own; `reachability` prints
`not-requested`. For an explicitly requested fixture check, a peer on the same
LAN can run `ping -n 3 <assigned-ip>` on Windows during a running interval.
Record its target, replies and timeouts separately from the assigned address.
DHCP failure is IP evidence even if MDIO and link are healthy.
A valid 10/half link does not satisfy this application's 100/full policy.

## Shared CLI

Open the board's UART0 serial bridge at 115200 baud, 8 data bits, no parity
and one stop bit. The example selects ESP-IDF log V1 and disables the secondary
USB console. If reusing a local `sdkconfig`, check these settings before building.
Both native examples use the same `ConsoleSources.cmake` to compile
`Dp8382xCli.cpp` and `Dp8382xUartConsole.cpp`. The host fixture compiles the
shared processor with its own byte transport. [CLI guide](../../../docs/cli.md)

Use `session <nonce>` for firmware identity and synchronization, and
`application status` for the cached network state. `application stop` stops and
fences the framework before admitting PHY maintenance. Use
`configuration apply` for a complete AN or forced profile and `registers bmcr`
for guarded raw maintenance. If a command leaves cleanup owed, wait for its
terminal record and use `recovery reinitialize` under a new token. With
verified settings, `application start` reconciles the stopped SDK state and
starts the framework. `cached link`, `health` and `cfg` copy retained evidence.
Peer reachability remains a separately requested fixture measurement.

## Ownership and limits

`app_main` owns the static callback context, netif, shared MDIO mutex, actual
MAC, native PHY and Ethernet driver. It initializes the shared lwIP facility,
creates or borrows the default event loop, creates a netif, MAC and PHY, prepares
board reset/clock readiness, installs the driver, creates glue, attaches netif,
checks the attached netif MAC against the EMAC, registers application handlers,
initializes/configures/reconciles the PHY,
then starts the framework. The SDK's own periodic timer is the only PHY link
poller. DHCP belongs to `esp_netif`; no static address or remote probe is
implicitly configured.

A healthy link still negotiating at startup remains under framework polling
until a resolved mode is available. The main task defaults to a 16 KiB stack.
The owner keeps the last delivered operation result in static application
storage and reports its token, possible writes and cleanup obligation on exit.
When reusing an older local `sdkconfig`, check `CONFIG_ESP_MAIN_TASK_STACK_SIZE`
before building. Actual task stack high-water marks need a running fixture.

Callbacks only copy bounded link/IP facts or request shutdown. The owner task
queues periodic cached management health, configuration trust, resolved link
and IP lines. A separate UART task drains bounded output. A withheld or slow
sink can drop ordinary status lines while the owner continues lifecycle service.
`local console` reports the output queues, backpressure and drops. An unexpected
STOP callback requests shutdown. The CLI runs on the same owner task and the
framework timer remains the link poller. Start and stop requests require a
live deadline before changing the application mode. The UART task also latches
receive errors that the driver's event queue can drop.

Every 100 ms service step also copies the adapter's lifecycle journal (no PHY
I/O) and acts on its obligations:

- `stopRequired` (observation or publication trust lost): the owner stops at
  its next service step, records the current observation cause and withholds
  restart. A busy snapshot or delayed owner task can delay that step. The
  100 ms is a requested interval during the demo.
- `eventLost` (the P4 MAC accepted a link change but its event was not
  delivered): the owner calls the event-only `retryLinkEvent`, which never
  replays MAC UP. The application retains the latest direct retry return
  and prints it at shutdown. After the tenth retry it checks the journal
  again. An accepted late post returns `ESP_ERR_TIMEOUT` and clears
  `eventLost`; a still-pending event ends the session.

The final output reports the primary and cleanup causes, why the session
ended and whether a core result is still owed. Historical lifecycle errors
can first be observed at a later successful call. Before start or restart,
the owner attempts up to three cached journal reads with up to two one-tick
yields. It withholds the transition if it cannot confirm the previous journal.
It checks for an unexpected STOP only after that confirmation, so a STOP
delivered during a yield still withholds the start.

Startup operations (initialize, configure) are polled to completion on the owner
task. On `BUSY`, the owner checks for a terminal result pending on that
token. The polling allowance is 6 s or 10,000 attempts. Cancellation and
result delivery then each allow up to 100 attempts with one-tick yields.
Only successful delivery releases CONTROL. An undelivered token remains
owed and preserves the frozen operation status and detail, including when
a transient gate BUSY led the owner to discover it through the snapshot.
Later shared CLI commands
use the separate budgets and terminal record rules in the CLI guide.

Link evidence prints `CURRENT` only when the cached sample is valid, not stale
and at most three poll periods old (`linkAgeMs`); otherwise `AGED`, `STALE`,
`UNKNOWN` or `BUSY`. The 100/full verdict prints `yes` or `no` for a
current resolved mode and `unknown` for unavailable evidence. Copying a
snapshot never refreshes it, and contended polls can leave an old sample.

Before successful driver installation, cleanup releases created PHY, MAC, mutex,
netif and the owned default loop in dependency order: the PHY borrows the MAC
and bus, so it goes first, and a PHY that cannot be destroyed keeps both alive.
Destruction deletes the adapter journal, so a PHY whose final journal copy
cannot be confirmed is also kept, with its dependencies, for an explicit retry.
A quarantined bus may still hold its mutex, which is then retained. A null SDK
factory result reports no cause, so it is recorded by stage, not as an
invented SDK error code. The failed-start cleanup allowance is set to 500 ms,
above the 101 ms notification allowance needed to withdraw an accepted UP. `esp_netif_init` is
process-wide and IDF v6.1 cannot deinitialize its lwIP thread; the example does
not claim to own or delete it. After installation, `native_rx_quiescence` keeps
driver, glue, netif, MAC, PHY, handlers and static context alive until system
restart, including failures after attachment. `esp_netif_attach` can publish
the netif RX pointer before reporting failure. `Phy::retire` stops/fences and
closes admission; a failure keeps everything alive for inspection. The first
primary result, first cleanup error and native lifecycle journal remain
separate. Posting STOP queues the glue handler. The owner has no receipt
confirming that netif traffic has ended. Even after the glue calls
`esp_netif_action_stop`, RX that already loaded the mediator can still
reach the retained netif input path. No event handler waits for its own unregister, and no installed glue
is deleted. Glue deletion has no independent deadline: unregister can wait on
the default-loop mutex for the longest handler. Handlers here only set atomics.

The example's transport and callback bounds are **conditional admission
values**, inherited from the linked fixture, not measured controller or RTOS
wall-time guarantees. Arbitrary task starvation cannot be bounded by them.
The source Git commit and clean/dirty state refresh into a build-directory
header on each build; archives report `unknown`. Semantic library version is
separate. Physical link, DHCP, peer reachability, waveform and usable board
power remain **NOT RUN**.

For an approved [PHY campaign](../../../docs/hardware-validation.md), record
the exact image and console, confirm a reset-silent attachment, and use
`application stop` before stopped-owner cases. Confirm
`application status` shows installed maintenance with traffic disabled.
Record the cable and peer's exact mode and pause policy for link tests. The
example's application admission uses 100/full. Other legal PHY profiles can
be exercised in stopped maintenance with a matching peer. Check restored
configuration and application readiness before a later start.

The stock image leaves the feature attestation hook unset. The
[diagnostic and control fixture cases](../../../docs/hardware-validation.md#feature-fixtures)
require case-specific application grants, qualified wiring, a known typed
restore path and independent measurements. First-board INT remains blocked
pending wiring proof. Preserve the installed, stopped owner and the exact
console, clock and management route during any authorized fixture work.
