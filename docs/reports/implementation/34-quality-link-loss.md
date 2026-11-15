# Prompt34: quality and hardware link-loss controls

Implemented the five owned ledger procedures: `configure_fast_drop`,
`signal_quality_sample`, `tx100_configure`, `tx10_configure` and
`link_recovery_configure`. The [quality guide](../../quality.md) documents the
typed profile, observations, restoration and qualification limits.

## Rules, scope and checkpoint

Started on clean, fetched `main` at
`e3c25db71c8520d83b997a04c5d107d64b69c707`, equal to `origin/main`.
Read the full attachment/repository prompt, COMMON and shared convention,
engineering, API/operation, coverage, evidence and verification rules, root and
parent instructions, architecture, variants and reference index. Stages18/20/33
provide verified link profiles, the sole collector and interface ownership.
The lead owns public headers, integration, shared guides, ledger and Git.
One application owner serializes instance access and complete controller batches.
Siblings remain read-only; vendor bytes and provenance are unchanged. Remote
CI is reserved for71; no workflow definition exists at this checkpoint.

Three parallel agents used the current capable model: `quality34_sources` for
independent original-PDF/sibling review and later ownership review,
`quality34_impl` for the cohesive quality source, and `quality34_tests` for
independent literal/failure tests and the compiling owner example. Scopes were
disjoint; reviewers did not edit code or Git. Their contributions were reviewed
against actual source and changes, not accepted from summaries alone.

## Source and sibling audit

Original SNLS647I August2025, SHA256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`,
sections7.6/8.3.12/8.3.13/8.3.17/8.4.1.1 and tables8-28..30/35/44/51..53/72
establish the field partitions and independent literals. The current
[TI datasheet URL](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) still resolves
to that revision. Targeted exact-part MSE/MLT3 clarification searches did not
locate evidence closing the recorded address/scaling/threshold conflicts.
SNLA423A and SNLA495 remain context; C2000/ESC timing and neighboring-part
register recipes are not adopted as P4 master behavior. No vendor code copied.

Fresh clean sibling revisions and inspected paths (relative to Projects):

- INA228 `4de84f2564eb6382835ca9a72d8688377657dcb4`:
  `include/INA228/INA228.h` DiagAlert/snapshot declarations;
  `src/INA228.cpp` readDiagAlert and retained raw evidence;
  `test/test_basic.cpp` cases2034/2070/2920/3136 and registrations;
  `examples/esp_idf/basic/main/main.cpp` cached diagsnap versus live diag;
  `tools/test_run_i2c_hil_parser.py` partial-health rejection.
- TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40`:
  `include/TMP1x2/TMP1x2.h`, `src/TMP1x2Configuration.cpp`
  validate/verify/update; `test/test_core.cpp` mismatch cases and registration;
  `examples/common/Tmp1x2Cli.cpp` snapshot/snapshotread;
  `tools/test_hil_tmp1x2_runner.py` trust/restoration-failure cases.
- LDC1614 `da104689de49ab8f492e7905f3eb7496f0866624`:
  `include/LDC1614/LDC1614.h` deferred status declarations;
  `src/LDC1614.cpp`1076-1124; `test/test_basic.cpp`357/386/2730 and registration;
  `examples/common/Ldc1614Cli.cpp` first destructive-status acquisition and
  cached batch projection; `tools/test_ldc1614_hil_runner.py` readiness and
  missing/duplicate/faulted-sample correlation. Each sibling AGENTS was read.

