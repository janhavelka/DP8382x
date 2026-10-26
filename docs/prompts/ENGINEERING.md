# Library engineering contract

These are requirements and design constraints for future implementation. Stage
03 resolves framework ownership and stage 04 fixes exact signatures in maintained
API documentation. Changes to those decisions need evidence and coordinated
updates, not compatibility aliases added to hide disagreement.
The [concrete conventions](CONVENTIONS.md) select the current planned Status,
DriverState, snapshot and function names, source responsibilities and Doxygen
requirements from actual sibling code. Stage 04 freezes their exact signatures;
do not independently choose competing names in later prompts.

[FULL_COVERAGE](FULL_COVERAGE.md) defines mandatory public-source research and
complete register/field/procedure coverage. Implement all documented programmable
features of supported parts; runtime opt-in, unconnected board pins and absent
first-use-case demand do not remove a feature from scope. Hardware-only features
retain documentation/observability without invented software controls.

## Product and naming

- Deliver a general microcontroller Ethernet PHY library, starting with exact
  **DP83826I / DP83826IRHBR**, then individually qualifying the documented family.
  Normal 10/100 Ethernet and an ESP32-P4 EtherCAT-master application are consumers.
  The PHY library supplies neither a TCP/IP stack nor an EtherCAT master/ESC,
  servo control, PoE negotiation, power delivery or a board support package.
- Use namespace `DP8382x`, primary class `DP8382x`, include
  `<DP8382x/DP8382x.h>`, portable target `DP8382x`. Follow peer header names
  `Config.h`, `Status.h`, `CommandTable.h`, generated `Version.h` when those
  responsibilities exist. Extra value headers need a concrete readability reason.
- PascalCase types, camelCase methods/fields/locals, `_camelCase` private members,
  uppercase enum values/constants, scoped enums and `constexpr` values. Keep
  names chip-appropriate: MDIO, PHY address, advertisement, link and duplex;
  do not rename I2C sensor methods mechanically or inherit I2C error names.
- Use `Status { Err code; int32_t detail; const char* msg; }` with static message
  lifetime, `ok()`, `is()`, `inProgress()`, explicit bool, `Ok()` and `Error()`
  factories, plus stable printable error names. Stage 04 specifies the actual
  MDIO, deadline, identity, unsupported, busy and configuration errors. Peer enum
  ordinals are not a cross-library ABI. Progress/admission is not success.
- Keep passive `HealthSnapshot`, `LinkSnapshot`, capability and configuration
  evidence distinct. Prefer common peer terminology where its meaning fits.
  Do not fabricate sensor samples, calibration, EEPROM or I2C scan semantics.

## Portable core and framework boundary

Core is standard C++17 with no Arduino/ESP-IDF/FreeRTOS headers, global bus,
platform clocks/delays, tasks, logging, heap allocation, exception dependence,
network stack or FieldCore contracts. Public portable headers must compile alone.
An ordinary C++ application must link and use real chip logic through borrowed
MDIO callbacks. Host test fakes must be clearly identified as simulated.

The native ESP-IDF adapter belongs under separately selected platform paths,
for example `include/DP8382x/esp_idf/` and `src/esp_idf/`. It implements the
supported `esp_eth_phy_t` contract and reuses the pinned IEEE 802.3 base where
appropriate. Do not rebuild the MAC, DMA, frame path or Ethernet stack. Do not
ship an IDF-only driver and describe a hypothetical callback layer as portable.

Before implementation, make an operation-by-operation ownership table for
identity, reset, power state, negotiation, advertisement, symmetric/asymmetric
pause and resolved RX/TX pause reporting, link polling, speed/duplex reporting,
MMD access, interrupts and diagnostics. MAC/application flow-control enable
policy stays outside core. In IDF mode each has
one executing owner. A portable generic path may serve non-IDF hosts while
IDF delegates standard behavior to its base, but they must not run independently
against the same PHY. Share chip-specific masks, validation and procedures.

One application owns each physical PHY's configuration and lifetime. It owns
MDIO controller setup, pins, clock wiring, locks, reset GPIO, monotonic time,
task/ISR scheduling, deadlines, retries and recovery policy. Borrowed objects
must outlive all callbacks and IDF activity. An ISR signals work; the owner
performs MDIO later. Cached getters, rendering and status projection do no I/O.

## MDIO and bounded operations

- Distinguish Clause 22, Clause 22-mediated MMD/extended access and native Clause
  45. Implement only the exact part's supported mapping; do not equate address
  spaces or accept arbitrary device addresses/register holes.
- Serialize the **whole indirect sequence**, and whole read/modify/write where
  needed, across every bus participant: core, base driver, mediator, ioctl,
  polling, CLI and other PHYs. Application-controlled serialization must have
  a documented entry point, lifetime and lock order. Per-frame locking alone
  is insufficient. Avoid nested locks and deadlocks through base callbacks.
- Choose either a terminal transaction/batch callback or an explicit owner-held
  transaction mechanism that permits IDF reuse. Specify the accounting unit.
  A logical MMD operation can contain multiple MDIO frames. A cooperative budget
  cannot split an indirect transaction and release ownership halfway through it.
- Transport callbacks are synchronous and terminal, bounded including lock
  acquisition and underlying controller waits, and never re-enter or retain
  borrowed buffers. Preserve full backend detail; normalize illegal callback
  statuses. A timeout argument or measuring lateness after return is not proof
  of a bound. Audit actual pinned SDK wait paths and document limitations.
