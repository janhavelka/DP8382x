# ESP32-P4 application admission example

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
submission must present the current generation to `allowFrame` against current
gated evidence. Actual RX/TX handoff follows in prompt 42.

Each service turn reads at most one console byte. A maintenance poll executes
at most one indivisible core batch and yields before the next poll. The result
slot applies backpressure until consumed. The core operation budget is 5 s
and 2,048 MDIO frames. The owner requests cancellation after 6 s or 6,400
service steps, then allows up to 100 cancellation and delivery attempts. If
delivery still fails, it retains the token. This example registers no application
frame callback and holds no application buffers, so release establishes
application quiescence immediately. The SDK RX context has a separate lifetime.
See the [application ownership guide](../../../docs/ethercat-integration.md).
