# Implementation progress

## Planning baseline

Date: 2026-10-26. Starting revision:
`35ced9bb9d0b05c1c0b41ecff7cee5e757bcb163`, branch `main`.
The initial DP8382x working tree was clean. A configured origin was observed;
the original setup instruction saying no remote was selected was historical.
This records the initial planning snapshot. Completed planning/maintenance work
now follows the user's commit/sync instruction; actual checkpoint/sync results
belong in the final handoff and the next progress update. No publication is implied.

The [audit reports](reports/README.md) and [80-prompt set](prompts/README.md)
are prepared. **No driver/API implementation, firmware example, library test
suite, CI, CLI or HIL runner has been created or executed here.**
Current executable verification remains the reference/documentation checker.

The follow-up scope requires complete documented chip functionality beyond the
initial board, with mandatory [full-coverage research](prompts/FULL_COVERAGE.md),
an exhaustive [DP83826I inventory](reference/dp83826i-feature-inventory.md) and
[additional public manufacturer sources](reference/public-research.md).
These are planning artifacts; all implementation stages remain NOT STARTED.

The concrete-conventions follow-up expands all stages from current sibling
source. See [CONVENTIONS](prompts/CONVENTIONS.md) for selected Status/health/method
names, file responsibilities and Doxygen requirements. Every prompt includes
rule reminders, focused current-source checks and stage-specific documentation.
Model selection follows task complexity and available tools. This maintenance
does not mark any implementation stage complete. The preceding guidance
checkpoint was `572f8b1f6946fe645f35d7ad5c16c5ef617cfdfb`, synchronized on `main`.

## Execution record

The [correctness audit](reports/prompt-correctness-audit.md) corrected silicon/SDK
instructions and reduced unnecessary mechanisms while retaining full scope.
The fresh-branch rule checkpoint `21a22c4` was synchronized on main during this
maintenance pass. EEE wire-address and A-generation strap/EMC conflicts remain
explicit evidence gates. This review does not execute implementation stages.

The [workload audit](reports/prompt-workload-audit.md) replaces the former 34
stages with 80 naturally numbered complete prompts. The preceding correctness
checkpoint `afe8445fa80edf65296497ffc1205c460258382f` was synchronized on main.
All old stage scopes are mapped; planning maintenance still implements no driver.

Execution branch/base: not started. Before the first stage of a new promptset
execution, create a new branch from the synchronized intended base and record
its name and original base commit. Do not use main or a previous set's branch.
Keep it for every stage and resumed session; resuming does not create a new branch.
Promptset maintenance does not start execution or require a new branch.

The user now authorizes routine commits/sync at every completed stage and for
coherent verified intermediate units. Root AGENTS owns that workflow. For this
first execution, remote CI is checked only at prompt 71; 1-70 and 72-80 use
focused local verification. Record the prompt-71 SHA separately from later local
results, and restore parent CI policy after the first execution.

