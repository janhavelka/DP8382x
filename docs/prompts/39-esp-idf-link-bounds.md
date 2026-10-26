# 39 - Prove native link notifications and adapter timing contracts

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md)
and [VERIFICATION](VERIFICATION.md). Prerequisites:
[37](37-esp-idf-lifecycle.md), [38](38-esp-idf-state-arbitration.md) and implemented
portable link/pause evidence. Earlier tasks already deliver working callbacks,
cleanup and finite transport behavior; this is conformance closure and fixes.

## Current evidence and ownership

Reopen current sibling native error/lifecycle tests and the pinned SDK base,
mediator, MAC pause notification and controller wait implementations. Inspect
source behind documented timeout parameters, including lock acquisition,
quantization and reset/negotiation paths. Compare relevant peer tests with the
production methods they execute. Record exact source revisions, paths,
adopted/adapted/rejected behavior and changes since task 37's audit.

The lead owns the method/ownership/bounds table and corrections. A read-only
reviewer can independently trace waits and notification meaning; disjoint
fault tests may be delegated after shared contracts freeze. Use available
models with sufficient reasoning for deadlines and concurrency, record the
choice, and keep shared headers and Git with the lead.

## Concrete public-source inputs

Use [actual SDK loop/default values](../reference/library-sdk.md), not assumed
deadlines: MAC busy polling is 100 us per iteration up to a nominal 1000 us
counter; PHY reset polls every 10 ms and AN every 100 ms; default link polling
is 2000 ms. The separate 100 ms MAC mutex is not acquired by PHY read/write
methods. Four MMD frames therefore do not prove a 4 ms wall-clock bound.
Exercise 2 ms reset readiness with coarse ticks (100 Hz can round it to zero),
and measure elapsed time from the real event. Distinguish source policy,
silicon limits, configured budgets and authorized measurements under
[EVIDENCE](EVIDENCE.md).

## Closed work unit

Complete a method-by-method conformance table covering every selected
`esp_eth_phy_t`/base entry, executing owner, complete transaction, callback
lifetime, side effects, errors, notification and worst supported blocking bound.
Close confirmed defects in owning code with focused regressions. Do not create
a second implementation to make tests pass or defer essential running behavior
from earlier tasks into this review.

Audit pause negotiation, partner ability and flow-control notification against
the exact pinned API. Preserve full portable symmetric/asymmetric RX/TX evidence.
The SDK's Boolean peer-pause MAC notification is a narrower projection, not
independent RX/TX resolution. Override base behavior where semantics differ;
document the projection and keep application/MAC enable policy explicit.

Prove notification ordering: unchanged state produces no duplicate event,
down/incomplete negotiation invents no resolved speed/duplex, and callback
failure preserves diagnostic detail without fabricating applied state. Check
polling, stop/start and teardown interleavings against the single owner.
Preserve structured backend detail internally and explain information that the
narrower SDK return type cannot carry; never leak SDK error types into the core.

Calculate the worst complete indirect transaction and aggregate operation bound
from actual controller loops and lock waits, including time quantization.
Exercise exhausted budgets and late terminal callbacks. Late-return detection
establishes a contract violation, not preemption. If a necessary finite bound is
unavailable, retain the exact affected SDK capability gate and constrain claims;
do not replace the controller with a successful mock or advertise a guessed
minimum/tested SDK range. Avoid private fragile APIs without a narrow justified
version contract and real tests.

## Acceptance and checkpoint

Run notification/flow-control tests, callback error injection, start/stop/delete
lifetimes and actual raw/base ownership regressions. Build/link the real P4
fixture, rebuild the portable consumer and record actual SDK/tool versions.
Separate host contract tests, SDK link and physical timing; missing SDK access
remains BLOCKED.

Maintain adapter Doxygen, `docs/esp-idf.md`, timing/ownership tables, README
support claims and CHANGELOG with actual restrictions. Review affected guides
and run documentation plus focused local checks. Update evidence/progress with
commands, counts and unresolved gates, then commit/sync coherent checked units
and the completed task on the recorded branch. Remote CI belongs only to 71.
