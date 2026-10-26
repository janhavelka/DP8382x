# 31 - Reconcile user-facing guides and feature coverage

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-29 implementation outputs; consume stage 30's actual
evidence or explicitly recorded block.

At entry apply CONVENTIONS' names, Status/health, layout and simplicity rules;
reopen current relevant sibling sources. Keep source Doxygen and affected
README/CHANGELOG/guides current, verify locally and checkpoint/sync on this
execution's recorded branch.

## Fresh audit and owned work

Re-audit peer README/integration/testing/feature coverage organization and FieldCore
source-map distinction between requirements and historical reports.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Document the complete register/field/procedure ledger and public research basis.
State full software feature coverage only when every required programmable row
is implemented; keep unsupported-silicon, hardware-only, unavailable-fixture and
unfinished-software rows visibly distinct. Explain runtime opt-in defaults without
describing unused first-board features as optional implementation scope.

Reconcile README, architecture, variants, API/integration, CLI, examples, testing,
hardware-validation, changelog and reference notices with actual implementation. Remove
outdated planning claims only when the behavior exists. Keep unqualified/unimplemented
features explicit and link durable documentation rather than making users read prompts.

Give concise ordinary C++ and native-IDF quick starts using actual buildable code.
Explain application ownership of pins/address/clocks/locks/time/recovery, generic 10/100
options and separate 100/full profile. Document live vs cached status, no-link vs
transport error, configuration uncertainty and how a FieldCore-like owner maps them.

Document package/component consumption, supported SDK range, backend bounds, firmware
identity and runner commands. Provide ordinary Ethernet networking and EtherCAT master
integration boundaries without pretending the library contains either stack. Explain
optional/disruptive diagnostics and variant restrictions.

Maintain a requirement-to-source/API/test/CLI/runner/evidence matrix with software and
physical columns. List unresolved silicon interpretations, adapter limits and remaining
hardware fixtures with concrete next steps. Avoid blanket production-ready or
whole-family support language.

Keep prompt execution history in progress/reports and permanent behavior in maintained
guides. Do not erase failure evidence or vendor source provenance while cleaning stale
planning text.

## Concrete guide set and claim checks

Fresh anchors are `../MB85RC/CONTRIBUTING.md` and maintained
`docs/DEVICE_REFERENCE.md`, `docs/IDF_PORT.md`, `docs/RELEASE_CHECKLIST.md`;
`../TMP1x2/tools/check_doxygen.py::check_html/check_api` and generated-doc input
rules; and `../TCA9548A/tools/test_hil_artifacts.py` for retained evidence/NOT RUN
semantics. Inspect current source/examples/CLI/help/runner commands and AGENTS
behind the prose, record revisions/dirty state and compare actual behavior.
Historical reports and planned sibling APIs do not prove current user features.

The lead owns README, maintained API/architecture/feature claims and integration.
Delegate read-only ordinary-user/IDF-owner guide review, or disjoint link/snippet
verification, with exact paths and available models suited to complexity.
Reviewers must run/read the documented path and report defects, not rewrite
shared contracts. Apply CONVENTIONS' exact names, simple English and clear
units/errors. This stage reconciles existing docs; it does not defer source
Doxygen or guides from earlier behavior stages until now.

Keep one authoritative destination per user need, using existing equivalent
files rather than duplicating guides:

| Maintained destination | User decision and verifiable content |
| --- | --- |
| `README.md` | actual support/status, minimum install and links to buildable examples; no whole-family promise |
| `docs/api-contracts.md` and source Doxygen | stable names, valid states, lifetime/time bounds, outputs/errors and capability behavior |
| `docs/architecture.md` / integration guide | portable and IDF operation ownership, transaction serialization, board/stack responsibilities |
| `docs/variants.md` / feature matrix | exact part/revision/interface/medium; software versus SDK/physical qualification |
| CLI/runner guides | actual grammar/help, finite commands, fixtures, firmware identity, artifacts/verdicts and restoration |
| `docs/testing.md`, `docs/packaging.md`, `docs/hardware-validation.md` | reproducible commands, supported toolchains, package contents, equipment and remaining gates |
| `CHANGELOG.md` | intentional user-visible changes and qualification limits, never invented release publication |

Trace every advertised capability from exact-part source/ledger to implemented
public declaration, source procedure, executing independent test, shared CLI
route and runner evidence where applicable. Explain source-proved hardware-only
or inapplicable rows; missing required software is an open gate. Keep desired,
observed and verified settings distinct, cached versus live calls explicit,
and transport health independent of cable/negotiation/policy/IP readiness.
Document invalidation and accepted-write uncertainty with a practical recovery
example using the actual bounded owner API. No invented `begin`/`pollJob` aliases.

Compile/link/run documented portable quick starts from clean consumer inputs;
build/link SDK snippets through the real supported example, not standalone
shim headers. Check help against actual shared CLI and runner `--help`/dry plan;
use explicit placeholders for application pin/address/clock/fixture choices.
Avoid unattended infinite waits or implicit destructive diagnostics in quick
starts. Keep EtherCAT stack/timing and power delivery application-owned.

Run existing documentation/local-link checks and inspect representative generated
API pages. Fix stale names, missing anchors, nonexistent commands and package
assumptions. Document external link verification limits without treating a local
link checker as an online source freshness audit. Preserve vendor provenance
and historical failure evidence; route execution history to reports/progress.

Update source Doxygen only where the audit finds a real omission or mismatch;
fix code/contract together if documentation exposes a behavioral defect. Review
README, CHANGELOG and all affected guides, recording no-change reasons rather
than padding Unreleased. Record commands, claim coverage and unresolved gates in
`docs/reports/implementation/31-user-documentation.md`, validation and progress.
Run focused local checks for fixes; retain stage 29's actual CI SHA and do not
dispatch/check/wait for remote CI. Documentation completion cannot close stage 30.

## Acceptance and handoff

Build documented snippets/consumers, exercise CLI/runner help and check all local
links/version names. Trace every advertised feature and supported variant to
implementation and tests. A stage 30 block remains visible; documentation completion
does not close it.

Expected output: Accurate usable guides and complete claim-to-evidence mapping. Include
source documentation, relevant guides, feature/evidence matrix and progress updates.
Apply COMMON's completion rules; report actual commands, results and remaining gates.
Continue only when the concrete required inputs exist; COMMON explains how unrelated
gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
