# Library engineering contract

Use [CONVENTIONS](CONVENTIONS.md) for exact names and file responsibilities.
Prompt 6 establishes framework ownership; prompts 7-8 define the transport,
value, lifecycle and operation contracts in `docs/api-contracts.md`. Subsequent
stages implement and extend those contracts with coordinated consumer updates.
Use [EVIDENCE](EVIDENCE.md) for chip facts and [FULL_COVERAGE](FULL_COVERAGE.md)
for the complete per-part feature inventory.

## Product and portable boundary

Deliver a general C++17 MCU Ethernet PHY core, starting with DP83826I/DP83826IRHBR,
then qualifying each planned family part. Keep generic supported 10/100 and
duplex choices in the library. Put the first board's 100/full advertisement,
recovery policy and EtherCAT scheduling in its application.

Core uses fixed-size values and borrowed terminal callbacks. Keep platform
headers, allocation, exceptions, tasks, delays, logging and network stacks in
adapters/applications. Public headers compile independently. A standalone C++
consumer links real chip behavior through its own MDIO transport.

Use the supported ESP-IDF `esp_eth_phy_t` interface and existing MAC/frame driver.
Put SDK code in explicitly selected `include/DP8382x/esp_idf/` and
`src/esp_idf/` paths. Reuse the IEEE 802.3 base only for operations whose actual
semantics fit the core contract. Share chip procedures between portable and SDK
paths. The MAC owns DMA/frame I/O; applications own network/EtherCAT stacks,
board power delivery, pins, clocks and reset GPIO.

For each identity, reset, power, advertisement, negotiation, pause, link,
interrupt, diagnostic and register operation, assign one executing owner.
Specify how SDK methods, ioctls, polling and application requests reach that
owner. Preserve SDK-cached negotiation state through supported control paths
and report the SDK's pause projection separately from full RX/TX pause resolution.

## Ownership and lifetime

One application owner serializes every call accessing a driver instance,
including cached snapshots and lifecycle calls. Bus-silent access still requires
race-free access to shared memory. Publish copied snapshots to other tasks under
application synchronization. ISR work signals the owner; MDIO runs in task context.

Borrowed callbacks, context and synchronization objects outlive every operation
and SDK callback. Define installation-time context resolution and multiple-instance
behavior using supported SDK APIs, including callbacks before handle publication.
Use explicit instance ownership rather than a hidden global singleton.

Model framework lifecycle calls as fallible sequences with partial effects.
Installation, start, stop and deletion errors can leave callbacks/resources or
state active. Track acquired resources and completed steps; release only after
callbacks are quiescent. Preserve recoverable ownership on failure and specify
the supported retry/cleanup route. Keep primary and cleanup errors separately.

## MDIO transactions and raw access

- Represent C22 register numbers and C22-mediated MMD DEVAD/offset separately.
  Validate the exact part's accessible namespaces, masks and legal encodings.
  Native C45 requires separate source-proved hardware support.
- Serialize each complete indirect sequence and read/modify/write across every
  participant on the controller, including other PHYs. Use one application-owned
  transaction lock or bounded terminal transaction callback with defined lock
  order and lifetime. Reuse that mechanism through SDK entry points.
- Keep long-operation admission separate from the short bus lock. Release the
  lock on every exit before settling waits, subsequent polls or console output.
  Count logical transactions and physical MDIO frames separately. Admit the
  whole indivisible transaction within the current budget.
- Treat C22 REGCR/ADDAR selector/data access as part of an owned MMD transaction.
  Route raw SDK/CLI access through the same admission and access policy. Guard
  destructive reads through the event collector and gate configuration writes
  against active operation/maintenance state. Enforce these routes in code.
- Transport callbacks are synchronous, terminal and bounded, including lock and
  controller waits. Specify callback budget, context/buffer lifetime and legal
  statuses; normalize invalid returns with preserved diagnostic detail. Define
  release-on-return guarantees. A fallible release retains ownership evidence
  and prevents further affected I/O until the application resolves it; keep
  this admission rule separate from passive health. Establish
  actual SDK limits from implementation, not just an API timeout parameter.
- Publish ordinary read outputs only after complete valid success. Immediately
  retain each successfully consumed event/counter sample with validity/time in
  the collector, even if later acquisition fails. Preserve the prior aggregate
  snapshot with explicit stale/invalid status. Record possible history loss if
  a failed read may have consumed an unreturned sample.
- A failed or late write may have reached hardware. Preserve partial effects,
  invalidate affected trust and return the primary error. Retry and recovery
  remain application decisions. Readback verifies only documented stable bits.

## Time, operations and teardown

Use direct bounded calls for short register work. Use one cooperative operation
slot for procedures requiring waits or deferred restoration, with retained
results and nonzero tokens. Freeze exact `start...`, `pollOperation`, `cancel`
and `takeResult` signatures in prompt 8. Apply the same procedure for any
consumer-required blocking wrapper.

