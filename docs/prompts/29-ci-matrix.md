# 29 - Complete the dedicated CI checkpoint

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-28; focused local tests have been required throughout.

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
