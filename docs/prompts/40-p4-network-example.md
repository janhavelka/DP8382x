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

Validate PHY address/interface, MDC/MDIO/reset mapping and clock
source/direction before backend creation. When authoritative wiring is
unavailable, require explicit application configuration and leave hardware
execution gated. A linked configuration with declared assumptions proves
compilation only. Keep reusable general 10/100 configuration available and
provide the initial 100/full-only advertisement as a separate application
profile.

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
