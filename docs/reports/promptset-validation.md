# Promptset validation

Planning review on 2026-10-26 for the [34-stage promptset](../prompts/README.md).
Starting DP8382x revision: `35ced9bb9d0b05c1c0b41ecff7cee5e757bcb163`.
This report concerns documentation and implementation instructions only.

## Scope and evidence

The lead and three delegated reviewers inspected sibling driver conventions,
FieldCore-node promptsets/owners and exact PHY/SDK boundaries. Reports record
concrete paths, revisions, dirty-state caveats and depth of review. Siblings
remained read-only; no build, test, Git mutation or hardware run was performed
there. The primary library itself remains unimplemented.

The resulting promptset covers portable C++17 and native IDF ownership, exact
DP83826I behavior, family expansion, health/status, examples, CLI, Python HIL,
packaging/versioning, CI, source/user documentation and separate qualification.
All stages require a fresh relevant sibling audit and scoped verification.
The [coverage map](../prompts/COVERAGE.md) links the request to stages and evidence.

## Independent review and corrections

Three read-only reviews examined the completed promptset after drafting:

| Review | Finding | Resolution |
| --- | --- | --- |
| FieldCore/executability | Optional silicon or unavailable remote-CI gates could block unrelated later work through blanket prerequisite ranges | COMMON, index and every stage footer now require concrete input outputs while retaining unrelated gates; final COMPLETE still requires required gates closed |
| Sibling/health | Cable disconnect wording incorrectly prohibited success/activity counters changing during real MDIO reads | Stage 11 now prohibits transport-failure increments caused merely by cable-down, while real reads count and rejected input remains bus-silent |
| Sibling/tooling | Artifact integrity/preservation could be more concrete | CLI contract and stage 20 require exclusive output bundles, retained prior runs, sizes/SHA-256 and collision/write/flush/close failure tests |
| PHY/family | DP83822 planning omitted RGMII | Stage 26 and engineering contract include RGMII and exact timing/delay configuration, distinct from any particular MAC's support |
| PHY/forced modes | Robust Auto-MDIX/manual MDI/MDIX needed explicit coverage | Stage 09 requires source-backed Force Mode setup, reset restoration and failure tests |
| PHY/MAC boundary | Pause advertisement, RX/TX resolution and notification ownership were implicit | Stages 03/09/10/15 now specify symmetric/asymmetric pause, full-duplex validity and separate MAC/application policy |
| PHY/cancellation | Bus-silent cancellation needed a precise relationship to diagnostic cleanup | Stages 04/08 and engineering contract require subsequent owner work with finite cleanup budget and preserved unresolved effects |

The lead checked the added PHY claims in archived SNLS647I and SNLS505H.
All three reviewers rechecked their corrected sections and reported no remaining
findings within that review scope. Review does not establish silicon behavior
beyond the cited source, and does not close future implementation/hardware gates.

## Local checks for the initial promptset

Checks ran locally with Python 3.12.10:

| Check | Result |
| --- | --- |
| `python tools/verify_references.py` | PASS: 27 vendor artifacts (43,240,385 bytes), 25 text extracts, 209 local links in 61 Markdown files |
| `git diff --check` | PASS: no tracked-diff whitespace errors |
| One-off changed/new-file inspection | PASS: all 54 changed/new files UTF-8, LF, final newline and no trailing whitespace; includes untracked new documents |
| One-off prompt manifest check | PASS: exactly 01-34, mandatory entry/audit/work/acceptance sections, each indexed once and all 34 progress rows NOT STARTED |
| One-off concrete sibling-path check | PASS: 26 paths in SOURCE_MAP and the additional family/tooling audit exist |

The one-off checks inspected the documentation; they are not new driver tests
or a claimed installed validation tool. Reference archive bytes and provenance
were preserved. Focused documentation verification is appropriate under the
parent policy; no remote CI dispatch was required for this planning-only change.

## Complete-feature and public-source follow-up

The user clarified that the first application's limited use must not limit
library scope. Added [FULL_COVERAGE](../prompts/FULL_COVERAGE.md),
[public manufacturer-source research](../reference/public-research.md) and the
[DP83826I inventory](../reference/dp83826i-feature-inventory.md). The contract now
requires every documented programmable feature, including runtime opt-in
functions, with complete register/field/procedure, API, CLI and test coverage.
Raw access alone cannot close multistep features. Hardware-only/absent silicon,
unavailable fixtures and unfinished software have distinct dispositions.

The source sweep inspected additional TI ICSS-EMAC and Zephyr drivers, TI Linux
variant evidence, upstream Zephyr DP83825 and official register-tool guides.
The source catalog records immutable revisions, per-file licenses, incomplete
driver behavior and concrete contradictions. No source payloads or installers
were imported, and no reference archive bytes were replaced.

A reviewer checked FULL_COVERAGE against the request. A separate focused review
found one remaining stage-12 phrase conflating unresolved evidence and unsupported
silicon; it was corrected to an open implementation/completion gate. The full
inventory exposes further datasheet/app-note conflicts for future source closure;
it does not claim those issues are solved by collecting register addresses.

Follow-up validation with Python 3.12.10 passed:

- All **79** catalogue section/address pairs match the archived datasheet's
  detailed register headings: 30 direct Clause 22 and 49 extended entries.
- Reference integrity and local Markdown links pass with all 27 original vendor
  artifacts and 25 derived text extracts preserved.
- All 34 prompt files remain indexed once and all progress rows NOT STARTED.
- All 59 changed/new Markdown files pass UTF-8/LF/final-newline/trailing-whitespace
  inspection, including untracked files; `git diff --check` also passes.

These are documentation/source-inventory checks, not driver, SDK or HIL results.

