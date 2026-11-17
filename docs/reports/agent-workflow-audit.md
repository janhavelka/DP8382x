# Agent guidance and checkpoint workflow audit

Historical numbering: DP8382x stage numbers in this report refer to the former
34-stage plan. Use the [current 80-prompt index](../prompts/README.md) and
[old-to-new mapping](prompt-workload-audit.md) for execution; its sole first-set
remote-CI checkpoint is now 71. The original findings below retain their dated
scope and are not current execution instructions.

Date: 2026-10-26. This follow-up adapts sibling agent guidance to the user's
explicit request for this repository. It changes planning and execution rules;
it does not start the implementation promptset or qualify any chip functionality.

## Sources inspected

Sibling paths are relative to the parent Projects directory. Repositories remain
read-only. Revisions identify observations; concurrently edited guidance is not
assumed to be a frozen release.

| Source | Observed HEAD | Practices adopted or adapted |
| --- | --- | --- |
| `TUSB32x/AGENTS.md` | `84dd5c269318f50b377243843bdc9306590fe517` | One execution branch, completed-unit checkpoints, explicit sync evidence, a dedicated first-set CI stage |
| `TMP1x2/AGENTS.md` | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | Simple responsibility-based code, explicit ownership, documented interfaces, honest verification and maintained tools |
| `INA228/AGENTS.md` | `b28c5ea69bcb4d26b351c52c24d3c6fae9fd282b` | Familiar library layout and naming, focused changes, source documentation and meaningful checks |
| `PCA9555/AGENTS.md` | `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | Portable driver boundaries, examples and tests as maintained consumers, concise completion evidence |
| Parent `AGENTS.md` | Local instruction file | Applicable baseline engineering and CI rules, with the user's explicit first-set CI exception |

Permissions do not transfer from sibling repositories. The user's current
request is the authority for DP8382x commit/sync checkpoints and reduced CI
frequency. Earlier audit statements declining to inherit sibling permissions
remain historical observations, not a prohibition on the new explicit workflow.

## Resulting rules

[Root AGENTS](../../AGENTS.md) owns the durable guidance. The shared prompt
contracts and all 34 numbered stages agree on these requirements:

- Use one recorded execution branch throughout a promptset, including resumed
  sessions and delegated work. Maintenance of the prompt documents does not
  itself create or start an implementation branch.
- Commit and sync every completed prompt and coherent verified units within a
  prompt. Include related tests and documentation, inspect staged changes, and
  preserve unrelated work. A no-change review does not require an empty commit.
- Sync includes fetching, safe integration if needed, pushing the working branch
  and verifying upstream equality. Only the lead changes shared Git state.
  Report blocked sync precisely; never force-push or bypass branch protection.
- During the first implementation promptset, only prompt 29 dispatches, polls,
  waits for or requires remote CI. Other stages retain focused local checks and
  actual SDK builds as appropriate. Preserve automatic triggers and required
  merge/release checks. Later changes do not inherit an older CI result.
- After the first implementation promptset, applicable parent CI rules resume.
  Release publication, fixture operation and sibling changes retain their own
  task scope.

The engineering guidance preserves full documented feature coverage and adapts
ownership to MDIO: serialize entire indirect transactions, distinguish PHY health
from link state, keep board/network/EtherCAT policy outside the portable core,
and reuse the supported ESP-IDF framework through a single owner. I2C-specific
transport semantics and unimplemented sibling APIs are not copied as facts.

## Review and evidence

An independent read-only review of root guidance, shared contracts, the progress
record and stages 29/32/33/34 found no actionable workflow contradictions.
Local documentation validation is recorded in
[promptset validation](promptset-validation.md). No driver, SDK, remote CI or
hardware result is implied. Checkpoint commit and synchronization results belong
in the final handoff and the next progress update, avoiding self-referencing SHA
updates.

## 2026-11-17 Current-code response

Review baseline: `77c544f415edb26a0e23de76591a0163b9b73147` on `main`.
The dated findings above remain history. The responses below account for every
resulting rule and the source/review qualifications; they do not execute another
numbered prompt. Root/parent AGENTS, COMMON, CONVENTIONS, ENGINEERING, current
progress, CMake registrations and the sibling sources were reopened.

| Finding / recommendation | Current response and evidence | Disposition |
| --- | --- | --- |
| One execution branch, including resumed/delegated work | Root AGENTS and COMMON now require `main` until version 0. The earlier execution branch was integrated on 2026-11-11. The former branch-creation policy is historical and must not be replayed. | SUPERSEDED by explicit current policy |
| Prompt maintenance must not execute implementation | COMMON still distinguishes maintenance from execution; this response changes reports only. Existing implementation status remains in `docs/implementation-progress.md`. | IMPLEMENTED rule |
| Commit each completed stage and coherent verified unit with tests/docs | Root AGENTS and COMMON retain this requirement, including no empty commit for a no-change review. The lead owns the final reviewed commit and synchronization for this maintenance unit. | IMPLEMENTED rule; final Git result belongs to handoff |
| Fetch/integrate safely/push/refetch/equality; exclusive lead Git ownership | Current root AGENTS gives that exact synchronization contract and preserves concurrent work, blocked sync and local SHA evidence. An old source snapshot or successful fetch is not a current push result. | IMPLEMENTED rule |
| First-set remote CI only at former prompt 29 | Current prompt `71-ci-checkpoint.md` owns workflow creation and exact-commit remote results. No workflow definitions exist at this baseline; prompts 1-70/72-80 retain local host/SDK/package checks. This is the explicit repository exception to parent policy. | SUPERSEDED numbering; OPEN at 71 |
| Later work cannot inherit an old green SHA; parent policy resumes after first set | COMMON and VERIFICATION preserve both rules. Current stage-39 host/P4 evidence is local evidence, not CI. | IMPLEMENTED rule |
| MDIO ownership, health separation, portable core and supported SDK integration | `Config.h`, `DP8382x.h`, `src/DP8382x.cpp`, `src/esp_idf/Phy.cpp`, `PhyControl.cpp` and `docs/design-decisions.md` implement one core/collector and controller-wide batches. `test/mmd.cpp`, `health.cpp` and `test/esp_idf/host/native.cpp` contain executing ownership/evidence cases. Reusable installed destruction remains blocked by `native_rx_quiescence`; wall-time claims remain conditional. | IMPLEMENTED admitted subset; named gates OPEN |
| Full feature scope; release, fixture and sibling scope remain separate | FULL_COVERAGE and `docs/feature-coverage.md` retain source-gated required controls and future exact-family/physical work. No sibling permissions, CI results or hardware authorizations are inherited. | IMPLEMENTED planning boundary; coverage OPEN |
| Dated independent review found no workflow contradiction | That is evidence for its recorded 2026-10-26 inputs only. Current policy was checked directly; former branch and CI numbers are explicitly superseded above. Historical model choices and sibling revisions are observations, not requirements. | HISTORICAL, correctly bounded |

Fresh sibling HEAD/clean-state observations and actual reused/rejected seams are
recorded in the [current sibling response](sibling-driver-audit.md). No sibling
was modified or tested. This report response is source/documentation evidence;
new host, SDK-link, package, remote-CI and physical results are not claimed here.
