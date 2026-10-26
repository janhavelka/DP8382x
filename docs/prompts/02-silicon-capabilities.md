# 02 - Establish exact silicon and family capabilities

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01.

Read [CONVENTIONS](CONVENTIONS.md) and briefly record the applicable rules,
evidence sources, owned paths, documentation/checks and same-branch checkpoint.
Use subagents for independent source/table review with complexity-appropriate
models; the lead resolves conflicting interpretations and owns Git.

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

## Concrete ledger and variant contracts

Reopen `TMP1x2/include/TMP1x2/Config.h` (`Model`, `isValidModel`, address
validators), `MB85RC/include/MB85RC/CommandTable.h` (`DeviceVariant`,
`VariantInfo`, optional capability fields), and the corresponding implementation
and rejection tests. The useful principle is exact declared model plus explicit
capabilities. Neither I2C address windows nor absence of a readable sensor ID
transfers to the PHY. Use the planned names `Variant`, `DeviceIdentity` and
`Capabilities`; stage 04 settles their public representation.

Choose one machine-readable evidence file, such as
`docs/registers/dp83826i.json`, with a documented schema. Prefer a small explicit
format over a generic register-description framework. Include these columns or
equivalent fields, separating register, field and procedure records:

| Record | Required data |
| --- | --- |
| Register | Exact variant/mode, address space/device address/register address, source name, width, reset value/mask, access type and reserved-bit rules |
| Field | Register key, bit range/mask, legal encodings, validity conditions, side effects and source section/table |
| Procedure | Ordered prerequisites/actions/waits, owned masks, variants/interfaces, timeout, partial effects, cleanup and source citations |
| Coverage | Planned typed API, CLI path, independent vector/test, software disposition, host/SDK/HIL evidence kept separate |
| Source/conflict | Official URL/document revision or source commit/file, license, competing interpretations, selected evidence or unresolved gate |

Use the existing 79-entry detailed-register inventory as a reconciliation input,
not a claim that it includes every field or every procedure-only address. Check
summary tables, detailed headings and feature text against each other. Preserve
unknown reset bits explicitly; never turn a blank or dash into zero. Source a
separate legal-value table where not every bit encoding is valid.

For each supported part distinguish the declared ordering variant, read PHY IDs,
revision, strap/interface evidence and software qualification. Do not infer a
board's variant from a shared ID. A capability must have exact evidence and a
defined unsupported path. Runtime-disabled features remain implementation scope.
Unresolved source facts stay named gates; unavailable physical fixtures stay a
different evidence state.

Have a second reviewer inspect representative arithmetic and difficult entries:
MMD addressing, write-one/read-clear behavior, reset defaults, mode-dependent
fields and the known PHYSTS/RECR contradictions. The reviewer uses original PDF
tables and independent literal values, not constants generated from the ledger.

## Documentation in this stage

Maintain the reference index/provenance only for actual new evidence, and update
device behavior, register map/vectors, variants and the feature matrix together.
Review README/CHANGELOG for accurate scope. Specify the Doxygen citation and
access-attribute information future `CommandTable.h` and procedures must carry;
do not create placeholder source just to attach comments. Keep vendor payloads
unchanged and separate public facts from code with reuse/license restrictions.

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
