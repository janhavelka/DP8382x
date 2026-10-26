# Changelog

## Unreleased

### 2026-10-26

- Require a new branch before every new promptset execution, retained through
  all stages and resumed sessions; do not reuse main or an earlier set's branch.
- Deepened all implementation prompts with source-backed sibling conventions,
  selected PHY Status/health/function names, concrete file responsibilities,
  failure cases, per-stage Doxygen/general documentation and model selection by
  task complexity. Implementation remains unstarted.
- Expanded AGENTS guidance from sibling conventions, with one branch per
  implementation execution, authorized commit/sync checkpoints per prompt and
  coherent intermediate unit, and first-set remote CI only at prompt 29.
- Extended the promptset to require complete documented chip functionality,
  broad manufacturer-first public-source research and exhaustive register,
  field and programming-procedure coverage beyond the first board's use case.
- Audited sibling chip-library structure, ownership, APIs, health, examples,
  CLI, Python runners, tests, packaging and CI, plus FieldCore-node promptsets.
- Added a 34-stage implementation promptset with shared engineering, CLI,
  verification and fresh sibling-audit requirements, coverage and progress records.
- Made the requested portable C++17 core explicit alongside ESP-IDF framework
  reuse, generic 10/100 operation and separate application/board ownership.
- Recorded PHY-specific implementation risks and per-variant qualification gates.

This change prepares implementation; it adds no driver, firmware or HIL result.

### 2026-10-24

- Created the DP8382x workspace using the sibling library directory conventions.
- Archived official TI device specifications and supporting technical documents,
  with source URLs, retrieval timestamps, checksums and searchable text.
- Recorded official TI and Espressif source references and licensing boundaries.
- Defined DP83826I-first scope, general 10/100 operation, ESP32-P4 integration and
  separate application policy for 100/full operation.
- Added local reference integrity and documentation-link verification.

No driver release or hardware qualification is associated with this setup.
