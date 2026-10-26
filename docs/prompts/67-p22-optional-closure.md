# 67 - Close DP83822 power, wake, LED and complete subtype coverage

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Inputs are the complete assigned feature
inventory in [62](62-p22-source-subtypes.md), implemented [63](63-p22-copper-core.md),
[64](64-p22-interface-clock.md), [65](65-p22-fiber.md) and
[66](66-p22-diagnostics.md) paths, plus existing primary optional-control APIs.

## Current evidence and ownership

Reopen TMP1x2 current configuration/capability and variant tests, MB85RC variant
metadata/actual dispatch, and peer tool tests distinguishing missing fixtures
from implemented controls. Inspect current public/source, SDK admission,
CLI/help, runner suites, registration and AGENTS. Record revisions, dirty state,
paths and adopted/adapted/rejected rules. Recheck SNLS505H and exact applicable
EEE/WoL/power/LED notes, including limitations of vendor reference APIs.

The lead owns shared capabilities, public types, integration and closure claims.
Delegate disjoint feature tests or read-only per-subtype ledger reconciliation
after contracts freeze; specify writable/forbidden files and independent
acceptance. Select available models by coding/inventory/semantics complexity
and review all contributions. Agents do not mutate Git or sibling sources.

## Work and acceptance

Implement the finite source-assigned differences for EEE/LPI and supported MAC
signalling, WoL/frame filtering, every PHY low-power state and wake transition,
LED/GPIO mux/polarity and remaining documented independent controls. Validate
inherited behavior rather than reimplementing common features. “Optional” means
runtime opt-in, not permission to omit software. Missing IRQ/LED/wake wiring,
optical equipment or P4 interface support limits physical/adapter evidence only.
If the source audit discovers a substantial unassigned family, repair the plan
explicitly instead of hiding it as unbounded residual closure.

For I/H/IF/HF independently, reconcile every register/non-reserved field/
documented procedure, all legal settings and reset/mode effects to source,
production symbol/typed path, CLI, independent test and fixture requirements.
Share source-proved identical records with explicit part applicability and
separate evidence; do not copy a family table unchecked. Hardware-only and
source-proved inapplicable rows need citations. Required unsupported-by-software
or ambiguous rows remain OPEN and prevent full software support claims.

Preserve whole-transaction ownership, one destructive collector, neutral mixed
register writes, bounded restoration and primary/cleanup uncertainty. Extend
independent literal vectors for every changed feature and each inherited
part/medium/interface applicability row. Test invalid masks/enums, incompatible
combinations with zero preflight callbacks, reset-loaded state, no-link versus
transport health, and zero-callback cached views. Inject each changed read/write/
cleanup failure, partial acquisition, accepted-write/error, reset mid-operation,
cancellation and exhausted primary/cleanup budgets. Verify complete traces,
output sentinels, retained evidence and unresolved effects. Reconcile capability
getters, SDK admission, CLI help/results and runner suite eligibility.

Run full subtype ledger reconciliation, focused host/CLI/Python/Doxygen checks,
previous-family regressions and actual supported SDK builds. Package checks
follow changed exports. Ship exact-part public/source Doxygen and README precise
support, CHANGELOG, variants/API/interface/feature/adapter/fixture guides; record
no-change reasons. Record counts, live audit and separate software/SDK/physical
gates in report, validation and progress. Check final diff against entry rules;
commit/sync coherent verified units and completion on the recorded execution
branch. Only prompt 71 executes/checks remote CI in this first set.
