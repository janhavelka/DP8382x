# 31 - Implement complete Wake-on-LAN controls

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

Prerequisites: [27](27-interrupt-service.md), [28](28-power-states.md), [5](5-silicon-power-interface-ledger.md), [13](13-side-effect-register-access.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Revalidate [WoL field/byte-order facts](../reference/library-ti-features.md#interrupts-power-eee-and-wol)
against SNLS647I tables 8-90..97. RXFCFG1 `04A0[11]` is W0C;
pulse selector `[10:9]` gives 8/16/32/64 cycles at 125 MHz
(derived 64/128/256/512 ns), and RXFS `04A1[12]` selects WoL/polarity IRQ.
Freeze explicit byte indexing: MAC words `04A2/3/4` hold high:low DS byte pairs
4:5, 2:3, 0:1; Secure-ON `04A5/6/7` hold 1:0, 3:2, 5:4. Test literal packets and words
from the exact DP83826 tables, preserving read-clear samples and neutral W0C
during unrelated updates. SNLA261A's DP83822 word order and custom-pattern
examples are not DP83826 implementation authority.

Deliver exact DP83826I WoL MAC matching, Secure-ON, CRC gate, pulse/level
indication, status/acknowledgement and routing, with readback and restoration.
Do not import DP83822 custom pattern matching or claim an application/MAC has
actually awakened because the PHY indication is set.

Reopen INA228 destructive alert acquisition, PCA9555 mixed evidence/cleanup
tests and MB85RC explicit sleep/wake uncertainty contracts. Inspect relevant
sibling public fields and CLI formatting. Reopen SNLS647I RXFCFG and MAC/Secure-ON
tables, especially 8-90 and 8-92 through 8-94; compare SNLA261A only to identify
part-specific differences.

Define complete typed settings with exact octet ordering, legal Secure-ON/CRC/
pulse-level combinations and pin-routing conflicts. Validate the whole request
before I/O. Produce independent literal MAC and password-word vectors and
readback expectations directly from exact-part tables. Do not assume the
DP83822 example's word order applies.

RXFCFG1 bit 11 is W0C: explicit zero acknowledges the level indication.
Unrelated updates must preserve pending WoL through the existing command-neutral
composition. Do not use a W1C-like acknowledgement or a generic all-command-zero
write. Separate policy configuration, consumed event/status and explicit
acknowledgement so an unrelated setting cannot erase wake evidence.

Apply/verify stable owned fields and extend reset/power/recovery restoration in
source-defined order. Opt-in WoL is never an undocumented initialization default;
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