Use aggregate deadlines that include controller/owner acquisition and all frames.
Specify monotonic units, wrap/overflow range, quantization and timestamp validity.
Take a fresh timestamp after a side-effecting callback before starting a minimum
wait; with caller-supplied time only, anchor to a conservative subsequent sample.
Use finite work limits even with a frozen/malformed clock. A zero work budget
performs no I/O; deadline/status bookkeeping can still run.

Binding, snapshots, invalidation and local teardown perform no bus operations.
Define which local calls reject active work/unconsumed results and which return
retained evidence. Cancellation records a request or terminal local state;
owner-driven restoration uses a separately admitted finite cleanup allowance.
Track its outcome independently of the primary deadline/status. Preserve pending
hardware effects if cleanup cannot run. Release context only after callbacks
are stopped and evidence has been delivered or explicitly acknowledged.

## Chip behavior and configuration

- Keep raw identity, declared orderable part, revision acceptance, operating mode
  and capabilities distinct. Validate identity before managed configuration and
  feature operations; define explicit low-level access admission separately.
  Use source-backed expected IDs and record conflicting reference-driver labels;
  qualify physical observations separately.
- Support legal advertised/forced 10/100 modes, next-page exchange and full
  pause resolution. Validate supported combinations before writes. Implement
  source-proved Auto-MDIX/robust/manual interactions and the exact 10BASE-Te
  `VOD_CFG3` initialization/restore requirement. The inventory owns precise data.
- Model BMCR reset, PHYRCR hard reset, PHYRCR restart and external reset/POR
  separately, with their strap sampling, configuration loss and readiness rules.
- Maintain desired persistent configuration, observed stable state and verified
  ownership separately. Give every shared field one update path. Temporary
  diagnostic/clock/power profiles use explicit masks and saved owned state.
  Plan ordering for interacting controls and restore persistent state through a
  finite procedure; separate reset-bearing recipes from ordinary field replay.
- Compose mixed control/status writes using owned stable bits and exact command-
  neutral values. Preserve consumed evidence during updates. Apply exact
  W0C/W1C/W1S/RC/latch semantics and per-part MAC byte ordering.
- Raw writes, external reset/writers and ambiguous mutations invalidate affected
  configuration, link and diagnostic evidence. Snapshots expose validity and
  observation time; old data can remain available as explicitly stale evidence.
- Distinguish link down, pending negotiation, invalid observation, transport
  failure and application policy mismatch. Publish resolved speed/duplex/pause
  only when its source evidence supports resolution. IP reachability is separate.
- Give read-clear/latch state one collector and expose cached observations to
  other clients. A safe dump reads stable registers and presents destructive
  status from retained evidence; an explicit live acquisition updates the collector.
- Health uses `UNINIT`, `READY`, `DEGRADED`, `OFFLINE` with defined managed-I/O
  accounting, saturation and timestamp validity. Keep all-source transport
  statistics and PHY error counters separate. Cable disconnect is a link event;
  invalid parameters perform no transport attempt. OFFLINE remains passive
  telemetry and permits application-driven retry.
- Gate disruptive diagnostics and optional activation through owner admission,
  exact capabilities and fixture prerequisites. Complete stop/restore and
  partial-failure behavior with each feature. Document low-power MDIO/clock
  availability and the external wake/reset actions the application must supply.
- Preserve exact interface/medium scope: DP83826I MII/RMII, DP83822's supported
  MII/RMII/RGMII and IF/HF fiber variants. Apply MII-only Fast Link Drop within
  its documented conditions; qualify the initial RMII board separately.

## Implementation and documentation

Group code by actual responsibilities. Prefer small values, named masks,
straight-line procedures and explicit state switches. Keep one chip procedure
and one shared example CLI. Add a helper or file when it serves a concrete
responsibility or eliminates repeated behavior.

Document public types/functions with Doxygen from their first implementation:
units, validity, ownership, callbacks, allowed states, errors, side effects,
blocking bounds and cleanup. Cite exact TI sections for non-obvious procedures.
Use independent literal test vectors and failure injection at each transaction
boundary. Update affected guides and changelog alongside behavior.

Use explicit source lists and tests OFF by default for consumers. Keep SDK,
examples, tests and reference archives outside portable production compilation.
Generate release version metadata reproducibly from `library.json`; keep example
firmware Git identity separate. Ship consumers sufficient source/generated
headers to build without Git, Python, Doxygen or sibling checkouts.

Use repository-established ownership and author details. Preserve applicable
notices for reused permissive source and keep GPL reference implementation out
of MIT source. Package only intended library/adaptor/example material and its
required licenses. Apply COMMON's verification and checkpoint workflow.
