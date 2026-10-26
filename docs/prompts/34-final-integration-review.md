# 34 - Close implementation, tooling and qualification claims

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-33 outputs; final COMPLETE requires all required
gates, including stage 30 where requested, actually closed.

## Fresh audit and owned work

Revisit current peer CLI/runner/packaging evidence and FieldCore handoff conventions.
Check final source, generated files, build registration and artifacts rather than
relying on stage summaries.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Apply the full-chip closure gate: every publicly documented programmable feature
of each supported exact part is implemented and tested, with no unexplained
register/field/procedure omissions. Review the source ledger itself, not only the
existing API list. Runtime opt-in and an absent board fixture cannot justify
omitting software support. Supported-but-unimplemented or unresolved essential
features remain open gates; do not report the complete library as delivered.

Review the finished user journey: portable consumer, native P4 adapter, ordinary
networking example, EtherCAT ownership example, shared CLI, Python runner, packages, CI
and documentation. Trace every requested aspect and family feature to its current
maintained implementation/evidence or explicit unsupported/unqualified status.

Run the relevant final integration checks against the final tree/commit. Verify actual
clean package consumers, SDK links, CLI/runner protocol, generated metadata and vendor
exclusion. Apply focused local checks to substantive review fixes; this first
set does not dispatch/check/wait for remote CI here. Report the final locally
tested revision separately from prompt 29's CI-tested SHA. Do not claim old CI
or a package built before fixes covers the new result.

Check family claims per exact part/revision/interface and the difference between code
support and physical qualification. Required unresolved silicon, SDK, remote or hardware
gates prevent an overall complete claim. Continue work that can close them; where
external input is necessary, leave a concrete pending list and reproducible next
command/fixture. No fake green, hidden skipped tests or successful hardware stubs.

Reconcile implementation-progress, validation, feature matrix, README/changelog and
final report. Commit and sync the final checked work on the same execution
branch and record actual branch/revision/dirty/sync state. There is no implied
publication, release tags or edits to sibling
consumers. The user receives a concrete reviewable local library, not an invented
registry release.

Summarize what works, examples/tools provided, tests actually run and remaining
application/physical limits. The library must remain useful to generic MCU owners and
ordinary internet/network applications as well as the first EtherCAT application. Power
delivery remains outside its API and examples.

## Acceptance and handoff

Use requirement-to-evidence closure with PASS/FAIL/NOT RUN/BLOCKED per layer. Validate
all documentation links and final diff. Confirm no remaining required work is mislabeled
complete, and no host/SDK result is described as physical or servo-system qualification.

Expected output: Truthful final implementation handoff and qualification status. Include
source documentation, relevant guides, feature/evidence matrix and progress updates.
Apply COMMON's completion rules; report actual commands, results and remaining gates.
Continue only when the concrete required inputs exist; COMMON explains how unrelated
gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
