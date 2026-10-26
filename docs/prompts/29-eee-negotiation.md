# 29 - Implement exact EEE negotiation and status

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

Prerequisites: [18](18-forced-crossover-settings.md), [20](20-event-counter-acquisition.md), [5](5-silicon-power-interface-ledger.md), [28](28-power-states.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Audit all eighteen printed address/value pairs in the
[TI EEE recipe](../reference/library-ti-features.md#interrupts-power-eee-and-wol)
against SNLS647I 8.3.3.1, beginning `001F=8000,203C=0002,04D1=008B`
and ending `0308=0000,04F4=0800,0000=3300`. These are printed facts,
not verified wire tuples. Bypass controls `04D1[0,3]=1` and printed
MMD7 `203C[1]=1` need exact-part addressing evidence; the TI Zephyr
`(0x4000 & devad)` helper is a rejected trace. Account for destructive reset,
`0123` loopback overlap and clock/profile writes. No inspected MCU/Zephyr
feature subset proves a complete EEE setup or disable/restore procedure.

Deliver complete DP83826I EEE capability, advertisement, partner/status and
documented configuration recipe with verified restoration. This task owns
negotiation/configuration behavior; explicit legacy-MAC TX LPI sessions are task
30. Neither a family app note nor a register name proves exact-part support.

Reopen peer configuration/readback and explicit power-transition source/tests:
TMP1x2 `src/TMP1x2Configuration.cpp`, MB85RC sleep/wake procedures and
PCA9555 ambiguous-apply/cleanup tests. Inspect current API/CLI error contracts.
Reconcile SNLS647I EEE fields, bypass polarity and all applicable exact-part
recipes with SNLA328 and pinned TI sources, preserving their respective authority.

Resolve the inventory's printed EEE addresses versus verified wire offsets
before freezing constants or issuing access. Do not strip address digits,
assume a neighboring part's MMD mapping or turn a questionable source example
into a reserved-write exception. Record independent literal selector/address/
data traces and provenance for every procedure-only access. Missing essential
mapping evidence blocks that procedure and remains required work; other
documented controls and unrelated tasks can proceed.

Implement every legal capability/advertisement/status setting and the complete
source-required setup/disable sequence. Separate local support, requested policy,
partner advertisement, negotiated result and current evidence with explicit
validity. Validate incompatible link/interface/power choices before any transfer.
MAC latency/flow policy stays with the application; no implied PHY buffering
or unsolicited LPI command belongs to this stage.

Apply and verify stable owned bits once using the common configuration engine.
Preserve bypass/control polarity, reserved fields, command neutrality and
10BASE-Te/other profile requirements. Keep opt-in defaults; initialization/recovery
replays only explicitly requested owned EEE settings. Extend each applicable
reset and power transition's restoration order, with source-defined minimum
waits and readback. Partial apply, accepted-write timeout and failed disable/
restore retain uncertainty and independent cleanup status.

Test independent literal EEE values/wire traces, each capability/partner
combination and legal/invalid advertisement. Exercise bypass polarity,
negotiation incomplete, stale status, unsupported zero-I/O rejection and absence
of optional default activation. Fail every write/readback/disable/restore phase,
cancel at effects and assert exact transaction/deadline bounds, retained primary
failure and no false negotiated-success publication.

Compile a configuration/status example and document MAC coordination needed
before future LPI operation. Expose source gaps precisely; host sequence tests
do not establish physical EEE interoperability, wake latency or energy savings.

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
