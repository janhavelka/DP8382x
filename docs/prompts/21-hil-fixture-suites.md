# 21 - Build finite observation, fault and diagnostic suites

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-20.

## Fresh audit and owned work

Re-audit TCA9548A and PCA9555 physical-fixture suites, MB85RC restore verification and
peer false-PASS regression tests. Borrow verdict discipline, not I2C electrical
assumptions.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Add explicit runner suites for identity/observation, configuration, lifecycle/reset,
generic modes, link flaps, diagnostics and bounded soak. Separate passive observation,
link-disrupting operations and manual/instrumented fixtures. Select only capabilities
supported by the actual firmware/device. Record unavailable requirements as
not-run/blocked instead of dropping them from the summary.

Create docs/hardware-validation.md with preconditions, exact steps, expected evidence
and cleanup for cold boot, reset kinds, 100/full advertisement, generic 10/100 and legal
duplex modes on a suitable partner, absent/wrong partner, unplug/replug, peer reset and
recover. Distinguish management loss from cable loss.

Cover loopback/BIST/cable tests with known fixtures and verified restoration. Add
optional IRQ, LED/clock/power/wake/EEE tests only for accepted capabilities. Do not
infer pin waveform, cable length or electrical compliance from serial output. Keep power
delivery excluded and servo actuation absent.

Define interrupted campaign handling, cleanup failure, firmware reboot and
incomplete-run verdicts. Test host-driven bounded soak without dumping unbounded logs or
starving owner work. Preserve primary failure even if cleanup succeeds. Provide
user-readable wiring/fixture instructions and machine-readable case requirements.

## Acceptance and handoff

Unit-test every suite's pass/fail/inconclusive classification with positive and negative
transcripts, including zero-executed-case and skipped-only campaigns. Exercise commands
against the host CLI fixture. Actual HIL remains stage 30; no fake fixture or dry-run
upgrades qualification.

Expected output: Ready-to-run PHY campaigns and explicit physical fixture requirements.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
