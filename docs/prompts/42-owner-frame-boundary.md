# 42 - Demonstrate bounded native MAC frame handoff

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md)
and [VERIFICATION](VERIFICATION.md). Prerequisites:
[41](41-owner-admission-example.md) and the pinned native SDK's actual supported
frame APIs. Keep the existing owner application and its readiness contract.

## Current evidence and ownership

Reopen the pinned SDK's receive/transmit callback declarations, Ethernet driver
implementation, frame-release paths and stop/delete behavior. Recheck current
FieldCore owner ingress, retained results and relevant peer bounded queue/tests,
using exact production paths rather than README claims. Read the applicable
SNLA344C/494/495 sections without importing ESC reset or MII Fast Link Drop
promises. Record revisions, source paths, limits and adopted/adapted/rejected
conventions.

The lead owns application frame arrangement and ownership contracts. Delegate
disjoint queue/lifetime tests or read-only callback analysis after signatures
freeze. Use available models suited to buffer and concurrency reasoning; shared
contracts, architecture, progress and Git have one lead writer.

## Closed work unit

Extend `examples/esp_idf/owner/` with the actual supported raw MAC frame boundary.
State exact SDK receive/transmit signatures, buffer owner, valid callback
context, release point and failure ownership. Select one arrangement that avoids
competing `esp_netif` and raw-frame consumers on the same interface. Keep frame
buffering, protocol demultiplexing and scheduling in the application; never
reimplement MAC/DMA or add an EtherCAT master facade to the PHY library.

Use fixed queue capacities, finite owner work/output budgets and explicit full
queue/drop/rejection evidence. Define startup, stop, drain and teardown so no
callback observes freed context, and no accepted buffer loses its release owner.
Frame servicing cannot bypass the PHY maintenance owner or introduce new reset,
configuration or link polling. Preserve the previous admission/loss/release/
recovery sequence and explicit application readmission.

Any harmless test frame traffic is explicit and disabled by default. Document
what actual traffic can establish and what remains only linked. If the exact SDK
cannot provide the required safe raw boundary, retain a precise BLOCKED gate and
continue independent eligible work; do not supply a success stub or call the
entire owner integration complete. This does not authorize physical fixture
operation, power actions or servo motion.

## Acceptance and checkpoint

Test full queue, failed transmit ownership, callback after stop, drain under
load, teardown failure and multiple buffer arrivals around state transitions.
Assert bounded memory/work and exact release counts, retaining primary and
cleanup errors. Build/link the actual P4 owner application with the same
implemented adapter. Record whether raw handoff was executed with host tests,
linked only, or physically exercised; none proves EtherCAT cyclic timing.

Document frame callbacks/helpers in Doxygen, including lifetime and scheduling
limits. Update the owner README and `docs/ethercat-integration.md` with the
selected netif/raw arrangement, exact build commands, unresolved SDK limits and
remaining master/application work. Review README/CHANGELOG and relevant guides,
run documentation and focused local checks, update evidence/progress, and
commit/sync coherent verified units plus completion on the same execution
branch. Only task 71 dispatches, polls or checks remote CI for this set.
