# 14 - Implement remaining documented PHY controls

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-13; independently unavailable optional capability
evidence stays explicitly gated.

## Fresh audit and owned work

Audit SCD41/RV3032-C7 explicit power/IRQ commands and LDC1614 deferred interrupt
service; review peer capability validation and raw register safeguards.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Complete the primary chip's full register/field/procedure ledger under
[FULL_COVERAGE](FULL_COVERAGE.md). Implement every remaining documented
software-accessible feature with its legal settings, readback/status, bounded
operations and tests. Supported-but-unimplemented features are open work, not
"unsupported" or outside scope. Source-proved hardware-only/inapplicable
properties receive an explicit disposition; raw access alone does not implement
stateful features. Missing evidence remains a completion gate to resolve.

Use the complete inventory to cover EEE capability/advertisement/status and LPI
control, legacy-MAC EEE, WoL MAC matching and Secure-ON setup where documented,
active sleep, IEEE power-down and deep power-down, every LED/GPIO source/mux/
polarity/blink control, clock/interface settings, RMII repeater configuration,
MII Fast Link Drop options, documented compliance stimuli and analog controls.
Check next-page negotiation, MSE/FCS/receive counters, polarity/jabber and other
controls against stages 09-13 so no feature falls between stages. Source-defined
legal settings and bit meanings take precedence over this grouping.
Do not add custom WoL pattern matching to DP83826 from another variant's recipe;
capability evidence must support that exact match mechanism.

Implement safe typed access for programmable controls while leaving physical
wiring, MAC policy and electrical certification to the application/test fixture.
MII-only or unwired IRQ functionality must remain in the general library even
though the initial RMII board cannot qualify it physically. Do not import an
ICSS/Zephyr/Linux runtime merely to reuse its feature list.

Implement documented interrupt masking/service with deferred owner execution and single
consumption of read-clear status. The initial board has no connected IRQ; polling
remains supported without a fake interrupt example. Expose LEDs, clock output and
interface-related configuration only with exact mode/reset constraints, preserving
application ownership of wiring.

Treat PHY power-down, wake/WoL and EEE separately. Verify presence for DP83826I itself;
an EEE app note is not capability proof. Do not enable them by default or copy another
family member's masks. Any MAC coordination and application latency policy belong
outside core. Power delivery/PoE remains entirely excluded.

Runtime opt-in controls are mandatory library scope and participate in
configuration trust, reset restoration, cancellation
and explicit maintenance admission. Clock/interface changes may break the active MAC;
reject unsupported live transitions and document required board/application sequencing.
Maintain unsupported-before-I/O behavior for every absent feature.

## Acceptance and handoff

Test each added control's exact masks, reserved fields, unsupported modes, IRQ service
races/read-clear ownership, restoration and interrupted writes. Confirm no ISR
MDIO/logging, no optional initialization side effects, and no generic EEE/WoL claim
without source evidence. Reconcile all authoritative feature sections, register
fields and procedure-only accesses with APIs, commands and tests. An incomplete
vendor driver or absent first-board fixture cannot close a missing feature row.

Expected output: Closed primary feature coverage with explicit capability and board
limits. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.
