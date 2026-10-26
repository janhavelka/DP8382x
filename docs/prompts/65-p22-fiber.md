# 65 - Implement exact DP83822IF/HF fiber behavior

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Required inputs are source-backed subtype/
medium facts from [62](62-p22-source-subtypes.md), shared reset/owner behavior
from [63](63-p22-copper-core.md) and interface controls from
[64](64-p22-interface-clock.md). Missing optical fixtures limit physical evidence,
not documented portable software scope.

## Current evidence and ownership

Reopen MB85RC exact variant capability/dispatch tests, TMP1x2 variant admission
tests and TUSB32x actual profile validators for incompatible combinations.
Inspect current public/source, SDK MAC admission, CLI/runner medium selection,
tests/check registration and AGENTS. Record revisions, dirty state, exact paths
and adopted/adapted/rejected conventions. Recheck SNLS505H fiber sections and
exact IF/HF source applicability rather than inferring from copper paths.

The lead owns shared medium/configuration types, capability data and integration.
A useful delegation is read-only fiber versus copper trace review or a disjoint
IF/HF literal-vector suite after signatures freeze. Assign writable/forbidden
paths and acceptance; select available models by coding or silicon complexity
and review all results. Agents do not mutate Git or sibling sources.

## Concrete source checks

Recheck [fiber controls and source conflicts](../reference/library-family.md#fiber-reset-diagnostic-and-power-procedures)
against SNLS505H: CR2 `0x000A[14]` FX, GENCFG `0x0465[0]` detect polarity,
SOR1 `0x0467` strap fields `[11:10]`/`[9:8]`. Linux passes set value1 with
clear mask `0x4000` in FX configuration; inspect helper semantics and reject
that shifted-write mistake using independent literal traces. A software enable
does not prove signal-detect wiring or optical medium. Include exact supported
forced duplex modes and 100-Mbit FX behavior without AN, then validate interface
independence and copper-recipe exclusion. Any unresolved essential control
meaning remains OPEN; real optical evidence is reported separately.

## Work and acceptance

Implement documented IF/HF 100BASE-FX medium selection, startup/configuration,
legal link modes, status/resolution and reset/restoration. Preserve observed ID
versus board-declared subtype and physical medium evidence. Copper-only I/H
reject unavailable fiber selections before callbacks. Do not infer optical
hardware or suffix from shared PHY IDs.

Fiber is independent of MII/RMII/RGMII MAC-side selection. Expose source-proved
fiber through a supported MAC interface; do not require a P4 RGMII build or erase
generic PHY interfaces because one adapter cannot use them. SNLS505H 7.4.6
excludes 100BASE-FX from autonegotiation. Implement its documented forced-mode/
link semantics explicitly; reject incompatible negotiation requests and avoid
copper advertisement, MDIX, TDR or 10BASE-Te recipes by analogy. Reconcile
interrupt/status/counter, power and diagnostic applicability with the following
feature tasks, preserving source-proved shared routines rather than duplicating
the driver.

Register independent literal SNLS505H vectors for every legal IF/HF medium/
interface/mode row and incompatible subtype/medium/interface selection. Cover
invalid enums, shared/unknown/revision-mismatched IDs, no-link and incomplete
observations. Pure preflight rejection causes zero callbacks; an identity-based
mismatch allows only bounded observation reads. Verify copper-only recipes never
execute in fiber, and trace media changes plus reset/restoration of owned fields.
Inject every changed read/write/cleanup failure, accepted-write/error, partial
configuration, cancellation, reset mid-operation and exhausted primary/cleanup
budgets. Preserve output sentinels, backend detail, primary/cleanup errors,
retained events and unresolved hardware effects. Cached capabilities/status
remain bus-silent and no optical link is not an MDIO transport error.

Reconcile adapter admission, CLI help/results and fixture suite eligibility.
Ship Doxygen medium restrictions/units/effects/citations and README/CHANGELOG/
variants/API/interface/optical-fixture guides, with inspected no-change reasons.
Run affected host/CLI/Python/docs and actual supported SDK links, preserving
previous copper/family regressions. Record counts, source audit and exact
untested optical rows in report, validation and progress. Check the final diff;
commit/sync verified units and completion on the execution branch. Only 71
executes/checks remote CI; compilation cannot establish physical fiber support.
