# Planning audits and later implementation evidence

The [prompt 45 report](implementation/45-cli-link-lifecycle.md) records shared
CLI link, event and lifecycle routes and executing verification.

The [prompts 43 and 44 review](implementation/43-44-independent-review.md)
records CLI clock, transport receipt and transcript fixes with local verification.

The [prompt 44 report](implementation/44-cli-configuration-registers.md)
records live CLI routes, bounded discovery and pinned SDK owner verification.

The [prompt 43 report](implementation/43-cli-foundation.md) records the
portable CLI source audit, terminal retention and real-core fixture tests.

The [prompt 40 report](implementation/40-p4-network-example.md) records the
ordinary P4 Ethernet application, native image and failure tests. Its
[board source audit](implementation/40-board-source-audit.md) verifies the latest
saved signal netlist and retains the unresolved PHY supply gate.

The [17-item audit follow-up](audit-followup-2026-11-17.md) corrects the BISCR
wire mask, removes drifted contract copies, qualifies source/native claims and
assigns remaining structural/test work. It supersedes named earlier dispositions.

The [2026-11-17 repository-wide responses](current-review-responses.md) revalidate
all 30 audit records individually against current code/prompts and source evidence.
Each audit now contains a dated response; the rolling promptset table covers
all 197 G/M/N/R findings. Historical counts and stage boundaries below remain
evidence of their own checkpoints; current progress is in the
[execution record](../implementation-progress.md).

The [prompt 38 report](implementation/38-esp-idf-state-arbitration.md) records public
SDK state reconciliation, native raw/collector routing, typed flow-control partial
effects and direct-start failure cleanup. Its
[source audit](implementation/38-native-source-audit.md) pins the live peer and SDK evidence.

The [prompt 37 report](implementation/37-esp-idf-lifecycle.md) records the native
factory/lifecycle, real pinned-driver host tests, P4 link and retained RX lifetime
gate. Its [source audit](implementation/37-native-source-audit.md) records current
sibling/SDK inputs separately from executing evidence.

The [2026-11-15 finding-by-finding dispositions](promptset-audit-dispositions.md)
independently check all 179 G/M/N findings and 18 contract-review mappings in the
remaining-prompts audit, plus the earlier prompt correctness/workload reports.
They distinguish implemented fixes, future acceptance requirements, superseded
recommendations and residual corrections; that maintenance left stage completion at 1-20.

The [prompt 21 report](implementation/21-passive-health.md) records passive
managed health, independent source/contract review, exact accounting tests and
the linked cached-health example.

The [prompt 22 report](implementation/22-reconcile-recover.md) records bounded
verification, retained mismatch and cleanup evidence, shared initialization
recovery, independent review and interrupted-restoration tests at its stage 22
checkpoint.

The [prompt 23 report](implementation/23-loopback-modes.md) records all loopback
session modes, verified stop/restoration, independent literal tests and retained
source/physical gates.

The [prompt 24 report](implementation/24-bist-generator-checker.md) records all
BIST controls, bounded raw acquisition and full restoration with retained
source/physical gates at its stage 24 checkpoint.

The [fresh prompts 21-22 audit](implementation/21-22-independent-audit.md)
checks actual source, tests and source evidence independently, fixes reset
observation retention on later errors, and records final local verification.

The [prompt 14 report](implementation/14-identity-capabilities.md) records exact
identity/capability implementation, current TI/peer review, independent failure
tests and the compiling probe example.

The [prompt 13 report](implementation/13-side-effect-register-access.md) records
the current named catalog, collector and mixed-register implementation, independent
PDF/failure review and COMMON's WP0–WP7 prerequisite corrections. It supersedes
earlier contract/source-policy decisions only where explicitly identified.

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
SDK correction dependencies as a historical design; the later owned wrapper
uses unmodified public SDK APIs and retains the RX lifetime gate.

The [prompt 7 report](implementation/7-value-transport-contracts.md) records the
value/Status/transport freeze, current sibling/TI source review, complete feature
mapping, independent error/validity review and local syntax/research checks.
Its [fresh independent audit](implementation/7-contract-audit.md) corrects BMSR
examples and health/time/release accounting without adding operation machinery.

The [prompt 8 report](implementation/8-operation-contracts.md) records exact
cooperative signatures, token/result/cleanup lifetimes, clock/budget arithmetic,
fresh peer/SDK evidence and the independent lifetime review. Executable timing
probes remain separate from future driver and native-link evidence.
The [fresh prompt-8 audit](implementation/8-contract-audit.md) records terminal
invalidation, fixed owner identity and native status-journal corrections.

The [prompt 9 report](implementation/9-portable-source-foundation.md) records
the shipped C++17 passive foundation, exact peer evidence, host/consumer tests,
source-document checks and remaining device/SDK/physical boundaries.

The [prompt 10 report](implementation/10-version-tooling.md) records strict
release-version generation, read-only checks, temporary preservation fixtures,
peer metadata precedents and ordinary consumer independence.

The [fresh prompts 9–10 audit](implementation/9-10-foundation-audit.md)
corrects current verification guidance and records independent code and scope
review without changing the shipped behavior.

The [prompt 11 report](implementation/11-clause22-transport.md) records usable
portable direct access, independent transport failure tests and PDF/ownership
review, with SDK and physical evidence kept separate.

The [prompt 12 report](implementation/12-atomic-mmd.md) records complete atomic
MMD batches, the five-frame RMW inferred from SNLS647I state retention (physical check pending), canonical alias policy,
phase-failure/exclusion regressions and current source review.

The [remaining-prompts audit](../prompts/REMAINING_PROMPTS_AUDIT.md) of 2026-11-02
reviews prompts 13-80, the shared contracts and the shipped code against primary
sources. It lists ordered corrections to apply before prompt 13; it changes no
status. It is kept with the prompts because the coder applies it from there.

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
