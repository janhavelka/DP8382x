# Prompt 5: primary power, wake, interface and source closure

Completed source research on 2026-10-28 for exact DP83826I / SNLS647I.
All 79 detailed-register entries now have complete field/access/reset
dispositions; 17 opaque recipe records retain unknown field/reset maps.
Five printed EEE associations remain explicitly blocked from the wire namespace.
The ledger contains 283 fields, 52 procedures, 28 exceptional actions and all
35 researched feature families. None of this claims a working PHY driver.

## Entry, rules and ownership

Read parent and repository AGENTS, COMMON, FULL_COVERAGE, ENGINEERING,
SOURCE_MAP, EVIDENCE, prompt 5, README, architecture, variants and reference
index. Prerequisites were the checked core and diagnostic ledger/schema,
independent vectors and source decisions from prompts 3–4.
Entry was clean, fetched and equal to the configured same-name upstream at
`2805cfdea3aacd1f870d27998cd99d0270cccb2a`.

Execution branch remains `promptset/dp8382x-implementation-20261028`;
original main/base remains `5e148fffde074b85fadc15d8ff66fe2c77111036`.
Remote is `https://github.com/janhavelka/DP8382x.git`. The final checked
commit and verified upstream equality are recorded in the handoff, avoiding
a self-referencing commit. Remote CI remains prompt 71 only; no workflow
definitions exist here yet and none were introduced or dispatched.

The lead owns the shared JSON, schema/checker, tests, behavior/vectors/coverage,
variant and source guides, report/progress and Git. Review agents own only their
reports and temporary integration fragments. Siblings, vendor PDFs, derived
archive payloads and provenance remain unchanged. No placeholder C++ or generic
register framework was added.

Apply one operation/collector owner, whole indirect/RMW exclusion, bounded
terminal transport, finite primary and cleanup durations plus frame counts,
retained accepted/uncertain effects and stop-before-restoration. Requested
policy, observed state, configuration trust and physical qualification remain
separate. Board wiring never removes documented software scope.

## Fresh source work and independent review

Three disjoint reviews ran in parallel:

| Review | Model rationale and owned scope | Original evidence |
| --- | --- | --- |
| [Power/wake](5-power-source-review.md) | Explicit gpt-6-astra/high, fresh context; stronger reasoning for W0C, unknown addressing, exit and cross-part semantics | SNLS647I power/EEE/WoL pages and tables, SNLA261A, SNLA328, EVM; current TI product/support and manufacturer sources |
| [Pin/electrical](5-pin-source-review.md) | Explicit gpt-6-astra/high, fresh context; stronger reasoning for reserved fields, split encodings and contradictory pin tables | SNLS647I detailed LED/IO/VOD/COMPT tables, SNLA266B, SNLU262A and SNLA344C |
| [Coverage/source](5-coverage-source-review.md) | Inherited capable model; live source inventory, rejection tests, schema coverage and semantic diff | TMP1x2/MB85RC/TUSB32x paths, current manufacturer commits/licenses, original later-family tables |

The two silicon agents cross-reviewed each other's integrated work. Reviewers
derived expected values from original PDFs before integration, not from the
ledger. The lead checked original CR2, power/EEE, WoL and compliance facts,
reviewed the actual changes and resolved confirmed findings.

TMP1x2 was reopened at `bafc34067530eb340e55c42dae3fad4eb795b6e9`:
`include/TMP1x2/Config.h`, configuration implementation, variant rejection
tests, CLI and runner. MB85RC was reopened at
`1bf7c293549b1038b2d8f3b7a8d655fa74201e6e`:
`include/MB85RC/CommandTable.h`, public power/result contracts, implementation,
rejection/uncertainty tests, CLI helper and HIL runner. TUSB32x planning at
`e1e79e6fb440fe2c3eb56d2df215b46722dcded3` remains a planning comparison;
its unrelated local runner edits were preserved. Actual paths, rules,
workflow inspection and adopted/adapted/rejected principles are in the reviews.
No sibling tests were run or changed.

Current manufacturer sources were pinned at MCU+ default `next`
`4b9a66f4e18e02a834961f5f834155c846cab041`, RTOS
`94bf4343004732983b0b8e1eec123d88ff70453b`, and combined TI Ethernet
`450af1144b8caeb58724bcbae081b409f6bf5c3a`. Per-file BSD-3-Clause,
Apache-2.0 and GPL-2.0 notices were inspected; no implementation was copied.
The combined TI tree is not mislabelled upstream Linux/Zephyr. Bounded TI
product/support searches found no inspected clarification closing the remaining
EEE, COMPT or GPIO conflicts. This is not proof that no errata exists.

Original SNLS647I August 2025, SNLA266B April 2025, SNLA261A March 2024,
SNLA328 October 2019 and SNLU262A June 2023 remain archived under their existing
hashes. Later-family boundaries use SNLS783A/SDAA067A, SNLS638C and SNLS505H,
with exact revisions/pages in the coverage review and [variants](../../variants.md).
Shared identities do not detect ordering suffixes; undocumented Linux names
remain reference-only.

## Completed contracts and ownership

The 39 remaining rows were reconciled against detailed definitions and recipe
prose, yielding 25 complete detailed records and 14 additional opaque records.
Shared core/diagnostic fields remain in their existing records. No reset-unknown
bit was filled with a guessed zero, and no all-reserved entry became writable.
No secondary numeric guide map was created.

