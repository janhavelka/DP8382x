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

Revalidate [WoL field/byte-order facts](../reference/library-ti-features.md#interrupts-power-eee-and-wol)
against SNLS647I tables 8-90..97. RXFCFG1 `04A0[11]` is W0C;
pulse selector `[10:9]` gives 8/16/32/64 cycles at 125 MHz
(derived 64/128/256/512 ns), and RXFS `04A1[12]` selects WoL/polarity IRQ.
Freeze explicit byte indexing: MAC words `04A2/3/4` hold high:low DS byte pairs
4:5, 2:3, 0:1; Secure-ON `04A5/6/7` hold 1:0, 3:2, 5:4. Test literal packets and words
from the exact DP83826 tables, preserving read-clear samples and neutral W0C
during unrelated updates. SNLA261A's DP83822 word order and custom-pattern
examples are not DP83826 implementation authority.

Reopen INA228 destructive alert acquisition, PCA9555 mixed evidence/cleanup
tests and MB85RC explicit sleep/wake uncertainty contracts. Inspect relevant
sibling public fields and CLI formatting. Reopen SNLS647I RXFCFG and MAC/Secure-ON
tables, especially 8-90 and 8-92 through 8-94; compare SNLA261A only to identify
part-specific differences.

Define complete typed settings with exact octet ordering, legal Secure-ON/CRC/
pulse-level combinations and pin-routing conflicts. Validate the whole request
before I/O. State that API byte 0 is the first transmitted octet; map datasheet
byte labels explicitly to that convention. Produce asymmetric literal MAC and
password-word vectors and readback expectations directly from exact-part tables;
a codec round trip cannot prove wire order. Do not assume the
DP83822 example's word order applies.

RXFCFG1 bit 11 is W0C: explicit zero acknowledges the level indication.
Unrelated updates must preserve pending WoL through the existing command-neutral
composition. Do not use a W1C-like acknowledgement or a generic all-command-zero
write. Separate policy configuration, consumed event/status and explicit
acknowledgement so an unrelated setting cannot erase wake evidence.

For reconfiguration of an already enabled detector, source and test the disable,
MAC/password/filter update, verification and re-enable order. Avoid exposing a
partially updated match, preserve pending events and coordinate RXFS source
selection with task 27. Extend reset/power/recovery restoration in source-defined
order; source conflicts about safe live reconfiguration remain explicit gates. Opt-in WoL is never an undocumented initialization default;
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