| Prompt | Implementation status | Evidence |
| --- | --- | --- |
| [1 Audit current driver APIs, state and ownership](prompts/1-ecosystem-core-audit.md) | NOT STARTED | None |
| [2 Audit examples, tools, repository structure and prompt workflows](prompts/2-ecosystem-tooling-audit.md) | NOT STARTED | None |
| [3 Establish the primary register ledger and core silicon behavior](prompts/3-silicon-core-ledger.md) | NOT STARTED | None |
| [4 Research diagnostic, event and counter procedures](prompts/4-silicon-diagnostic-ledger.md) | NOT STARTED | None |
| [5 Complete power, wake, interface and remaining source coverage](prompts/5-silicon-power-interface-ledger.md) | NOT STARTED | None |
| [6 Resolve portable core and native SDK ownership](prompts/6-framework-ownership.md) | NOT STARTED | None |
| [7 Freeze value, Status and direct transport contracts](prompts/7-value-transport-contracts.md) | NOT STARTED | None |
| [8 Freeze cooperative operation, time and cleanup contracts](prompts/8-operation-contracts.md) | NOT STARTED | None |
| [9 Build the portable values, tests and source-documentation foundation](prompts/9-portable-source-foundation.md) | NOT STARTED | None |
| [10 Add deterministic version metadata and generation](prompts/10-version-tooling.md) | NOT STARTED | None |
| [11 Implement bounded Clause 22 transport](prompts/11-clause22-transport.md) | NOT STARTED | None |
| [12 Implement complete atomic MMD transactions](prompts/12-atomic-mmd.md) | NOT STARTED | None |
| [13 Implement side-effect-aware register access](prompts/13-side-effect-register-access.md) | NOT STARTED | None |
| [14 Implement exact identity and capability selection](prompts/14-identity-capabilities.md) | NOT STARTED | None |
| [15 Implement initialization and complete PCS reset operations](prompts/15-initialize-pcs-reset.md) | NOT STARTED | None |
| [16 Implement hard reset, restart and external-reset reconciliation](prompts/16-hard-reset-restart.md) | NOT STARTED | None |
| [17 Implement negotiated link settings and verified application](prompts/17-negotiated-link-settings.md) | NOT STARTED | None |
| [18 Implement forced modes and coupled crossover settings](prompts/18-forced-crossover-settings.md) | NOT STARTED | None |
| [19 Implement live and cached link observations](prompts/19-link-observation.md) | NOT STARTED | None |
| [20 Implement complete event and counter acquisition](prompts/20-event-counter-acquisition.md) | NOT STARTED | None |
| [21 Implement passive driver health and exact accounting](prompts/21-passive-health.md) | NOT STARTED | None |
| [22 Implement explicit reconciliation and bounded recovery](prompts/22-reconcile-recover.md) | NOT STARTED | None |
| [23 Implement all supported loopback modes](prompts/23-loopback-modes.md) | NOT STARTED | None |
| [24 Implement complete BIST generator and checker sessions](prompts/24-bist-generator-checker.md) | NOT STARTED | None |
| [25 Implement complete manual cable capture](prompts/25-cable-manual.md) | NOT STARTED | None |
| [26 Implement cable autorun and shared result acquisition](prompts/26-cable-autorun.md) | NOT STARTED | None |
| [27 Implement bounded interrupt configuration and service](prompts/27-interrupt-service.md) | NOT STARTED | None |
| [28 Implement active-sleep policy and explicit power states](prompts/28-power-states.md) | NOT STARTED | None |
| [29 Implement exact EEE negotiation and status](prompts/29-eee-negotiation.md) | NOT STARTED | None |
| [30 Implement bounded legacy-MAC LPI sessions](prompts/30-legacy-mac-lpi.md) | NOT STARTED | None |
| [31 Implement complete Wake-on-LAN controls](prompts/31-wake-on-lan.md) | NOT STARTED | None |
| [32 Implement all LED and GPIO controls](prompts/32-led-gpio.md) | NOT STARTED | None |
| [33 Implement clock, interface and repeater controls](prompts/33-clock-interface-repeater.md) | NOT STARTED | None |
| [34 Implement quality and hardware link-loss controls](prompts/34-quality-link-loss.md) | NOT STARTED | None |
| [35 Implement electrical controls and finite compliance procedures](prompts/35-electrical-compliance-calibration.md) | NOT STARTED | None |
| [36 Audit complete primary-chip feature coverage](prompts/36-primary-feature-closure.md) | NOT STARTED | None |
| [37 Implement the native ESP-IDF factory and lifecycle](prompts/37-esp-idf-lifecycle.md) | NOT STARTED | None |
| [38 Complete SDK-aware configuration and raw MDIO arbitration](prompts/38-esp-idf-state-arbitration.md) | NOT STARTED | None |
| [39 Prove native link notifications and adapter timing contracts](prompts/39-esp-idf-link-bounds.md) | NOT STARTED | None |
| [40 Build the complete ESP32-P4 ordinary Ethernet example](prompts/40-p4-network-example.md) | NOT STARTED | None |
| [41 Implement explicit application admission and maintenance ownership](prompts/41-owner-admission-example.md) | NOT STARTED | None |
| [42 Demonstrate bounded native MAC frame handoff](prompts/42-owner-frame-boundary.md) | NOT STARTED | None |
| [43 Implement the portable CLI, bounded console and record schema](prompts/43-cli-foundation.md) | NOT STARTED | None |
| [44 Add complete identity, configuration and register CLI routes](prompts/44-cli-configuration-registers.md) | NOT STARTED | None |
| [45 Add link, lifecycle and retained-operation CLI commands](prompts/45-cli-link-lifecycle.md) | NOT STARTED | None |
| [46 Expose every diagnostic and opt-in feature through typed commands](prompts/46-cli-diagnostics-controls.md) | NOT STARTED | None |
| [47 Integrate the shared CLI in both P4 applications and close coverage](prompts/47-cli-native-integration.md) | NOT STARTED | None |
| [48 Implement the offline protocol verifier and durable evidence bundles](prompts/48-runner-protocol-artifacts.md) | NOT STARTED | None |
| [49 Implement bounded serial sessions without implicit hardware actions](prompts/49-runner-serial-session.md) | NOT STARTED | None |
| [50 Build finite observation, configuration and link-mode campaigns](prompts/50-hil-observation-modes.md) | NOT STARTED | None |
| [51 Build diagnostic and opt-in control fixture campaigns](prompts/51-hil-diagnostic-fixtures.md) | NOT STARTED | None |
| [52 Prove finite soak and interrupted-campaign failure behavior](prompts/52-hil-soak-interruption.md) | NOT STARTED | None |
| [53 Close cross-entry transaction and SDK ownership regressions](prompts/53-regression-transaction-ownership.md) | NOT STARTED | None |
| [54 Close lifecycle, deadline and retained-evidence regressions](prompts/54-regression-lifecycle-deadlines.md) | NOT STARTED | None |
| [55 Audit independent coverage, portability and executing test registration](prompts/55-regression-coverage-portability.md) | NOT STARTED | None |
| [56 Qualify the complete DP83826E software path](prompts/56-e-support.md) | NOT STARTED | None |
| [57 Establish exact AI/AE source and identity policy](prompts/57-a-source-policy.md) | NOT STARTED | None |
| [58 Implement A-generation identity, reset and link differences](prompts/58-a-core-link.md) | NOT STARTED | None |
| [59 Close A-generation diagnostic and optional-control coverage](prompts/59-a-feature-closure.md) | NOT STARTED | None |
| [60 Establish DP83825I sources and implement its core differences](prompts/60-p25-source-core.md) | NOT STARTED | None |
| [61 Close complete DP83825I feature and integration coverage](prompts/61-p25-feature-closure.md) | NOT STARTED | None |
| [62 Establish exact DP83822 subtype and feature evidence](prompts/62-p22-source-subtypes.md) | NOT STARTED | None |
| [63 Implement DP83822 copper identity, reset and link behavior](prompts/63-p22-copper-core.md) | NOT STARTED | None |
| [64 Implement DP83822 interface, clock and fixed RGMII shifts](prompts/64-p22-interface-clock.md) | NOT STARTED | None |
| [65 Implement exact DP83822IF/HF fiber behavior](prompts/65-p22-fiber.md) | NOT STARTED | None |
| [66 Implement DP83822 diagnostics and documented test procedures](prompts/66-p22-diagnostics.md) | NOT STARTED | None |
| [67 Close DP83822 power, wake, LED and complete subtype coverage](prompts/67-p22-optional-closure.md) | NOT STARTED | None |
| [68 Prove independent two-instance application consumption](prompts/68-portable-owner-consumer.md) | NOT STARTED | None |
| [69 Finalize deterministic version and supported-package metadata](prompts/69-package-metadata.md) | NOT STARTED | None |
| [70 Validate bounded archives and clean extracted consumers](prompts/70-package-export-consumers.md) | NOT STARTED | None |
| [71 Execute the single dedicated remote CI checkpoint](prompts/71-ci-checkpoint.md) | NOT STARTED | None |
| [72 Execute admitted physical boot, link and recovery campaigns](prompts/72-hw-link-campaign.md) | NOT STARTED | None |
| [73 Execute admitted feature, medium and timing qualification](prompts/73-hw-feature-campaign.md) | NOT STARTED | None |
| [74 Verify user quick starts and consumer journeys](prompts/74-docs-user-consumers.md) | NOT STARTED | None |
| [75 Reconcile complete feature claims and evidence guides](prompts/75-docs-feature-evidence.md) | NOT STARTED | None |
| [76 Audit exported source contracts and executable Doxygen checks](prompts/76-docs-api-audit.md) | NOT STARTED | None |
| [77 Verify maintenance guidance, tools and ownership boundaries](prompts/77-docs-maintainer-tools.md) | NOT STARTED | None |
| [78 Review silicon completeness and configuration independently](prompts/78-review-silicon.md) | NOT STARTED | None |
| [79 Review ownership, bounds and evidence independently](prompts/79-review-ownership-evidence.md) | NOT STARTED | None |
| [80 Close final implementation and qualification claims](prompts/80-final-integration.md) | NOT STARTED | None |

