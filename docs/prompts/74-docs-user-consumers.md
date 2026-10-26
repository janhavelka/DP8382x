# 74 - Verify user quick starts and consumer journeys

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Required inputs are actual portable/SDK
examples, shared CLI/runner, consumer/package outputs from 68–70 and physical
results or explicit blocks from 72–73. Hardware blocks do not prevent guide
verification and cannot be closed by documentation.

## Current evidence and ownership

Reopen `../MB85RC/CONTRIBUTING.md` and maintained `docs/DEVICE_REFERENCE.md`,
`docs/IDF_PORT.md`, `docs/RELEASE_CHECKLIST.md`; TMP1x2 Doxygen checker/input
rules; and TCA9548A artifact tests for NOT RUN semantics. Inspect current source,
examples, CLI help, runner commands and AGENTS behind the prose. Record revisions,
dirty state, exact paths and adopted/adapted/rejected conventions; historical
reports and planned peer APIs do not prove user features.

The lead owns maintained README/API/architecture/integration claims. Delegate
read-only ordinary-user or IDF-owner journeys, or disjoint snippet/link checks,
with explicit paths and acceptance. Choose available models by verification
complexity; reviewers run/read the documented path and report defects rather
than rewrite shared contracts. Review all contributions; agents do not mutate
Git or siblings.

## Work and acceptance

Reconcile actual install/build/use instructions in existing authoritative
destinations instead of duplicating guides. README states precise implemented
support and links buildable examples. API/Doxygen explain stable names, errors,
output validity, callback lifetimes, live/cached effects and bounded operations.
Architecture/integration describe one configuration/MDIO owner and application
ownership of pins, PHY address, clocks, locks, time, scheduling and recovery.
Keep desired/observed/verified settings and transport health/link/configuration/
application readiness distinct; show actual bounded recovery after uncertainty,
without inventing `begin` or `pollJob` aliases.

Give concise ordinary C++ and native-IDF quick starts using actual buildable
code. Compile/link/run the portable path from clean consumer inputs, with tests
OFF and no source/sibling fallback. Build/link SDK snippets through the real
supported example and pinned SDK, not shim headers. Explain generic legal
10/100 versus the separate board 100/full profile. Use explicit application
pin/address/clock/fixture placeholders, finite waits and no implicit disruptive
diagnostics or automatic flashing.

Verify package/component consumption, supported SDK evidence/range, backend
timing restrictions, firmware identity, CLI grammar/help, runner help/dry plans,
fixture admission, artifacts and restoration verdicts. Document ordinary
Ethernet/IP and EtherCAT ownership boundaries without claiming either stack,
master/servo timing or power delivery belongs to this library. Keep optional
diagnostics opt-in and exact variant restrictions discoverable.

Run existing local links/docs/Doxygen checks, inspect representative generated
pages and fix stale names, anchors, commands and package assumptions. State online
freshness limits separately from local link validation. Ship source Doxygen or
code/test corrections when a guide exposes a real defect; review README,
CHANGELOG and affected guides, recording no-change reasons instead of padding
Unreleased. Preserve vendor provenance and failed evidence; keep execution
history in reports/progress. Record commands/counts and remaining limits in this
prompt's report, validation and progress. Check final diff, commit/sync checked
units on the execution branch, and retain 71's CI SHA without remote run checks.
