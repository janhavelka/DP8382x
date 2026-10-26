# 52 - Prove finite soak and interrupted-campaign failure behavior

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [50](50-hil-observation-modes.md) and
[51](51-hil-diagnostic-fixtures.md), using the existing runner/session/artifacts.
This task tests campaign software; physical qualification is later scoped work.

## Current evidence and ownership

Reopen PCA9555's aggregate destructive-failure/recovery classifier tests,
MB85RC restoration verification and TCA9548A bounded soak/session artifacts.
Inspect current producers, runner implementation and synthetic/compiled fixture
tests together. Compare actual interruption and finalization paths, not only
help/options. Record source revisions, paths and adopted/adapted/rejected
conventions; prior PASS words and configuration files are not execution evidence.

The lead owns aggregate verdicts and cleanup policy. Delegate disjoint
interruption transcripts or read-only durability/false-PASS review after schema
and case IDs freeze. Select available models by lifetime/failure reasoning;
shared contracts, progress and Git have one lead writer.

## Closed work unit

Implement finite soak campaigns with explicit positive requested duration/count,
actual completed case count, total deadlines, maximum output/artifact size and
stop conditions. Use the existing owner path, never another poller or background
retry loop. Preserve owner fairness: slow/full console cannot starve operation
deadlines or required cleanup. Report console loss, truncation, all-source MDIO
frames, managed health and chip evidence as distinct fields.

Plan interruption after every mutating step, before restoration, during
restoration and after restoration before result capture. Cover firmware reboot,
serial disappearance, cancellation, stopped run, incomplete record and artifact
finalization failure. An accepted write may have changed hardware; later
readback/serial success cannot erase uncertainty or first failure. Do not
reconnect, replay, reset or extend a primary deadline automatically.

Only explicitly admitted cleanup runs, under its own finite budget. Preserve
primary, restoration and evidence-finalization failures independently. Untested
remaining cases stay NOT RUN/BLOCKED. Zero attempts, skipped-only plans, early
cancellation, missing terminal results or failed restoration cannot become a
passing campaign. A command expected to reject passes its test only with the
precise allowed error and unchanged state/frame proof.

Keep raw transcripts and structured summaries bounded and durable through the
existing exclusive-create bundle/manifest implementation. Summaries identify
actual firmware/session, runner, fixture declarations, exact commands and time
domains. Device monotonic values do not establish host timing correlation or
real-time guarantees. Hardware counters or link LEDs alone cannot qualify
electrical behavior, EtherCAT timing or servo safety.

## Acceptance and checkpoint

Execute bounded host soak against the compiled CLI fixture. Test complete
positive runs and every interruption boundary with failure verdicts, artifact
errors and missing/corrupt case evidence. Assert no hidden retry, no unbounded
output, finite cleanup and preserved first failure. Demonstrate zero-case and
skip-only campaigns cannot pass. Actual HIL remains NOT RUN until separately
authorized execution, with missing fixtures visible.

Ship docstrings for budgets/verdicts and update hardware-validation/HIL-runner
guides, example fixture instructions, README and CHANGELOG. Review affected
source docs, run focused local tests/documentation, record counts and gates,
and commit/sync verified units and completion on the same execution branch.
Only task 71 dispatches or checks remote CI in this set.
