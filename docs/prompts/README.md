# DP8382x implementation promptset

Execute these **80 ordered prompts** to build a framework-independent C++17
Ethernet PHY library and native ESP-IDF adapter. Start with DP83826I/DP83826IRHBR
and ESP32-P4, then qualify DP83826E, DP83826AI/AE, DP83825I and DP83822I/H/IF/HF.
All stages are currently **NOT STARTED**.

Implement each exact part's complete publicly documented software-accessible
feature set. Keep generic 10/100 operation and supported duplex modes available.
The loading-machine example advertises 100/full; ordinary Ethernet/IP is another
consumer. Applications own EtherCAT, networking, board policy and power delivery.

## Execute one prompt

Give the agent repository access and the selected file:

```text
Execute docs/prompts/1-ecosystem-core-audit.md completely.
Follow root AGENTS.md, parent ../AGENTS.md, docs/prompts/COMMON.md and the shared
contracts relevant to the task. Verify prerequisite outputs and current sibling
conventions. Complete the scoped work, tests, Doxygen and affected guides.
Use disjoint subagents/models appropriate to the task, with lead review.
Record evidence and progress, then commit and sync on the execution branch.
Finish with this stage's handoff.
```

Change the filename for each subsequent stage. For the whole set:

```text
Execute prompts 1 through 80 in docs/prompts/README.md, following COMMON and root
AGENTS. For a new execution, create a new branch from the synchronized intended
base and record its name/base. For a resumed execution, use its recorded branch
and progress. Keep that branch through the entire set.
Before each stage verify its actual dependencies, re-audit relevant current
sibling libraries and primary chip/SDK sources, and state the applicable rules
briefly. Implement the complete scoped behavior with tests and documentation.
Use suitable available subagents with disjoint ownership and lead integration.
Run focused local checks, update progress/evidence, commit and sync each completed
stage and coherent intermediate unit. Remote CI belongs to prompt 71 only in
this first execution; preserve triggers, hooks and required merge/release checks.
Record blocked requirements and continue independent eligible work. Perform
physical tests only on identified authorized fixtures. Finish with actual
software/build/hardware coverage, remaining gates and verified Git sync state.
```

Planning maintenance does not start implementation. COMMON defines completion,
blocked dependencies, collaboration and handoff. Root AGENTS owns Git/CI policy.

## Shared instructions

| Contract | Use |
| --- | --- |
| [COMMON](COMMON.md) | Every stage: entry, focused audits, delegation, checks, checkpoint and handoff |
| [ENGINEERING](ENGINEERING.md) | Portable/SDK boundary, ownership, MDIO, lifecycle, state and chip behavior |
| [CONVENTIONS](CONVENTIONS.md) | Selected Status/health/method names, source layout and Doxygen |
| [EVIDENCE](EVIDENCE.md) | Exact source/specification comparison, numeric provenance and physical evidence |
| [FULL_COVERAGE](FULL_COVERAGE.md) | Exhaustive per-part register/field/procedure ledger and feature closure |
| [CLI_CONTRACT](CLI_CONTRACT.md) | Example grammar, bounded console, runner protocol and artifacts |
| [VERIFICATION](VERIFICATION.md) | Host, SDK, package, CI and physical checks |
| [SOURCE_MAP](SOURCE_MAP.md) | Relevant sibling and primary technical source locations |
| [COVERAGE](COVERAGE.md) | Requirement-to-stage ownership and acceptance |

Use the [public-library catalogues](../reference/library-evidence.md) for concrete
register, timing and procedure inputs, verifying adopted facts under EVIDENCE.
Maintained architecture/API guides own implemented behavior; the ledger owns
source applicability and coverage; stage reports retain actual execution evidence.
Update affected documents together when implementing a changed contract.

## Ordered prompts

Execute in the numeric order below, **1 through 80**. Filenames use natural
numbers without leading zeros; use this index rather than a lexicographic file
sort. Each prompt names its concrete input evidence, complete outcome and
acceptance checks. The shared instructions apply to every prompt.

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
