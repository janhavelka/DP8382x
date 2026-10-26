# DP8382x implementation promptset

Prepared on **2026-10-26** after auditing the local chip libraries and FieldCore-node.
This set contains **34 ordered prompts** to create a general microcontroller PHY
library. **The driver, examples, CLI, tests and runners are not implemented yet.**
The audit and promptset are the deliverables of this preparation.

The result must be a portable C++17 core with an ESP-IDF adapter, initially for
DP83826I/DP83826IRHBR and ESP32-P4, followed by explicit family qualification.
Ordinary Ethernet/IP and an application-owned EtherCAT master are separate
consumers. Generic 10/100 operation remains available; the initial loading-machine
profile advertises 100/full only. Power delivery is outside the library.

The endpoint is complete publicly documented chip functionality, not a limited
bring-up driver. Manufacturer GitHub/SDK code, exact register maps, feature
procedures and other useful primary sources must be researched and reconciled.
Runtime opt-in features are mandatory library scope when supported by silicon.

## How to execute

For one stage, give an agent repository access and this prompt:

```text
Execute docs/prompts/01-ecosystem-audit.md completely.
Read AGENTS.md, ../AGENTS.md, docs/prompts/COMMON.md and its shared contracts first.
Verify prerequisites, re-audit the current relevant sibling I2C libraries, then
complete the stage's scoped work, tests and documentation. Use disjoint subagents
when useful; the lead owns shared contracts and Git. Preserve sibling/concurrent
work. Update docs/implementation-progress.md and the stage report with actual
evidence. Commit and sync checked work, including coherent completed units during
the prompt, on the same execution branch. Follow root AGENTS: first-set remote
CI belongs only to prompt 29. Do not start the next stage in this invocation.
Read CONVENTIONS.md for concrete selected names and file responsibilities.
Start with a brief rule reminder, keep code and language simple, and update
Doxygen plus affected general docs with the work. Select available subagent
models/effort by complexity; the lead reviews and integrates their results.
```

Change the filename for subsequent stages. For the whole set:

```text
Execute the 34 stages in docs/prompts/README.md in their listed dependency order.
Read AGENTS.md, ../AGENTS.md and all shared prompt contracts first. Resume the
recorded execution and incomplete stage; don't recreate completed work.

For every new promptset execution create a new branch from the synchronized
intended base before its first stage. Use the requested branch name/base when
provided; do not reuse main or a previous set's branch. Record branch/base in
implementation-progress. Keep it for this whole set, agents and resumed sessions;
never create a branch per prompt or return to main between prompts. Promptset
editing/review alone is maintenance, not the start of implementation.
Resuming this execution uses its recorded branch; it does not create another one.

Before EVERY stage re-audit relevant current sibling I2C headers, implementation,
examples, CLI, tests, Python runners, workflows and AGENTS. Record revisions,
dirty state, exact paths and adopted/adapted/rejected conventions. Re-open exact
silicon evidence for each feature and actual SDK source for adapter decisions.
FieldCore-node is a read-only consumer/design reference, not a dependency.

Deliver working scoped behavior with meaningful tests, source documentation and
guides at each stage. Keep one executing owner per PHY operation and serialize
whole indirect MDIO transactions across every participant. No hardware stubs,
hidden retries, copied I2C assumptions or duplicated independent IDF/core drivers.

Use subagents for disjoint work and independent reviews when useful; the lead
integrates and owns shared contracts and Git. Commit and sync every completed
prompt and each coherent verified unit ready during a prompt. Include its tests,
docs and relevant progress. Routine commits/pushes are authorized. Sync means
fetch, safe integration, push and verified equality with the branch's upstream.
Preserve unrelated changes, do not force-push, and report actual sync failures.
Do not modify siblings or publish releases as part of this execution.

Use CONVENTIONS for concrete Status/health/method names and source layout.
Choose available subagent models/effort by task complexity, with scoped paths
and a short rationale. At each stage restate its applicable rules briefly,
write simple readable code and Doxygen, update affected README/CHANGELOG/guides,
and review the final diff against those rules before the checkpoint.

Run applicable local and real SDK/package checks. For this first promptset only,
remote CI dispatch, run-status checking and waiting belong to prompt 29. Prompts
01-28 and 30-34 do not require them, even after major fixes or commit/sync. Keep
existing automatic triggers, hooks and required merge/release checks. Record the
actual CI-tested SHA at prompt 29; later local checks do not extend that result
to a newer commit. Parent CI policy resumes after this first execution.
Host, SDK link, CI and physical evidence stay separate. Hardware work needs an
identified fixture and authorized scope. Missing hardware stays BLOCKED/NOT RUN.

Resolve routine design choices from evidence. If an external fact/tool/fixture
blocks a gate, retain it visibly and continue independent eligible work. Later
documentation and review may proceed around the hardware gate, but do not claim
the entire set COMPLETE with required gates unresolved. Unsupported features
must fail explicitly and are never silently promoted to supported.

After each stage update implementation-progress, feature/evidence coverage and
its report, then commit and sync before continuing. At final handoff report
actual parts/APIs, examples/tools/packages, tests/builds, physical limits,
remaining gates, branch/commit and verified sync state.
```

