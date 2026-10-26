# 42 - Demonstrate bounded native MAC frame handoff

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [41](41-owner-admission-example.md) and the pinned native SDK's
actual supported frame APIs. Keep the existing owner application and its
readiness contract.

## Current evidence and ownership

Reopen the pinned SDK's receive/transmit callback declarations, Ethernet
driver implementation, frame-release paths and stop/delete behavior. Recheck
current FieldCore owner ingress, retained results and relevant peer bounded
queue/tests, using exact production paths rather than README claims. Read the
applicable SNLA344C/494/495 sections without importing ESC reset or MII Fast
Link Drop promises. Record revisions, source paths, limits and
adopted/adapted/rejected conventions.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Implement

Extend `examples/esp_idf/owner/` with the actual supported raw MAC frame
boundary. State exact SDK receive/transmit signatures, buffer owner, valid
callback context, release point and failure ownership. Select one arrangement
that avoids competing `esp_netif` and raw-frame consumers on the same
interface. Keep frame buffering, protocol demultiplexing and scheduling in the
application; never reimplement MAC/DMA or add an EtherCAT master facade to the
PHY library.

Use fixed queue and frame-size capacities, finite owner work/output budgets
and explicit full queue/drop/rejection evidence. Preserve SDK-specific RX
release ownership on enqueue failure and TX ownership through actual
completion. Apply length/overflow validation before copying; byte counts alone
do not prove buffer lifetime. Tag queued traffic with the admitted session
generation and drop/release old-session frames before transmission after loss,
stop or readmission. Define startup, stop, drain and teardown so no callback
observes freed context, and no accepted buffer loses its release owner. Frame
servicing cannot bypass the PHY maintenance owner or introduce new reset,
configuration or link polling. Preserve the previous admission/loss/release/
recovery sequence and explicit application readmission.

Any harmless test frame traffic is explicit and disabled by default. Document
what actual traffic can establish and what remains only linked. If the exact
SDK cannot provide the required safe raw boundary, retain a precise BLOCKED
gate and continue independent eligible work; do not supply a success stub or
call the entire owner integration complete. This does not authorize physical
fixture operation, power actions or servo motion.

## Acceptance

Test full queue, oversize frames, failed transmit ownership, stale-session
queued frames, callback after stop, drain under load, teardown failure and
multiple buffer arrivals around state transitions. Assert bounded memory/work
and exact release counts, retaining primary and cleanup errors. Build/link the
actual P4 owner application with the same implemented adapter. Record whether
raw handoff was executed with host tests, linked only, or physically
exercised; none proves EtherCAT cyclic timing.

Document frame callbacks/helpers and RX/TX lifetime, queue limits and
teardown. Update the owner README and `docs/ethercat-integration.md` with the
chosen raw/netif arrangement, actual build commands and remaining
master/application work.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
