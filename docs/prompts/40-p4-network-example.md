# 40 - Build the complete ESP32-P4 ordinary Ethernet example

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites:
[39](39-esp-idf-link-bounds.md) and the adapter's concrete
configuration/lifecycle contract. Physical networking is a later evidence
layer, not a build substitute.

## Current evidence and ownership

Reopen `TMP1x2/examples/esp_idf/basic/main/main.cpp` and its component CMake,
plus INA228's native `initExample`/`app_main`. Inspect current
startup/cleanup, build identity, console seams, tests, workflows and
applicable rules. Recheck ESP32-P4NRW32X board material and authoritative
netlist before selecting hardware values. Record revisions, paths and
adopted/adapted/rejected conventions; old pin snapshots and I2C targets are
not board authority.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Check [SDK readiness](../reference/library-sdk.md) with
[EVIDENCE](EVIDENCE.md). Require explicit pin/address/clock choices and
power/reset readiness. A 100 us assertion alone does not provide the 2 ms
post-reset SMI interval. Set the link poll period as application policy; 2000
ms is the SDK default, not a PHY response guarantee. Keep build configuration
separate from observed hardware identity.

## Implement

Create `examples/esp_idf/basic/` with native project/component build files,
dependency metadata, `sdkconfig.defaults`, `main/main.cpp` and README. Put
board values in a small application-owned `main/BoardConfig.h`, or a genuinely
shared helper if both P4 examples use it. Avoid board registries and another
driver wrapper. Label facts, user inputs and unresolved assumptions.

Set every BoardConfig field explicitly: smi_gpio={40,39}, RMII,
clock_config.rmii={EMAC_CLK_EXT_IN,44}, dataif RMII tx_en49,txd0=41,txd1=42,
crs_dv51,rxd0=52,rxd1=53, reset54. Startup checks every configured field against
its constant and all pins for uniqueness; SDK defaults otherwise silently select
wrong data pins. Handle a null esp_eth_mac_new_esp32 result. P4 IO_MUX restricts
signal pads. Confirm this2026-10-27 mapping against the final netlist before
flashing; older board paragraphs contain superseded pins. Keep address/clock
inputs explicit and hardware gated when authority is missing.

Use supported native MAC/PHY, event loop and `esp_netif` integration.
Application code owns DHCP/static addressing and an optional local-peer probe.
State the SDK-verified acquisition order and who releases each resource:
event/network context, MAC/PHY, driver, handlers/network attachment, explicit
start. Implement direct partial cleanup, retain primary and cleanup errors,
and drain callbacks before destroying context. Delete only resources created
by this application; shared event-loop/netif facilities remain with their
owner. An event handler must signal shutdown rather than synchronously wait
for its own callback to finish. Cover SDK failures that leave start/stop or
uninstall partly applied. Demonstrate explicit stop/start without infinite
recovery loops.

Callbacks publish bounded evidence or signal the existing owner. Console work
stays outside callbacks; leave the shared CLI attachment seam for task 47.
Never create a second PHY poller. Report management health, configuration
trust, link/profile resolution, IP state and explicitly requested reachability
separately. DHCP failure with healthy MDIO/link remains network evidence; a
valid generic 10/half link can fail the selected application profile.

Include library/source revision and dirty-or-unknown firmware identity, SDK,
target and board settings without secrets. Retain a standalone useful example
without FieldCore, servo actions or Internet provisioning dependencies.

Default-loop handlers must not block. Glue deletion has no independent deadline;
its bound is the longest default-loop handler because unregister can wait for
the recursive loop mutex. Use prompt 37's pinned-driver host layer for lifecycle
failure injection and a real P4 link; physical networking stays NOT RUN.

## Acceptance

Inject every resource-acquisition, handler-registration, start/stop and
shutdown callback failure, including withheld console and bursts. Build/link
the full actual pinned P4 image; no Arduino facade or header shim. Record
physical DHCP/link/local-peer results NOT RUN until authorized fixture work.

Document application callbacks/helpers, resource ownership and service limits.
Update the example README and `docs/esp-idf.md` with build/flash/monitor
commands, explicit wiring assumptions and separately gated fixture
instructions.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