A stage can be complete as software while its separately tracked physical
qualification remains NOT RUN, if its acceptance explicitly permits that.
Prompts 72-73 cannot complete required physical measurements without actual evidence;
final overall completion must disclose and retain any outstanding required gates.
Do not change statuses merely because a prompt or future command exists.

## Per-stage report template

Create `docs/reports/implementation/N-topic.md` during execution, with:

- baseline branch/revision and dirty state; prerequisites and owned files;
- current sibling audit with exact paths and decisions adopted/adapted/rejected;
- short rule reminder, delegated scope/model choices and selection rationale;
- exact TI/SDK references, decisions, implementation/API changes and consumers;
- Doxygen/source comments and README/CHANGELOG/guide changes or no-change reasons;
- commands and results for host, CLI/Python, SDK, package and remote CI separately;
- physical fixture/image identity and evidence, or explicit NOT RUN/BLOCKED;
- unresolved gates, next eligible stage and any actual authorized Git result.

Also record the execution branch/base, verified local/upstream equality and zero
ahead/behind, or exact sync failure. Commit/sync checked prompt output before
moving on, and checkpoint complete units during work. Do not create empty or
self-referencing progress commits.

Record tested commit identity without a self-referential commit-hash update loop.
Historical evidence does not verify a later changed tree.
