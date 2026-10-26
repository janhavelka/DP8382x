# Planning audits and later implementation evidence

These reports describe the **2026-10-26 planning review**. They do not implement
or qualify a PHY. Sibling repositories were read-only; reported dirty states and
changing revisions are observations of concurrent work, not changes by this task.

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

Earlier audit comments about not inheriting sibling Git/CI permissions describe
the initial planning scope. The later explicit user instruction is implemented
in root AGENTS and the workflow follow-up; use those for current execution rules.

Use [the promptset](../prompts/README.md) to execute future work and
[implementation progress](../implementation-progress.md) for status. Stage
execution reports will live under `docs/reports/implementation/` when work occurs.
Reports retain evidence; architecture/API guides own enduring behavior contracts.
