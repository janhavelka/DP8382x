# 32 - Audit source documentation and maintenance ownership

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-31 outputs; hardware blocks do not prevent this
independent review.

At entry apply CONVENTIONS' names, Status/health, layout and simplicity rules;
reopen current relevant sibling sources. Keep source Doxygen and affected
README/CHANGELOG/guides current, verify locally and checkpoint/sync on this
execution's recorded branch.

## Fresh audit and owned work

Inspect current peer Doxygen/API contracts, CONTRIBUTING, AGENTS, version wrappers and
reviewer ownership. Adopt only facts that apply to this repository.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Review every exported type and method at its declaration: units, legal ranges,
callback/context lifetime, ownership/serialization, live versus cached effects, blocking
bounds, output-on-error, cancellation/reset consequences and capability restrictions.
Add useful explanations beside non-obvious register sequences and access masks with
exact source citations.

Review Python session/artifact docstrings and CLI grammar contracts. Make maintenance
commands exact and tested, including version generation, focused tests, package checks,
documentation, CI and HIL evidence handling. Keep runtime prompt numbers and temporary
milestone names out of source/public APIs.

Update root AGENTS for implemented reality while retaining sibling read-only, preserved
concurrent work, one Git owner and fresh peer audit requirements. Preserve the
user-authorized per-prompt/intermediate commit/sync checkpoints, single execution
branch and first-set remote-CI-only-at-29 exception; parent policy resumes afterward.
Add CODEOWNERS only from verified maintainer information. Do not import unrelated
model, release or fixture permissions.

Check organization and simplicity: one authoritative mask/procedure, one configuration
owner, one shared CLI grammar and one verification source. Remove obsolete scaffolding
or duplicate docs only when real consumers are updated; preserve unmodified reference
payloads and required notices.

Review dynamic allocation/exception/tool dependency claims against actual core and
adapter builds. Document deliberate framework-only allocation and SDK limitations
clearly.

## Concrete declaration and maintenance audit

Reopen `../TUSB32x/include/TUSB32x/Config.h` callback/validator Doxygen and
`CONTRIBUTING.md` for explicit planned-versus-implemented and dependency-free
consumer commands; `../MB85RC/Doxyfile`/`CONTRIBUTING.md` for strict public/enum/
parameter documentation; and `../TMP1x2/tools/check_doxygen.py`,
`tools/test_check_doxygen.py`, `Doxyfile` and registered CMake tests for executable
API/link checks. Record current revisions and dirty source. Do not copy sibling
remote-CI stage numbers, release policies, maintainer identities or model pins.
Read current AGENTS, source/examples/CLI/runner and source/check registration
behind guidance; inspect changed workflow definitions without remote run queries.

The lead owns shared docs/architecture/AGENTS and source-contract fixes. Delegate
read-only public-header and Python/tool maintainability reviews with explicit
scope and independent findings; a disjoint checker-fixture improvement may have
its own writable test file. Choose available models for reasoning complexity,
forbid shared contract/Git mutation and review every contribution. Source
documentation started with implementation; this stage closes residual gaps.

Walk `include/DP8382x/Status.h`, `Config.h`, `CommandTable.h`, `DP8382x.h`,
generated `Version.h`, SDK-only headers and each exported value/method. Check
file/type purpose; field units/ranges/validity; enum restrictions; complete
parameters/results/errors; borrow lifetime; owner/context/reentry; cached/live
I/O; whole-transaction serialization; blocking/deadline bounds; cancellation,
reset, uncertainty and cleanup semantics. `READY`, valid link and trusted
configuration must have separate meanings. Review actual source against these
comments; a plausible Doxygen block cannot fix wrong behavior.

Use exact source citations beside non-obvious command masks/procedures in
`src/DP8382xRegisters.cpp`, `DP8382xConfiguration.cpp`,
`DP8382xOperations.cpp` and diagnostics where these modules exist. Explain order
and effects, not line-by-line restatements. Keep one implementation of each
procedure and one CLI grammar in `examples/common/`. Remove abandoned layers,
duplicate aliases and scaffolding only after updating real consumers/tests.
Avoid class/file-per-register refactors and arbitrary line-count targets.

Audit existing `Doxyfile`/`tools/check_doxygen.py` rather than creating a second
checker. Require useful warnings for missing public fields/enums/parameters,
actual public-page/member visibility and local links/assets; exclude archived
vendor inputs. Test a missing public page, missing anchor/asset, unintended
private/vendor exposure, failed generation and tool timeout. Generate in a
controlled temporary directory; preserve the last validated output on failure
and constrain cleanup to that owned directory. No warning suppression or blanket
extraction to disguise missing documentation. These checks supplement source
review and do not prove hardware behavior.

Make CONTRIBUTING and AGENTS commands execute from a clean checkout: version
`sync/check`, host Debug/Release checks, portable tests-OFF consumer, actual SDK
builds, strict Doxygen, package tests and bounded HIL preparation. Keep consumer
builds free of Git/Python/Doxygen and tests/tools optional. Document managed
Windows PlatformIO only if used. Verify source/package/license ownership before
adding author accounts or CODEOWNERS; absent facts remain absent.

Preserve the execution branch/base, lead-only Git, coherent intraprompt and
per-prompt commit/sync workflow and first-set CI-only-at-29 rule. Distinguish
post-first-set parent policy and actual CI-tested SHA from current local evidence.
Do not grant physical fixture or publication authority through maintainer prose.
Update Python protocol/artifact docstrings, README/CHANGELOG/affected guides,
source Doxygen and `docs/reports/implementation/32-maintainer-documentation.md`
with real changes and no-change reasons. Record commands/counts/findings in
validation/progress; run only affected local checks and leave hardware gates open.

## Acceptance and handoff

Run Doxygen/docs checks without weakening warnings, source/metadata guards, applicable
tests and diff review. Confirm new public docs match behavior and no planned API is
accidentally advertised. Record reviewer findings and fixes with evidence.

Expected output: Maintainable source contracts and accurate agent/contributor guidance.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
