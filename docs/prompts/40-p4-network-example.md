# 40 - Build the complete ESP32-P4 ordinary Ethernet example

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites:
[39](39-esp-idf-link-bounds.md) and the adapter's concrete configuration/lifecycle
contract. Physical networking is a later evidence layer, not a build substitute.

## Current evidence and ownership

Reopen `TMP1x2/examples/esp_idf/basic/main/main.cpp` and its component CMake,
plus INA228's native `initExample`/`app_main`. Inspect current startup/cleanup,
build identity, console seams, tests, workflows and applicable rules. Recheck
ESP32-P4NRW32X board material and authoritative netlist before selecting hardware
values. Record revisions, paths and adopted/adapted/rejected conventions;
old pin snapshots and I2C targets are not board authority.

The lead owns application lifecycle and board/adapter integration. A disjoint
agent may implement failure cases or review cleanup/guide accuracy after resource
contracts freeze. Choose available models by coding and ownership complexity;
shared contracts and Git remain with the lead.

## Closed work unit

Create `examples/esp_idf/basic/` with native project/component build files,
dependency metadata, `sdkconfig.defaults`, `main/main.cpp` and README. Put board
values in a small application-owned `main/BoardConfig.h`, or a genuinely shared
helper if both P4 examples use it. Avoid board registries and another driver
wrapper. Label facts, user inputs and unresolved assumptions.

Validate PHY address/interface, MDC/MDIO/reset mapping and clock source/direction
before backend creation; do not infer pins from a sibling. Keep reusable general
10/100 configuration available and provide the initial 100/full-only
advertisement as a separate application profile.

Use supported native MAC/PHY, event loop and `esp_netif` integration. Application
code owns DHCP/static addressing and an optional local-peer probe. State the
SDK-verified acquisition order and who releases each resource: event/network
context, MAC/PHY, driver, handlers/network attachment, explicit start. Implement
direct partial cleanup, retain primary and cleanup errors, and drain callbacks
before destroying context. Demonstrate explicit stop/start without infinite
recovery loops.

Callbacks publish bounded evidence or signal the existing owner. Console work
stays outside callbacks; leave the shared CLI attachment seam for task 47.
Never create a second PHY poller. Report management health, configuration trust,
link/profile resolution, IP state and explicitly requested reachability
separately. DHCP failure with healthy MDIO/link remains network evidence; a
valid generic 10/half link can fail the selected application profile.

Include library/source revision and dirty-or-unknown firmware identity, SDK,
target and board settings without secrets. Retain a standalone useful example
without FieldCore, servo actions or Internet provisioning dependencies.

## Acceptance and checkpoint

Inject every resource-acquisition, handler-registration, start/stop and shutdown
callback failure, including withheld console and bursts. Build/link the full
actual pinned P4 image; no Arduino facade or header shim. Record physical
DHCP/link/local-peer results NOT RUN until authorized fixture work.

Ship callback/helper Doxygen for context ownership, task context and work limits;
update example README, `docs/esp-idf.md`, root README and CHANGELOG with exact
build/flash/monitor commands, wiring assumptions and local fixture requirements.
Review affected guides, run source-documentation and focused local checks,
record progress/evidence, and commit/sync checked units and completion on the
same execution branch. Remote CI stays at task 71.
