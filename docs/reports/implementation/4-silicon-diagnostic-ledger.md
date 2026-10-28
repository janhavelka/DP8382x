# Prompt 4 diagnostic, event and counter ledger

Completed source research for exact DP83826I/SNLS647I on 2026-10-28. The
primary ledger now has 54 detailed records (20 core plus 34 diagnostic), three
exact recipe records with unknown fields/reset, 39 prompt-5 pending records,
226 fields, 28 ordered procedures and four exceptional write actions. All eight
prompt-4 feature families have source decisions, implementation/CLI owners and
host/fixture acceptance plans. Driver implementation and qualification remain
unstarted; source research can close with explicit evidence gates.

## Entry, rules and ownership

Continued `promptset/dp8382x-implementation-20261028`, clean and synchronized at
`42a908b28d9ad471a60b5ca220127eb9e7dbf9ec`. Original base remains
`5e148fffde074b85fadc15d8ff66fe2c77111036`; same-name upstream is on
`https://github.com/janhavelka/DP8382x.git`. Prompt 3's audited schema, original
source archive, core reset/identity/link records and source policy are the
prerequisites actually used. Root/parent AGENTS, COMMON, FULL_COVERAGE,
SOURCE_MAP, EVIDENCE, ENGINEERING, CONVENTIONS, CLI_CONTRACT and VERIFICATION
were consulted. This repository has no workflow definition; sibling workflows
were inspected as source evidence. No remote CI was dispatched or inspected:
the first-execution exception reserves that work for prompt 71.

The lead owns JSON/schema/checker/tests, shared guides, progress and Git. Two
**gpt-6-astra / high-reasoning** reviewers independently researched diagnostic
procedures and destructive event/counter semantics, then cross-reviewed each
other against original PDFs. This stronger selection follows the prompt's
explicit request for contradictory silicon tables and destructive reads. An
inherited capable-model reviewer audited sibling/current-driver patterns and
the actual checker changes. Reviewers wrote only separate reports and temporary
integration fragments; they did not edit siblings, vendor payloads or Git.

- [Diagnostic review](4-diagnostic-source-review.md): original loopback/BIST/TDR
  tables, literal matrix, exact recipe actions, applicability and gates.
- [Event review](4-event-source-review.md): read effects, masks, counter limits,
  RMII/MSE conflicts, interrupts, calibration and independent cross-review.
- [Peer/current-driver review](4-peer-source-review.md): actual declarations,
  failure/cleanup implementations/tests, CLI/runners, revisions and file licenses.

## Source coverage and decisions

Original SNLS647I feature text and detailed tables were read together, with PDF
rendering for ambiguous columns. SNLA423A, SNLU262A, SNLA344C, SNLA494 and
SNLA495 were inspected for exact applicability. SNLA330A was checked because
of TDR similarity; its DP83825I calibration does not transfer to DP83826I.
Current exact TI support searches did not close the retained contradictions.
An unstable search snippet was explicitly excluded as an authoritative answer.
Vendor bytes, hashes, notices and text extracts remain unchanged.

Fresh TI RTOS, MCU+ ICSS-EMAC, combined Linux/Zephyr and TI Linux sources were
inspected with per-file licenses and immutable pins in the reports. MCU+'s
default `next` branch remains `4b9a66f`; its older `main` branch `517a277` is
not a newer baseline. Read/discard interrupt patterns and incomplete diagnostic
methods do not define this collector. No GPL implementation was copied.

Live clean sibling revisions were TMP1x2 `bafc340`, PCA9555 `498178e` and
MB85RC `1bf7c29`; full IDs and exact paths are in the peer report. Adopt retained
terminal results, partial-sample validity, accepted/uncertain effects and
separate primary/cleanup outcomes. Adapt MDIO transaction exclusion and
destructive reads explicitly. Reject sensor/memory timing, pointer-parking
commands and memory readback as specifications for PHY completion or lost history.

