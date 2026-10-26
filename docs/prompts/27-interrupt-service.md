# 27 - Implement bounded interrupt configuration and service

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [20](20-event-counter-acquisition.md), [22](22-reconcile-recover.md),
[5](5-silicon-power-interface-ledger.md). Verify the concrete inputs below; retain
unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver all documented interrupt controls and deferred bounded service:
global/output enable, polarity, software test, every MISR1/MISR2 source and
interrupt pin routing/mux constraints. Reuse one collector for polling and IRQ;
LED/GPIO tasks later extend the same pin-conflict rules rather than override them.

Use the full [TI interrupt maps](../reference/library-ti-features.md#interrupts-power-eee-and-wol),
rechecking SNLS647I tables 8-35..37/91. PHYSCR `0x0011[2:0]`
is test/global-enable/output-enable; MISR1/2 at `0x0012/13` pair
RC events `[15:8]` with enables `[7:0]`. Link enable `0x0020`
maps to sampled event `0x2000`. RXFS `0x04A1[12]` selects the shared
polarity/WoL source in MISR2. Implement both interrupt-source maps and retain each consumed read-clear word.
Test every source, consumed events after later failure and explicit test-IRQ stop.

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
condition distinct. Retain the selector that identifies a multiplexed event at
acquisition time. Drain/preserve pending polarity/WoL evidence before a selector
change; events crossing an uncertain change remain ambiguously attributed rather
than relabelled using the new setting. Coordinate WoL's automatic selector change
with task 31 and preserve a single pin/source owner. A software-test command proves its documented register
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

Update public Doxygen, procedure citations, affected API/user/test/tool guides,
coverage, stage report and progress with this behavior. Review README/CHANGELOG
and record relevant no-change reasons. Run the focused registered host tests
above with active Release checks, affected portable-consumer and Doxygen checks,
`python tools/verify_references.py` and `git diff --check`.

Apply COMMON's final maintainability review and evidence reporting. Keep source
gaps, SDK/package results and physical qualification separate. The lead commits
and syncs this complete task and coherent verified units on the same execution
branch, verifies upstream equality, and reports any sync failure. First-set
remote CI runs and status checks belong only to prompt 71.
