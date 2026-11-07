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

Use fixed queue/frame capacities and finite service budgets with explicit drops.
esp_eth_transmit borrows its buffer only during the call; ESP_OK means queued to
DMA, not transmitted. NO_MEM is backpressure; INVALID_STATE is cached link-down.
With CONFIG_ETH_TRANSMIT_MUTEX off (default), transmit from one task only.
stack_input owns each RX buffer on every return including error and frees it
with free(). Tag traffic by session generation; reject stale frames before TX.
Frame service never bypasses maintenance or adds PHY polling/reset.

Any harmless test frame traffic is explicit and disabled by default. Document
what actual traffic can establish and what remains only linked. If the exact
SDK cannot provide the required safe raw boundary, retain a precise BLOCKED
gate and continue independent eligible work; do not supply a success stub or
call the entire owner integration complete. This does not authorize physical
fixture operation, power actions or servo motion.

RX application context has static lifetime through mac->del. Its nonblocking
callback drops/frees after session end. Teardown swaps the stub mediator first,
then waits for the application RX count zero with a finite deadline, uninstalls,
and deletes MAC. The counter does not prove RX-task idle or close the pre-call
window documented by prompt 37; retain that SDK limit and at-most-one-frame leak.
Use header-free owner tests, pinned-driver host failure injection and P4 link;
none proves actual DMA timing or EtherCAT cycles.

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