| Family / implementation owner | Completed research and limits |
| --- | --- |
| Loopback / 23 | All documented modes, speed/interface prerequisites, MII per-speed selection, analog termination, reverse/external path exclusion, exact digital recipe and restart, finite observation and restoration. |
| BIST / 24 | Generator/checker modes, PRBS15, single versus continuous traffic, packet/IPG encodings, sync/generator status, saturation/wrap and distinct freeze/clear action. No activity-only or zero-before-lock pass. |
| Cable / 25–26 | Manual/autorun, channels/crossover, DC/averaging/segments, pulse/threshold/shadow controls, supported raw result banks, freshness and finite stop/restore. No reserved-bank peaks or unproved metres/fault classification. |
| Fast Link Drop / 34 | All detector settings and cause bits, MII-only scope, strap/default conflict, exact MLT3 recipe and conflicting threshold/window interpretation. Physical limits remain separate from host polling deadlines. |
| Interrupts / 20,27 | Global/pin polarity/test, all MISR sources/enables, shared polarity/WoL source, RXFS events, owner-routed configuration and bounded service. Pin mux/wake policy remains prompt 5. |
| Counters / 20–21 | FCSCR/RECR, cross-clearing PHYSTS mirrors, BMSR/ANER histories, RMII status and per-word retained evidence on partial failure. Width/saturation and fault rearm remain gated. |
| Quality / 34 | Raw datasheet MSE, SNLA495 corroboration, explicit SNLA423A address/scaling discrepancy. No invented alias, dB result or universal quality threshold. |
| Calibration / 15,16,25,35 | Conditional deassertion immediately followed by hard reset, reserved adjacent pair budget, owed cleanup after uncertainty/cancellation and persistent-state replay. No calibration-start API inferred. |

Each procedure specifies admitted state, complete owned masks, prerequisites,
ordered actions, waits, finite primary/cleanup duration and work limits, partial
effects, cancellation, verification and reset interaction. Application budgets
are explicit caller-supplied policy, not invented silicon maxima. Every future
host suite must inject failure at transaction boundaries. Identified MII/RMII
boards, loop fixtures/100-ohm termination, quiet/disconnected partners, controlled
fault/noise injection, MDIO traces and oscilloscope/traffic instrumentation are
separate physical prerequisites. EtherCAT ESC register addresses and C2000
workarounds do not enter the PHY map or P4 RMII master configuration.

## Schema, review findings and corrections

The existing small JSON format is retained. RECIPE distinguishes researched
literal-only offsets from unknown pending controls. `writeActions` represents
BICSR1's overlapping read-counter/write-command and three opaque digital-loop
writes. An exact part-specific allowance rejects invented targets/masks/values/
neutral values, lost evidence gates and missing required procedure references;
it does not create a generic register-description engine. CDCR retains its
published RW label with explicit neutral zero for the self-clearing trigger.

Fixed stage-4 register and procedure inventories prevent silent demotion or
deletion. Independent original-PDF tests cover every diagnostic field partition,
access grouping and reset mask, plus legal encodings, exact actions and procedure
ownership. Malformed temporary records test schema/coverage rejection separately.
The checker reviewer also exercised 1,279 scalar/container mutations without an
uncaught exception; valid empty masks on read-only procedures remain allowed.
Semantic ownership oracles cover all 16 diagnostic procedures, and actions must
retain their explicit gates.

Cross-review found and corrected digital-loopback ownership of PHYRCR restart
and all stable controls changed by its whole-word writes, reverse-loopback BMCR
exclusion, and external-loopback exclusion of internal paths. It also added
autorun partner-activity uncertainty and clarified retained history across reset.
Original-PDF cross-review confirmed all diagnostic and event numeric matrices;
the lead independently checked FLD/BIST/reset source pages and the actual diff.

Nineteen ledger gates remain with owners and exact next evidence. Newly detailed
ones include BIST capture/result lifecycle, TDR calibration/freshness/abort,
RXFS SFD-reference contradiction and MLT3 recipe interpretation. Existing RECR,
RMII, MSE, PHYSTS and reserved-bank gates remain. These withhold affected claims;
they do not prevent raw acquisition research or erase required later work.

## Verification and handoff

Environment: Windows, PowerShell, Python 3.12.10. The executable checker/tests
use only the Python standard library; PDF rendering was research tooling.

| Evidence layer | Result and scope |
| --- | --- |
| Host research tooling | PASS: `python tools/verify_ledger.py` (96 registers, 28 procedures, 35 features); `python -m unittest discover -s test -p "test_*.py" -v` (57 tests). |
| Source/reference/local links | PASS: `python tools/verify_references.py`; all 27 vendor artifacts and 25 extracts preserved, local links validated. |
| Diff/self-review | PASS: `git diff --check`, task-only review, independent silicon and checker review. |
| Driver host behavior | NOT RUN: no driver implementation was added. |
| Portable/package consumers | NOT RUN: no build/package implementation in this research stage. |
| Native SDK link | NOT RUN: source inspection is not a linked SDK result. |
| Remote CI | NOT RUN: reserved for prompt 71. |
| Physical hardware | NOT RUN: no fixture was operated or qualified. |

README/CHANGELOG, architecture/variants, device behavior, register/schema/vectors
guides, source navigation, feature coverage, test instructions and progress were
reviewed/updated together. Future Doxygen must cite clearing sources, command
actions, validity/units, bounds and gate scope; no placeholder C++ was introduced.
Prompt 5 still owns remaining power/interface/control research. Final checked
commit and verified same-name upstream equality are supplied in the handoff,
avoiding a self-referencing commit in this report.
