# 41 - Implement explicit application admission and maintenance ownership

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md)
and [VERIFICATION](VERIFICATION.md). Prerequisites:
[39](39-esp-idf-link-bounds.md), implemented health/recovery and retained-operation
contracts. Reuse audited P4 board/application patterns from
[40](40-p4-network-example.md) where applicable.

## Current evidence and ownership

Reopen `FieldCore-node/src/devices/i2c/Ina228Module.h/.cpp`: `bind`, `start`,
`poll`, `takeResult`, `refreshStatus`, `snapshot`, `copyStatus`,
`mapLibraryStatus` and their owner/result contracts. Review current peer owner
examples and SNLA344C/494/495 applicability. Record source revisions, paths and
adopted/adapted/rejected behavior. FieldCore is read-only ownership evidence,
not proof it has Ethernet; C2000 ESC/MII recipes are not P4 RMII-master recipes.

The lead owns readiness/maintenance states and public contract integration.
Delegate disjoint transition tests or a read-only ownership review with exact
paths and frozen semantics. Select available models with stronger reasoning
for admission/lifetime analysis; no reviewer edits shared headers or Git.

## Closed work unit

Create a compact native `examples/esp_idf/owner/` application with build files,
board settings and README. A small `main/PhyOwner.h/.cpp` is justified when it
makes states testable; avoid generic registries and stage-named classes. Freeze
example-local names while retaining the selected public core vocabulary.

Request 100/full advertisement and verify resolved policy before explicit
application-session admission. Keep transport health, configuration trust, valid
link and application readiness separate. Startup/invalid configuration admits
bounded initialization but no application session. Healthy unresolved/down link
publishes evidence and waits under application policy without forced-speed
fallback. Valid 100/full evidence permits explicit admission; it does not start
an EtherCAT master.

An active session rejects raw mutation, resets, disruptive diagnostics and
power/clock changes before any transfer. Link loss or stale/invalid evidence
withdraws readiness and informs the application. Later link-up does not resume
the session. Explicit release plus maintenance admission permits one bounded
operation and retains primary/restoration results before replacement. Expose
cached status without I/O and preserve cancellation effects and uncertain writes.

Keep bounded service fair to framework work, with no independent reset or
polling path. Demonstrate startup → ready evidence → explicit admission → link
loss → release → maintenance → verified recovery → explicit readmission. Raw
frame integration belongs to task 42; the application remains useful and linked
without invented frame/master success stubs. Servo control, master stack,
process data and machine recovery policy remain separate application work.

## Acceptance and checkpoint

Build/link the real P4 example. Test policy mismatch, stale old link-up,
active-session rejection with unchanged frame counters, framework contention,
bounded service, retained result under backpressure and cancellation with visible
effects. Do not claim EtherCAT traffic, cyclic timing or machine safety.

Ship owner Doxygen for admission, service budget, cached getters and retained
result lifetime. Update `docs/ethercat-integration.md`, example README, root
README and CHANGELOG with exact boundaries, loading-machine decisions left to
the application and hardware/timing gates. Review affected guides and run local
behavior/documentation checks. Record evidence and commit/sync coherent checked
units and task completion on the recorded branch; remote CI stays at 71.
