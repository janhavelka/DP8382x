# 17 - Demonstrate EtherCAT application ownership boundaries

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-16.

Before the audit, apply COMMON's short rule reminder: current sibling sources,
selected conventions, owned paths, source/general documentation, focused local
checks and same-branch commit/sync. Choose subagent models by task complexity.

## Fresh audit and owned work

Revisit FieldCore owner scheduling/status/CLI ingress and comparable peer owner-service
examples. Read SNLA344C/494/495 for applicability while preserving master-versus-ESC
distinction.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Create a separate ESP32-P4 owner/integration example and guide for a future
EtherCAT-master application. Configure the requested autonegotiation advertisement of
100/full, verify resolved policy before admitting an application session, and expose
link loss/recovery evidence to the owner. Do not automatically force speed, reset the
PHY or resume servo operation.

Demonstrate the actual supported raw MAC frame boundary only where the pinned SDK
permits it. Frame buffers, scheduling, protocol demultiplexing, EtherCAT master stack,
process data and servo control remain application responsibilities. Avoid competing
esp_netif/raw-frame ownership on the same interface; document the selected arrangement
and buffer lifetime.

Implement a small owner example with bounded link service, maintenance admission and
cached health/status projection. A diagnostic/reset request is rejected while an
application session owns the link. The example may exercise lifecycle and frame handoff
with harmless explicit test traffic; it must not masquerade as a working EtherCAT master
or create a success stub for master start.

Document startup/stop/recovery decisions for a loading machine, required integration
work and the separate timing/servo qualification boundary. Do not transplant C2000 ESC
resets or MII Fast Link Drop promises into the RMII P4 design. Adding a real third-party
master stack is a separately scoped application project, not completion padding for this
library.

## Concrete owner behavior

Apply [CONVENTIONS](CONVENTIONS.md). Inspect
`FieldCore-node/src/devices/i2c/Ina228Module.h/.cpp`: `bind`, `start`, `poll`,
`takeResult`, `refreshStatus`, `snapshot`, `copyStatus` and `mapLibraryStatus`.
Read the related device-result/status contracts and owner transport. Borrow
single-owner ingress, retained completion and cached projection; do not import
TunnelMonitor types, task topology, I2C error mappings or its health enum values.
This source is an ownership example, not evidence of FieldCore Ethernet support.

Keep one compact application under `examples/esp_idf/owner/` with native build
files, board settings and README. A small `main/PhyOwner.h/.cpp` is justified if
it makes the state transitions independently testable; otherwise keep the owner
with the application. Use descriptive permanent names, never stage-numbered
classes or a generic service/task registry. Freeze example-local entry names in
its guide; the public core operation names remain those from stage 04.

| Owner condition | Required behavior |
| --- | --- |
| Startup or invalid configuration | Permit bounded initialization through its single owner; do not admit an application session |
| Valid management and verified profile, link unresolved/down | Publish evidence and wait within application policy; no silent forced-speed fallback |
| Resolved 100/full with required evidence | Allow explicit application admission; admission alone does not start an EtherCAT master |
| Active application session | Reject reset, raw mutation, disruptive diagnostic and power/clock changes before any MDIO transfer |
| Link loss or stale/invalid observation | Withdraw readiness and notify the application; do not resume automatically on a later link-up |
| Explicitly released session and maintenance admission | Permit one bounded operation and retain primary/restoration results before accepting replacement |

Test the sequence startup -> ready evidence -> explicit admission -> link loss ->
session release -> maintenance -> verified recovery -> explicit readmission.
Test active-session rejection with unchanged transfer counters, stale-link
rejection despite a previous link-up, contention with framework polling, result
retention under output backpressure and cancellation that leaves hardware effects
visible. A boolean named ready must never collapse health, link, trust and policy.

For any frame hook, document exact SDK receive/transmit signatures, buffer owner,
release point, maximum queued frames and stop/drain behavior. Test a full queue,
callback after stop and failed transmit ownership. Keep frame traffic disabled
by default and explicit; a compile-only frame consumer cannot prove traffic or
timing. If the supported raw boundary is unresolved, retain that precise gate
without inventing a successful EtherCAT start function.

Add Doxygen for owner admission, service budget, result lifetime, cached getter
semantics and frame ownership. Maintain `docs/ethercat-integration.md`, this
example README, root README and CHANGELOG with the exact implemented boundary,
application work still required and hardware/timing limits. Verify source docs
and guide names alongside behavior. Delegate read-only ownership/timing review
when useful; use an agent with sufficient reasoning for concurrency analysis,
not a fixed model/version. The lead integrates shared headers and architecture.

## Acceptance and handoff

Build/link the example. Test policy mismatch, link loss, admitted/rejected maintenance,
bounded owner fairness and cleanup/buffer ownership. Report whether raw-frame handling
was actually exercised or only built. No claims of EtherCAT cyclic timing, servo motion
or safety qualification without real application evidence.

Expected output: Buildable owner example and honest EtherCAT-master integration
contract. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
