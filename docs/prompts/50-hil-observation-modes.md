# 50 - Build finite observation, configuration and link-mode campaigns

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites:
[49](49-runner-serial-session.md) and the matching working CLI/feature routes.
This task creates/tests campaigns; physical execution belongs to tasks 72–73.

## Current evidence and ownership

Reopen TCA9548A/PCA9555 fixture suites and actual parser/classifier source.
Inspect PCA9555's `test_fault_guard_snapshot_invariant`,
`test_expected_rejection_classification_is_fail_closed` and
`test_aggregate_destructive_failure_runs_recovery_before_stopping`; compare
MB85RC restoration proof and false-PASS tests. Read current producer CLI code,
runner, registration and agent rules. Record revisions/paths and adopted/adapted/
rejected behavior; an I2C scan or GPIO fixture cannot establish PHY modes.

The lead owns stable case IDs, schema and aggregate verdicts. Delegate disjoint
transcripts or read-only fixture requirement review with frozen contracts.
Choose available models by fixture/protocol complexity, preserving lead ownership
of shared source, coverage, progress and Git.

## Closed work unit

Extend the one runner with a small explicit typed suite/case table: semantic
ID, exact-part capabilities, starting owner/application state, fixture,
mutations, commands, deadlines/output bounds, expected typed evidence and cleanup.
A JSON file is justified only when runner and guide checker both consume it.
Do not introduce a fake fixture service or another hardware automation layer.

Deliver passive identity/observation, owned configuration/reset, generic legal
mode and link-fault campaigns. Require known board declarations and reported
part/mode; passive snapshots show zero frame delta. Configuration cases save the
owned profile, admit exact reset/action kinds and require restoration/readback
with truthful trust after partial failure. Distinguish advertisement, partner
ability, resolved speed/duplex and directional pause.

Cover cold boot, applicable reset/restart, 100/full-only advertisement, legal
10/100 and half/full on a suitable partner, 10BASE-Te procedure evidence,
absent/wrong partner, unplug/replug, peer reset and bounded recovery. Cable-down
alone cannot become management failure. Manual steps or explicitly configured
fixture controllers identify interventions; never infer cable/switch state or
record unsupplied partner/cable/instrument details as facts.

Create `docs/hardware-validation.md` with exact preconditions, wiring, command
sequence, waits, observations, stop and cleanup for every case. Board declarations
are provenance, not measured results. Source-proved unsupported silicon, absent
fixture and incomplete software use distinct dispositions; generic required
software cases survive first-board fixture limits. Missing required evidence
must remain NOT RUN/BLOCKED, never silently disappear from summaries.

## Acceptance and checkpoint

Test suite selection and every case with complete positive transcripts plus
one missing/corrupt required evidence item. Exercise real commands/admission
against the compiled CLI fixture. Test wrong firmware, unavailable partner,
partial restoration and zero-case/skip-only aggregate verdicts. No fixture is
operated merely to fill rows; offline/dry-run tests cannot upgrade qualification.

Ship case docstrings, hardware-validation/HIL-runner guides and per-example
fixture instructions with clearly offline artifact examples. Review README,
CHANGELOG and affected source docs; run targeted local tests/documentation,
record counts/gates, and commit/sync coherent checked units and completion on
the same execution branch. Remote CI remains task 71 only.
