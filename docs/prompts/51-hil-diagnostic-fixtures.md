# 51 - Build diagnostic and opt-in control fixture campaigns

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites:
[50](50-hil-observation-modes.md), [46](46-cli-diagnostics-controls.md) and the
implemented exact-part feature procedures. Physical execution remains 72–73.

## Current evidence and ownership

Reopen MB85RC's
`tools/test_hil_runner.py::test_diagnostic_prompt_requires_all_counters_and_restore_proof`
and the actual classifier, alongside PCA9555/TCA9548A destructive-fixture
recovery tests and current CLI producers. Recheck exact TI diagnostic, IRQ,
power/wake/EEE and pin-control sections. Record revisions, source/test paths,
fixture restrictions and adopted/adapted/rejected conventions. FRAM restoration
and GPIO loopback are evidence patterns, not PHY electrical fixtures.

The lead owns case identities, feature prerequisites and restoration verdicts.
Delegate disjoint transcript families or read-only false-PASS/fixture reviews,
with frozen schema and forbidden shared paths. Choose available models for
silicon/protocol complexity; shared coverage/contracts and Git remain lead-owned.

## Closed work unit

Extend the existing case table and runner with explicit loopback, BIST/generator/
checker, manual/automatic cable diagnostics and accepted IRQ, LED/GPIO,
clock/interface, power, WoL and EEE/LPI control suites. Reconcile each required
programmable feature with a fixture case or a precise reason the measurement
needs external equipment; runtime opt-in does not remove software coverage.
Select capabilities from actual firmware/device evidence, not assumed family
similarity or first-board wiring.

Each case specifies starting owner/application state, exact typed commands,
admission, stimulus, finite waits, required result validity, primary and cleanup
statuses, restoration/readback and stop conditions. Save/restore only owned
configuration. A diagnostic failure plus cleanup success stays failed; failed
restoration and uncertain effects remain additionally visible. Test active-session
rejection without transfers and retained results under output pressure.

Name required known cable/termination/partner arrangements and instruments.
IRQ tests require actual wiring and the single event collector; LED/clock
waveforms, wake source and EEE partner requirements remain explicit. Register or
serial text alone cannot prove pin waveform, cable length, electrical compliance
or wake behavior. An operator fixture declaration is provenance rather than a
measurement. Do not invent a fake fixture interface to manufacture PASS.

Document exact wiring, manual observations, safe start/stop, restoration and
failure steps in `docs/hardware-validation.md` and example fixture READMEs.
Power delivery/PoE, automatic flashing/reset and servo motion stay outside scope.
Absent fixtures, unfinished software and source-proved absent silicon use
different dispositions and remain visible in run summaries.

## Acceptance and checkpoint

Unit-test every suite's positive and negative classifications with complete
transcripts and one missing/corrupt evidence item per case. Exercise command
grammar, capability/admission and cleanup paths against the compiled host CLI.
Test timeout, cancellation, serial reboot/loss and failed readback; dry-run or
skips cannot qualify hardware. Keep actual measurements NOT RUN/BLOCKED until
authorized physical work.

Ship Python case/protocol docstrings and update hardware/HIL/CLI guides and
example READMEs, root README and CHANGELOG. Run affected source-documentation
checks and focused local suites, record evidence/gates, and commit/sync checked
units and completion on the recorded branch. Remote CI remains exclusive to 71.
