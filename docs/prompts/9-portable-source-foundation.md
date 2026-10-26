# 9 - Build the portable values, tests and source-documentation foundation

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites:
[7](7-value-transport-contracts.md) and [8](8-operation-contracts.md)'s accepted
foundational contracts. Restate owned code/tool paths, source review, simple
language, documentation/checks and the same-branch checkpoint.

## Complete outcome

Deliver a consumable C++17 foundation with implemented passive values, Status
helpers and pure validators, executing host tests and actual Doxygen checking.
No device facade, hardware success stub or unused operation engine is required.
Version generation is independently completed in prompt 10; consumers must
already work without Git, Python, Doxygen, SDK or sibling dependencies.

## Work and source audit

Inspect current TMP1x2 `Status.h`, `Config.h`, `CommandTable.h`, CMake/test
registration, `Doxyfile` and `tools/check_doxygen.py`; compare TCA9548A/24Cxx
standalone headers and separate consumers. Record actual paths/revisions and
adopt/adapt/reject choices. Do not copy an unfiltered sensor IDF source list
or private documentation machinery without a real local responsibility.

Implement only actual pure functionality under `include/DP8382x/`: selected
Status/error names, required model/config validators and source-proved constants
with access rules under `DP8382x::cmd::REG_*`/`MASK_*`. Driver/snapshot methods
enter `DP8382x.h` when they work. Prefer honest header-only passive code to
an empty `.cpp`; add source files when logic needs them. No future feature is
advertised as implemented merely because an enum exists.

Create the portable CMake target `DP8382x`, C++17 requirements, explicit source
selection and `DP8382X_BUILD_TESTS` default OFF. Exclude SDK and vendor/reference
trees. Add strict supported warnings, registered runtime tests and separate
tests-OFF consumer. Every portable header compiles alone. Verify error helpers,
invalid enums/combinations, unchanged inputs and meaningful mask/validation
behavior without thousands of constant-mirroring assertions. Keep runtime checks
active in optimized/Release builds and demonstrate a temporary failing case
causes a nonzero result; zero registered tests cannot pass.

Add useful public Doxygen immediately: parameters/fields, units, validity,
ownership, error/output behavior and any actual side effects. Introduce a small
`Doxyfile` plus `tools/check_doxygen.py` that runs Doxygen in temporary output,
checks required public pages/links and fails on generation errors. Exclude private
and vendor/build trees; no last-good publishing cache or generic docs framework.
Check missing public documentation and an invalid parameter reference with
temporary negative fixtures. `EXTRACT_ALL` page presence alone proves neither
comments nor correctness. Required missing Doxygen is NOT RUN/BLOCKED, not PASS.

## Verification, delegation and documentation

Use scoped coding subagents for build/test or documentation tooling only after
shared types are frozen; a stronger reviewer checks Status/validator semantics.
Record available model choices/rationale; the lead owns headers, integration and
Git. Give agents separate build directories and never edit siblings.

Run native configure/build/CTest with nonzero test counts, a real separate
tests-OFF consumer and isolated-header compilation. Adapt commands to actual
compiler/multi-config tools, for example `cmake -S . -B build/host
-DDP8382X_BUILD_TESTS=ON`, build and `ctest --test-dir build/host
--output-on-failure`. Execute Doxygen and its negative checks; retain reference
and diff checks. These commands become documented as working only after they run.

Update README/CHANGELOG, test/build/API/tool guides, evidence and progress with
the support actually delivered. Distinguish portable foundation from protocol,
SDK and physical evidence. Review the final diff for readable control flow and
unnecessary abstractions. Commit and sync checked output and coherent complete
units on the recorded branch. Remote CI remains prompt 71 only.
