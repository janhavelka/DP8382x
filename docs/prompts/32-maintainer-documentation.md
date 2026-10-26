# 32 - Audit source documentation and maintenance ownership

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-31 outputs; hardware blocks do not prevent this
independent review.

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
