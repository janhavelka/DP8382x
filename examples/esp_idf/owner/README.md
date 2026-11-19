# ESP32-P4 application admission and raw frames

This ESP-IDF application has one owner task and uses the framework timer for
link observations. It advertises 100BASE-TX full duplex only. A quiet cable,
negotiation in progress or a policy mismatch leaves the application waiting.
The status output shows management health, configuration trust, observed link,
session phase, generation and pending result.

Build from the repository root with the pinned ESP-IDF v6.1 wrapper.

```bat
scripts\idf.cmd -C examples/esp_idf/owner -B C:\esp\build\dp8382x-p41 set-target esp32p4 build
```

The image starts with the same closed board gate as the [ordinary Ethernet
example](../basic/README.md). Confirm the PHY supply path, address 0, reset,
straps and PHY-provided 50 MHz RMII REF_CLK on the actual board before enabling
the menuconfig board option. The owner reuses its checked GPIO mapping and
build identity. The console needs nonblocking input.

| Key | Owner action |
|---|---|
| `s` | Show the cached owner view. The periodic service copies framework evidence without issuing MDIO. |
| `a` | Explicitly admit a session using the current evidence. The owner assigns a generation on success. |
| `r` | Release the active or withdrawn generation. |
| `x` | When the test-frame option is enabled, queue one experimental frame for a later service turn in the active session. |
| `m` | Admit one maintenance operation, stop the framework, request a PCS reset and replay owned settings. |
| `c` | During maintenance, cancel the current operation and retain its result and possible effects. |
| `t` | Take the pending result. Primary and restoration receipts have separate storage. |
| `v` | Request a separate initialization with reset kind NONE for restoration, under its own budget. |
| `q` | Stop after releasing the session and taking any pending result. Installed resources stay alive until restart. |

On an authorized fixture, wait for READY and press `a`. A cable pull withdraws
the session when sampled. Reconnection alone leaves the old generation withdrawn.
Release with `r`, then use `m` for maintenance. Use `c` during the operation
to exercise cancellation. Take the primary result with `t` before requesting
`v`. After verified recovery and framework restart, a new resolved sample
permits another `a`. Capture the new generation and both operation receipts.

The clock domain is monotonic `esp_timer` microseconds. Link polling is set to
1,000 ms. The maximum sample age is 3,000 ms from the start of acquisition.
Owner checks target 100 ms and allow a maximum gap of 250 ms before withdrawing
an active session. A delayed task checks again when scheduled. Every queued
submission presents its original generation to `allowFrame`. The owner holds
the native evidence gate through the call to `esp_eth_transmit`.

Each service turn reads at most one console byte. A maintenance poll executes
at most one indivisible core batch and yields before the next poll. The result
slot applies backpressure until consumed. The core operation budget is 5 s
and 2,048 MDIO frames. The owner requests cancellation after 6 s or 6,400
service steps, then allows up to 100 cancellation and delivery attempts. If
delivery still fails, it retains the token.

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
both enabled for a dedicated fixture, `x` queues one 60-byte frame, using the
actual MAC source and destination `02:00:00:00:00:2A`. The single owner task
submits it at most once. `s` displays application drops, DMA acceptance,
backpressure, cached link-down and the first and latest SDK transmit errors.
Cached link-down withdraws the session. Select the example's mutex-off TX
and unchunked cache configuration and a maximum log level of INFO for the
bounded submit path.

Release closes ingress. Before maintenance the owner makes one attempt to
discard outstanding session slots. A contending copy leaves maintenance
unadmitted. An RX callback can still finish freeing its SDK buffer or enter
later against the retained context. A DMA-queued frame may still be on its
way to the wire. Installed storage stays alive until system restart.
See the [application ownership guide](../../../docs/ethercat-integration.md).
