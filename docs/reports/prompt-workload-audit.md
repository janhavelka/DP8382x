# Prompt workload audit and numbering migration

Reviewed on **2026-10-26** against planning commit
`afe8445fa80edf65296497ffc1205c460258382f`. This maintenance replaces 34 broad
stages with **80 complete prompts**, numbered `1` through `80` without leading
zeros. It does not execute the set or implement any library behavior. All
implementation progress remains NOT STARTED and no execution branch has begun.

## Findings and split criteria

The original files ranged from **837 to 1407 words**, with
median **978**. Several similar-length prompts had very different
workloads. The old remaining-controls stage combined interrupt service, power,
EEE, LPI, wake, pins, clocks, quality and electrical procedures. SDK, CLI,
runner and DP83822 stages also hid several independent implementation lifetimes.

The replacement files range from **412 to 686 words**, with
median **477** at this audit. Counts use whitespace-separated
words, including local instructions. The shared contracts are additional required
context. Total numbered text changes from 33,935 to 41,687 words:
smaller execution units need their own inputs, evidence and handoff. Word count
is a warning signal, not an acceptance quota or a promise of fixed runtime.

Each implementation prompt owns one usable feature, integration boundary or
finite verification matrix, with failure paths, tests, Doxygen/general docs and
commit/sync in the same task. Complete MMD transactions, diagnostic stop/restore,
CLI terminal retention and runner verdict/artifact finalization remain intact.
Research produces complete topic records with explicit source conflicts; it does
not turn unresolved facts into support. Family closures implement source-proved
differences and verify inherited paths, rather than reimplementing the core.

Coherent tasks such as the framework ownership decision, ordinary networking
example, portable consumer and sole CI checkpoint remain intact despite their
breadth. Thin diagnostic CLI routes share already-implemented procedures; manual
cable capture keeps its indivisible acquisition/cleanup lifetime. Later closure
and review tasks audit finite matrices and fix focused defects. Major omissions
return to the owning prompt; they cannot disappear into a final catch-all.

## Complete migration map

Old numbers below refer only to the former plan. Every former stage is mapped,
and each new prompt appears once in this table. Use the
[current ordered index](../prompts/README.md) for execution.

