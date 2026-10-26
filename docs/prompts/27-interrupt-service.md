# 27 - Implement bounded interrupt configuration and service

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

Prerequisites: [20](20-event-counter-acquisition.md), [22](22-reconcile-recover.md), [5](5-silicon-power-interface-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver all documented interrupt controls and deferred bounded service:
global/output enable, polarity, software test, every MISR1/MISR2 source and
interrupt pin routing/mux constraints. Reuse one collector for polling and IRQ;
LED/GPIO tasks later extend the same pin-conflict rules rather than override them.

Reopen INA228 `DiagAlert`/`readDiagAlert`, PCA9555 deferred input
and interrupt cleanup tests, and current LDC1614 deferred interrupt source/tests.
Inspect SCD41/RV3032-C7 explicit IRQ commands when adopting their conventions;
record actual paths/symbols, not repository-name guesses. Recheck SNLS647I
interrupt configuration, MISR read-clear, pin mux and software test tables.

Define complete typed settings and source-backed validation before I/O.
Mask/output changes must preserve pending evidence and apply command-neutral
composition. Capture consumed status before changing mixed fields. Apply/verify
stable owned bits, record partial write uncertainty and extend reset/recovery
restoration. Interrupt enable is opt-in, not a hidden initialization side effect.

The ISR only signals the application owner; it performs no MDIO, logging,
allocation or unbounded queue work. Owner service has an explicit maximum
transaction/event-sample budget. Define events arriving during collection and
retention for later service without an unlimited drain loop. Polling and IRQ
cannot independently consume MISR status. Repeated cached views return retained
evidence without further reads or duplicated notifications.

Keep configured mask, physical output, consumed event samples and current
condition distinct. A software-test command proves its documented register
behavior only; it does not prove an unwired IRQ reached an MCU. Read-clear
failures retain per-source evidence/history-loss flags from task 20. A successful
later read cannot recover a lost sample. Pin conflicts and unsupported
part/interface choices reject before transfers.

Test independent literal masks/encodings, all sources and simultaneous events,
global/source disable combinations, mux/polarity and software test. Inject
events between each service read and failures after destructive effects.
Exercise mask changes while evidence is pending, collector contention, partial
validity, bounded repeated service and cached readers. Fail each apply/readback/
restore phase and verify accepted effects plus primary/cleanup status.

Compile a deferred-owner example without pretending the initial board has a
connected IRQ. Document polling equivalence, application ISR/lifetime ownership,
source acknowledgement semantics and physical fixture limits. Pin electrical
qualification is separate from the host tests and register-level software test.

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
