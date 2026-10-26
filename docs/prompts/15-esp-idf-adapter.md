# 15 - Implement the native ESP-IDF PHY adapter

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-14 and the approved stage 03 ownership matrix.

## Fresh audit and owned work

Re-audit native-IDF component lifecycle/cleanup and error mapping in
INA228/PCA9555/TCA9548A; inspect actual pinned Espressif PHY/base/mediator sources and
compare to the design.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Implement the supported esp_eth_phy_t factory/adapter in platform-only paths. Reuse the
selected IEEE 802.3 base methods for standard behavior and the common chip-specific
implementation for DP83826I handling. Follow the ownership matrix exactly: no second
independent reset, autonegotiation, link polling or MAC notification path. Audit the
SDK's pause negotiation/peer ability and flow-control notification contract; map
resolved RX/TX evidence correctly without enabling application/MAC flow-control policy
implicitly.

Wire borrowed mediator/MDIO transport and complete-transaction serialization across base
MMD, raw ioctls, application operations and CLI. Audit address discovery and reset
semantics rather than inheriting them blindly. Establish lock order and callback
reentrancy, init/deinit/delete lifetime, start/stop handoff and callback error
translation. Document any framework allocation and cleanup outside the allocation-free
core.

Prove underlying timeout behavior for the pinned SDK/MAC. If callbacks lack configurable
aggregate limits, constrain supported operations and document actual finite bounds;
don't pretend the core timeout can preempt an SDK block. Avoid private/version-fragile
APIs unless justified and tested with a narrow version contract.

Create component metadata/source selection without exposing SDK headers to portable
consumers. Use accurate minimum/tested SDK ranges, not a guessed broad semver range. Add
a real linked P4 integration fixture now; it may be small but must instantiate the
actual MAC/PHY framework. No successful hardware shims.

## Acceptance and handoff

Run host adapter contract/fault tests where meaningful and build/link real native IDF P4
firmware. Verify all esp_eth_phy_t methods, delete/error unwinding, no duplicate
notifications and transaction arbitration. Rebuild the plain host consumer to detect
leakage. Report missing SDK as BLOCKED, not simulated success.

Expected output: Native adapter and real SDK integration evidence. Include source
documentation, relevant guides, feature/evidence matrix and progress updates. Apply
COMMON's completion rules; report actual commands, results and remaining gates. Continue
only when the concrete required inputs exist; COMMON explains how unrelated gates stay
open.
