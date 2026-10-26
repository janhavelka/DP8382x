# Verify chip and framework evidence

Apply this contract to each changed address, mask, encoding, identity, access
rule, timing and procedure. Start with the relevant
[public-library catalogue](../reference/library-evidence.md), then inspect its
primary sources. Use actual data and retain unknowns explicitly.

## Establish the fact

1. Read the pinned source file, callers, helpers and field definitions. Check
   relevant official fixes and record the selected revision. Identical code in
   several repositories counts as one inherited implementation.
2. Compare with the exact TI part/revision's feature description, detailed field
   table, access notation, reset domain, timing columns/footnotes and applicable
   errata or official clarification. Inspect the original PDF when extraction
   loses columns or symbols. Exact-part specifications govern silicon behavior;
   SDK source governs that SDK implementation.
3. Record the fact, applicability and disposition in the maintained register/
   feature/procedure ledger: observed in code, corroborated by specification,
   conflicting, unresolved or measured on identified hardware. These labels are
   evidence descriptions, separate from driver statuses.
4. Derive literal expected vectors independently from the specification. Cover
   legal extremes, reserved/prohibited encodings, access effects, ordering and
   failure boundaries. Verify field partitions and units with original tables.
5. Update affected instructions, ledger, contracts, code, tests and documentation
   to the supported behavior. Retain source provenance and material unresolved
   discrepancies in the evidence record. Keep original vendor payloads intact.

Record address space explicitly: C22 register, MMD DEVAD plus wire offset, or a
printed address awaiting a supported translation. Keep bit masks distinct from
addresses. Limit recipe-only offsets/reserved-field writes to the documented
values, prerequisites and order of that exact procedure.

For every timing value record units, MIN/NOM/MAX, start/end events and applicable
clock/mode/partner conditions. Distinguish silicon requirements, SDK polling
policy, application budgets and measurements. Include scheduling, lock/controller
execution and cleanup when establishing an operation bound. Anchor minimum
post-write waits to an observed completion time or a conservative later sample.

Use the exact-part specification for source-backed software behavior when a
reference implementation disagrees. Keep the discrepancy visible and qualify
physical support separately. If authoritative sources contradict one another
on essential behavior, retain an explicit gate with the next evidence needed;
continue work justified independently. Missing data stays unknown: register
meanings, timing limits, calibration, support and test results require evidence.

## Compare with real hardware

For authorized fixtures, compare PHY IDs, straps/mode, reset-loaded readback,
MDIO traces, clocks and behavior with source-backed expectations. Record exact
part/board revision, firmware/runner identity, instrumentation, conditions, raw
observations and errors. Investigate discrepancies and qualify only the tested
conditions. Preserve required assertions when results differ.

Record unavailable physical evidence as NOT RUN/BLOCKED while completing its
software, test cases and fixture instructions. Pure preflight uses declared or
valid cached facts. A live prerequisite check uses bounded reads before mutation.
Keep those two phases distinct in API, CLI and test expectations.

## Handoff

Report checked facts, source locations, current dispositions, independent tests
and unresolved evidence needed. Keep the implementation instructions focused on
current behavior; source findings belong in the ledger/catalogues. Use the
COMMON verification and commit/sync workflow.
