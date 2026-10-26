# 34 - Close implementation, tooling and qualification claims

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-33 outputs; final COMPLETE requires all required
gates, including stage 30 where requested, actually closed.

At entry apply CONVENTIONS' names, Status/health, layout and simplicity rules;
reopen current relevant sibling sources. Keep source Doxygen and affected
README/CHANGELOG/guides current, verify locally and checkpoint/sync on this
execution's recorded branch.

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

## Concrete final evidence and handoff

Reopen current `../TCA9548A/tools/check_package.py` and
`tools/test_hil_artifacts.py`, `../TMP1x2/tools/check_doxygen.py` and registered
host/variant suites, and `../SCD41/tools/check_target_package_consumer.py` for
claims about clean consumers versus linked-only firmware. Inspect current
source, CLI/help, Python runner, workflows as source, CONTRIBUTING/AGENTS and
maintained guides, not just release checklists. Record sibling revisions/dirty
state, exact inspected symbols and adopted/rejected conventions.

The lead owns final claims/contracts/progress and Git. A useful independent
assignment is read-only claim-to-artifact review or a clean-consumer journey;
assign exact paths and acceptance, choose available models by complexity and
review all findings. Follow CONVENTIONS' simple names/types, Status/health
semantics and source Doxygen. Do not introduce new layers or API aliases merely
to make a final checklist look uniform.

Create `docs/reports/implementation/34-final-integration-review.md` with an
explicit evidence table. Each row names scope, command, input revision/hash,
environment, result/artifact and remaining limit. Keep these rows distinct:

| Layer | Required final accounting |
| --- | --- |
| Software coverage | exact supported part/revision/mode, complete register/field/procedure ledger, implemented typed behavior and open essential rows |
| Host behavior | registered test counts, independent vectors, strict builds/Release-active checks, CLI/Python negative verdict and restoration cases |
| Portable/package | actual ordinary consumer runs, artifact hash/content, extraction-only link/run, tests OFF and no Git/Python/Doxygen/sibling requirement |
| SDK firmware | actual SDK/target/example/configuration and linked image identity; missing build remains BLOCKED/NOT RUN |
| Remote CI | stage 29's actual tested SHA, jobs/URLs/results; explicitly list later substantive local changes outside that result |
| Physical | exact stage-30 fixture/part/revision/interface/mode evidence, failures and untested capabilities; no inheritance across variants |
| Application | ordinary networking and EtherCAT ownership example scope; actual master/servo timing and power delivery stay separate |

Review closure from the authoritative ledger denominator, not only implemented
methods. Every supported part needs all documented programmable features,
legal values, errors and procedure tests; optional runtime defaults and absent
first-board fixtures do not excuse software omissions. Hardware-only and
source-proved inapplicable entries need citations. Essential unresolved source,
implementation, adapter, CI or requested physical evidence prevents an overall
COMPLETE claim. Keep a finite pending list with dependency, owner and exact next
command/fixture; continue independent work that can close gates.

Select final local verification by changed inputs and stale evidence. Reuse
current real commands; rerun extraction consumers after source/export fixes,
SDK links after adapter/public/build changes, behavioral suites after logic
changes and Doxygen/local links after declarations/docs. Reusing an artifact is
valid only when its input identity and dependencies still match; state that
identity. Do not claim a package built before a fix covers final source. Do not
repeat unchanged full matrices or remote CI to make a documentation SHA green.

Reconcile source Doxygen, README precise support/install claims, CHANGELOG
Unreleased, API/architecture/variants/feature/testing/package/hardware guides,
CONTRIBUTING, validation and implementation-progress with those actual rows.
Retain archived vendor bytes, provenance/notices and historical failed evidence.
No new release tag, registry publication, sibling edits, automatic flashing or
fixture operation follows from this review.

Checkpoint coherent verified fixes with tests/docs during review, then commit
and sync the completed stage on the recorded execution branch. Fetch and safely
integrate without rewriting history, push and verify local/upstream equality
and zero ahead-behind after the final fetch; inspect remaining task changes.
Report exact branch/commit/sync state and any dirty unrelated work. No empty
review commit or self-referencing hash update loop. A blocked push remains
unsynced with local commit/error/next step, not a synchronized completion claim.

Deliver a short useful handoff: what actually works, precise supported variants,
portable/SDK examples and tooling, commands/results, CI-tested versus final
locally-tested revisions, physical/application limits and pending gates. Overall
status may be partial with checked completed work; documentation and a clean Git
state cannot substitute for required implementation or evidence.

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
