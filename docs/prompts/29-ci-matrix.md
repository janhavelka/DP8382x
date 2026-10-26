# 29 - Complete the dedicated CI checkpoint

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-28; focused local tests have been required throughout.

At entry apply CONVENTIONS' names, Status/health, layout and simplicity rules;
reopen current relevant sibling sources. Keep source Doxygen and affected
README/CHANGELOG/guides current, verify locally and checkpoint/sync on this
execution's recorded branch.

## Fresh audit and owned work

Re-audit peer workflows and local verification entry points, test registration and
pinned dependency versions. Apply root AGENTS' user-requested first-set exception:
this is the only stage that dispatches, checks or waits for remote CI. Review
sibling workflow source without checking sibling run status.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Create or complete a modest CI matrix for reference integrity, portable host tests,
CLI/Python runner tests, documentation/version guards, clean package consumers and
actual native ESP-IDF P4 examples. Use explicit supported compilers/SDKs and pinned
tools/actions. Add additional platform jobs only for actual implemented support.

Make local verification commands and workflow commands share the same checks; avoid two
unrelated sources of truth. Register every suite/source and fail zero-test or
missing-required-artifact conditions. Set job/time/log/artifact bounds, minimal
permissions and appropriate caches. Keep hardware-specific jobs opt-in to named
fixtures.

Validate Doxygen and docs links with useful warnings; don't suppress failures to obtain
green. Add necessary framework leakage, CLI/runner parity, metadata, package and
test-registration guards, backed by behavioral tests. Avoid copying a large firmware
build-profile framework.

Commit and sync the locally verified CI/source/test inputs on the recorded
execution branch, then verify the supported remote matrix for that actual commit.
Dispatch if necessary, inspect terminal results and fix confirmed failures.
Repeat affected jobs here only for those failures or changed inputs. Workflow
configuration or older green builds are not execution evidence. Preserve existing
triggers/hooks/protection; missing access is a reported gate, not permission to
bypass checks or publish. Continue independent work while access is unavailable.

Record the actual tested SHA/jobs/results for later handoff. Prompts 30-34 use
focused local verification and retain this CI result as evidence for its own SHA;
even substantive later fixes do not require another remote run during this first
execution. Do not restart this checkpoint merely because later source changed,
or imply its green result verifies that changed commit. A documentation-only
commit recording CI evidence does not silently inherit the tested SHA's result.

Update docs/testing.md and contributor instructions with exact supported commands,
focused-check mapping and hardware exclusions. Keep Windows usage compatible with
existing tools; use the managed PlatformIO wrapper if PlatformIO is actually needed
rather than installing another Core.

## Concrete executing job contract

Reopen `../TMP1x2/CMakeLists.txt` for explicit test registration/tests-OFF,
finite test timeouts and Python-only tests-ON; `tools/check_doxygen.py` and
`tools/test_check_doxygen.py` for generated public API/local-link checks; and
`../TCA9548A/.github/workflows/ci.yml` for source-pinned actions, permission,
time bounds and actual package/firmware commands. Compare
`../SCD41/.github/workflows/ci.yml` package-consumer registration and pinned
Doxygen installation/hash, not merely its job names. Record current revisions,
dirty state, exact checks and rejected unrelated S2/S3 I2C targets. Inspect
sibling CI source only; no sibling dispatch or remote-run checking.

The lead owns workflow/build registration and integration. Delegate a read-only
workflow-command parity audit or disjoint checker failure fixtures after freezing
entry points. Assign explicit paths, available models by complexity and no Git
or workflow mutations to reviewers. Use the simple existing local commands;
add a small `tools/verify_local.py` dispatcher only if it removes duplicated
selection rather than inventing a build-profile framework.

Populate `.github/workflows/ci.yml` with jobs whose outputs can prove a claim:

| Job responsibility | Required executable evidence |
| --- | --- |
| References/metadata | reference hashes, generator `check`, metadata/source membership, no generated drift |
| Portable behavior | real registered core/variant/operation suites, strict warnings, active Release checks, applicable Debug/sanitizer/second-compiler runs |
| CLI/Python | native grammar/owner/output suites, runner correlation/negative verdict/artifact tests, nonzero discovered case counts |
| Documentation | strict Doxygen plus local links/public visibility, actual maintained input selection |
| Package | bounded archive negatives, extracted tests-OFF consumer executes, exported implementation/source membership |
| Native SDK | actual pinned supported SDK/ESP32-P4 component and examples link, required image artifacts present |

Set explicit job/test/subprocess deadlines and bounded artifact retention.
Record compiler/SDK/tool versions; pin action commits and downloaded tool
hashes according to the established repo policy. Check caches include all
relevant dependency/toolchain inputs and do not inject a source-tree build into
package qualification. A cache hit, configured workflow, green skipped job or
zero discovered tests is not executable evidence. Keep required assertions,
documentation warnings and suite registration active. Optional hardware jobs
need an explicitly named fixture; no fixture means NOT RUN, not host CI PASS.

First run affected local inputs and inspect the staged complete CI change, then
commit/sync it on the recorded execution branch. Resolve the actual immutable
remote commit before dispatch/inspection. For each expected job record tested
SHA, workflow revision, run/job URL, matrix dimensions, command/artifact and
terminal conclusion in `docs/reports/implementation/29-ci-matrix.md` and
validation. Canceled, timed-out, missing and skipped required jobs cannot close
the gate. Fix confirmed failures, run focused local regressions, checkpoint/sync
and verify the affected remote inputs here. Do not poll unchanged green jobs.

Keep `ciTestedCommit` and the later evidence-document commit distinct in the
report, whether represented as fields or plain prose. A final local change to
record results does not require a self-referencing CI/commit loop or inherit
the previous run. Prompts 30-34 retain this actual SHA and use local checks even
for substantive fixes; never dispatch/poll CI merely to make the newest docs
commit match it during this first set. Do not change triggers/hooks/protection
or use skip markers to implement the exception.

Update Doxygen/source/Python documentation for any checker behavior fixed here;
README verification/support claims, CHANGELOG Unreleased, `docs/testing.md`,
CONTRIBUTING and progress must contain only implemented, executed commands and
separate host/package/SDK/CI/physical evidence. Missing access preserves checked
local work and an exact blocked remote gate; continue independent eligible work.

## Acceptance and handoff

Run focused local checks, workflow/config validation and the relevant complete matrix.
Record exact remote job URLs/SHA and terminal states; missing remote access stays
blocked/not-run. Re-run affected jobs here after actual fixes/input changes, not
unchanged matrices after trivial prose edits. Later final review needs no new
remote run under this first-set exception; required merge/release gates still apply.

Expected output: Executing, documented CI aligned with local verification. Include
source documentation, relevant guides, feature/evidence matrix and progress updates.
Apply COMMON's completion rules; report actual commands, results and remaining gates.
Continue only when the concrete required inputs exist; COMMON explains how unrelated
gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
