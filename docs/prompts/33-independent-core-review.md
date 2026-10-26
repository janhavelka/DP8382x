# 33 - Review silicon correctness and ownership independently

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-32 software outputs; retain any hardware/remote
evidence blocks.

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
