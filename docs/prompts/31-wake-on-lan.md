# 31 - Implement complete Wake-on-LAN controls

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [27](27-interrupt-service.md), [28](28-power-states.md),
[5](5-silicon-power-interface-ledger.md), [13](13-side-effect-register-access.md).
Verify the concrete inputs below; retain unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver exact DP83826I WoL MAC matching, Secure-ON, CRC gate, pulse/level
indication, status/acknowledgement and routing, with readback and restoration.
Do not import DP83822 custom pattern matching or claim an application/MAC has
actually awakened because the PHY indication is set.

Revalidate SNLS647I tables8-90..97 and the
[WoL comparison](../reference/library-ti-features.md#interrupts-power-eee-and-wol).
RXFCFG1[11] is exact-part W0C. Pulse[10:9] gives8/16/32/64 cycles at125MHz
(64/128/256/512ns). DS MAC byte5 is API octet0:11:22:33:44:55:66 packs
04A2=2211,04A3=4433,04A4=6655; SecureON2A..2F packs2B2A/2D2C/2F2E.
SNLA261A and TI Linux yield the same wire words despite different byte labels;
the different custom-pattern feature is not imported.

Reopen INA228 destructive alert acquisition, PCA9555 mixed evidence/cleanup
tests and MB85RC explicit sleep/wake uncertainty contracts. Inspect relevant
sibling public fields and CLI formatting. Reopen SNLS647I RXFCFG and MAC/Secure-ON
tables, especially 8-90 and 8-92 through 8-94; compare SNLA261A only to identify
part-specific differences.

Define typed MAC/SecureON/CRC/pulse-level settings and pin conflicts with
whole-request validation. API octet0 is first transmitted; asymmetric literals
must match the original tables independently of codec round trips.

Keep exact-part W0C: one neutral, zero acknowledgement. SNLA261A/DP83822/TI Linux
use write1 on family paths, so qualify with Q3. Before any RXFCFG1 write harvest
RXFS and MISR2 through the collector; skip unrelated updates unchanged under
stable mask17E1. Explicit acknowledgement still writes. This retains evidence
under either physical polarity without importing the DP83822 recipe.

For reconfiguration of an already enabled detector, source and test the disable,
MAC/password/filter update, verification and re-enable order. Avoid exposing a
partially updated match, preserve pending events and coordinate RXFS source
selection with task 27. Extend reset/power/recovery restoration in source-defined
order; source conflicts about safe live reconfiguration remain explicit gates. Initialization does not add an undocumented WoL write;
owned requested settings may be replayed. Use the same collector as IRQ/polling,
retaining each consumed source immediately if a later read fails. A destructive
read-after-effect timeout records possible history loss.

Bound any multi-step enable/disable/acknowledgement procedure and use existing
maintenance and lifetime rules. Preserve accepted writes and independent
primary/cleanup statuses through cancellation or failed restoration. Actual
external wake routing, MCU power policy, MAC behavior and physical pins remain
application/fixture responsibilities.

Test independent MAC/Secure-ON byte vectors, legal extrema, invalid settings,
CRC/pulse/level combinations and every routing conflict. Exercise pending WoL
during unrelated RXFCFG1 changes, explicit W0C acknowledgement, simultaneous IRQ
sources, partial evidence and repeated cached views. Fail every write/readback/
ack/restore boundary and test cancellation, exact transfer bounds, no retries
and reset/power replay. Unsupported features reject before I/O.

Compile configuration and event-handling examples. Document exact-part matching
scope, pending-event preservation and how PHY indication differs from measured
application wake; absent board wiring never removes the software feature.

Reset is not WoL-off: RXFCFG1 resets108X with enable bit 7 set and bit 0 unknown;
RXFS[12] selectsWoL. Initialization writes RXFCFG1 only when owned settings
require it. Publish observed fields. Explicit disable clears bits 7,5,0 with W0C
neutral; bit 0 is RH and its stability is part of Q3 qualification.

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
