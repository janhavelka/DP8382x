# 52 - Prove finite soak and interrupted-campaign failure behavior

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [50](50-hil-observation-modes.md) and
[51](51-hil-diagnostic-fixtures.md), using the existing
runner/session/artifacts. This task tests campaign software; physical
qualification is later scoped work.

## Current evidence and ownership

Reopen PCA9555's aggregate destructive-failure/recovery classifier tests,
MB85RC restoration verification and TCA9548A bounded soak/session artifacts.
Inspect current producers, runner implementation and synthetic/compiled
fixture tests together. Compare actual interruption and finalization paths,
not only help/options. Record source revisions, paths and
adopted/adapted/rejected conventions; prior PASS words and configuration files
are not execution evidence.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Implement

Implement finite soak campaigns with explicit positive requested
duration/count, actual completed case count, total deadlines, maximum
output/artifact size and stop conditions. Use the existing owner path.
Validate count-times-duration/output arithmetic before starting and stop at
the first applicable count, duration or fault limit. Repeated authorized soak
cases are explicit campaign work, never hidden retries. Preserve owner
fairness: slow/full console cannot starve operation deadlines or required
cleanup. Report console loss, truncation, all-source MDIO frames, managed
health and chip evidence as distinct fields.

Plan interruption after every mutating step, before restoration, during
restoration and after restoration before result capture. Cover firmware
reboot, serial disappearance, cancellation, stopped run, incomplete record and
artifact finalization failure. An accepted or partially transmitted command
may have changed hardware; later readback/serial success cannot erase the
first failure. Do not send cleanup into a damaged input line or a rebooted
session; apply prompt 49's framing and correlation rules. Classify inability
to restore separately from the primary failure, retaining the unresolved
effects. Do not reconnect, replay, reset or extend a primary deadline
automatically.

Only explicitly admitted cleanup runs, under its own finite budget. Preserve
primary, restoration and evidence-finalization failures independently.
Untested remaining cases stay NOT RUN/BLOCKED. Zero attempts, skipped-only
plans, early cancellation, missing terminal results or failed restoration
cannot become a passing campaign. A command expected to reject passes its test
only with the precise allowed error and unchanged state/frame proof.

Keep raw transcripts and structured summaries bounded and durable through the
existing exclusive-create bundle/manifest implementation. Summaries identify
actual firmware/session, runner, fixture declarations, exact commands and time
domains. Device monotonic values do not establish host timing correlation or
real-time guarantees. Hardware counters or link LEDs alone cannot qualify
electrical behavior, EtherCAT timing or servo safety.

## Acceptance

Execute bounded host soak against the compiled CLI fixture. Test complete
positive runs and every interruption boundary with failure verdicts, artifact
errors and missing/corrupt case evidence. Assert no hidden retry, no unbounded
output, finite cleanup and preserved first failure. Demonstrate zero-case and
skip-only campaigns cannot pass. Actual HIL remains NOT RUN until separately
authorized execution, with missing fixtures visible.

Document budgets/verdicts and update hardware-validation/HIL-runner guides
with interruption, framing-loss and finite cleanup outcomes.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
