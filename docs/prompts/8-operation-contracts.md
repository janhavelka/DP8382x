# 8 - Freeze cooperative operation, time and cleanup contracts

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[ENGINEERING](ENGINEERING.md). Prerequisites:
[7](7-value-transport-contracts.md)'s values/direct calls and
[6](6-framework-ownership.md)'s complete transaction owner.
Record scope, rules, fresh source review, documentation/checks and checkpoint.

Use the frozen [value/direct-call contract](../api-contracts.md), including its
absolute microsecond deadline, terminal batch/result shape and non-movable
owners. Resolve the explicit clock-fault cleanup dependency: a detected backward
clock blocks timed I/O until repaired/rebound, while owed cleanup/results prevent
end/rebind. Define owner-assisted quiescence, retained cleanup evidence and
acknowledgement without erasing effects or pretending cleanup ran on invalid time.

## Complete outcome

Complete `docs/api-contracts.md` with one precise operation state machine,
time/result/cleanup rules and executable-design scenarios. It must be possible
to implement initialization/reset next without inventing another lifecycle.
This task is a reviewed contract; working operation code arrives with its first
real consumer, not as an unused engine or successful placeholder facade.

## Concrete public-source inputs

Use [SDK timing observations](../reference/library-sdk.md) and
[EVIDENCE](EVIDENCE.md) to define preflight and completion. Zero budget performs
no I/O; a budget below the selected poll quantum must have an explicit outcome.
Distinguish accepted command, protocol completion, current link and restored
configuration. Poll expiry returns the promised timeout instead of completion.
Include tick rounding, late callback effects and actual readiness intervals.

## Work and current precedents

Trace TMP1x2 `startInitialize`, `startConfigure`, `poll`, `takeResult`, `cancel`
and teardown, PCA9555 `pollOperation`, and MB85RC retained effects/results to
implementation and tests. Review FieldCore Ina228Module owner admission and
cached publication. Record exact paths/revisions, contradictions and selected
adaptations; do not inherit EEPROM waits or an offline admission latch.

Specify `startInitialize`, `startReset`, `startConfigure`, `startRecover`,
`pollOperation`, `getOperationSnapshot`, `takeResult` and `cancel` with the
chosen values. Feature-specific methods are added only alongside real behavior.
Use one fixed active-operation slot, explicit operation kind and fixed-size
result. Allocate monotonic nonzero `OperationToken` values with an invalid
sentinel and explicit exhaustion; no registry, scheduler or stale token reuse.

Define admission, progress, terminal publication, consumption and retention
across cancel/end/rebind. BUSY distinguishes active work from unconsumed result;
TOKEN_MISMATCH and RESULT_NOT_AVAILABLE preserve output and retained evidence.
No synchronous helper silently duplicates a separate chip procedure. Direct
register calls remain direct; only actual waits or retained cleanup require
cooperative state.

Specify time domain, units, wrap/overflow intervals, monotonicity assumptions,
zero budget, remaining aggregate allowance, completion-time classification and
late callback effects. A synchronous terminal backend cannot be preempted;
IN_PROGRESS from it is a contract failure, not deferred hardware work. Include
controller/lock time and quantization without holding locks across owner polls.
Specify how time is refreshed after a synchronous callback. A pre-write timestamp
cannot start a post-write minimum wait: use a completion timestamp or an upper
bound on callback completion, then add the full minimum interval. Distinguish a
bounded poll call from bounded operation completion when the caller stops polling
or supplies a frozen clock; expose incomplete effects instead of promising an
unconditional wall-clock bound. Test callback duration across a tick boundary.

Cancellation and local `end` are bus-silent. Define how subsequent authorized
owner polling performs cleanup, with a reserved or separately admitted finite
budget distinct from the primary deadline. Preserve primary status, cleanup
status, accepted/uncertain hardware effects and restoration evidence. If teardown
prevents cleanup, retain or return that fact; never erase it or imply that
cancel undid a write. Hardware reset/power-down are separate explicit operations.

Specify invalidation after raw/external mutation, partial configuration and
reset. Restoration uses validated owned desired settings and verifies stable
owned bits to re-establish trust; it cannot require hardware to be trusted
before recovery. Define persistent desired settings separately from temporary
session overrides. Capture every touched documented field or use a source-backed
reset/reapply route for procedure-only fields that cannot safely be read. Build
an acyclic restore order: a reset within EEE/compliance setup cannot recursively
invoke that same complete recipe, and cleanup cannot replay a temporary stimulus. Consumed event samples remain retained even if aggregate
operation output is unpublished. Application session admission and physical
controller locking remain distinct responsibilities.

## Review, documentation and acceptance

Walk successful and failed initialization, unplugged cable, dead MDIO,
wrong identity, zero/wrapping/frozen clock, reset in flight, partial write,
cancellation before/after mutation, cleanup failure, late result, result
backpressure and teardown/rebind. Each has one status/state/effect outcome and
finite next action. Include plain C++ owner and SDK callback design examples
using exact signatures with error and retained-result paths.

Use an independent stronger-reasoning reviewer for lifetime/deadline cases;
the lead resolves shared API decisions and records model/path ownership.
Maintain Doxygen-style declaration contracts and non-obvious procedure notes;
review architecture, example plans, README/CHANGELOG and feature map now.
Run local reference/link/diff checks and update report/progress with unresolved
facts explicitly gated. Commit and sync this completed contract and coherent
checked units on the recorded branch. Remote CI remains prompt 71 only.
