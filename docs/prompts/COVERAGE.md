# Requirement and completion coverage

This map makes omissions visible. Stage numbers refer to the
[ordered index](README.md); implementation must later link actual source, tests
and evidence from the maintained feature matrix. A listed stage is planned work.

| User requirement / engineering need | Stages | Acceptance evidence |
| --- | --- | --- |
| Audit other chip libraries comprehensively | 01; repeated in every stage | Current revision/path inventory, deeper code samples, adopted/adapted/rejected decisions |
| Follow sibling naming, layout and project ownership | 01, 04-05, 28, 32 | Real headers/metadata, maintained agent/contributor rules, verified owner information |
| Framework-agnostic MCU library | 03-06, 22, 27-28 | Actual portable core and isolated tests-OFF consumers; no SDK/sibling dependencies |
| Use supported ESP-IDF PHY/MAC framework | 03, 15-17, 29 | Method/ownership matrix, real adapter and linked P4 images |
| Exact DP83826I first | 02, 06-14, 22 | Datasheet-backed IDs/sequences, independent failure tests, primary capability coverage |
| Complete chip feature set beyond initial board use | 02, 04, 06-14, 19, 22-26, 31-34 | Exhaustive register/field/procedure ledger; usable APIs, CLI and tests for every documented programmable feature; runtime opt-in is not deferred scope |
| Broad useful public/manufacturer research | 02 and every feature/family stage | Pinned TI GitHub/SDK, exact docs, EVM/tool guides and applicable primary comparisons with provenance, license and conflict records |
| General DP8382x family | 23-26, 30, 34 | Per-exact-part register/interface/feature differences and separate qualification rows |
| Generic 10/100 and supported duplex modes | 09-10, 21-22, 30 | Validated masks/forced modes, 10BASE-Te setup and suitable physical peer tests |
| Loading-machine EtherCAT application | 16-17, 19, 30 | Separate 100/full owner policy, maintenance admission and raw MAC boundary; no master/servo claims |
| Ordinary internet/Ethernet PCB application | 16, 27, 30 | General network example; PHY/link separated from DHCP/internet outcomes |
| Exclude power delivery | 03, 14, 16-17, 34 | No PoE/power-delivery API; optional PHY power state explicitly distinct |
| Bus/controller and configuration ownership | 03-04, 06, 08, 15, 22, 33 | Whole indirect/RMW arbitration, context lifetime, callback bounds, IDF no-bypass tests |
| Status, passive health and freshness | 04, 06-11, 18-20 | Structured errors; separate transport/link/config/policy evidence and no-I/O snapshots |
| Full practical diagnostics | 12-14, 19, 21, 30 | Exact-capability loopback/BIST/cable/IRQ/LED/wake/power coverage, cleanup evidence |
| Useful example CLI | 18-19 | Shared portable executable processor, strict input, bounded output, owner routing |
| Python runners | 20-21 | Real session/artifact tooling, compiled-CLI and parser negatives, explicit fixtures |
| Tests and honest test claims | All implementation stages; 22, 29-30, 33-34 | Registered independent behavior tests; SDK, CI, hardware evidence separated |
| Packaging/version/license/maintainability | 05, 28-29, 31-34 | Deterministic metadata, clean package consumers, correct notices, current guides |
| FieldCore-style promptsets and consumer integration | 01, 03-04, 17, 27, 31-34 | Shared contracts/source map, prerequisites, ownership, reports and no FieldCore dependency |
| Re-audit sibling I2C libraries every time | COMMON and each numbered file | Fresh stage evidence; old report never substitutes for source inspection |
| Subagent collaboration | COMMON and each numbered file | Disjoint ownership, lead-controlled shared contracts/Git, integrated reviewed results |
| Commit/sync each prompt and complete intermediate units | AGENTS, COMMON, every prompt | Checked coherent commits, same execution branch, actual push and verified upstream equality |
| First-set CI without repeated remote checks | AGENTS, 29, VERIFICATION | Remote execution/status only at 29; local checks throughout, original tested SHA retained, triggers/hooks preserved |
| Concrete peer conventions fitted to MDIO | CONVENTIONS, 01-05, each feature stage | Actual source symbols and decisions for Status, health, methods, layout and ownership; conflicting peer behavior resolved explicitly |
| Maintainability and source/general documentation each prompt | AGENTS, COMMON, CONVENTIONS, every stage | Readable simple code, public Doxygen and procedure comments, affected guides/changelog updated, focused documentation checks |
| Model choice by task complexity and rule reminders | AGENTS, COMMON, 01 | Available models/effort selected with rationale; scoped delegation, entry reminder and final diff review |
| Preserve references and avoid GPL contamination | 02, 28, 32-34 | Original hashes/notices, source provenance and package exclusions |

## Important intentional limits

- Framework-neutral does not mean a working MDIO/MAC backend for every MCU.
  Native P4 integration and a portable consumer are required; further backends
  need actual platform evidence.
- Chip feature coverage means every documented programmable feature is
  implemented and tested; hardware-only or silicon-inapplicable properties have
  evidence-backed dispositions. Supported-but-unimplemented or unresolved
  features remain open completion gates. Raw access alone does not close them.
- Per-variant software tests do not certify every silicon revision, interface,
  medium or physical board. Physical gaps remain separate visible records.
- EtherCAT slave-controller notes cannot establish a P4 master's timing.
  Master stack, motion and system timing are application-level work.
- Current user instructions authorize DP8382x commit/sync checkpoints, one branch
  per execution and first-set remote CI only at prompt 29. Other sibling-specific
  model/fixture/release permissions do not transfer here.
- Scope does not include publishing a package, modifying siblings, controlling a
  loading machine or adding power delivery merely to finish metadata/examples.

## Promptset readiness review

Check links, all 34 stages, source-map paths, dependency order, naming consistency,
missing-gate treatment and source conflicts before execution. See the
[planning validation record](../reports/promptset-validation.md) for checks and
review performed while preparing this set. No host/SDK/HIL implementation result
is inferred from promptset validation.
