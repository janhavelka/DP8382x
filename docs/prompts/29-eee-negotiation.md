# 29 - Implement exact EEE negotiation and status

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [18](18-forced-crossover-settings.md),
[20](20-event-counter-acquisition.md), [5](5-silicon-power-interface-ledger.md),
[28](28-power-states.md). Verify the concrete inputs below; retain unrelated open gates
under COMMON.

## Scope, current evidence and work

Deliver complete DP83826I EEE capability, advertisement, partner/status and
documented configuration recipe with verified restoration. This task owns
negotiation/configuration behavior; explicit legacy-MAC TX LPI sessions are task
30. Neither a family app note nor a register name proves exact-part support.

Audit all eighteen printed address/value pairs in the
[TI EEE recipe](../reference/library-ti-features.md#interrupts-power-eee-and-wol)
against SNLS647I 8.3.3.1, beginning `001F=8000,203C=0002,04D1=008B`
and ending `0308=0000,04F4=0800,0000=3300`. These are printed facts,
not verified wire tuples. Bypass controls `04D1[0,3]=1` and printed
MMD7 `203C[1]=1` need exact-part addressing evidence. Derive full address-mode
and data-mode words independently and test all reserved bits remain zero. Account for destructive reset,
`0123` loopback overlap and clock/profile writes. Establish complete setup,
disable and restore from source-backed procedures and independent tests.

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
source-required setup/disable sequence. Reconcile next-page exchange with task
17's existing owner so host page control and hardware EEE exchange cannot compete.
Plan reset-bearing setup once within an acyclic configuration procedure; replay
owned settings in documented order without restarting the EEE recipe recursively. Separate local support, requested policy,
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
restore retain uncertainty and independent cleanup status. A recipe writing
undocumented readable state needs a sourced reset/reapply exit, not a fabricated
inverse sequence or readback proof for unknown bits.

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
