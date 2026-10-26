# 76 - Audit exported source contracts and executable Doxygen checks

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Inputs are actual public/core/SDK/tool source,
existing strict Doxygen tooling and reconciled guides from 74–75. Source
documentation began with implementation; this prompt closes residual omissions
and mismatches rather than providing permission to defer them.

## Current evidence and ownership

Reopen TUSB32x `Config.h` callback/validator Doxygen, MB85RC `Doxyfile` and
CONTRIBUTING for public/enum/parameter coverage, and TMP1x2
`tools/check_doxygen.py`, `tools/test_check_doxygen.py`, `Doxyfile` and CMake
registration. Inspect actual relevant public/source, CLI/runner contracts and
AGENTS. Record revisions, dirty state, paths and adopted/adapted/rejected rules;
do not import unrelated peer release/model/maintainer policies.

The lead owns shared headers, source-contract fixes and integration. Delegate
read-only public-header or tool-docstring review, or disjoint existing checker
fixture tests, with explicit writable/forbidden paths and independent acceptance.
Choose available models by mechanical/coding/ownership complexity and review
all contributions. Agents do not mutate Git, shared contracts or siblings.

## Work and acceptance

Walk every exported type/value/method in `include/DP8382x/Status.h`, `Config.h`,
`CommandTable.h`, `DP8382x.h`, generated `Version.h` and SDK-only public headers.
Verify file/type purpose, field units/ranges/validity, enum restrictions, all
parameters/results/errors, output-on-error, callback/context borrowing lifetime,
serialization/reentry, live versus cached I/O, aggregate blocking/deadline bounds,
reset/cancellation, uncertain writes and cleanup. `READY`, valid link and trusted
configuration have distinct meanings. Read actual implementation against each
claim; a plausible comment cannot repair incorrect behavior.

Explain non-obvious masks/order/effects with exact source sections beside actual
register/configuration/operation/diagnostic procedures. Use existing responsibility
modules where present; do not mandate new files merely to match a list. Review
Python session/protocol/artifact docstrings and one shared CLI grammar. Remove
obsolete aliases/scaffolding only after updating real consumers/tests; avoid
line-count refactors, class-per-register designs and duplicate checker layers.

Audit the existing Doxyfile/checker for missing public fields/enums/parameters,
actual member/page visibility and local links/assets, excluding archived vendor
inputs. Add meaningful adverse fixtures for missing public page, anchor or asset,
unintended private/vendor exposure, generation failure and tool timeout. Generate
and check in an owned temporary directory, propagate failure and verify cleanup
stays inside it. No warning suppression, blanket extraction, last-good-output
cache or publication subsystem hides missing documentation. Inspect representative
generated pages and confirm checker negatives execute; page presence alone is
not API coverage.

Ship confirmed source/contract defects with executing regressions and coordinated
Doxygen/README/CHANGELOG/guide/ledger updates. Record inspected no-change reasons.
Run strict documentation and focused host/tool checks; actual SDK/package links
follow changed interfaces/exports. Preserve physical/source gates and 71's
historical CI SHA without remote dispatch/poll/wait. Record findings, commands,
counts and limits in report, validation and progress. Check entry rules against
final diff, then commit/sync checked units and completion on the execution branch.
