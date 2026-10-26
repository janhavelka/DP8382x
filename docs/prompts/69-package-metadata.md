# 69 - Finalize deterministic version and supported-package metadata

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md)
and [VERIFICATION](VERIFICATION.md). Prerequisites:
[10](10-version-tooling.md), [68](68-portable-owner-consumer.md), actual adapter
metadata and supported exact-part decisions. Preserve one existing version
source and generator rather than introducing another metadata layer.

## Current evidence and ownership

Reopen `TMP1x2/scripts/generate_version.py` (`_expected_outputs`, `_sync_outputs`,
`main`) and `tools/test_version_sync.py::VersionSyncTests`; inspect MB85RC's
metadata consistency tool, manifests, source lists and current agent/contributor
rules. Review this repository's actual license, remote and maintainer facts.
Record revisions, paths and adopted/adapted/rejected behavior. Do not copy sibling
company names, CODEOWNERS accounts, release versions or untested SDK ranges.

The lead owns version truth, source membership and support claims. Delegate
disjoint generator negative fixtures or read-only license/metadata review after
expected outputs freeze. Select available models for bounded inventory versus
generator/build complexity; shared metadata/contracts and Git stay lead-owned.

## Closed work unit

Finalize portable and selected ESP-IDF package metadata with `library.json` as
release version truth. Extend the existing `scripts/generate_version.py` to
regenerate `Version.h` and applicable CMake/component/Doxygen version fields
actually present. Do not hand-edit generated versions, bump a version per task,
or create manifests for hypothetical support.

`sync` is deterministic and idempotent; `check` detects stale output without
mutation. Invalid/missing metadata fails clearly. Ordinary source/archive
consumers receive required generated artifacts and do not invoke Python/Git/
Doxygen merely to configure/link. Keep firmware source SHA/dirty/SDK/build
identity separate from release version; a source archive without provenance
reports unknown instead of inheriting a parent checkout identity.

Declare accurate tested/minimum justified SDK constraints, actual required
dependencies and supported exact variants/interfaces. No broad guessed semver,
registry publication claim, invented repository URL or maintainer. Explicit
source selection must include necessary implementation and isolate SDK sources
from portable compilation. Review MIT project ownership and retained notices
for source actually reused; GPL reference implementations are not MIT source.

Start/finalize `docs/packaging.md` metadata and dependency sections with artifact
formats/layout expected by task 70, toolchain commands, version generation and
identity limits. Update CONTRIBUTING/ownership guidance using established facts.
Vendor archives and extracted reference text are evidence rather than build or
redistributable driver dependencies. Actual archive safety/extracted-consumer
execution remains the next task, not a completed claim here.

## Acceptance and checkpoint

Run isolated positive/negative generator fixtures for every actual output,
malformed metadata, stale generated fields, nonmutating check, repeated sync and
missing-Git archive identity. Run metadata/source membership and generated-drift
checks. Rebuild affected ordinary/SDK consumers if metadata changes their inputs;
record missing tools separately and do not substitute a successful shim.

Ship Python docstrings and generated/public-header Doxygen where contracts
change. Review README install/support scope, CHANGELOG, packaging/integration/
testing docs and CONTRIBUTING, recording no-change reasons where appropriate.
Run source-documentation and local checks, record actual identities/gates, then
commit/sync coherent verified units and completion on the same branch. Only
task 71 dispatches/checks remote CI; tags/publication remain separate scope.
