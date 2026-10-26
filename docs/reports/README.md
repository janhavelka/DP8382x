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

Use [the promptset](../prompts/README.md) to execute future work and
[implementation progress](../implementation-progress.md) for status. Stage
execution reports will live under `docs/reports/implementation/` when work occurs.
Reports retain evidence; architecture/API guides own enduring behavior contracts.
