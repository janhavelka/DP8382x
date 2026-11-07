# 71 - Execute the single dedicated remote CI checkpoint

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites: relevant software/local checks
through [70](70-package-export-consumers.md), including actual SDK consumers.
Under root AGENTS' first-execution exception, this is the only task that
dispatches, polls, waits for or checks remote CI. Later tasks use focused local
verification even for substantive fixes and retain CI evidence for its own SHA.

## Current evidence and ownership

Reopen TMP1x2 CMake registration/tests-OFF, finite timeouts and tests-ON Python,
`tools/check_doxygen.py`/`tools/test_check_doxygen.py`, TCA9548A's pinned
workflow actions/permissions/package/firmware commands, and SCD41
package-consumer and Doxygen installation/hash registration. Record current
revisions, dirty state, exact paths and adopted/adapted/rejected choices.
Sibling workflow audit means source inspection, never dispatch or sibling remote
status checking.

The lead owns workflows, test/source registration and integration.

## Closed work unit

Create/complete a modest `.github/workflows/ci.yml` using real supported
compilers/SDKs, pinned action commits/tools and verified downloaded hashes.
Local and workflow commands execute the same checks; a small local dispatcher is
justified only if it removes duplicated selection. Do not create a firmware
profile framework or import I2C S2/S3 matrices.

Required executing responsibilities are reference/metadata/version drift;
portable core/variant/operation behavior with strict warnings, active optimized
assertions and applicable sanitizer/second compiler; native CLI/Python
correlation, verdict/artifact tests; strict Doxygen/local links/public
visibility; bounded archive negatives, extracted and relocated-installed
executing tests-OFF consumers; actual pinned ESP32-P4 component/examples linked
with required image artifacts.

Register every suite/source and reject zero cases, missing artifacts, skipped
required jobs or disabled assertions. Keep documentation warnings meaningful.
Set finite job/test/subprocess/output bounds, minimal permissions, bounded
artifact retention and caches keyed by actual dependency/toolchain inputs.
Caches cannot inject source-tree builds into package qualification. Hardware
jobs are opt-in to explicitly named fixtures; no fixture is NOT RUN, not hosted
hardware PASS.

Create ci.yml with `push` on all branches, `pull_request` and
`workflow_dispatch`. Run affected local checks, inspect/stage and commit/sync
inputs before remote verification. Find the push run with
`gh run list --commit <sha>`; workflow_dispatch needs the file on the default
branch, so do not merge main merely to enable dispatch. Inspect terminal jobs
for the immutable SHA, fix first-run Linux/toolchain failures here, checkpoint
and rerun affected inputs. Do not repoll unchanged green jobs or bypass
protection; report unavailable access precisely.

## Acceptance and checkpoint

Record tested SHA, workflow revision, run/job URLs, dimensions, versions,
commands/artifacts and terminal results. Canceled/timed-out/missing/skipped
required jobs cannot close the gate. Keep the tested SHA distinct from the
later evidence-document commit; no self-referencing CI loop. Later changes do
not inherit this SHA's green result or require a new first-set remote run.

Preserve automatic triggers, hooks and merge/release checks; no skip markers.
Update Doxygen/docstrings for checker fixes, README, CHANGELOG, testing and
contributor guides with real commands and separate host/package/SDK/CI/physical
layers. Record progress/report evidence and actual sync state; commit/sync
checked units and completion on the same branch, including final evidence docs.