## Shared instructions

- [COMMON](COMMON.md): mandatory current-source audit, ownership, execution and handoff.
- [ENGINEERING](ENGINEERING.md): names, portable/IDF boundary, MDIO, state and chip rules.
- [CONVENTIONS](CONVENTIONS.md): selected Status/health/method names, exact sibling
  precedents, file responsibilities, readable code and per-stage Doxygen/docs.
- [FULL_COVERAGE](FULL_COVERAGE.md): complete chip features, exhaustive register/
  field/procedure evidence and broad manufacturer-first public research.
- [CLI_CONTRACT](CLI_CONTRACT.md): shared example CLI, framing and Python artifacts.
- [VERIFICATION](VERIFICATION.md): host/SDK/package/CI/hardware gates and honest claims.
- [SOURCE_MAP](SOURCE_MAP.md): concrete sibling and silicon sources to revisit.
- [Requirement coverage](COVERAGE.md): request-to-stage and acceptance mapping.

Maintained architecture and API documents become the durable behavior authority;
prompts describe how to establish and verify them. Reports preserve evidence.
When implementation changes a decision, reconcile all affected contracts instead
of leaving two specifications.

## Ordered stages

Numerical order is the default. Each file states its prerequisites. Stage 30
requires physical hardware; stages 31-34 can review available work with that gate
explicitly open. Unrelated family research may proceed around a recorded variant
gate, but dependent implementation cannot claim that gate closed. Prerequisite
ranges mean the concrete software inputs needed by the work, as defined in
COMMON; unrelated optional capability or remote-CI gaps do not stop independent
examples, packaging, documentation, hardware preparation or review. Preserve
those gaps and required final gates explicitly.

