# Planning audits and later implementation evidence

These reports describe the **2026-10-26 planning review**. They do not implement
or qualify a PHY. Sibling repositories were read-only; reported dirty states and
changing revisions are observations of concurrent work, not changes by this task.

The [current source comparison](../library-comparison.md), [prompt 1 report](implementation/1-core-api-state-ownership.md)
and [prompt 2 report](implementation/2-ecosystem-tooling-audit.md) record the
2026-10-28 live audits. They supersede older observations for those scopes.
The [prompt 3 report](implementation/3-silicon-core-ledger.md) and
[source re-audit](implementation/3-source-audit.md) add primary core silicon
research, independently reviewed vectors and executing ledger checks.

The [prompt 4 report](implementation/4-silicon-diagnostic-ledger.md) and
[prompt 5 report](implementation/5-silicon-power-interface-ledger.md) close the
remaining primary diagnostic and power/interface source inventory. Research
completion includes explicit source gates; implementation and qualification
remain separate.

The [fresh prompts 4–5 audit](implementation/4-5-silicon-ledger-audit.md)
checks completed artifacts against original PDFs and executable regressions,
recording the corrections and final local verification separately from the
original completion checkpoints.

The [prompt 6 report](implementation/6-framework-ownership.md) records the
2026-10-29 sibling/SDK re-audit, complete ownership design, poisoned-storage
experiment and independent concurrency/lifetime review. It selects explicit
SDK correction dependencies without claiming native implementation support.

The [prompt 7 report](implementation/7-value-transport-contracts.md) records the
value/Status/transport freeze, current sibling/TI source review, complete feature
mapping, independent error/validity review and local syntax/research checks.
Its [fresh independent audit](implementation/7-contract-audit.md) corrects BMSR
examples and health/time/release accounting without adding operation machinery.

The [prompt 8 report](implementation/8-operation-contracts.md) records exact
cooperative signatures, token/result/cleanup lifetimes, clock/budget arithmetic,
fresh peer/SDK evidence and the independent lifetime review. Executable timing
probes remain separate from future driver and native-link evidence.

- [Sibling drivers](sibling-driver-audit.md): primary driver source audit and
  broader inventory, naming, ownership, Status/health, CLI, runners, tests and CI.
- [FieldCore conventions](fieldcore-conventions-audit.md): actual promptset,
  module ownership, cached projection, test/verdict and collaboration patterns.
- [Additional family/tooling review](family-tooling-audit.md): TMP1x2, 24Cxx and
  TUSB32x source comparison and choices for this library.
- [PHY implementation boundaries](phy-implementation-audit.md): exact silicon,
  SDK reuse, MDIO ownership, EtherCAT scope and unresolved source interpretations.
- [Promptset validation](promptset-validation.md): local checks and independent
  review of the resulting planning documents.
- [Agent workflow follow-up](agent-workflow-audit.md): sibling guidance adapted
  to the user's commit/sync, single-branch and first-promptset CI instructions.
- [Core conventions follow-up](core-conventions-follow-up.md): concrete Status,
  health, lifecycle and register-procedure precedents fitted to PHY behavior.
- [Consumer conventions follow-up](consumer-conventions-follow-up.md): live CLI,
  owner, runner, example and test symbols used to deepen implementation stages.
- [Maintenance conventions follow-up](maintenance-conventions-follow-up.md):
  family, package, version, Doxygen and review evidence behind later stages.
- [Prompt workload audit](prompt-workload-audit.md): per-stage workload findings,
  complete-task boundaries, 1-80 execution order and old-to-new migration map.
- [Prompt correctness audit](prompt-correctness-audit.md): chip and SDK defects,
  source conflicts, simpler implementation choices and the new-branch rule.
- [Public-library data audit](public-library-data-audit.md): pinned TI and SDK
  register/procedure/timing extraction, independently corrected findings and
  mandatory datasheet comparison in every prompt.

Earlier audit comments about not inheriting sibling Git/CI permissions describe
the initial planning scope. The later explicit user instruction is implemented
in root AGENTS and the workflow follow-up; use those for current execution rules.
The correctness audit strengthens this to a new branch for every new execution;
root AGENTS owns current rules and older reports retain their historical scope.
Older reports use the former 34-stage numbers. The workload audit maps every
former stage to the current 80 prompts; the sole first-set CI stage is now 71.

Use [the promptset](../prompts/README.md) to execute future work and
[implementation progress](../implementation-progress.md) for status. Stage
execution reports will live under `docs/reports/implementation/` when work occurs.
Reports retain evidence; architecture/API guides own enduring behavior contracts.
