# 41 - Implement explicit application admission and maintenance ownership

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [39](39-esp-idf-link-bounds.md), implemented health/recovery
and retained-operation contracts. Reuse audited P4 board/application patterns
from [40](40-p4-network-example.md) where applicable.

## Current evidence and ownership

Reopen `FieldCore-node/src/devices/i2c/Ina228Module.h/.cpp`: `bind`, `start`,
`poll`, `takeResult`, `refreshStatus`, `snapshot`, `copyStatus`,
`mapLibraryStatus` and their owner/result contracts. Review current peer owner
examples and SNLA344C/494/495 applicability. Record source revisions, paths
and adopted/adapted/rejected behavior. FieldCore is read-only ownership
evidence, not proof it has Ethernet; C2000 ESC/MII recipes are not P4
RMII-master recipes.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Use [link/timing evidence](../reference/library-sdk.md) under
[EVIDENCE](EVIDENCE.md). Admit the application only from fresh resolved
100/full evidence and verified owned configuration. AN-control completion is
not link completion. Define an application freshness limit against its actual
collector cadence (SDK default: 2000 ms); MII-only Fast Link Drop is not an
RMII guarantee.

## Implement

Create a compact native `examples/esp_idf/owner/` application with build
files, board settings and README. A small `main/PhyOwner.h/.cpp` is justified
when it makes states testable; avoid generic registries and stage-named
classes. Freeze example-local names while retaining the selected public core
vocabulary.

Request 100/full advertisement and verify resolved policy before explicit
application-session admission. Keep transport health, configuration trust,
valid link and application readiness separate. Startup admits bounded
initialization only after configuration validates; invalid configuration
admits neither initialization I/O nor an application session. Healthy
unresolved/down link publishes evidence and waits under application policy
without forced-speed fallback. Valid 100/full evidence permits explicit
admission; it does not start an EtherCAT master.

An active session rejects raw mutation, resets, disruptive diagnostics and
power/clock changes before any transfer. Link loss or stale/invalid evidence
withdraws readiness and informs the application. Synchronize admission and
link evidence with the owner so a queued request cannot use an obsolete
link-up. Freeze the freshness limit, clock domain and maximum check/service
gap; every frame submission rechecks the session generation and current
admission. Later link-up does not resume the session. Withdrawal invalidates
queued application traffic, but does not itself grant maintenance while the
application may still own buffers. Explicit release and bounded frame
quiescence plus maintenance admission permit one bounded operation. Retain
primary/restoration results before replacement. Expose cached status without
I/O and preserve cancellation effects and uncertain writes.

Keep bounded service fair to framework work, with no independent reset or
polling path. Demonstrate startup → ready evidence → explicit admission → link
loss → release → maintenance → verified recovery → explicit readmission. Raw
frame integration belongs to task 42; the application remains useful and
linked without invented frame/master success stubs. Servo control, master
stack, process data and machine recovery policy remain separate application
work.

## Acceptance

Build/link the real P4 example. Test policy mismatch, stale old link-up,
active-session rejection with unchanged frame counters, framework contention,
bounded service, retained result under backpressure and cancellation with
visible effects. Do not claim EtherCAT traffic, cyclic timing or machine
safety.

Document owner admission, service budget, cached getters and result lifetime.
Update the owner README and `docs/ethercat-integration.md` with the state
sequence, application responsibilities and physical/timing evidence still
required.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
