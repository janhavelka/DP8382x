# 79 - Review ownership, bounds and evidence independently

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Required inputs are actual core/SDK/CLI/
runner/package implementation, frozen public contracts and registered tests.
Consume [78](78-review-silicon.md)'s relevant findings; unrelated silicon or
physical gates do not prevent independent ownership/evidence review. Freeze
the reviewed revision/dirty patch before assignments.

## Current evidence and ownership

Reopen `../TMP1x2/src/TMP1x2.cpp::track/terminalTransportStatus`, public
`HealthSnapshot` and operation tests; MB85RC actual variant/operation and strict
runner tests; TCA9548A HIL artifact tests; and relevant real SDK/base source.
Inspect current public/source, examples/CLI, runner, test registration, changed
workflow definitions and AGENTS. Record revisions, dirty state, paths and
adopted/adapted/rejected conventions. Workflow source review does not authorize
querying remote run status outside 71.

Delegate finite read-only ownership/state and test/evidence reviews with exact
paths, forbidden mutations and independent findings. Select available models
for difficult deadline/ownership reasoning or scoped evidence inventories, not
fixed versions/counts. The lead reviews findings and owns all shared fixes,
integration, progress and Git; reviewers inspect implementation rather than
trust predecessor completion claims.

## Concrete public-source inputs

Reopen [SDK and controller evidence](../reference/library-sdk.md) and
[EVIDENCE](EVIDENCE.md). Check the final adapter, not just the upstream baseline:
zero-initialized MMD reserved bits, actual whole-transaction arbitration, raw
ioctl/cache reconciliation, real reset-ready intervals, poll quantization and
late effects. Verify each stated time bound's source and actual backend behavior.
Physical timing requires identified measurements; no successful compiled fixture
or source grep can substitute for a waveform or elapsed-time observation.

## Work and acceptance

Trace complete MMD/RMW transactions across core, IDF base, raw access, ioctl,
polling and CLI. Check lock order, cross-owner contention, callback/context
lifetime, aggregate controller/lock deadlines, cleanup budgets, no reentry,
release before waits and teardown. Inspect actual supported SDK APIs and timing
limits; a timeout parameter or shim compile cannot prove bounded native behavior.
Look for duplicate reset/configuration/polling/MAC notifications and stale SDK
mirrors after raw changes. Keep board wiring/policy and framework headers out
of core.

Trace accepted-write/error, partial configuration, external writer/raw
invalidation, reset/cancel/end, terminal tokens/results and unresolved hardware
effects. One destructive collector retains consumed samples through later read
failure. Challenge conflation of transport health, link, trusted configuration
and application readiness; successful cleanup/probe alone does not restore them.
Verify cached no-I/O claims with callback counts/traces and logical operations
versus actual MDIO frames, not grep alone.

Audit tests for circular production-derived oracles, unused suites, zero test
execution, inactive Release assertions, missing boundary injections and wrong
traffic attribution. Inspect CLI malformed-input/no-transfer behavior, bounded
console saturation, runner fragmented/duplicate/reboot/truncated/stale output,
firmware provenance, restoration and artifact failures. Successful serial text
is not packets/hardware timing; mocks and syntax shims are not real SDK links.
Check clean extracted package consumers, source selection/vendor exclusion and
ordinary tests-OFF dependency claims. Preserve one procedure/grammar and remove
speculative layers only with updated consumers/tests.

Findings require severity, source/path, reproducible expected/actual behavior
and closure criterion; close as fixed with evidence, rejected with cited reason,
or OPEN with next dependency. The lead ships focused regressions, Doxygen,
README/CHANGELOG/guides and ledger changes together, with no-change reasons.
Run affected host/tool/docs and actual SDK/package checks for changed inputs.
Record review scope/revision, findings/counts and risks in report, validation
and progress. Keep 71's historical CI SHA and physical gates separate; no remote
dispatch/poll/wait. Check final diff and commit/sync verified units/completion
on the recorded execution branch without empty review commits.
