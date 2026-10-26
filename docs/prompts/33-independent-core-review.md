# 33 - Review silicon correctness and ownership independently

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-32 software outputs; retain any hardware/remote
evidence blocks.

At entry apply CONVENTIONS' names, Status/health, layout and simplicity rules;
reopen current relevant sibling sources. Keep source Doxygen and affected
README/CHANGELOG/guides current, verify locally and checkpoint/sync on this
execution's recorded branch.

## Fresh audit and owned work

Freshly sample relevant peer code and exact TI/SDK source again; do not review only
generated plans or previous audit conclusions.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Review completeness against the exhaustive authoritative register/field/procedure
ledger, including features absent from partial TI drivers and documented recipes
outside summary tables. Compare the public/manufacturer source catalog, exact
silicon evidence, typed APIs, commands and tests; do not let the existing source
tree define a smaller review denominator. Unsupported first-board wiring is not
a justification for missing portable software functionality.

Conduct a finite independent review of actual source and tests against the exact-part
register/procedure evidence. Delegate read-only silicon, ownership and test-oracle
reviews where useful, with the lead owning fixes and shared files. Reviewers must
inspect final implementation, not assume earlier stage completion proves correctness.

Trace every accepted part/mode, register access attribute, reset/restore path, 10 Mbit
setup, advertisement/forced mode, link observation and diagnostic through success and
injected failures. Reconcile PHYSTS/RECR or other remaining ambiguities; withhold
derived claims where evidence remains insufficient.

Inspect actual IDF base/adapter interoperation, all raw/control entry points, atomic
MMD/RMW scope, lock order, callback bounds and teardown. Look for duplicate
polling/configuration/MAC notifications and accidental external-writer trust. Check
app-only wiring/policy and absent framework dependencies in core.

Audit tests for circular oracles, unused suites, inactive Release asserts, fake hardware
success, wrong traffic counting, stale result acceptance and no-I/O claims verified only
by grep. Assess simplicity and remove speculative abstractions when their removal
improves clarity without losing behavior.

Fix confirmed defects with focused regression tests and update contracts/docs/matrix
together. Use focused local checks for fixes under the first-set CI exception;
do not dispatch/check/wait for remote CI here. Retain prompt 29's actual tested
SHA without claiming it covers later changes. Do not broaden hardware claims
because a review passed.

## Concrete independent review assignments

Freeze the reviewed local revision/dirty patch and public contract before
delegation. Assign read-only reviewers disjoint questions with exact source and
test paths, forbidden mutations and an evidence-based finding format. Use
available models suited to difficult silicon/ownership reasoning; do not hard-code
model versions from sibling guidance or launch agents merely to satisfy a count.
The lead owns all source/contracts/progress fixes, integration and Git. Reviewers
must inspect source and primary evidence, not endorse earlier reports.

Reopen `../TMP1x2/src/TMP1x2.cpp::track/terminalTransportStatus`, public
`HealthSnapshot` and variant tests; `../MB85RC/src/MB85RC.cpp::_selectVariant`
and `CommandTable.h::VariantInfo`; and relevant peer operation/runner tests for
the reviewed question. Record revisions/dirty state and adopted/rejected
conventions. Inspect current test registration and changed workflows as source,
without dispatching or checking remote CI. Apply CONVENTIONS' selected names,
Status meanings, explicit ownership and simple implementation.

Use a finite review matrix, expanding it only for concrete findings:

| Review | Trace and required challenge |
| --- | --- |
| Silicon completeness | every exact-part ledger field/procedure to source/API/CLI/test; independently recheck authority/reset/access attributes, recipe-only addresses and unsupported dispositions |
| Configuration and link | ID/mode/revision, reset kinds/straps/restore, 10BASE-Te, MDIX/forced/advertised modes, pause resolution, copper/fiber/interfaces, valid versus stale/no-link observations |
| Ownership and bounds | full MMD/RMW owner across core/base/raw/ioctl/CLI, lock order, SDK controller/lock waits, terminal callbacks, borrowed context, teardown and deadlines including cleanup |
| State and effects | accepted-write/error, partial apply, external writer/raw invalidation, destructive-status collector, cancellation before/after writes, retained result/token, unresolved hardware effects |
| Evidence and maintainability | independent literal oracles, registered/runtime Release-active tests, actual SDK/package link claims, runner correlation/restore/artifact failures, one procedure/grammar and no speculative layers |

For each claim start with the exact TI/SDK evidence, derive expected behavior
independently, then compare actual code and tests. A production mask used on both
sides of a test cannot establish correctness. Trace every exit of changed
procedures, including final write failure, failed cleanup and exhausted budget.
Check zero-I/O properties with callback counts/traces, not grep alone. Challenge
accidental conflation of transport health, link, trusted configuration and
application policy. Mock link-up and successful serial parsing are not packets
or hardware timing; syntax shims are not real SDK links.

Findings need severity, exact part/mode/source section, affected symbol/path,
reproducible case, expected versus actual behavior and closure criterion.
Mark each fixed with regression evidence, rejected with a source-backed reason,
or OPEN with required evidence/next step. Absence of reported findings is not
proof of complete coverage; retain scope and unavailable review inputs explicitly.
Unresolved essential register interpretations, SDK bounds or software coverage
remain gates rather than being rewritten as unsupported silicon.

Fix confirmed defects with focused executing tests and coordinated Doxygen,
README/CHANGELOG/relevant guides and ledger changes. Recheck affected real SDK
builds/package extraction if public contracts/source membership changed; do not
rerun unchanged full matrices. Produce
`docs/reports/implementation/33-independent-core-review.md` with reviewer scope,
revision, findings, fixes, commands/counts and residual risk, plus validation and
progress. Retain stage 29's CI-tested SHA and physical evidence boundaries; no
remote CI dispatch/poll/wait here even for substantive fixes in this first set.

## Acceptance and handoff

Close every finding with source/test evidence or an explicit unresolved gate. Run
affected host/SDK/local verification checks for the actual fixed revision. Revalidate packages if
public/source membership changed. Produce a review report listing scope, findings,
resolutions and residual risks.

Expected output: Independently reviewed core with evidenced corrections and explicit
gaps. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
