# DP8382x implementation promptset

Prepared on **2026-10-26** after auditing the local chip libraries and FieldCore-node.
This set contains **80 ordered prompts** to create a general microcontroller PHY
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
Execute docs/prompts/1-ecosystem-core-audit.md completely.
Read AGENTS.md, ../AGENTS.md, docs/prompts/COMMON.md and its shared contracts first.
Verify prerequisites, re-audit the current relevant sibling I2C libraries, then
complete the stage's scoped work, tests and documentation. Use disjoint subagents
when useful; the lead owns shared contracts and Git. Preserve sibling/concurrent
work. Update docs/implementation-progress.md and the stage report with actual
evidence. Commit and sync checked work, including coherent completed units during
the prompt, on the same execution branch. Follow root AGENTS: first-set remote
CI belongs only to prompt 71. Do not start the next stage in this invocation.
Read CONVENTIONS.md for concrete selected names and file responsibilities.
Start with a brief rule reminder, keep code and language simple, and update
Doxygen plus affected general docs with the work. Select available subagent
models/effort by complexity; the lead reviews and integrates their results.
```

Change the filename for subsequent stages. For the whole set:

```text
Execute the 80 prompts in docs/prompts/README.md in their listed dependency order.
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
remote CI dispatch, run-status checking and waiting belong to prompt 71. Prompts
1-70 and 72-80 do not require them, even after major fixes or commit/sync. Keep
existing automatic triggers, hooks and required merge/release checks. Record the
actual CI-tested SHA at prompt 71; later local checks do not extend that result
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

## Ordered prompts

Execute in the numeric order below, **1 through 80**. Filenames use natural
numbers without leading zeros; use this index rather than a lexicographic file
sort. Each prompt names its concrete input evidence, complete outcome and
acceptance checks. The shared instructions apply to every prompt.

The [workload audit and migration map](../reports/prompt-workload-audit.md)
explains the split from the former 34 stages. It preserves every original scope
while keeping working behavior, failure/cleanup tests and documentation together.
Each prompt is one invocation by default. Authorizing one does not authorize
starting the next; the whole-set instruction above authorizes continuation.

Prompts 1-8 establish evidence and contracts; 9-36 implement the portable primary
part; 37-42 integrate native SDK/applications; 43-55 deliver CLI, runners and
cross-module verification; 56-67 qualify exact family variants; 68-71 establish
consumption, packaging and the single remote-CI checkpoint; 72-80 qualify physical
fixtures, reconcile documentation and close independent reviews.

Prompts 72-73 require actual authorized fixtures. Later documentation/review can
proceed around recorded hardware gates. An unrelated source/variant/fixture gap
must not block independent work whose concrete inputs exist, nor be silently
marked complete. COMMON defines this distinction. Closure/review prompts fix
focused gaps; substantial missing features return to their owning prompt.

