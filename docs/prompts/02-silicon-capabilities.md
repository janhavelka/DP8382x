# 02 - Establish exact silicon and family capabilities

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01.

## Fresh audit and owned work

Revisit TMP1x2 and MB85RC variant selection and TUSB32x capability/source-evidence
planning. Read their actual rejected-variant tests and public model declarations; do not
inherit their chip protocols.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Execute [FULL_COVERAGE](FULL_COVERAGE.md). Start with the
[public research catalog](../reference/public-research.md) and
[DP83826I inventory](../reference/dp83826i-feature-inventory.md), then perform a
broad manufacturer-first public-source sweep and targeted searches for every
remaining feature gap. Inspect TI RTOS, ICSS-EMAC, Linux/Zephyr and relevant SDK
headers, source, bindings, fixes and examples; compare current primary upstream
implementations. Pin inspected revisions, per-file licenses and actual findings.
An existing driver or header is not the complete chip specification.

Build a machine-readable register/field/procedure ledger covering the full
datasheet, feature text and source-backed application-note recipes. Include all
summary-table entries, documented addresses used only by a feature sequence,
reserved/invalid holes, access attributes, legal values, reset/mode constraints
and side effects. Reconcile the original PDF instead of copying partial vendor
macros. Source every row and distinguish factual register data from reusable code.

Create `docs/device-behavior.md`, `docs/register-map.md`, `docs/register-vectors.md` and
a feature matrix with separate documented, implemented, host-tested, SDK-built and
hardware-qualified columns. Start DP83826I/SNLS647I; enumerate later DP83826E,
DP83826AI/AE, DP83825I and DP83822I/H/IF/HF scope. Keep undocumented Linux suffixes
reference-only.

Re-open original PDF tables for IDs/modes/revision, Clause 22/indirect addressing,
access attributes, reset defaults, straps, waits, advertisement and link decoding.
Explain BASIC/ENHANCED model IDs rather than equating them with order suffixes. Specify
which reset resamples which strap and what survives each reset/restart. Include source
sections/pages and independent literal vectors.

Audit 10BASE-Te VOD_CFG3, latch-low/read-clear registers, interrupt handling,
loopback/BIST/TDR, LED/clock and power features. Flag PHYSTS forced-mode and RECR
width/saturation conflicts from the PHY audit; resolve through stronger official
evidence or retain explicit gates. Never encode undocumented reserved writes to make a
reference recipe work.

Recheck applicable official product errata/support and pinned TI/Espressif sources.
Preserve downloaded payloads/provenance; record newly reviewed revisions separately.
Review TI reset-address and revision-check pitfalls and per-file licenses. Make a
written procedure for each accepted sequence with prerequisites, side effects, timeout,
failure uncertainty and restoration. EtherCAT ESC notes are contextual; first-board RMII
cannot inherit MII-only Fast Link Drop.

## Acceptance and handoff

Reference integrity passes. Independently inspect representative PDF register tables and
arithmetic. Verify every supported feature/variant claim has exact source evidence,
conflicts have dispositions, and vectors are not generated from future production
constants. Require no unexplained gaps in the full register/field/procedure
inventory, and a source/conflict/implementation plan for every documented
software-accessible feature. Source coverage is not driver support.

Expected output: Authoritative register/procedure evidence and explicit capability
gates. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