| # | Prompt | Required outcome |
| --- | --- | --- |
| 01 | [Audit the current library ecosystem](01-ecosystem-audit.md) | Current conventions, evidence inventory and consumer boundaries. |
| 02 | [Establish exact silicon and family capabilities](02-silicon-capabilities.md) | Authoritative register/procedure evidence and explicit capability gates. |
| 03 | [Resolve the portable core and ESP-IDF ownership boundary](03-framework-ownership.md) | One implementable ownership design, transport boundary and IDF method matrix. |
| 04 | [Specify API, Status, state and timing contracts](04-public-contracts.md) | Concrete public/state/time specification without successful stubs. |
| 05 | [Build the portable foundation and version tooling](05-host-foundation.md) | Working portable build, passive contracts and reproducible metadata. |
| 06 | [Implement bounded MDIO and register transactions](06-mdio-register-access.md) | Actual portable MDIO/register implementation with atomicity and fault evidence. |
| 07 | [Implement identity and supported-device selection](07-identity-capabilities.md) | Exact DP83826I identity and honest capability evidence. |
| 08 | [Implement lifecycle, reset and bounded restoration](08-reset-lifecycle.md) | Usable lifecycle/reset operations with explicit effects and time bounds. |
| 09 | [Implement generic 10/100 link configuration](09-link-configuration.md) | Verified general link settings and a separate board-policy profile. |
| 10 | [Implement link snapshots and change evidence](10-link-observation.md) | Truthful passive/live link observations and adapter-ready change evidence. |
| 11 | [Complete health, invalidation and recovery primitives](11-health-recovery.md) | Passive health and trustworthy configuration/recovery evidence. |
| 12 | [Implement supported loopback and BIST](12-loopback-bist.md) | Bounded loopback/BIST with precise result and cleanup semantics. |
| 13 | [Implement supported cable diagnostics](13-cable-diagnostics.md) | Exact supported cable diagnostics and a reproducible fixture plan. |
| 14 | [Implement remaining documented PHY controls](14-interrupt-power-led.md) | Closed primary feature coverage with explicit capability and board limits. |
| 15 | [Implement the native ESP-IDF PHY adapter](15-esp-idf-adapter.md) | Native adapter and real SDK integration evidence. |
| 16 | [Create the ESP32-P4 ordinary Ethernet example](16-p4-network-example.md) | Buildable general Ethernet/IP example with application-owned hardware. |
| 17 | [Demonstrate EtherCAT application ownership boundaries](17-ethercat-owner-example.md) | Buildable owner example and honest EtherCAT-master integration contract. |
| 18 | [Build the shared portable CLI and console](18-cli-foundation.md) | Working shared CLI foundation and frozen automation grammar. |
| 19 | [Expose implemented PHY behavior through the examples](19-cli-feature-coverage.md) | Complete primary command coverage and shared CLI in real examples. |
| 20 | [Implement the bounded Python session and artifact runner](20-python-runner.md) | Executable runner with strict correlation, verdicts and durable evidence. |
| 21 | [Build finite observation, fault and diagnostic suites](21-hil-fixture-suites.md) | Ready-to-run PHY campaigns and explicit physical fixture requirements. |
| 22 | [Close protocol, ownership and portability test gaps](22-core-regression.md) | Primary implementation regression evidence and closed ownership gaps. |
| 23 | [Qualify the DP83826E software path](23-dp83826e.md) | Evidence-backed DP83826E software support with separate hardware status. |
| 24 | [Implement separately qualified DP83826AI and DP83826AE](24-dp83826a.md) | Explicit A-generation paths, differences and qualification gates. |
| 25 | [Implement the documented DP83825I](25-dp83825i.md) | DP83825I software support without invented family compatibility. |
| 26 | [Implement exact documented DP83822 variants](26-dp83822-family.md) | Documented DP83822 subtype support and explicit medium/interface limits. |
| 27 | [Prove independent application and framework consumption](27-portable-consumers.md) | Independent portable consumer and useful application integration guidance. |
| 28 | [Validate packages, metadata and ownership](28-packages-metadata.md) | Reproducible consumable packages and accurate project metadata. |
| 29 | [Complete the dedicated CI checkpoint](29-ci-matrix.md) | Executing CI for the recorded commit; the first set's sole remote checkpoint. |
| 30 | [Execute the available physical qualification matrix](30-hardware-qualification.md) | Actual available hardware evidence or a precise unclosed fixture gate. |
| 31 | [Reconcile user-facing guides and feature coverage](31-user-documentation.md) | Accurate usable guides and complete claim-to-evidence mapping. |
| 32 | [Audit source documentation and maintenance ownership](32-maintainer-documentation.md) | Maintainable source contracts and accurate agent/contributor guidance. |
| 33 | [Review silicon correctness and ownership independently](33-independent-core-review.md) | Independently reviewed core with evidenced corrections and explicit gaps. |
| 34 | [Close implementation, tooling and qualification claims](34-final-integration-review.md) | Truthful final implementation handoff and qualification status. |

Tests and documentation accompany behavior throughout. Stages 22 and 31-34 close
cross-cutting gaps; they do not excuse earlier test or documentation debt.
Implementation status and evidence are tracked in
[implementation-progress](../implementation-progress.md). All stages start as
NOT STARTED; this planning audit is not a firmware implementation milestone.

## Audit basis

See [audit index](../reports/README.md) for actual observed sibling conventions,
FieldCore owner/promptset evidence, family/tooling comparisons and PHY risks.
The audits deliberately distinguish deeper source review from broad inventory.
Relevant sibling files must still be reopened before every stage.

The initial priority is DP83826I with real portability and native P4 integration.
Family stages cover DP83826E, DP83826AI/AE, DP83825I and DP83822I/H/IF/HF separately.
No ordering suffix, interface or optional diagnostic inherits support merely
because a related part passed. Extra frameworks require working backends and
build evidence; the I2C peers' S2/S3 Arduino matrix is not an Ethernet promise.
