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

Apply [SDK timing/notification evidence](../reference/library-sdk.md).
Base reset/AN loops do not run. Bounds include 100 us MAC polling steps to nominal
1000 us/frame, MAC-init reset polling in 10 ms steps to sw_reset_timeout_ms,
mac->stop up to 2500 us busy-wait, owner polls and check_link_period_ms.
The separate 100 ms MAC mutex does not serialize MDIO. State scheduling assumptions;
nominal loop counts do not prove wall time. Check 2 ms readiness at 100 Hz ticks.

## Implement

Complete a method-by-method conformance table covering every selected
`esp_eth_phy_t` entry, executing owner, complete transaction, callback
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

For the pinned P4 MAC, set_link cannot fail: after on_state_changed(LINK,x)
returns, record published=x and never replay it. An error with valid argument
sets eventLost and the owner may re-post. For another MAC, including instrumented
tests, a notification error means publication uncertain and requires stop.
The factory selects this policy; the mixed error code cannot distinguish it.
Reject forced100/half plus extendedFullDuplex before mutation because the adapter
cannot publish a resolved tuple. Keep PHY entry conformance free of base methods.

## Acceptance

Run notification/flow-control tests, callback error injection,
start/stop/delete lifetimes and actual raw/typed ownership regressions.
Build/link the real P4 fixture, rebuild the portable consumer and record
actual SDK/tool versions. Separate host contract tests, SDK link and physical
timing; missing SDK access remains BLOCKED.

Update adapter Doxygen, `docs/esp-idf.md` and timing/notification tables with
proved bounds, scheduler assumptions and exact supported SDK restrictions.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