## Agent workflow follow-up

The [guidance audit](agent-workflow-audit.md) records the sibling rules selected
for this repository and the user's explicit checkpoint/CI instructions. An
independent read-only reviewer found no actionable contradictions across root
AGENTS, shared contracts, progress and the CI/maintenance/final-review prompts.

Focused local validation passed:

- `python tools/verify_references.py`: all 27 vendor artifacts and 25 extracts
  remain intact; local links in all 65 Markdown documents resolve.
- `git diff --check`: no tracked-diff whitespace errors.
- One-off inspection: all 46 changed/new files are UTF-8/LF, have a final newline
  and contain no trailing whitespace, including the new audit report.
- One-off manifest/policy review: exactly 34 numbered prompts, each indexed once
  with explicit per-prompt and intermediate commit/sync instructions; all 34
  implementation stages remain NOT STARTED. Shared rules retain local checks and
  place first-set remote CI only at prompt 29.

These checks cover documentation only. No implementation stage or remote CI run
was started. Git checkpoint and synchronization outcomes are reported separately
in the handoff; writing a workflow rule is not proof of a successful push.

## Concrete-conventions and comprehensive-prompt follow-up

The user requested more concrete instructions grounded in actual sibling code,
task-based model selection, simple engineering language, maintainability and
Doxygen/general documentation at every stage. All 34 numbered prompts were
expanded, adding 2,206 lines of stage-specific source comparisons, implementation
responsibilities, behavior/failure cases and acceptance/documentation work.
[CONVENTIONS](../prompts/CONVENTIONS.md) records the selected planned Status,
health and function names, file responsibilities and source-documentation rules.
Exact signatures remain stage-04 decisions; these are not shipped APIs.

The three focused audit reports preserve paths, symbols, revisions, observed
behavior and the chosen PHY adaptations. The core source/procedure task used
`gpt-6-astra` with high reasoning; consumer/tooling and family/maintenance tasks
used `gpt-6.1-sol` with high reasoning. Their scopes included state, ownership,
failure evidence and cross-stage contracts. The lead owned shared contracts and
Git; agents wrote disjoint prompt groups and performed read-only cross-reviews.
Simple manifest/format checks ran locally without another agent. These model
names record this execution, not requirements for future agents.

Independent review identified and resolved these ambiguities:

| Finding | Resolution |
| --- | --- |
| Generated API pages could be mistaken for source-documentation coverage | Stage 05/CONVENTIONS require negative missing-documentation and invalid-parameter fixtures; TMP1x2's `EXTRACT_ALL` limitation is explicit |
| Identity failure could conflate missing evidence, wrong part and MDIO error | CONVENTIONS/04/07 select distinct mismatch/not-found/transport semantics and retain observed identity/reasons |
| Shared Git wording could prohibit authorized disjoint agents | Stage 01 explicitly allows shared-branch work while reserving Git mutations to the lead |
| Family identity rejection was described as zero-I/O | Stages 24/26 allow bounded identity reads, prohibit subsequent writes/feature I/O, and reserve zero callbacks for pure preflight |
| Reset recovery could require already trusted hardware configuration | Stage 08 restores validated owned desired settings, then verifies hardware to re-establish trust |
| Optional initialization wording could forbid explicitly requested settings | Stage 14 prohibits hidden/default activation while allowing requested settings and documented replay |

Focused local checks passed: reference integrity and all local Markdown links;
`git diff --check`; UTF-8/LF/final-newline/trailing-whitespace checks including
new files; and a one-off manifest inspection confirming all 34 stages are indexed
once, carry entry rules, CONVENTIONS, Doxygen/general-doc requirements and their
commit/sync footer. All 34 implementation rows remain NOT STARTED. First-set
remote CI still belongs only to prompt 29, with local checks throughout.
The integrated repository has 69 Markdown documents and preserves all 27 vendor
artifacts and 25 extracts. No implementation, SDK, Doxygen generation, remote CI
or hardware run was performed for this documentation-only change.

Commit/sync applies to this completed maintenance work too. The actual new
checkpoint and verified upstream result belong in its final handoff and next
progress update; the preceding checkpoint is recorded in implementation progress.

## Correctness and simplicity review

The [correctness audit](prompt-correctness-audit.md) records a later read-only
silicon/family/SDK review against original TI documents and the pinned IDF source.
Confirmed prompt defects were corrected, reviewers rechecked them, and remaining
EEE wire-address and A-generation strap/EMC conflicts are explicitly open.
Short calls, operation/lock ownership, shared family records and local tool scope
were simplified without reducing required functionality or documentation.

The new-branch rule was committed/synced separately as `21a22c4`. Final local
checks cover reference integrity, local links, changed/new-file formatting,
prompt manifest/checkpoint rules and preserved NOT STARTED statuses. All 27
vendor artifacts and 25 extracts remain unchanged. These are documentation/source
review results, not driver/SDK/Doxygen/CI/hardware results. Exact final commit and
verified sync state are reported in the handoff.

## Limits

- No driver, library API, firmware, CLI, runner, build metadata or CI was created.
- No host driver tests, native SDK build, remote CI or physical HIL ran for this
  documentation-only task. Planned commands are instructions, not test evidence.
- PHYSTS forced-mode and RECR interpretation conflicts remain named silicon
  review items for implementation; unsupported derived behavior must stay gated.
- No EtherCAT master/servo functionality, electrical compliance or power delivery
  is claimed. No sibling changes or package publication were made.
- The initial planning passes did not perform a remote push. Subsequent
  user-authorized checkpoint rules are documented in the
  [workflow follow-up](agent-workflow-audit.md); actual commit/sync outcomes belong
  in their handoffs and subsequent progress updates. Publication is separate.
