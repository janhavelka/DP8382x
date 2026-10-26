# 45 - Add link, lifecycle and retained-operation CLI commands

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [44](44-cli-configuration-registers.md), implemented link/event,
reset/recovery and operation-result contracts. Reuse the existing parser,
console, machine schema and single owner.

## Current evidence and ownership

Reopen TMP1x2's `Tmp1x2CliDiagnostics.cpp`/`Tmp1x2CliOutput.cpp`, including
`startDiagnostic`, `tickDiagnostic`, `finishDiagnostic` and retained output.
Compare INA228/TCA9548A admission/result handling and MB85RC's
`tools/hil_runner.py::classify` plus restoration tests. Inspect actual runner
expectations and native front ends. Record revisions/paths and selected
adopted/adapted/rejected behavior, not merely matching help strings.

The lead owns grammar and shared dispatch; disjoint transcript tests or a
read-only result/cleanup review may be delegated after names freeze. Select
available models for the reasoning needed by cancellation, output retention
and side effects. Keep shared headers and Git with the lead.

## Concrete public-source inputs

Use [the actual link/event defects](../reference/library-ti-core.md) and
[EVIDENCE](EVIDENCE.md) in dispatcher tests: first BMSR low must not return
an unassigned success value; MISR1 success followed by MISR2 failure must retain
consumed history. Cached status prints no MDIO frames. Distinguish BMCR reset,
PHYRCR `0x8000` hard reset and `0x4000` restart in both grammar and output;
a successful control call is not a fresh link or completed restoration.

## Closed work unit

Add explicit live link/partner observations, advertised/resolved mode evidence,
event collection and cached projections. Side-effect/latch evidence has one
collector; viewing a consumed sample must not read silicon again. Mark stale,
pending, down, wrong-policy and management-failure observations separately.
Invalid/incomplete negotiation cannot invent speed/duplex or resolved pause.

Expose initialization, reset, restart, reinitialize/recovery and end with
distinct names/effects consistent with the selected public API. CLI aliases
`init/begin` route to the one initialization operation, never duplicate core
facades. Startup performs no implicit reset, scan, initialization or writes.
Disruptive commands require maintenance admission; active application sessions,
busy owner and retained unconsumed results reject conflicting work before I/O.

Implement operation start/poll/status/result/cancel through existing owner
procedures. Cached status does not advance work. An admission or `IN_PROGRESS`
record cannot satisfy completion; retain terminal primary/cleanup results until
the console accepts them. Cancellation/stop cannot undo accepted writes or
prove restoration. Keep residual effects and failed cleanup visible.

Provide finite link watch and stress commands with positive count/duration,
output budget and stop/fault paths. Zero rounds is rejected. Poll only through
the existing owner and release controller ownership between complete atomic
transactions and waits. Distinguish logical operations, all-source MDIO frames,
managed health attempts and chip counters; one MMD operation is several frames.
Never starve mandatory cleanup to print another watch line.

## Acceptance and checkpoint

Execute success, malformed, unsupported, busy, timeout, cancellation and cleanup
failure transcripts against real shared CLI code. Test reset distinctions,
stale link, one collector, retained results, full/slow console and finite stop
behavior with before/after state/frame assertions. Expected rejection is not
automatically a passing runner verdict. No host test proves physical timing.

Ship command Doxygen and CLI/example guides with units, time bounds, operation
lifetime, maintenance admission and truthful success/failure records. Review
README/CHANGELOG and affected docs, run source documentation plus targeted local
checks, record coverage/evidence, and commit/sync coherent verified units and
completion on the same branch. Remote CI belongs only to task 71.
