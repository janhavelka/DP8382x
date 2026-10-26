# 21 - Build finite observation, fault and diagnostic suites

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-20.

Before the audit, apply COMMON's short rule reminder: current sibling sources,
selected conventions, owned paths, source/general documentation, focused local
checks and same-branch commit/sync. Choose subagent models by task complexity.

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

## Concrete campaign plan and fixture evidence

Apply [CONVENTIONS](CONVENTIONS.md). Inspect
`PCA9555/tools/test_run_i2c_hil_parser.py` cases
`test_fault_guard_snapshot_invariant`,
`test_expected_rejection_classification_is_fail_closed` and
`test_aggregate_destructive_failure_runs_recovery_before_stopping`, plus the
actual `tools/run_i2c_hil.py` implementation. Reopen
`MB85RC/tools/test_hil_runner.py::test_diagnostic_prompt_requires_all_counters_and_restore_proof`
and the classifier it exercises. Reuse evidence discipline; an I2C address scan,
GPIO loopback or FRAM scratch restoration is not a PHY/link fixture.

Extend the existing runner with a small explicit suite/case table containing
stable semantic case ID, required exact-part capabilities, fixture description,
mutations, deadline/output limits, commands, expected evidence and cleanup.
Choose a JSON file only if both runner and guide checker consume it; otherwise a
readable typed Python table is enough. Do not introduce a hardware automation
service or fake fixture interface to make unavailable measurements pass.

| Campaign | Required distinctions and evidence |
| --- | --- |
| Passive identity/observation | Exact reported part/mode, known board declaration, valid management results; cached views show zero new frames |
| Configuration/reset | Saved owned profile, admitted action, exact reset kind, restoration/readback, configuration trust after partial failure |
| Generic modes | Partner supporting each legal 10/100 and half/full mode; advertisement/partner/resolved evidence kept separate; 10BASE-Te procedure evidence |
| Link faults | Cable unplug/replug and partner reset with named manual steps or configured controller; link down alone leaves transport healthy |
| Diagnostics | Documented loopback/BIST/TDR fixture and stimulus, completion validity, primary/cleanup results and verified restored profile |
| Opt-in controls | IRQ wiring/event collector, LED/clock measurement, wake source or EEE-capable partner as applicable; no electrical conclusions from register text alone |
| Finite soak | Requested/actual duration, complete case count, bounded aggregate output, console loss and all-source frame/health evidence |

For each case describe starting owner/application state, exact command sequence,
expected typed records, finite waits, intervention steps and stop conditions.
Record partner model/configuration, cable/termination and instruments only when
actually supplied. An operator claim or fixture setting is provenance, not a
measured result. Unsupported silicon, missing fixture and not-yet-implemented
software use distinct dispositions. Board-limited fixture scope cannot remove
the required generic software cases.

Plan interruption after every mutating step, before restoration, during
restoration and after restoration but before result capture. Preserve primary
failure and uncertain state even if serial reconnect or readback later works;
the runner must not reconnect/replay automatically. Keep the remaining required
cases visible as NOT RUN/BLOCKED. A plan containing only skips, zero attempts or
early cancellation has no passing campaign verdict.

Test suite selection and verdicts with complete positive transcripts and one
missing/corrupt evidence item per case. Run host CLI fixture cases to prove
actual grammar and admission behavior. Keep fixture execution for authorized
stage 30 work; this stage does not operate hardware merely to fill evidence rows.

Update `docs/hardware-validation.md`, `docs/hil-runner.md`, per-example fixture
READMEs, root README and CHANGELOG. Include safe start/stop, wiring prerequisites,
manual observations and artifact examples labeled offline. Python docstrings
must explain each suite's limits and verdicts; update Doxygen for any changed
firmware producer. A read-only reviewer can audit false-PASS and restoration
paths independently; choose agents by fixture/protocol complexity, with shared
case IDs and schema owned by the lead.

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