Adopted complete preflight, staged ordinary output, partial retained evidence,
passive rendering and mismatch preservation. Rejected sensor scaling/timing,
I2C ACK rules, CONFIG bookends as simultaneous PHY sampling, and LDC's
output-reset-on-failure convention. Existing pinned ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643` remains the adapter baseline;
no SDK code is introduced in this portable stage.

## Implemented behavior and review

`startConfigureQuality` copies a completely validated owned profile without
I/O/clock reads. The explicit quality phase switch uses the existing slot,
tokens, poll/deadline/stall rules and shared restore tail. Every complete RMW
stays under one controller transaction. A full profile uses at most52frames in
19callbacks; its read-only quality pass uses23frames in9callbacks. A sole
zero-cause disable uses8frames/3callbacks, avoiding a duplicate CR3 write.

The MII-only nonzero cause policy checks both declared and live interface.
Source threshold prerequisites are applied with causes disabled, then selected
causes enabled. Raw threshold overrides require explicit opt-in and do not
claim physical maxima. FIFO refers specifically to far-end loopback. Every
10BTSCR read retains polarity through the sole collector; PHYSTS is sampled
before its mirror is cleared. CR2 odd-nibble and GENCFG IPG remain separate
interface-owned fields. No additional VOD write is added.

Desired, partial observed and verified state remain independent. Reset, external
mutation, cancellation and failed restoration retain uncertainty and OP-9 debt.
Initialization/recovery replay the same pass after interface restoration and
before dependent outputs/final power. VERIFY performs no semantic write;
earlier mismatch survives a later failed read. Raw MSE has a four-frame staged
read and original-time stale cache, with no derived units or alternate probe.

Independent review corrected two evidence gaps before completion: changed
successful link observations now stale the MSE context, and invalidated quality
trust withdraws the FLD maximum-applicability flag. Public wording identifies
far-end-loopback FIFO and full replacement of previously owned subsections.
The finite source-specific pass adds no allocator, scheduler, registry, retry
loop, generic register framework or second destructive reader.

## Documentation and evidence

Updated public Doxygen, API/architecture/behavior, initialization, access,
interface/link/event guides, ledger ownership/evidence, feature coverage,
independent vectors, source notes, README/CHANGELOG, test guide, progress and
the [portable owner example](../../../examples/portable/Quality.h).
Variant and generated-version declarations need no change: no new silicon
family or release version is supported. Tool commands remain unchanged.

Windows GNU15.1/Ninja, C++17 Release, strict warnings and active checks:

- PASS: `cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DDP8382X_BUILD_TESTS=ON`, build and
  `ctest --test-dir build/host --output-on-failure --no-tests=error`:214 tests,
  including eight quality suites and their deliberately failing assertion probe.
- PASS: the same214 tests in `build/quality34-ubsan`, configured with
  `-fsanitize=undefined -fsanitize-undefined-trap-on-error`.
- PASS: separately configured `test/consumer` in `build/quality34-consumer`,
  tests OFF, build and two registered consumer tests, including compilation of
  the quality owner example without SDK dependencies.
- PASS: `python -m unittest discover -s test -p "test_*.py" -v`:103 tests.
- PASS: `python tools/verify_ledger.py`:97registers/52procedures/35features.
- PASS: `python tools/verify_references.py`:29 vendor artifacts,27 text extracts
  and all local links; vendor byte total46,886,437 unchanged.
- PASS: `python tools/check_doxygen.py`:250 HTML pages and two rejected negative
  fixtures; `python scripts/generate_version.py check`; `git diff --check`.

The first timing run exposed an early expected stall timeout in the new test:
corrected it to the frozen contract's initial non-stalled poll plus eight equal
clock stalls. The final suite passes. Final review also added literal BMSR/
PHYSTS jabber observation and failed-prefix retention, verified power/loopback
shared-field restoration, and withheld MLT3 maximum applicability for unexpected
preserved reserved readback. Those corrections are covered by executed tests.

SDK/controller link, installed-package qualification and physical hardware
remain NOT RUN in this portable stage. Remote CI is NOT RUN, reserved for71.
`mse_address`, `fld_threshold_recipe`, `fld_reset_defaults`, `analog_rx_gain`,
RECR arithmetic and RMII RC interpretation remain explicit gates. Missing MII
fixtures do not remove the implemented controls. Final commit/upstream equality
is recorded in the handoff, avoiding a self-referencing commit.

The later [independent prompts34-35 audit](34-35-independent-audit.md) adds
reset/mode-evidence and quarantined-MSE regressions and corrections. The results
above describe the original checkpoint; the audit records its own verification.
