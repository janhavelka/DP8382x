# 35 - Implement electrical controls and finite compliance procedures

Read [COMMON](COMMON.md), [ENGINEERING](ENGINEERING.md),
[CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Use [SOURCE_MAP](SOURCE_MAP.md) for current
source locations; historical reports do not replace a fresh audit.

At entry record owned paths, applicable rules, current source revisions/dirty
state, selected names, one MDIO owner, local checks and the recorded execution
branch. Resume that branch. Use disjoint implementation/test work or independent
read-only review when useful; choose available models/effort by task complexity
and record the rationale. The lead owns shared contracts, integration and Git.
Siblings remain read-only.

Prerequisites: [23](23-loopback-modes.md), [33](33-clock-interface-repeater.md), [5](5-silicon-power-interface-ledger.md), [22](22-reconcile-recover.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver documented analog/MAC electrical and timing controls, compliance
stimuli and the resistor-calibration contingency. Keep persistent legal
configuration distinct from finite disruptive test sessions. Reuse existing
configuration, diagnostic engine and restoration; no autonomous calibration/
reset loop or class per register.

Reopen TMP1x2 diagnostic owner/result handling and PCA9555 primary/cleanup,
ambiguous write and detach-refusal tests. Inspect public documentation and actual
registrations. Reconcile SNLS647I electrical/calibration fields and exact
DP83826 recipes in SNLA266B/troubleshooting sources. Applicability must be explicit;
a family compliance guide does not authorize unrelated silicon values.

Enumerate every documented legal analog drive/timing/compatibility setting and
stimulus option with mode/interface/speed requirements, prerequisite hardware
and reset effects. Reserved bits are not tuning controls. Procedure-only
addresses and values remain bound to their exact source-backed recipe; no
arbitrary undocumented write escape. Reuse task 23's correct speed-specific
loopback where a procedure requires it.

Simple persistent controls use whole-profile validation, bounded application and
stable-owned readback. Extend reset/power/recover restoration and pin/interface
dependencies. An admitted resistor-calibration action has a finite sequence,
documented completion evidence, error/effect result and required restoration;
never retry-reset until a measurement looks good.

Disruptive compliance patterns require maintenance admission, finite duration/
count, explicit start/stop and verified restoration even when silicon offers
continuous output. Capture only necessary owned state and handle untrusted
starting settings explicitly. Use the one operation slot, aggregate deadline
and existing separately bounded cleanup. No bus lock across waits. Cancellation
retains accepted output effects; failed stop/restore means the stimulus may
remain active and configuration is untrusted.

Test independent literal values/sequences for all documented options and legal
boundaries; reject reserved, wrong speed/interface and unavailable admission
before I/O. Inject failure/cancel after every mutation, including start accepted
before error, calibration completion, stop and each restore/readback. Assert
exact maximum transfers, token/result isolation, primary/cleanup precedence and
visible remaining stimulus after abandoned cleanup. Test persistent settings
across every applicable reset/power kind.

Compile a bounded maintenance-owner example and write fixture instructions
covering termination, probes, reference conditions and operator-controlled
entry/exit. Source sequences and host tests establish software behavior only.
Electrical compliance, EMC, resistor calibration accuracy and signal integrity
require actual suitable measurements and remain unqualified until then. Missing
fixtures do not excuse missing documented software controls.

## Documentation, verification and checkpoint

Ship accurate public Doxygen and procedure citations with this behavior: units,
legal states, ownership, output validity, I/O effects, deadlines and cleanup.
Update API contracts, affected guides, restoration/feature evidence and the stage
report/progress. Review README, CHANGELOG and relevant example/test/tool guides;
update affected claims and record why others need no change. Future APIs and
physical qualification remain explicitly unimplemented/unverified.

Run registered focused host tests with active Release checks, relevant portable
consumer/source-documentation/link checks, the reference checker and
`git diff --check`. Record actual commands and separate PASS/FAIL/NOT RUN/BLOCKED
for host, SDK, packages and hardware where relevant. Missing source evidence
blocks the affected requirement; it cannot become unsupported silicon or close
a required gate. Continue independent eligible work.

Review the final diff for simple control flow, stale comments, duplicated
procedures and undocumented errors. Commit and sync checked tests/docs/code on
the same execution branch after this task and each coherent verified unit.
The lead fetches, safely integrates, pushes and verifies upstream equality;
report exact sync failures. First-execution remote CI dispatch, polling and
waiting belong only to checkpoint **71**, never this task.
