# 45 - Add link, lifecycle and retained-operation CLI commands

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [44](44-cli-configuration-registers.md), implemented
link/event, reset/recovery and operation-result contracts. Reuse the existing
parser, console, machine schema and single owner.

## Current evidence and ownership

Reopen TMP1x2's `Tmp1x2CliDiagnostics.cpp`/`Tmp1x2CliOutput.cpp`, including
`startDiagnostic`, `tickDiagnostic`, `finishDiagnostic` and retained output.
Compare INA228/TCA9548A admission/result handling and MB85RC's
`tools/hil_runner.py::classify` plus restoration tests. Inspect actual runner
expectations and native front ends. Record revisions/paths and selected
adopted/adapted/rejected behavior, not merely matching help strings.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Use [link/event/reset evidence](../reference/library-ti-core.md) and
[EVIDENCE](EVIDENCE.md). A first BMSR low produces defined latch/current
evidence; MISR1 consumed successfully before MISR2 failure remains retained.
Cached status performs no MDIO. Grammar and results distinguish BMCR reset,
PHYRCR `0x8000` hard reset and `0x4000` restart, including restoration and
fresh-link validity.

## Implement

Add explicit live link/partner observations, advertised/resolved mode
evidence, event collection and cached projections. Side-effect/latch evidence
has one collector; viewing a consumed sample must not read silicon again. Mark
stale, pending, down, wrong-policy and management-failure observations
separately. Invalid/incomplete negotiation cannot invent speed/duplex or
resolved pause.

Expose initialization, reset, restart, reinitialize/recovery and end with
distinct names/effects consistent with the selected public API. In an attached
IDF instance, local core end cannot free a context still used by SDK polling;
route release through the application's quiesce/teardown contract. CLI aliases
`init/begin` route to the one initialization operation, never duplicate core
facades. CLI attachment performs no implicit reset, scan, initialization or
writes. Disruptive commands require maintenance admission; active application
sessions, busy owner and retained unconsumed results reject conflicting work
before I/O. Result retrieval and cancellation remain available for the active
operation; do not gate the only way to finish or release it behind
new-operation admission.

Implement operation start/poll/status/result/cancel through existing owner
procedures. Cached status does not advance work. An admission or `IN_PROGRESS`
record cannot satisfy completion; retain terminal primary/cleanup results
until the console accepts the complete record. Distinguish the request ID of a
status, result or cancel query from the original operation ID; progress
queries do not produce a second terminal completion for the original
operation. If automatic result output is chosen, explicit result queries
return that documented cached result rather than consuming a different owner
result. Cancellation/stop cannot undo accepted writes or prove restoration.
Keep residual effects and failed cleanup visible.

Provide finite link watch and stress commands with positive count/duration,
output budget and stop/fault paths. Zero rounds is rejected. Poll only through
the existing owner and release controller ownership between complete atomic
transactions and waits. Distinguish logical operations, all-source MDIO
frames, managed health attempts and chip counters; one MMD operation is
several frames. Never starve mandatory cleanup to print another watch line.

## Acceptance

Execute success, malformed, unsupported, busy, timeout, cancellation and
cleanup failure transcripts against real shared CLI code. Test reset
distinctions, stale link, one collector, retained results, full/slow console
and finite stop behavior with before/after state/frame assertions. Expected
rejection is not automatically a passing runner verdict. No host test proves
physical timing.

Document command units/bounds, owner/result lifetime, cancellation and
retained output. Update CLI/example guides and coverage with truthful terminal
records.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
