# Use source evidence and correct it before implementation

This contract applies to every prompt through [COMMON](COMMON.md). Read the
[extracted library evidence](../reference/library-evidence.md) for the topic
being changed. These findings are reviewed starting points, not instructions to
trust a vendor driver, this promptset or a previous agent without checking them.

## Required comparison

Before adding or changing a register value, mask, access method, reset default,
timing, identity rule or feature sequence:

1. Reopen the actual source file at its recorded commit. Read the calling code,
   helper implementation and field definition, not just a macro or search hit.
   Check relevant newer official fixes and record the exact revision selected.
   Several repositories containing identical code are one inherited source,
   not independent confirmation.
2. Confront the claim with the exact TI part/revision's datasheet: feature
   description, detailed field table, access notation, reset domain, timing
   table/footnotes and applicable errata or official clarification. Inspect the
   original PDF when text extraction loses columns, subscripts or reserved bits.
   An app note for another part and a function name cannot override those facts.
3. Record the actual evidence and disposition in the one maintained feature/
   register/procedure ledger. Distinguish **observed in library code**,
   **corroborated by exact datasheet**, **conflicting**, **unresolved** and
   **measured on identified hardware**. These are evidence descriptions, not
   new driver status enums. Planning source review has no physical measurements.
4. Derive independent literal vectors from the original specification. Exercise
   legal extremes, prohibited encodings, side effects, ordering and failure
   boundaries. A test that repeats the production constant or copied vendor
   algorithm cannot establish its correctness.
5. Correct confirmed errors in this repository's prompts, evidence, ledger,
   contracts, code, tests and affected documentation together. Explain the old
   assumption, actual source/table, corrected fact and regression case. Do not
   preserve a known defect just to match TI software or an earlier plan. Keep
   original vendor payloads unchanged; do not edit sibling/upstream repositories.

Record exact address space and operation: C22 register, MMD DEVAD plus wire
offset, or a printed/document-only address whose translation is still unknown.
Never silently truncate an address, strip high digits, infer an alias or use a
bit mask as a register address. A documented fixed recipe may authorize that
exact sequence without authorizing arbitrary writes to its reserved fields.

For timing, preserve value, unit, MIN/NOM/MAX classification, measurement start
and end, clock/mode/partner conditions and whether it is a silicon requirement,
SDK polling choice, application budget or actual measurement. A loop count,
typical value, sleep argument or successful mock is not a worst-case bound.
Account for scheduling, controller execution, transaction ownership and cleanup.

Never invent missing register meanings, supported variants, default values,
settling times, formulas, calibration, test results or upstream fixes. Mark
unknown bits unknown, keep conflicts visible and withhold the affected support
claim. Continue independently justified work. Manufacturer branding or passing
tests cannot resolve contradictory silicon evidence by themselves.

## Check against hardware when the claim requires it

When authorized fixtures are available, compare actual PHY IDs, mode/straps,
reset-loaded readback, MDIO traces, clocks and behavior with the source-backed
expectation. Record part/board revision, firmware and runner identity, conditions,
instrumentation, raw observations and errors. Investigate discrepancies instead
of weakening the test to pass. A physical observation qualifies its recorded
conditions, not all devices or a different datasheet revision.

Missing hardware does not excuse omitted software or justify invented results;
keep physical evidence NOT RUN/BLOCKED separately. Pure preflight uses declared
or already-valid cached facts. Verifying a live hardware prerequisite may require
bounded reads before mutation; do not promise both fresh observation and zero I/O.
Source research does not authorize flashing, resetting fixtures or moving servos.

## Handoff

Each affected prompt reports facts checked, sources/sections, rejected library
behavior, corrections and unresolved questions with the next evidence needed.
Use plain engineering language. Preserve the existing full feature scope,
independent tests/Doxygen, single execution branch and commit/sync workflow.
This audit rule adds no repeated remote CI requirement; checkpoint 71 remains
the only remote-CI gate for the first execution.