| Former stage | Old words | Current prompts | Workload decision |
| --- | --- | --- | --- |
| 01 - Audit the current library ecosystem | 977 | [1](../prompts/1-ecosystem-core-audit.md), [2](../prompts/2-ecosystem-tooling-audit.md) | Core API/state review separated from tools, broader inventory and consumer workflow. |
| 02 - Establish exact silicon and family capabilities | 1033 | [3](../prompts/3-silicon-core-ledger.md), [4](../prompts/4-silicon-diagnostic-ledger.md), [5](../prompts/5-silicon-power-interface-ledger.md) | Core register facts, diagnostics and power/interface research share one ledger with distinct owners. |
| 03 - Resolve the portable core and ESP-IDF ownership boundary | 1157 | [6](../prompts/6-framework-ownership.md) | Kept one coupled ownership decision: SDK state, indirect transactions and lifetime must agree. |
| 04 - Specify API, Status, state and timing contracts | 1266 | [7](../prompts/7-value-transport-contracts.md), [8](../prompts/8-operation-contracts.md) | Foundational values/direct calls separated from cooperative time/result/cleanup contracts. |
| 05 - Build the portable foundation and version tooling | 954 | [9](../prompts/9-portable-source-foundation.md), [10](../prompts/10-version-tooling.md) | Working passive build/tests/Doxygen separated from independent deterministic version tooling. |
| 06 - Implement bounded MDIO and register transactions | 1120 | [11](../prompts/11-clause22-transport.md), [12](../prompts/12-atomic-mmd.md), [13](../prompts/13-side-effect-register-access.md) | C22 transport, atomic MMD and side-effect-aware access each have complete failure tests. |
| 07 - Implement identity and supported-device selection | 930 | [14](../prompts/14-identity-capabilities.md) | Kept one exact identity/capability deliverable. |
| 08 - Implement lifecycle, reset and bounded restoration | 956 | [15](../prompts/15-initialize-pcs-reset.md), [16](../prompts/16-hard-reset-restart.md) | Initialization/PCS reset and hard reset/restart each include full restoration and cleanup. |
| 09 - Implement generic 10/100 link configuration | 1035 | [17](../prompts/17-negotiated-link-settings.md), [18](../prompts/18-forced-crossover-settings.md) | Negotiated settings and forced/crossover settings each deliver complete supported transitions. |
| 10 - Implement link snapshots and change evidence | 1023 | [19](../prompts/19-link-observation.md), [20](../prompts/20-event-counter-acquisition.md) | Link observation separated from complete destructive event/counter acquisition. |
| 11 - Complete health, invalidation and recovery primitives | 942 | [21](../prompts/21-passive-health.md), [22](../prompts/22-reconcile-recover.md) | Passive telemetry separated from explicit verification/reconciliation/recovery. |
| 12 - Implement supported loopback and BIST | 1017 | [23](../prompts/23-loopback-modes.md), [24](../prompts/24-bist-generator-checker.md) | Persistent/finite loopbacks separated from generator/checker BIST sessions. |
| 13 - Implement supported cable diagnostics | 902 | [25](../prompts/25-cable-manual.md), [26](../prompts/26-cable-autorun.md) | Manual cable capture remains atomic; hardware autorun integration is separate. |
| 14 - Implement remaining documented PHY controls | 1407 | [27](../prompts/27-interrupt-service.md), [28](../prompts/28-power-states.md), [29](../prompts/29-eee-negotiation.md), [30](../prompts/30-legacy-mac-lpi.md), [31](../prompts/31-wake-on-lan.md), [32](../prompts/32-led-gpio.md), [33](../prompts/33-clock-interface-repeater.md), [34](../prompts/34-quality-link-loss.md), [35](../prompts/35-electrical-compliance-calibration.md), [36](../prompts/36-primary-feature-closure.md) | Nine independently usable feature families plus a bounded primary coverage audit replace a catch-all. |
| 15 - Implement the native ESP-IDF PHY adapter | 1134 | [37](../prompts/37-esp-idf-lifecycle.md), [38](../prompts/38-esp-idf-state-arbitration.md), [39](../prompts/39-esp-idf-link-bounds.md) | Working adapter/lifecycle first, then SDK state/raw ownership and conformance verification. |
| 16 - Create the ESP32-P4 ordinary Ethernet example | 865 | [40](../prompts/40-p4-network-example.md) | Kept the complete ordinary network example and resource lifetime together. |
| 17 - Demonstrate EtherCAT application ownership boundaries | 897 | [41](../prompts/41-owner-admission-example.md), [42](../prompts/42-owner-frame-boundary.md) | Application admission/readiness first; complete bounded raw-frame handoff second. |
| 18 - Build the shared portable CLI and console | 849 | [43](../prompts/43-cli-foundation.md) | Kept parser, bounded console, terminal retention, producer schema and compiled fixture together. |
| 19 - Expose implemented PHY behavior through the examples | 1006 | [44](../prompts/44-cli-configuration-registers.md), [45](../prompts/45-cli-link-lifecycle.md), [46](../prompts/46-cli-diagnostics-controls.md), [47](../prompts/47-cli-native-integration.md) | Split handlers by configuration/register, lifecycle/link and diagnostics, then native integration. |
| 20 - Implement the bounded Python session and artifact runner | 982 | [48](../prompts/48-runner-protocol-artifacts.md), [49](../prompts/49-runner-serial-session.md) | Usable offline protocol/artifacts verifier first; bounded real serial session second. |
| 21 - Build finite observation, fault and diagnostic suites | 895 | [50](../prompts/50-hil-observation-modes.md), [51](../prompts/51-hil-diagnostic-fixtures.md), [52](../prompts/52-hil-soak-interruption.md) | Observation/mode, diagnostic-fixture and interruption/soak suites each close their own evidence. |
| 22 - Close protocol, ownership and portability test gaps | 1026 | [53](../prompts/53-regression-transaction-ownership.md), [54](../prompts/54-regression-lifecycle-deadlines.md), [55](../prompts/55-regression-coverage-portability.md) | Transaction ownership, lifecycle/deadline and coverage/portability audits have separate matrices. |
| 23 - Qualify the DP83826E software path | 875 | [56](../prompts/56-e-support.md) | Kept a sourced I/E comparison and exact E software qualification as one coherent delta. |
| 24 - Implement separately qualified DP83826AI and DP83826AE | 979 | [57](../prompts/57-a-source-policy.md), [58](../prompts/58-a-core-link.md), [59](../prompts/59-a-feature-closure.md) | A source/revision policy, core/link changes and remaining proven feature deltas are separate. |
| 25 - Implement the documented DP83825I | 837 | [60](../prompts/60-p25-source-core.md), [61](../prompts/61-p25-feature-closure.md) | Exact-I source/core paths first; inherited feature applicability/differences and closure second. |
| 26 - Implement exact documented DP83822 variants | 1016 | [62](../prompts/62-p22-source-subtypes.md), [63](../prompts/63-p22-copper-core.md), [64](../prompts/64-p22-interface-clock.md), [65](../prompts/65-p22-fiber.md), [66](../prompts/66-p22-diagnostics.md), [67](../prompts/67-p22-optional-closure.md) | Subtype sources, copper core, interfaces, fiber, diagnostics and remaining proven controls are separate. |
| 27 - Prove independent application and framework consumption | 899 | [68](../prompts/68-portable-owner-consumer.md) | Kept one complete two-instance framework-free consumer and integration guide. |
| 28 - Validate packages, metadata and ownership | 884 | [69](../prompts/69-package-metadata.md), [70](../prompts/70-package-export-consumers.md) | Metadata validation separated from bounded export and extracted-consumer verification. |
| 29 - Complete the dedicated CI checkpoint | 1125 | [71](../prompts/71-ci-checkpoint.md) | Kept the sole remote-CI checkpoint over existing local commands and actual commit inputs. |
| 30 - Execute the available physical qualification matrix | 907 | [72](../prompts/72-hw-link-campaign.md), [73](../prompts/73-hw-feature-campaign.md) | Link/recovery/traffic measurements separated from diagnostic/pin/power/instrument campaigns. |
| 31 - Reconcile user-facing guides and feature coverage | 973 | [74](../prompts/74-docs-user-consumers.md), [75](../prompts/75-docs-feature-evidence.md) | Executable user journeys separated from full feature/claim-to-evidence reconciliation. |
| 32 - Audit source documentation and maintenance ownership | 892 | [76](../prompts/76-docs-api-audit.md), [77](../prompts/77-docs-maintainer-tools.md) | Public-source Doxygen audit separated from clean-checkout maintainer/tool ownership review. |
| 33 - Review silicon correctness and ownership independently | 1003 | [78](../prompts/78-review-silicon.md), [79](../prompts/79-review-ownership-evidence.md) | Independent silicon/coverage review separated from ownership/deadline/evidence integrity review. |
| 34 - Close implementation, tooling and qualification claims | 1172 | [80](../prompts/80-final-integration.md) | Kept one final integration and honest gate accounting task; it is not unfinished implementation storage. |

