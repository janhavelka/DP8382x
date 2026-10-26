# 39 - Prove native link notifications and adapter timing contracts

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [37](37-esp-idf-lifecycle.md),
[38](38-esp-idf-state-arbitration.md) and implemented portable link/pause
evidence. Earlier tasks already deliver working callbacks, cleanup and finite
transport behavior; this is conformance closure and fixes.

## Current evidence and ownership

Reopen current sibling native error/lifecycle tests and the pinned SDK base,
mediator, MAC pause notification and controller wait implementations. Inspect
source behind documented timeout parameters, including lock acquisition,
quantization and reset/negotiation paths. Compare relevant peer tests with the
production methods they execute. Record exact source revisions, paths,
adopted/adapted/rejected behavior and changes since task 37's audit.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Apply [SDK timing/notification evidence](../reference/library-sdk.md) and
[EVIDENCE](EVIDENCE.md). Account for 100 us MAC polling increments up to
nominal 1000 us, 10 ms reset polling, 100 ms AN polling and default 2000 ms
link polling. The separate 100 ms MAC mutex does not serialize MDIO. Four
nominal frame loops are not a 4 ms wall-clock guarantee. Verify the 2 ms reset
interval from release with coarse ticks, including 100 Hz quantization.
Distinguish finite instruction counts, scheduling assumptions, silicon limits
and measured wall time.

## Implement

Complete a method-by-method conformance table covering every selected
`esp_eth_phy_t`/base entry, executing owner, complete transaction, callback
lifetime, side effects, errors, notification and worst supported blocking
bound. Close confirmed defects in owning code with focused regressions. Do not
create a second implementation to make tests pass or defer essential running
behavior from earlier tasks into this review.

Audit pause negotiation, partner ability and flow-control notification against
the exact pinned API. Preserve full portable symmetric/asymmetric RX/TX
evidence. The SDK's Boolean peer-pause MAC notification is a narrower
projection, not independent RX/TX resolution. Override base behavior where
semantics differ; document the projection and keep application/MAC enable
policy explicit.

Prove notification ordering for both link transitions and speed/duplex/pause
changes while link remains up. Track acquired observations separately from
each successfully published SDK field. If a later notification fails, retain
earlier side effects and retry only through the defined publication policy; do
not mark the whole tuple applied. Unchanged complete state produces no
duplicate event, and down/incomplete negotiation invents no resolved
speed/duplex. Stop/start must resynchronize both adapter and SDK publication
state. Check polling, stop/start and teardown interleavings against the single
owner. Preserve structured backend detail internally and explain information
that the narrower SDK return type cannot carry; never leak SDK error types
into the core.

Establish complete indirect-transaction and aggregate-operation bounds from
controller execution, owner acquisition, scheduling assumptions, quantization
and cleanup. State the supported scheduler/workload conditions; arbitrary task
preemption does not have a guaranteed finite wall-clock bound. Exercise
exhausted budgets and late terminal callbacks. Late-return detection
establishes a contract violation, not preemption. If a necessary finite bound
is unavailable, retain the exact affected SDK capability gate and constrain
claims; do not replace the controller with a successful mock or advertise a
guessed minimum/tested SDK range. Avoid private fragile APIs without a narrow
justified version contract and real tests.

## Acceptance

Run notification/flow-control tests, callback error injection,
start/stop/delete lifetimes and actual raw/base ownership regressions.
Build/link the real P4 fixture, rebuild the portable consumer and record
actual SDK/tool versions. Separate host contract tests, SDK link and physical
timing; missing SDK access remains BLOCKED.

Update adapter Doxygen, `docs/esp-idf.md` and timing/notification tables with
proved bounds, scheduler assumptions and exact supported SDK restrictions.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
