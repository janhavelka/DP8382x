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
runner, registration and agent rules. Record revisions/paths and
adopted/adapted/ rejected behavior; an I2C scan or GPIO fixture cannot
establish PHY modes.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Implement

Extend the one runner with a small explicit typed suite/case table: semantic
ID, exact-part capabilities, starting owner/application state, fixture,
mutations, commands, deadlines/output bounds, expected typed evidence and
cleanup. A JSON file is justified only when runner and guide checker both
consume it. Do not introduce a fake fixture service or another hardware
automation layer.

Deliver passive identity/observation, owned configuration/reset, generic legal
mode and link-fault campaigns. Require known board declarations and reported
part/mode; passive snapshots show zero command-attributed frame delta. Quiesce
unrelated polling or use owner accounting when asserting whole-bus deltas;
concurrent framework polling is not evidence of a passive-command violation.
Configuration cases require a known restorable owned profile before mutation.
Save the desired profile and independently valid observed/trust evidence;
admit exact reset/action kinds and require restoration/readback with truthful
trust after partial failure. Distinguish advertisement, partner ability,
resolved speed/duplex and directional pause.

Cover cold boot, applicable reset/restart, 100/full-only advertisement, legal
10/100 and half/full on a suitable partner, 10BASE-Te procedure evidence,
absent/wrong partner, unplug/replug, peer reset and bounded recovery.
Cable-down alone cannot become management failure. Manual steps or explicitly
configured fixture controllers identify interventions. Manual acknowledgements
have finite deadlines and are labeled declarations; never infer cable/switch
state or record unsupplied partner/cable/instrument details as facts.

Create `docs/hardware-validation.md` with exact preconditions, wiring, command
sequence, waits, observations, stop and cleanup for every case. Board
declarations are provenance, not measured results. Source-proved unsupported
silicon, absent fixture and incomplete software use distinct dispositions;
generic required software cases survive first-board fixture limits. Missing
required evidence must remain NOT RUN/BLOCKED, never silently disappear from
summaries.

## Acceptance

Test suite selection and every case with complete positive transcripts plus
one missing/corrupt required evidence item. Exercise real commands/admission
against the compiled CLI fixture. Test wrong firmware, unavailable partner,
partial restoration and zero-case/skip-only aggregate verdicts. No fixture is
operated merely to fill rows; offline/dry-run tests cannot upgrade
qualification.

Document cases and `docs/hardware-validation.md`, HIL-runner guidance and
per-example fixture instructions. Label example artifacts as offline evidence.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
