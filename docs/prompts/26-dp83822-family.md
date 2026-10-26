# 26 - Implement exact documented DP83822 variants

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-25 software outputs; independent unresolved variant
gates remain explicit.

At entry apply CONVENTIONS' names, Status/health, layout and simplicity rules;
reopen current relevant sibling sources. Keep source Doxygen and affected
README/CHANGELOG/guides current, verify locally and checkpoint/sync on this
execution's recorded branch.

## Fresh audit and owned work

Re-audit peer per-model capability and nonuniform hardware validation patterns. Read
SNLS505H, relevant DP83822 TDR/app notes and exact orderable-part tables.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Repeat FULL_COVERAGE's complete public-source and register/field/procedure audit
for every exact part in this stage. Implement all documented programmable
features, including those absent from the first board or upstream driver's API.
Track inherited, different and newly available controls explicitly. Identity and
common link operation alone do not complete variant support; software gaps stay
open while hardware qualification remains a separate evidence layer.

Qualify DP83822I, H, IF and HF individually. Build a matrix of observed identity,
board-declared subtype, copper/fiber and RGMII/RMII/MII capabilities,
clock/reset/straps, optional features and documented temperature/package distinctions.
Include exact RGMII timing/delay controls where supported; a particular MAC adapter may
reject an interface without removing it from the generic device model. Do not infer
suffix from shared IDs or expose fiber on copper-only parts.

Review all standard and extended register differences, startup/10 Mbit requirements,
interrupt/latch/counter behavior, link decoding and diagnostic procedures. DP83822 TDR
instructions apply only to supported exact conditions. Avoid flattening all variants
into a shared feature bitset without source evidence.

Implement shared behavior plus explicit differences with maintained bounded ownership
and reset/restoration. Include copper/fiber configuration validation and supported
forced/negotiated mode semantics. Any feature requiring an absent medium/fixture remains
honestly unqualified, even if software paths compile.

Update portable API capability queries, native adapter, CLI feature coverage and Python
suite selection. Keep undocumented variants unsupported and preserve all previous part
regressions. A large family namespace is not evidence of family-wide hardware support.

## Concrete subtype, interface and medium closure

Use stage 04's exact-part/interface/medium and operation signatures; keep one
shared API. The lead owns those contracts, shared variant metadata and adapter
integration. Useful delegated work is a read-only subtype/interface audit and
disjoint `test/variants/test_dp83822.cpp` vectors after freeze. Assign exact
paths/forbidden files, select available models by complexity and review every
contribution. Follow CONVENTIONS' naming, Status, passive health and simplicity.

Reopen `../MB85RC/include/MB85RC/CommandTable.h::VariantInfo/KNOWN_VARIANTS`
and source/tests separating observed identity, capability and implemented
support; `../TMP1x2/test/variants/test_variants.cpp` for absent wiring versus
internal controls; and current `../TUSB32x/include/TUSB32x/Config.h::Variant`
and actual profile validators/tests for revision differences. Record current
revisions/dirty state and relevant CLI/runner/build/AGENTS evidence. No inherited
family-wide support or sibling hardware claim is transferable evidence.

Create individual I/H/IF/HF ledger rows covering every register/field/procedure.
Record observed ID/mode/revision, declaration, copper/fiber, MII/RMII/RGMII,
clocks/delays, reset/straps, legal link modes and optional features, plus
hardware-only package/temperature facts. A source-backed disposition is needed
for each subtype; do not copy one table across four devices unchecked. Shared
IDs cannot identify a suffix, interface wiring or fiber presence.

Implement documented RGMII timing/delay/clock controls with units, legal
encodings, reset values and safe programming order. Portable support includes
documented interfaces even when the current P4 MAC cannot use them. Adapter
admission rejects its unavailable interface combinations explicitly and leaves
the generic PHY capability intact. Do not require an impossible P4 RGMII/fiber
build as the only way to prove portable behavior.

Implement medium-specific selection, link resolution and forced/negotiated
semantics. Do not apply copper advertisement, MDIX, TDR or 10BASE-Te recipes to
fiber by analogy. Reconcile all extended register differences, read-clear/latch
evidence, counters, interrupts, power/wake/EEE/LED and test modes. Use existing
bounded procedures plus small explicit differences, one collector and one
whole-transaction owner; no copied driver or per-register class. Missing fixtures
limit physical evidence, not documented software coverage. Reconcile capability
getters, adapter admission, shared CLI commands and runner suite selection.

Register an explicit host row for each part/medium/interface with Release-active
checks and independent literal SNLS505H/SNLA253 vectors. Test legal/boundary
delay encodings/units, invalid enum casts, incompatible subtype-medium-interface
combinations, shared/unknown IDs/revisions and inaccessible registers. Pure
preflight rejection causes zero callbacks. Observed-identity rejection permits
bounded identity reads but no writes or further feature I/O after mismatch.
Verify changed startup/link/diagnostic and reset/restore traces; inject
every read/write/cleanup failure, accepted-write/error, reset mid-operation,
deadline exhaustion and cancellation. Check output sentinels, preserved primary
and cleanup detail, bounded restore and unresolved effects. Include no-link
versus transport failure and zero-callback cached snapshots.

Ship Doxygen exact-subtype/mode restrictions, units, effects and citations beside
declarations/sequences. Update README precise support table, CHANGELOG Unreleased,
`docs/variants.md`, API/interface/feature guides and adapter/fixture/runner docs.
Keep each physical row tied to exact subtype/revision/medium/interface. Record
actual commands/counts, live audit and remaining gates in
`docs/reports/implementation/26-dp83822-family.md`, validation and progress.
Run affected local host/CLI/Python/docs and supported SDK checks; remote CI is
reserved for stage 29. Essential unresolved software keeps support incomplete.

## Acceptance and handoff

Run a per-exact-part host matrix including interface/medium rejection, shared/ambiguous
IDs, register differences, link and optional diagnostics, plus regression for original
DP83826I. Build supported SDK configurations. Hardware fixture rows name exact
subtype/medium and cannot inherit another member's pass.

Expected output: Documented DP83822 subtype support and explicit medium/interface
limits. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
