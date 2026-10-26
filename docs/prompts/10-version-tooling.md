# 10 - Add deterministic version metadata and generation

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[VERIFICATION](VERIFICATION.md). Prerequisite:
[9](9-portable-source-foundation.md)'s actual portable target and checks.
Briefly record owned files, live-source audit, documentation, validation and
same-branch commit/sync obligations.

## Complete outcome

Make `library.json` the version truth with working
`scripts/generate_version.py sync` and `check`, generated
`include/DP8382x/Version.h`, tests and tool documentation. Metadata describes
existing outputs only. Later packaging adds applicable SDK/package fields;
this stage is a complete version workflow, not speculative publication setup.

## Work and current precedents

Reopen TMP1x2 `scripts/generate_version.py`, `library.json`, generated Version
header and relevant tests; compare MB85RC/24Cxx metadata checks and ordinary
CMake consumers. Read their generated-file/agent guidance. Record current
paths/revisions and concrete adopted/adapted/rejected rules, including failures
not covered by a sibling test.

Use deterministic output and strict metadata/version parsing. Specify supported
version syntax, missing/mistyped/duplicate data behavior and stable newline/
encoding. `sync` produces all applicable outputs predictably and idempotently;
`check` is read-only and returns nonzero for stale/missing/invalid output. Fully
validate and prepare output before replacement so bad metadata cannot leave
partially updated generated files. Handle I/O failures explicitly with small,
maintainable code rather than an artifact-management framework.

Preserve the repository's actual license/owner/remote facts. Do not fill absent
metadata with invented maintainer handles or URLs, claim Arduino/PlatformIO
support from sibling matrices, publish packages or increment version per prompt.
Only generate manifests for build paths that exist. Keep semantic version
separate from future flashed-image Git identity; archive builds must report
unknown Git identity when it cannot be established. Define how later examples
refresh identity incrementally without requiring Git for ordinary core use.

Normal CMake consumers include committed generated output. They must not run
Python/generation, discover Git, invoke Doxygen or depend on sibling paths at
configure/build/runtime. Keep SDK-specific metadata out until that adapter
exists. If build version fields are already present, reconcile them from the
same validated source rather than creating another hand-maintained authority.

## Acceptance and documentation

Test valid versions, invalid syntax/type/missing keys, stale/missing generated
output, deterministic repeated sync, check mode's no-write behavior, failed
output writes and invalid input preserving previous valid bytes. Use temporary
fixtures; never damage committed output to prove failure. Run actual sync/check
and compare the second sync diff. Rebuild the portable tests/consumer only as
affected, proving consumption remains independent of generation tools.

Update source Doxygen for real version declarations, script/module help and
contributor/build/tool guides; review README/CHANGELOG and record no-change
reasons where appropriate. Run existing documentation/reference/diff checks,
record commands/results, source findings and generated-file ownership in the
stage report and progress. Support remains the implemented passive foundation;
version tooling does not establish a driver or SDK capability.

Use a capable coding model for generator/error handling, a lighter model for
bounded metadata inventory or an independent reviewer for preservation cases.
Assign disjoint paths after freezing generated-file ownership; the lead reviews
all results and alone mutates Git. Record available model choice/rationale.
Commit and sync the checked task and coherent verified units on the same branch.
Remote CI remains reserved for prompt 71.
