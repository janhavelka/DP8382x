# Requirement and completion coverage

This map makes omissions visible. Stage numbers refer to the
[ordered index](README.md); implementation must later link actual source, tests
and evidence from the maintained feature matrix. A listed stage is planned work.

| User requirement / engineering need | Stages | Acceptance evidence |
| --- | --- | --- |
| Audit other chip libraries comprehensively | 1-2; repeated in every prompt | Current revision/path inventory, deeper code samples, adopted/adapted/rejected decisions |
| Follow sibling naming, layout and project ownership | 1-2, 7-10, 69, 77 | Real headers/metadata, maintained agent/contributor rules, verified owner information |
| Framework-agnostic MCU library | 6-13, 53-55, 68-70 | Actual portable core and isolated tests-OFF consumers; no SDK/sibling dependencies |
| Use supported ESP-IDF PHY/MAC framework | 6, 37-42, 71 | Method/ownership matrix, real adapter and linked P4 images |
| Exact DP83826I first | 3-5, 11-36, 53-55 | Datasheet-backed IDs/sequences, independent failure tests, primary capability coverage |
| Complete chip feature set beyond initial board use | 3-8, 11-36, 44-47, 53-67, 74-80 | Exhaustive register/field/procedure ledger; usable APIs, CLI and tests for every documented programmable feature; runtime opt-in is not deferred scope |
| Broad useful public/manufacturer research | 3-5 and every feature/family prompt | Pinned TI GitHub/SDK, exact docs, EVM/tool guides and applicable primary comparisons with provenance, license and conflict records |
| Concrete public-library data checked against reality | EVIDENCE through COMMON; 3-8, 11-41, 44-46, 53-67, 72-73, 78-80 | Pinned source catalogues; exact-part tables and independent literal vectors; observed/corroborated/conflicting/measured evidence kept distinct; confirmed mistakes corrected across prompts, implementation and tests |
| General DP8382x family | 56-67, 72-73, 80 | Per-exact-part register/interface/feature differences and separate qualification rows |
| Generic 10/100 and supported duplex modes | 17-20, 50, 53-55, 72 | Validated masks/forced modes, 10BASE-Te setup and suitable physical peer tests |
| Loading-machine EtherCAT application | 40-42, 47, 72 | Separate 100/full owner policy, maintenance admission and raw MAC boundary; no master/servo claims |
| Ordinary internet/Ethernet PCB application | 40, 68, 72 | General network example; PHY/link separated from DHCP/internet outcomes |
| Exclude power delivery | 6, 28, 40-42, 80 | No PoE/power-delivery API; optional PHY power state explicitly distinct |
| Bus/controller and configuration ownership | 6-8, 11-16, 37-39, 53-54, 79 | Whole indirect/RMW arbitration, context lifetime, callback bounds, IDF no-bypass tests |
| Status, passive health and freshness | 7-8, 11-22, 43-49 | Structured errors; separate transport/link/config/policy evidence and no-I/O snapshots |
| Full practical diagnostics | 23-35, 46, 51, 73 | Exact-capability loopback/BIST/cable/IRQ/LED/wake/power coverage, cleanup evidence |
| Useful example CLI | 43-47 | Shared portable executable processor, strict input, bounded output, owner routing |
| Python runners | 48-52 | Real session/artifact tooling, compiled-CLI and parser negatives, explicit fixtures |
| Tests and honest test claims | Every implementation prompt; 53-55, 71-73, 78-80 | Registered independent behavior tests; SDK, CI, hardware evidence separated |
| Packaging/version/license/maintainability | 9-10, 69-71, 74-80 | Deterministic metadata, clean package consumers, correct notices, current guides |
| FieldCore-style promptsets and consumer integration | 1-2, 6-8, 41-42, 68, 74-80 | Shared contracts/source map, prerequisites, ownership, reports and no FieldCore dependency |
| Re-audit sibling I2C libraries every time | COMMON and each numbered file | Fresh stage evidence; old report never substitutes for source inspection |
| Subagent collaboration | COMMON and each numbered file | Disjoint ownership, lead-controlled shared contracts/Git, integrated reviewed results |
| New branch per promptset; commit/sync each prompt and complete intermediate units | AGENTS, COMMON, every prompt | New branch before a new set's first stage, retained through all stages/resumes; checked coherent commits, actual push and verified upstream equality |
| First-set CI without repeated remote checks | AGENTS, 71, VERIFICATION | Remote execution/status only at 71; local checks throughout, original tested SHA retained, triggers/hooks preserved |
| Concrete peer conventions fitted to MDIO | CONVENTIONS, 1-10, each feature prompt | Actual source symbols and decisions for Status, health, methods, layout and ownership; conflicting peer behavior resolved explicitly |
| Maintainability and source/general documentation each prompt | AGENTS, COMMON, CONVENTIONS, every prompt | Readable simple code, public Doxygen and procedure comments, affected guides/changelog updated, focused documentation checks |
| Model choice by task complexity and rule reminders | AGENTS, COMMON, every prompt | Available models/effort selected with rationale; scoped delegation, entry reminder and final diff review |
| Preserve references and avoid GPL contamination | 3-5, 69-70, 77-80 | Original hashes/notices, source provenance and package exclusions |
| Manageable complete tasks in natural order | 1-80, COMMON, workload audit | One closed outcome per prompt; tests/docs/cleanup stay with behavior; all 34 former stages mapped; no missing or duplicate number |

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
  per execution and first-set remote CI only at prompt 71. Other sibling-specific
  model/fixture/release permissions do not transfer here.
- Scope does not include publishing a package, modifying siblings, controlling a
  loading machine or adding power delivery merely to finish metadata/examples.

## Promptset readiness review

Check links, all 80 prompts, source-map paths, dependency order, naming consistency,
missing-gate treatment and source conflicts before execution. See the
[workload audit and migration map](../reports/prompt-workload-audit.md) for checks and
review performed while preparing this set. No host/SDK/HIL implementation result
is inferred from promptset validation.