## Instructions and scope preserved

- A new execution creates a new branch before prompt 1 and retains it for all
  prompts/agents/resumptions. Maintenance of prompts alone does not start it.
- Every completed prompt and coherent verified intermediate unit is committed
  and synchronized by the lead, with tested code/docs and actual upstream checks.
- First-set remote CI remains one checkpoint, renumbered from old 29 to **71**.
  Prompts 1-70 and 72-80 use focused local checks, preserve automatic triggers
  and keep the historical CI-tested SHA distinct from later local results.
- Current relevant sibling I2C source inspection, concrete naming/Status/health,
  framework boundaries, independent vectors and complete documented functionality
  remain mandatory. The existing feature inventory now names the new owning tasks.
- Each prompt retains scoped subagent use and model selection by actual task
  complexity. Shared contracts/Git have one lead owner; siblings remain read-only.
- All silicon and SDK corrections from the
  [correctness audit](prompt-correctness-audit.md) remain in the source contracts
  and owning prompts, including source conflicts and honest hardware gates.
- Hardware evidence moves to 72-73; missing fixtures cannot become preparation
  PASS. Documentation/review can continue with those gates visibly open.
- Reports use `docs/reports/implementation/N-topic.md` matching the prompt number.
  Historical reports retain old numbering with explicit notices and this mapping.

## Review evidence

The lead reviewed all former prompt scopes and shared maps. Three agents divided
read-only workload analysis and then wrote disjoint prompt ranges: a stronger
reasoning model (`gpt-6-astra`, high) for core state/register/task boundaries;
capable coding/review models (`gpt-6.1-sol`, high) for SDK/tools and family/evidence.
These are this audit's observed model choices, not required future model names.
The lead authored prompts 1-10, shared guidance and migration maps and retained
sole Git ownership. Agents independently cross-reviewed other ranges for lost
requirements, unfinished procedure halves, dependency errors and workload.

The cross-review restored explicit health reset/naming decisions in prompt 7
and explicit address/discovery and unfinished-entry restrictions in prompt 37.
It also tightened the distinction between source-proved unsupported silicon and
future unimplemented reset kinds, pure preflight versus live observations, and
the hardware campaign's sustained-traffic case, reused fixture schema and
separate runner/firmware identities. Tests, source documentation and cleanup remain
part of the behavior prompt rather than debt for a later audit.

## Local verification

- Reference checker: PASS, 27 vendor artifacts (43,240,385 bytes), 25 text
  extracts and 1,006 local links in 117 Markdown files.
- One-off manifest review: PASS, exactly 80 naturally numbered files/headings,
  80 index/progress rows, every status NOT STARTED, all 34 old scopes mapped
  once to all 80 successors and 183 backward input links. Two forward links
  explicitly describe later family closure, not prerequisites.
- Independent semantic reviews: preserved complete procedures and family/SDK
  restrictions; reviewed concrete inputs as well as linked prerequisites.
- UTF-8/LF, final-newline/trailing-whitespace and active-numbering checks: PASS.
  Former filenames are removed; historical reports mark their earlier numbers.
- `git diff --check`: PASS. Vendor payload and extracted-source bytes are unchanged.

No driver/host/SDK/physical tests or remote CI runs are
claimed by this documentation maintenance.
