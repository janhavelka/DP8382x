# 16 - Create the ESP32-P4 ordinary Ethernet example

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-15.

## Fresh audit and owned work

Inspect sibling native-IDF example configuration, startup/cleanup, build identity and
shared CLI seams. Recheck the current board reference and authoritative netlist before
hardware use.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Create examples/esp_idf/basic as a complete native application with explicit board
config, PHY address/interface, clock direction, MDC/MDIO/reset mapping and configurable
link profile. Keep the general reusable configuration available and provide the initial
100/full-only advertisement profile separately. Verify current ESP32-P4NRW32X settings
from authoritative board material; don't silently freeze old pin snapshots.

Use supported ESP-IDF MAC/PHY/network integration for ordinary Ethernet and IP. Keep
esp_netif, DHCP/static addressing, optional local network probe and event loop in the
application. Distinguish management-ready, link-up, address-acquired and remote
reachability in output; don't diagnose every DHCP/internet failure as PHY failure.

Demonstrate resource ownership, start/stop/restart and cleanup on partial
initialization. A console/CLI integration seam belongs outside callbacks; stage 19
attaches the shared processor. Build identity reports library, source dirty/unknown,
SDK, target and board settings. No secrets in status.

Document wiring assumptions, native build/flash/monitor commands, local peer fixture and
scope of claims. Power delivery and internet provisioning policy are outside core.
Retain a minimal example useful without FieldCore or the loading machine.

## Acceptance and handoff

Build and link the full image against the pinned SDK, verify current dependencies and no
Arduino façade. Exercise supported lifecycle paths with adapter tests. Record physical
networking as NOT RUN until actual fixture work. Inspect defaults to ensure no
power/servo activity or hidden diagnostic stimulus.

Expected output: Buildable general Ethernet/IP example with application-owned hardware.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
