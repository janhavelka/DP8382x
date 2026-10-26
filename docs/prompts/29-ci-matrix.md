# 29 - Establish the actual CI and local verification matrix

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-28; focused local tests have been required throughout.

## Fresh audit and owned work

Re-audit peer workflows and local verification entry points, test registration and
pinned dependency versions. Apply Projects/AGENTS.md, not a sibling's special CI
exception.

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

Follow the parent's major-change CI policy. When remote execution is available and
authorized, verify results for the actual final commit and fix failures. Workflow
configuration or older green builds are not execution evidence. Preserve existing
triggers/hooks/protection; missing access is a reported gate, not permission to bypass
CI or publish.

Update docs/testing.md and contributor instructions with exact supported commands,
focused-check mapping and hardware exclusions. Keep Windows usage compatible with
existing tools; use the managed PlatformIO wrapper if PlatformIO is actually needed
rather than installing another Core.

## Acceptance and handoff

Run focused local checks, workflow/config validation and the relevant complete matrix.
Record exact remote job URLs/SHA and terminal states when run; otherwise state
blocked/not-run and why. Re-run affected jobs after fixes, not unchanged matrices after
trivial prose edits.

Expected output: Executing, documented CI aligned with local verification. Include
source documentation, relevant guides, feature/evidence matrix and progress updates.
Apply COMMON's completion rules; report actual commands, results and remaining gates.
Continue only when the concrete required inputs exist; COMMON explains how unrelated
gates stay open.