| Primary owner | Added procedure contracts | Required future acceptance |
| --- | --- | --- |
| 28 | IEEE power request, active-sleep policy, PLL control, deep power entry/exit gate | Enabled-but-awake success, external PWRDN dependency, preserved isolation, ambiguous command and clock-readiness outcomes |
| 29–30 | Exact EEE initialization/negotiation/status, legacy LPI | Rejection before unknown reads/reset prefix, literal order, nonrecursive replay, finite TX inhibit/exit and peer combinations |
| 31 | Quiesced WoL configuration and explicit level acknowledgement | Own-table MAC/password mapping, CRC/Secure-ON/pulse/level, W0C neutrality, retained pending samples and every partial credential prefix |
| 32–33 | LED, pin, clock, MII, RMII, repeater, compatibility and isolation | Per-pin legal values, source/routing conflicts, POR-only distinctions, collector read effects and coordinated two-PHY failure accounting |
| 34–35 | Transmit controls, analog configuration and finite compliance/collision tests | Legal amplitudes/split fields, polarity/jabber/IPG, reserved-write rejection, stopped stimulus before persistent restore |
| 18 | Link recovery and extended duplex/crossover | Bit-5-only ownership, forced/robust crossover coupling and configured-versus-resolved state |

Existing next-page exchange retains prompt 17. All software families have primary
owners in 11–36; later SDK/CLI/test/fixture owners remain additional dependencies.
Resistor calibration retains the prompt-4 procedure and exact hardware/recovery
scope. Physical plans identify part, board, mode/straps, firmware/runner identity,
peer/cable, routing and calibrated instruments separately from software behavior.

The schema stays small and explicit. Exceptional actions now cover only their
audited masks/values and exact consuming procedures. Action gates must propagate
to consumers. All detailed and opaque rows, required procedures, printed
dispositions and fixed feature scopes are checked; programmable features cannot
be reclassified as hardware-only to evade ownership.

## Findings fixed during review

- Corrected the draft MAC oracle after cross-review: TI byte 5 is explicitly
  MSB, so the first API MAC octet maps there. Ledger, procedure, review report
  and independent vectors now agree; the DP83822 example is not the authority.
- Restricted opaque compliance preservation reads before I/O until safe
  read/composition semantics are established. Exact harmonic stimulus remains
  an independently sourced exception.
- Reordered compliance cleanup to stop/reset before changing COMPT or restoring
  a persistent profile, and counted delegated source/IRQ/WoL work in pin budgets.
- Moved extended full-duplex ownership to prompt 18, preserving reserved bit 6;
  removed stale prompt-5-pending text from already researched shared controls.
- Closed validator gaps for same-feature borrowing of exceptions, dropped
  consumer gates and software-to-hardware scope relabelling. Added finite
  subprocess timeouts and malformed temporary-record rejection tests.

## Gates and documentation

Twenty-four gates remain in the ledger, each with affected scope, selected
decision, next evidence and owning stage. Newly completed power/interface
research explicitly retains:

- EEE wire association and exact control/read/reset/disable semantics:
  require exact-part MDIO traces or TI clarification, not digit stripping or
  DP83825 status/counter/bypass assumptions.
- Deep-power exit and shared opaque composition: require complete exit,
  clock/SMI readiness and unrelated-bit preservation evidence.
- WoL named mask semantics and live-update guarantees: require exact feature
  definition and event/update ordering; ordinary quiesced setup remains specified.
- Pin/clock/PHYCR contradictions, IOCTRL reserved example and analog receiver
  code-to-voltage mapping: require per-pin exact clarification and identified
  electrical traces; documented independent LED and IO fields remain eligible.
- COMPT enable/stop and opaque composition: require exact safe 100M access and
  stop evidence; a sourced exceptional word does not authorize general writes.

Earlier core/diagnostic gates remain intact, including PHYSTS forced mode,
reset/strap domains, next-page handshake, MSE/RECR/RMII evidence, TDR freshness/
calibration/reserved banks and BIST/FLD conflicts. Research may close with these
explicit dispositions; affected support cannot be claimed until resolved.

Updated README, CHANGELOG, architecture, device behavior, register/schema/vector
guides, feature coverage, variants, source navigation, sibling comparison,
verification guide and progress. Future Doxygen must cite exact access/sequence
sources, neutral writes, octet numbering, legal pin combinations, physical units,
nominal qualifiers, source gates and partial/cleanup results at the real API.

## Verification and handoff

Environment: Windows/PowerShell, Python 3.12.10. Validators and tests use the
standard library; PyMuPDF was used only for original-PDF research.

| Evidence layer | Result and scope |
| --- | --- |
| Host research tooling | PASS: `python tools/verify_ledger.py`; 96 records, 52 procedures, 35 features |
| Registered research suite | PASS: `python -m unittest discover -s test -p "test_*.py" -v`; 74 tests, independent source literals and malformed temporary records |
| Reference/link integrity | PASS: `python tools/verify_references.py`; all 27 vendor artifacts and 25 text extracts preserved; local links checked |
| Diff review | PASS: `git diff --check`; reviewed task-only final changes and semantic retention of prior records |
| Driver host / portable-package consumer | NOT RUN: no driver or package implementation in this research stage |
| SDK link | NOT RUN: no target image introduced |
| Remote CI | NOT RUN: reserved for prompt 71 |
| Physical qualification | NOT RUN: no fixture was operated |
| Source-dependent operations | BLOCKED only for the named affected branches; all research dispositions recorded |

The final handoff records the checked commit, push and post-fetch upstream
equality. Prompt 6 and the later implementation owners can consume this ledger;
no later stage is marked complete by this source-coverage result.
