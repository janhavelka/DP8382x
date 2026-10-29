# Planned ESP-IDF examples

The first example should bring up the ESP32-P4 MAC with DP83826I, report PHY
identity and negotiated link state, and provide bounded register diagnostics.
It should accept board-specific pins, address, clock and advertisement policy.

There is no buildable example yet. Use the [board context](../../docs/esp32p4-board.md)
and [official SDK references](../../docs/reference/source-code.md) when implementing
it. Keep EtherCAT master integration in the application that owns that stack.

The [promptset](../../docs/prompts/README.md) separately plans an ordinary
Ethernet/IP example and an owner example demonstrating the 100/full EtherCAT
integration boundary, followed by a shared diagnostic CLI. Neither supplies an
EtherCAT master or servo-control implementation. Application power delivery is
outside the library and examples.

Both native examples will own MAC/controller creation, board settings, PHY
address, clock, scheduling and console transport. They will admit one executing
owner per PHY operation, serialize complete indirect MDIO transactions across
all bus participants and publish copied snapshots. They must retain partially
created resources and report cleanup failures until release is known. The
shared CLI will attach without starting hardware work; startup belongs to the
example. A separate plain C++ MCU consumer proves core portability, while the
native examples must be linked with the selected ESP-IDF SDK. Existing sibling
I2C examples do not establish an Ethernet backend or P4 link result.

The [prompt-6 decision](../../docs/design-decisions.md) now fixes these routes:

- Keep native lifecycle/SDK handles inside the application owner. Reserve control
  work, release the instance gate before synchronous SDK calls, and drain callbacks
  without holding locks they need. Timer polling is the only running link poller.
- Register the rejecting raw frame hooks. CLI register/feature commands submit
  typed owner requests to the shared core/batch path; destructive dumps use the
  collector. Forward batches directly to the actual MAC under one shared bus lock.
- Stop and drain before disruptive configuration. Apply AN/speed/duplex through
  public SDK controls and verify hardware/cache agreement before restart. Full
  advertisement and diagnostics use the shared chip procedures. MAC flow-control
  policy is separate from full portable RX/TX pause resolution.
- Pin v6.1 source plus the required verified lifecycle correction before claiming
  native support. Failed install may retain a cleanup-only handle under that
  corrected contract; failed stop/uninstall never licenses freeing live contexts.
- Select GPIO54 reset, MDC49, MDIO43 and REF_CLK input GPIO44 from the current
  board evidence, with declared/verified PHY address 0. Recheck actual netlist,
  R21/C24 reset pulse and power/clock readiness before firmware qualification.
  Do not inherit GPIO5, auto-detection, no post-reset wait or 2000 ms polling as
  correct board defaults. Use elapsed-time checks for the 2 ms/50 ms SMI windows.
  Close shared-controller admission before external power-up/hardware reset and
  through readiness;
  other PHYs must remain silent too, although the bus mutex is released.

The portable owner example will bind two core instances to borrowed batch
contexts sharing one controller lock, explicitly initialize/configure/poll/take
results and publish copied snapshots. It needs no SDK headers, tasks, heap or
FieldCore dependency. This is a design walkthrough, not a completed consumer.

The [value/direct-call contract](../../docs/api-contracts.md) now supplies exact
declarations and a plain-owner example. Bind explicit Variant/interface/address,
borrowed batch and monotonic clock callbacks, and proved time allowances. A live
link query stages the result and assigns valid down on success; failed aggregate
queries leave caller output unchanged while retaining consumed samples. Publish
copied health/link/configuration views separately. `resetStatistics` clears only
managed cumulative totals, and a later transport success does not verify settings.
Native examples must preserve original signed errors and release/clock faults;
the complete stopped-driver SDK transition remains in the owner design. These
are implementation instructions, not a firmware build result.

Owner examples return binding failures to the caller and use the core's staged
output contract directly. Health's last error identifies a counted transport,
release or timing failure; an earlier admission error can still be the public
primary result. Copy the retained transport/collector evidence before subsequent
requests replace the last batch receipt.

The [cooperative examples](../../docs/api-contracts.md#operation-acceptance-scenarios-and-examples)
now give exact plain-owner and native reset callback paths. Retain full primary
and cleanup evidence, keep a token when take fails, and budget native scheduling
inside the existing lifecycle reservation. A cancelled/failed operation may
still require later owner cleanup. Deletion follows disposition, successful
result delivery, local end and actual SDK callback/resource drain. Tick sleeps
are scheduling only; elapsed-time admission uses the qualified monotonic clock.
