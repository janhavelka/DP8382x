# 14 - Implement remaining documented PHY controls

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-13; independently unavailable optional capability
evidence stays explicitly gated.

Read [CONVENTIONS](CONVENTIONS.md) and restate the applicable stage rules before
coding. Re-audit live sibling source; use simple engineering language, source
Doxygen, affected guides and local checks. Keep the same execution branch and
coherent-unit/per-prompt commit+sync; first-set remote CI belongs only to 29.

## Fresh audit and owned work

Audit SCD41/RV3032-C7 explicit power/IRQ commands and LDC1614 deferred interrupt
service; review peer capability validation and raw register safeguards.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Concrete source pass and implementation boundaries

Read [CONVENTIONS](CONVENTIONS.md). Reopen
`MB85RC/src/MB85RC.cpp::enterSleep`, `wake`, `_ensureAwakeForI2c` and public
sleep/capability declarations: uncertainty survives transition failure and timed
wake differs from acceptance. Its no-clock caller-wait rule is not automatically
valid for PHY; use 04's time contract. Reopen INA228 `DiagAlert`/`readDiagAlert`
declarations/implementation and PCA9555 deferred input/interrupt cleanup tests.
Inspect current SCD41/RV3032-C7/LDC1614 power/IRQ source/tests when adopting their
additional conventions; record found symbols instead of relying on repo titles.

Extend existing typed config, collector, restore and operation engine. Split a
cohesive source only for an implemented responsibility with callers, such as
power/wake procedures together. No class per LED, interrupt or field. Keep
register access/admission/error mapping/cleanup shared. New operations extend
04's frozen naming contract before exposure; a bag of raw setter aliases must
not bypass trust or ownership. Group readable helpers by actual chip procedure.

## Work packages and decisions

1. Reconcile every ledger row with code, Doxygen, guide, independent tests and
   future CLI/runner mapping. Close coherent feature groups with verified commits.
   Constants/raw access are not feature completion. Source conflicts remain open;
   hardware-only/inapplicable dispositions require exact-part evidence.
2. Interrupt configuration/service covers global/output enable, polarity,
   software test, all MISR1/MISR2 sources, mux and read-clear evidence. Define
   events arriving during service without an unlimited drain loop. ISR only
   signals the owner; no MDIO/logging/allocated queue there. IRQ and polling use
   the same collector and passive cached results.
3. Distinguish active-sleep policy from explicit IEEE/deep-power-down operations.
   SNLS647I 8.3.6.1 enables active sleep through PHYSCR[14:12]=110; hardware
   enters/wakes automatically according to partner detection. BISCR[8] reports
   current sleep/normal state. Policy configuration can succeed while linked and
   awake; do not wait for sleep entry or promise partner-dependent wake/link time.
   Keep explicit power-down/disable procedures bounded, with retained config and
   clock/PLL effects. Unknown write effects cannot become known sleep/awake state.
   End is local; pin/clock/power sequencing remains application-owned.
4. Complete EEE capability/advertisement/partner/status and exact DP83826I recipe,
   including procedure-only writes. Resolve bypass polarity against the datasheet.
   Keep printed EEE addresses separate from verified wire offsets; resolve the
   exact-part access trace noted in the inventory before freezing MMD constants.
   Do not strip address digits or borrow another part's mapping by assumption.
   Legacy-MAC TX LPI requires explicit coordination with no implied PHY buffering;
   result/cleanup shows whether TX remains inhibited. No default EEE/LPI enable.
5. WoL covers documented MAC match/Secure-ON, CRC gate, pulse/level indication,
   acknowledgement/status and routing. Verify octet order with literal vectors
   and readback. No imported DP83822 custom pattern support. PHY indication and
   actual application/MAC wake are separate facts and responsibilities.
   RXFCFG1[11] is W0C: an explicit zero acknowledges level indication; unrelated
   writes preserve it through command-neutral composition. SNLA261A's DP83822
   acknowledgement example and MAC-word order are not DP83826 programming values.
   Use literal vectors from exact-part tables 8-90 and 8-92 through 8-94. Preserve
   partial event acquisition on later failure through the existing collector.
6. Complete LED/GPIO source/mux/polarity/drive/blink/stretch and clock/interface
   options. Validate conflicting pin users and reset-loaded versus runtime
   settings. Unsupported live clock/interface transitions reject before writes
   and document application sequencing. Unwired IRQ and initial RMII do not
   remove generic silicon features from scope.
7. Close next-page, crossover/polarity, event/error counters, MSE/raw quality,
   MII-only Fast Link Drop and hardware link-loss-recovery settings. Latched chip
   events are separate from transport counters and owner recovery policy. Resolve
   source/scaling conflicts before claiming SNR/quality units from raw MSE.
8. Complete repeater/interface options, analog/MAC electrical controls, timing/
   compatibility and compliance patterns with documented legal values. Reserved
   bits are not tuning controls. Disruptive stimuli need finite sessions and
   stop/restore. Resistor-calibration contingency is an admitted bounded action,
   not an autonomous reset loop. Extend the one restoration ledger for each
   newly owned persistent field across every applicable reset/power kind.

## Required test oracles

For every work package provide source-to-test mapping with independent literal
masks/encodings/sequences, legal extrema, reserved/invalid values, part/mode
rejection and failures at every callback boundary. Include simultaneous IRQ
sources, event during collection, repeated cached views, read-clear preservation
and mask changes with pending evidence. Exercise ambiguous power entry, wake
timeout, EEE negotiation/LPI exit, WoL byte order/acknowledgement, pin conflicts,
reset replay and failed restoration. Local invalid/unsupported input is bus-
silent and opt-in controls never activate without explicit requested configuration;
initialization/recovery may apply or replay such validated owned settings. Raw tests
cannot replace stateful procedure tests; no fake physical qualification.

## Documentation shipped in this stage

Doxygen new types/fields/methods with applicability, legal units/values, pin/MAC
coordination, read effects, bounds, validity and failure/cleanup. Maintain guides
grouped by use (interrupts, power/wake/EEE, pins/interfaces, diagnostics) with
compiling examples or source-backed procedures and fixture limits. Avoid empty
one-field pages. Update README, CHANGELOG, API contracts, restoration guide,
ledger and validation with each coherent feature group. Run Doxygen generation,
warning and link checks. Do not claim later CLI/SDK/physical completion from
planned mappings. Keep the execution branch and coherent-unit/per-prompt sync;
this first set dispatches/polls/waits for remote CI only at 29.

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
MDIO/logging, no unrequested optional initialization side effects, and no generic EEE/WoL claim
without source evidence. Reconcile all authoritative feature sections, register
fields and procedure-only accesses with APIs, commands and tests. An incomplete
vendor driver or absent first-board fixture cannot close a missing feature row.

Expected output: Closed primary feature coverage with explicit capability and board
limits. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
