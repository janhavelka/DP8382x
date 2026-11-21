# ESP32-P4 application admission and raw frames

This ESP-IDF application has one owner task and uses the framework timer for
link observations. It starts with a 100BASE-TX full-duplex advertisement.
A quiet cable, negotiation in progress or a policy mismatch leaves the
application waiting. The shared CLI reports management health, configuration
trust, observed link, session phase, generation and pending result.

Build from the repository root with the pinned ESP-IDF v6.1 wrapper.

```bat
scripts\idf.cmd -C examples/esp_idf/owner -B C:\esp\build\dp8382x-p47-owner set-target esp32p4 build
```

The image starts with the same closed board gate as the [ordinary Ethernet
example](../basic/README.md). Confirm the PHY supply path, address 0, reset,
straps and PHY-provided 50 MHz RMII REF_CLK on the actual board before enabling
the menuconfig board option. The owner reuses its checked GPIO mapping and
build identity. With the gate off, the UART console provides identity, help
and local status while PHY GPIO, MAC and MDIO stay inactive.

## Shared CLI

Open the board's UART0 serial bridge at 115200 baud, 8 data bits, no parity
and one stop bit. The example selects ESP-IDF log V1 and disables the secondary
USB console. The [network application](../basic/README.md) uses the same
`ConsoleSources.cmake`, which compiles `Dp8382xCli.cpp` and
`Dp8382xUartConsole.cpp`. The UART task handles bytes. The application owner
serializes commands, all PHY access and frame service. [CLI guide](../../../docs/cli.md)

| Command | Owner action |
|---|---|
| `application status` | Copy the cached application and frame view. The query itself performs no MDIO. |
| `application admit` | Admit a session from current 100/full evidence. The owner assigns a generation on success. |
| `application release` | Close ingress, quiesce the frame queues and release the active or withdrawn generation. |
| `application send` | Queue the opt-in fixed test frame for a later turn in an active session. |
| `application stop` | After release and queue quiescence, stop and fence the framework for maintenance. |
| `application start` | Require the verified 100/full-only AN profile, reconcile the stopped SDK state and start the framework. |
| `owner cancel <token>`, `owner result [token]` | Request cancellation of the active operation or read the latest delivered outcome and possible effects. |
| `recovery reset pcs`, `recovery reinitialize none` | Use shared maintenance operations. Each has its own token, budget and terminal record. |

On an authorized fixture, wait for `phase=ready` and use `application admit`. A cable
pull withdraws the session when sampled. Reconnection alone leaves the old
generation withdrawn. Issue `application release` and `application stop` for
maintenance. If an operation is cancelled, wait for its terminal record before
requesting a separate restore. After verified recovery and `application start`,
a fresh resolved sample allows another explicit admission. Capture the new
generation and both operation receipts.

The clock domain is monotonic `esp_timer` microseconds. Link polling is set to
1,000 ms. The maximum sample age is 3,000 ms from the start of acquisition.
Owner checks target 100 ms and allow a maximum gap of 250 ms before withdrawing
an active session. A delayed task checks again when scheduled. Every queued
submission presents its original generation to `allowFrame`. The owner holds
the native evidence gate through the call to `esp_eth_transmit`.

Each owner turn calls the bounded shared console service. A cooperative CLI
poll executes at most one indivisible core batch and releases the controller
before the next turn. Backpressure retains pending terminal evidence while
owner servicing and cleanup can continue. `local console` reports queue
depths, drops and stalls. The UART task latches receive errors that the
driver's event queue can drop. Startup still uses its own 5 s, 2,048-frame owner
allowance. CLI operations use the limits in the CLI guide.

## Raw frame service

The owner installs one raw RX callback before framework start. The separate
basic example owns its own netif/glue. On this port the EMAC RX task copies
an admitted frame into a fixed queue, or drops it on contention, invalid
length, closed admission or capacity. The callback calls `free()` on every
path. There are four RX slots and four TX slots of 1,518 bytes, excluding FCS.
The minimum accepted length is the 14-byte Ethernet header. Each service turn
handles at most one RX and one TX. Status rendering and maintenance requests
do not send frames.

The owner counts untagged experimental EtherType `0x88B5` separately.
RX generation identifies when the callback acquired admission. The SDK can
deliver a frame captured earlier, even across a stop or a new session.
Applications requiring request/response correlation need a protocol identifier.

Both the board gate and `CONFIG_DP8382X_ENABLE_TEST_FRAME` default off. With
both enabled for a dedicated fixture, `application send` queues one 60-byte frame, using the
actual MAC source and destination `02:00:00:00:00:2A`. The single owner task
submits it at most once. `application status` displays application drops, DMA acceptance,
backpressure, cached link-down and the first and latest SDK transmit errors.
If transmit returns cached link-down, the application reports that the
framework must restart and withdraws the current generation. Release it and
wait for any pending PHY result, then stop and request verified restoration
if needed. A successful `application start` clears the restart requirement.
A fresh post-maintenance 100/full sample allows another `application admit`.
A PHY UP sample alone can leave the SDK
transmit cache DOWN. Select the example's mutex-off TX
and unchunked cache configuration and a maximum log level of INFO for the
bounded submit path.

Release closes ingress. Before maintenance the owner makes one attempt to
discard outstanding session slots. A contending copy leaves maintenance
unadmitted. An RX callback can still finish freeing its SDK buffer or enter
later against the retained context. A DMA-queued frame may still be on its
way to the wire. Installed storage stays alive until system restart.
See the [application ownership guide](../../../docs/ethercat-integration.md).

For an approved [PHY campaign](../../../docs/hardware-validation.md), record
the exact image and console and confirm a reset-silent attachment. Use
`application release` for a raw session and `application stop` to establish
the framework fence. Require `phase=maintenance`, `maintenance=1` and
`retired=0` in `application status` before stopped-owner cases. The runner
leaves maintenance in place. A later owner decision to start or admit raw
frames requires restored settings, current readiness and a fresh 100/full
observation.

The stock image leaves the feature attestation hook unset. The
[diagnostic and control fixture cases](../../../docs/hardware-validation.md#feature-fixtures)
require an authorized fixture owner to establish the transmit drain, feature
grant, instrumentation and exit route for each case. Where two repeater
owners are involved, capture each owner's separate typed readback and restore
receipt along with the two-direction traffic and clock traces.
