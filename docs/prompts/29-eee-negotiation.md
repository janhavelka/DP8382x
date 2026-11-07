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

Audit all eighteen SNLS647I8.3.3.1 recipe pairs, beginning
`001F=8000,203C=0002,04D1=008B` and ending
`0308=0000,04F4=0800,0000=3300`. DEVAD3/7 is established by8.3.11.1.
Adopt `(3,0001),(3,0014),(3,0016),(7,003C),(7,003D)` as source-implied,
not physically proven, in one table. Record the SNLS638C low-offset prose,
SNLS505H worked trace and SNLS603D notation analogy, plus AI/AE and SNLA328
full-address counter-readings. Do not route these through vendor31 aliases.
Literal selection vectors include0003/0014/4003 and0007/003C/4007.

Reopen peer configuration/readback and explicit power-transition source/tests:
TMP1x2 `src/TMP1x2Configuration.cpp`, MB85RC sleep/wake procedures and
PCA9555 ambiguous-apply/cleanup tests. Inspect current API/CLI error contracts.
Reconcile SNLS647I EEE fields, bypass polarity and all applicable exact-part
recipes with SNLA328 and pinned TI sources, preserving their respective authority.

Before the recipe's HARD write, read(3,0014) and require the whole word0002;
otherwise stop with zero writes. After HARD and its OP-14 floor, write(7,003C)=0002
as row2 and require whole-word0002 readback before the other16 rows. Failure
leaves a hard-reset PHY and OP-9/13 obligations. Keep both fail-closed checks until
Q1 qualifies the mapping. Read-only raw admission is exactly (3,0014), (7,003C),
(7,003D); reject all other3/7 offsets and all raw writes. Collector owns latched
3.1 and read-clear 3.22. Record independent literal traces for each procedure-only
write; unrelated tasks proceed while physical qualification is pending.

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

Both `eee_wire` and `eee_control_semantics` retain their IDs with physical-check
pending status. Adopt cross-part bits 3.1[11:8],3.20[1],7.60[1],7.61[1] and
read-clear 3.22 as source-implied under Q1; the recipe itself remains opaque full
words. For04D1 admit only exact-part bits 0/3 from8.3.3.2 and12 from8.3.4,
with masked readback. Disable is HARD plus replay, never an invented inverse.
The sourced0308=0000 row is not blocked by `pin_fields`; instead reject before
I/O if an owned clock/LED route conflicts. Preserve0123 loopback overlap and
all reset/profile effects; minimum recipe floor includes the two self-checks.

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
