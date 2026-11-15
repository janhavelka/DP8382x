# Prompt32: LED and GPIO controls

Implemented the source-admitted `led_configure` and `pin_configure` procedures
through one typed profile, bounded direct call and shared restoration pass.
[The guide](../../led-gpio.md) defines legal modes, ownership and retained evidence.

## Rules, scope and checkpoint

Started on clean fetched `main` at
`328b5edbfe3b33c45d680aa34327bd96ef79b4a0`, equal to `origin/main`.
Read the complete pasted/repository prompt32, COMMON, CONVENTIONS, EVIDENCE,
FULL_COVERAGE, parent/root instructions, architecture, variants, reference index
and the existing IRQ/WoL/access/restoration contracts. Siblings are read-only;
vendor payloads and hashes are unchanged. The lead owns shared public values,
integration, guides/progress and Git. No workflow exists; first-set remote CI
and automatic triggers remain prompt71.

PascalCase values, camelCase methods, existing Status and aggregate terminal
MDIO contracts remain authoritative. Applications serialize every instance access
and all complete indirect/RMW bus transactions, and own MAC stop, board pin/clock
availability, output quiescence and retry policy. The new source has one current
responsibility, with no pin registry, alternate collector or generic job engine.

## Original-source and sibling review

Original SNLS647I August2025, archived SHA256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`,
section8.3.18 and tables8-42/43/49/73..78/85/89 establish the field vectors.
The independent source reviewer checked source0..E, four-bit LED0 placement,
separate LED1/2/3 selectors, reserved LEDCFG low nibble/LEDCFG2 upper fields,
blink/stretch, per-pin polarity/force, IO_CFG1 runtime override and resulting
read-only0308 status. BMSR consumption clears link-loss indication; BICSR1's
capture/clear command affects PRBS indication, not an ordinary BICSR1 read.

Bounded current TI/product/E2E searches found no inspected stronger resolution
of the retained `pin_fields` contradictions. This is not proof that no
clarification exists. No external implementation was copied or licensed payload
changed. GPIO0/3 read-only mux, pin31 codes5/6, BASIC polarity override,
LED3 MII and PHYCR[5] remain explicit Q12 gates.

Fresh clean sibling revisions and actual reopened paths, relative to each root:

| Reference/revision | Inspected declarations, implementation and executing cases |
| --- | --- |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | `include/PCA9555/PCA9555.h` settings/operation declarations; `src/PCA9555.cpp`261-334 shadow invalidation and tracked writes; `test/test_basic.cpp`1124/1143 ambiguous effects and registrations; example CLI cached settings, runner and agent guidance. |
| LDC1614 `da104689de49ab8f492e7905f3eb7496f0866624` | `include/LDC1614/LDC1614.h` interrupt/pin contract; `src/LDC1614.cpp`1092 readDataReady/1270 readIntb; `test/test_basic.cpp`2954 optional INTB case/3137 registration; live versus cached CLI, runner and guidance. |
| SCD41 `0b9d3d02c27feacf9b1354dc2a8b83a9e5a04879` | `include/SCD41/SCD41.h` readDataReady declaration639; `src/SCD41.cpp`1129/1544 command/response path, tests/dispatcher/examples and guidance. No public IRQ API exists; data-ready response does not define PHY pin behavior. |
| RV3032-C7 `15d7a2782f7d6e136d29ff7339c1bee27bc5b8aa` | `include/RV3032/RV3032.h` IRQ/CLKOUT controls; `src/RV3032.cpp`1330 clock guard; `test/test_native/test_datetime.cpp`8970 final-byte mismatch/11089 registration, CLI, runner and guidance. |

Adopt separate desired/observed/trusted state, explicit pin consumers, staged
results and retained uncertainty. Reject I2C ACK, sensor conversion and RTC timing
as MDIO/PHY procedure evidence.

## Implementation and review

Three inherited capable-model agents had disjoint scopes: `led32_sources`
reviewed original tables and siblings, then authored only the pin guide;
`led32_impl` authored only the cohesive pin source; `led32_tests` authored
independent literals/failure tests and the affected IRQ cases. The inherited
model was appropriate for ownership/deadlines and silicon review; no alternative
model was needed. Review builds used separate directories. The lead inspected
actual source/diffs and integrated shared headers/state, docs, ledger and Git.

`configurePins` validates the entire profile and full direct budget before intent
changes. Sources/value preparation precedes clock release, mux and final force
enable. Every RMW is indivisible; readback compares owned stable masks. The
maximum all-control profile is84frames/23callbacks; smaller profiles admit their
exact plan. There is no silicon wait, automatic retry or compensating write.
Reset/verify reuse this pass through the existing cooperative operation slot.

WAKE/IRQ entries require existing source ownership and never duplicate source
mux writes. IRQ now disables delivery before configuring/restoring its own narrow
route. Source setters cannot abandon a declared dependent pin. Cached pin trust
also depends on matching verified source ownership. Empty profiles release intent
without changing physical routing or inventing a fresh hardware timestamp.

Confirmed review findings corrected and regression-tested:

- Empty pin restoration must continue later source/power tails.
- IRQ mux restoration must not require an unrelated WoL owner; initial setup
  also needs a source-owned route procedure rather than circular prerequisites.
- Later source setters cannot move a still-owned pin dependency.
- Independent reads that contradict proved fields freeze replay before further
  writes; failed requested words lose current validity without fabricated loss.
- Source/value preparation precedes runtime clock release; raw read-only mux/clock
  contradictions withdraw proof, and passive shared-role views follow source trust.
- A successful later source-owner read of an unowned mux does not falsely stale
  the already restored LED profile; real contradictions still withdraw proof.

Ordinary pin observations remain separate from destructive collector history.
Accepted/uncertain failures retain OP-9 debt; separately budgeted initialization
restores persistent ownership. Cancellation/teardown cannot undo accepted writes.
NONE/PCS/HARD/RESTART and power/diagnostic recovery share the same path. No claim
of POR strap resampling, GPIO input/tri-state or physical glitch-free switching.

## Documentation and verification

Updated README/CHANGELOG, public Doxygen, API/architecture/behavior/initialization,
side-effect and pin guides, ledger/coverage, independent vectors, source comparison,
test guide, progress and [portable example](../../../examples/portable/Pins.h).
Variant admission and version truth need no change. No additional family or SDK
support is claimed. Planned CLI uses these same typed owners and cached views.

Windows GNU15.1/Ninja/C++17 Release, strict warnings and active test assertions:

- PASS:196 registered host CTests, including eight pin suites and their deliberate
  Release failure-path probe; existing IRQ tests now cover source-owned mux setup.
- PASS: separately configured tests-OFF `build/pins32-consumer`, two CTests.
- PASS:103 Python tests; ledger97registers/52procedures/35features; reference/local
  links; Doxygen228 pages and two negative fixtures; generated version and diff checks.
- NOT RUN: SDK/controller and installed-package qualification; this stage verifies
  portable source consumption and host protocol behavior.
- NOT RUN: remote CI, reserved for71; physical Q12 output levels, load/visibility,
  blink/stretch timing, clock/COL handoff, polarity and contradictory mux evidence.

Final synchronized commit/equality is recorded in the handoff, avoiding a
self-referencing report commit. Existing unrelated source and physical gates remain.