- Reads publish outputs only on valid complete success. A write followed by a
  timeout/error can have affected silicon: preserve uncertainty; do not retry
  automatically or restore trusted state just because a later read succeeds.
- Use aggregate operation deadlines, not a fresh full timeout at every step.
  Specify monotonic domain, wrap/overflow rules, timestamp validity, quantization
  and minimum waits. No unbounded busy loops. Long reset/negotiation/diagnostic
  waits must allow owner scheduling; freeze the smallest suitable poll/result
  API at stage 04. Retain terminal results and cancellation effects explicitly.
- Binding, cached snapshots, invalidation and local teardown are bus-silent.
  Hardware reset, software reset, restart, power-down and recovery are separate
  fallible actions. Cancellation cannot undo an accepted register write. Do not
  destroy context or release resources while work still references it.
  Bus-silent cancellation only requests/records cancellation; any restoration
  occurs through subsequent owner-driven work with an explicit finite cleanup
  budget. Do not silently extend the primary operation deadline. If cleanup
  cannot run, preserve untrusted state and expose remaining hardware effects.

## Chip behavior and state

Exact variant/revision datasheets govern masks, resets, waits and ordering.
The [PHY audit](../reports/phy-implementation-audit.md) identifies source conflicts
to resolve, not alternative silicon authority.

- Separate identity evidence from board-declared ordering suffix, operating mode,
  capabilities and revision policy. Reject unsupported IDs/modes deliberately;
  shared IDs do not prove identical devices. Do not blindly accept or reject all
  revisions using a vendor driver's literal revision check.
- Generic link configuration exposes supported 10/100, half/full, advertisement,
  autonegotiation and legal forced modes. Validate unsupported masks/combinations
  before writes. Verify Auto-MDIX/robust/manual MDI/MDIX interactions, including
  SNLS647I's robust Auto-MDIX requirement in Force Mode. The starting board's
  100/full-only advertisement is an example
  profile, never an unconditional core default or silent 10 Mbit fallback policy.
- Review SNLS647I section 8.3.13, `VOD_CFG3` at `0x030E = 0x4A40`, for 10BASE-Te
  applicability and reset restoration. Test placement and write/error handling;
  neither TI reference code nor generic status reads establish this behavior.
- Distinguish BMCR reset, PHYRCR hard reset, PHYRCR restart and external reset/POR.
  Document which configuration survives and which straps are actually resampled.
  Restore only owned configuration in the documented order; preserve reserved
  bits and do not use generic RMW on command, W1C or read-clear fields.
- Model link down, negotiation pending, invalid/stale observation, transport
  failure and application policy mismatch separately. Do not report resolved
  speed/duplex from invalid/incomplete evidence or equate link with IP reachability.
- Specify latch-low, read-clear, write-clear, self-clearing and counter-overflow
  semantics per register. Give destructive status one collector and expose cached
  evidence to other consumers. A dump must not accidentally consume event history.
- Cached desired configuration, observed hardware values and verified ownership
  are different. Raw writes, resets, external writers and ambiguous write failures
  invalidate the relevant evidence; readback compares stable owned bits only.
- Use passive health, normally the sibling four-state vocabulary UNINIT, READY,
  DEGRADED, OFFLINE, with explicitly documented transitions and saturating counters.
  Do not classify an unplugged cable as MDIO failure, a successful status read as
  recovered configuration, or a rejected argument as a physical bus attempt.
  Track all-source MDIO activity separately from managed driver health and from
  chip RX/error counters. Preserve the last error and timestamp validity.
- Loopback, BIST, TDR/cable diagnostics, LEDs, interrupts, WoL, power-down and EEE
  are capability-gated. Disruptive procedures require owner admission, bounded
  completion, truthful results and verified restoration or explicit uncertainty.
  Never enable optional features as undocumented initialization side effects.
- Preserve each later device's documented interface scope. In particular,
  DP83822 includes RGMII as well as RMII/MII; qualify interface timing/delay
  controls per exact variant even if the initial MAC adapter cannot use them.
- Fast Link Drop in SNLS647I section 8.4.1.1 is MII-only. Initial RMII operation
  is not evidence for that feature. C2000/ESC app notes are not P4 master recipes.

## Code, documentation and build ownership

Group implementation by real responsibilities; avoid a huge source file and
avoid a class per register. Keep one chip-specific implementation and shared CLI
processor. Use small fixed buffers and named masks, with citations for non-obvious
sequences. Doxygen describes units, ownership, allowed states, side effects,
blocking bounds, validity and errors. Python docstrings describe protocol and
artifact limits. Tests use independent literal datasheet vectors, not production
constants as their own oracle.
Write Doxygen from the first implemented public file and update it in every
behavior-changing checkpoint. Review affected README, CHANGELOG, API, example,
testing and tool guides at every stage. Use simple engineering language, named
units and explicit conditions; comments explain consequences and ownership.

Use explicit source lists and a dependency-free portable consumer. Tests default
OFF for consumers. SDK-only headers/sources must be excluded from host compilation.
Choose `library.json` as release version source when metadata is added; generate
`Version.h` and applicable manifests reproducibly. Git/build identity is example
metadata, distinct from release version and unknown in untracked source archives.
Do not require Python, Git, Doxygen or a sibling checkout merely to link the core.

Preserve MIT project ownership and actual author/owner information from this repo;
do not copy sibling company names or CODEOWNERS accounts without evidence. Any
reused code retains its original applicable notices. GPL reference implementations
are not source for MIT files. Vendor archives, extracted documents, tests and
fixture logs are excluded from firmware compilation and distributable library
packages as appropriate; notices for actually reused source remain included.