| # | Prompt and complete task |
| --- | --- |
| 1 | [Audit current driver APIs, state and ownership](1-ecosystem-core-audit.md) |
| 2 | [Audit examples, tools, repository structure and prompt workflows](2-ecosystem-tooling-audit.md) |
| 3 | [Establish the primary register ledger and core silicon behavior](3-silicon-core-ledger.md) |
| 4 | [Research diagnostic, event and counter procedures](4-silicon-diagnostic-ledger.md) |
| 5 | [Complete power, wake, interface and remaining source coverage](5-silicon-power-interface-ledger.md) |
| 6 | [Resolve portable core and native SDK ownership](6-framework-ownership.md) |
| 7 | [Freeze value, Status and direct transport contracts](7-value-transport-contracts.md) |
| 8 | [Freeze cooperative operation, time and cleanup contracts](8-operation-contracts.md) |
| 9 | [Build the portable values, tests and source-documentation foundation](9-portable-source-foundation.md) |
| 10 | [Add deterministic version metadata and generation](10-version-tooling.md) |
| 11 | [Implement bounded Clause 22 transport](11-clause22-transport.md) |
| 12 | [Implement complete atomic MMD transactions](12-atomic-mmd.md) |
| 13 | [Implement side-effect-aware register access](13-side-effect-register-access.md) |
| 14 | [Implement exact identity and capability selection](14-identity-capabilities.md) |
| 15 | [Implement initialization and complete PCS reset operations](15-initialize-pcs-reset.md) |
| 16 | [Implement hard reset, restart and external-reset reconciliation](16-hard-reset-restart.md) |
| 17 | [Implement negotiated link settings and verified application](17-negotiated-link-settings.md) |
| 18 | [Implement forced modes and coupled crossover settings](18-forced-crossover-settings.md) |
| 19 | [Implement live and cached link observations](19-link-observation.md) |
| 20 | [Implement complete event and counter acquisition](20-event-counter-acquisition.md) |
| 21 | [Implement passive driver health and exact accounting](21-passive-health.md) |
| 22 | [Implement explicit reconciliation and bounded recovery](22-reconcile-recover.md) |
| 23 | [Implement all supported loopback modes](23-loopback-modes.md) |
| 24 | [Implement complete BIST generator and checker sessions](24-bist-generator-checker.md) |
| 25 | [Implement complete manual cable capture](25-cable-manual.md) |
| 26 | [Implement cable autorun and shared result acquisition](26-cable-autorun.md) |
| 27 | [Implement bounded interrupt configuration and service](27-interrupt-service.md) |
| 28 | [Implement active-sleep policy and explicit power states](28-power-states.md) |
| 29 | [Implement exact EEE negotiation and status](29-eee-negotiation.md) |
| 30 | [Implement bounded legacy-MAC LPI sessions](30-legacy-mac-lpi.md) |
| 31 | [Implement complete Wake-on-LAN controls](31-wake-on-lan.md) |
| 32 | [Implement all LED and GPIO controls](32-led-gpio.md) |
| 33 | [Implement clock, interface and repeater controls](33-clock-interface-repeater.md) |
| 34 | [Implement quality and hardware link-loss controls](34-quality-link-loss.md) |
| 35 | [Implement electrical controls and finite compliance procedures](35-electrical-compliance-calibration.md) |
| 36 | [Audit complete primary-chip feature coverage](36-primary-feature-closure.md) |
| 37 | [Implement the native ESP-IDF factory and lifecycle](37-esp-idf-lifecycle.md) |
| 38 | [Complete SDK-aware configuration and raw MDIO arbitration](38-esp-idf-state-arbitration.md) |
| 39 | [Prove native link notifications and adapter timing contracts](39-esp-idf-link-bounds.md) |
| 40 | [Build the complete ESP32-P4 ordinary Ethernet example](40-p4-network-example.md) |
| 41 | [Implement explicit application admission and maintenance ownership](41-owner-admission-example.md) |
| 42 | [Demonstrate bounded native MAC frame handoff](42-owner-frame-boundary.md) |
| 43 | [Implement the portable CLI, bounded console and record schema](43-cli-foundation.md) |
| 44 | [Add complete identity, configuration and register CLI routes](44-cli-configuration-registers.md) |
| 45 | [Add link, lifecycle and retained-operation CLI commands](45-cli-link-lifecycle.md) |
| 46 | [Expose every diagnostic and opt-in feature through typed commands](46-cli-diagnostics-controls.md) |
| 47 | [Integrate the shared CLI in both P4 applications and close coverage](47-cli-native-integration.md) |
| 48 | [Implement the offline protocol verifier and durable evidence bundles](48-runner-protocol-artifacts.md) |
| 49 | [Implement bounded serial sessions without implicit hardware actions](49-runner-serial-session.md) |
| 50 | [Build finite observation, configuration and link-mode campaigns](50-hil-observation-modes.md) |
| 51 | [Build diagnostic and opt-in control fixture campaigns](51-hil-diagnostic-fixtures.md) |
| 52 | [Prove finite soak and interrupted-campaign failure behavior](52-hil-soak-interruption.md) |
| 53 | [Close cross-entry transaction and SDK ownership regressions](53-regression-transaction-ownership.md) |
| 54 | [Close lifecycle, deadline and retained-evidence regressions](54-regression-lifecycle-deadlines.md) |
| 55 | [Audit independent coverage, portability and executing test registration](55-regression-coverage-portability.md) |
| 56 | [Qualify the complete DP83826E software path](56-e-support.md) |
| 57 | [Establish exact AI/AE source and identity policy](57-a-source-policy.md) |
| 58 | [Implement A-generation identity, reset and link differences](58-a-core-link.md) |
| 59 | [Close A-generation diagnostic and optional-control coverage](59-a-feature-closure.md) |
| 60 | [Establish DP83825I sources and implement its core differences](60-p25-source-core.md) |
| 61 | [Close complete DP83825I feature and integration coverage](61-p25-feature-closure.md) |
| 62 | [Establish exact DP83822 subtype and feature evidence](62-p22-source-subtypes.md) |
| 63 | [Implement DP83822 copper identity, reset and link behavior](63-p22-copper-core.md) |
| 64 | [Implement DP83822 interface, clock and fixed RGMII shifts](64-p22-interface-clock.md) |
| 65 | [Implement exact DP83822IF/HF fiber behavior](65-p22-fiber.md) |
| 66 | [Implement DP83822 diagnostics and documented test procedures](66-p22-diagnostics.md) |
| 67 | [Close DP83822 power, wake, LED and complete subtype coverage](67-p22-optional-closure.md) |
| 68 | [Prove independent two-instance application consumption](68-portable-owner-consumer.md) |
| 69 | [Finalize deterministic version and supported-package metadata](69-package-metadata.md) |
| 70 | [Validate bounded archives and clean extracted consumers](70-package-export-consumers.md) |
| 71 | [Execute the single dedicated remote CI checkpoint](71-ci-checkpoint.md) |
| 72 | [Execute admitted physical boot, link and recovery campaigns](72-hw-link-campaign.md) |
| 73 | [Execute admitted feature, medium and timing qualification](73-hw-feature-campaign.md) |
| 74 | [Verify user quick starts and consumer journeys](74-docs-user-consumers.md) |
| 75 | [Reconcile complete feature claims and evidence guides](75-docs-feature-evidence.md) |
| 76 | [Audit exported source contracts and executable Doxygen checks](76-docs-api-audit.md) |
| 77 | [Verify maintenance guidance, tools and ownership boundaries](77-docs-maintainer-tools.md) |
| 78 | [Review silicon completeness and configuration independently](78-review-silicon.md) |
| 79 | [Review ownership, bounds and evidence independently](79-review-ownership-evidence.md) |
| 80 | [Close final implementation and qualification claims](80-final-integration.md) |

Tests and documentation accompany each behavior. Prompts 36, 53-55 and 74-80
review completeness and integration; they do not supply the first tests or
Doxygen for earlier code. The [progress record](../implementation-progress.md)
starts all 80 prompts as NOT STARTED. Preparing this set does not implement or
qualify the driver.

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
