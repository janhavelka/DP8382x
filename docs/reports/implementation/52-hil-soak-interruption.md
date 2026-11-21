# Prompt 52 · Finite soak and interrupted campaigns

## Implementation checkpoint

Phase (b) started on clean `main` at `93390a8368d51228efd035f4c6406761d20f8f1a`.
The wrapper verified equality with `origin/main` after prompt 51. The
[observation campaigns](50-hil-observation-modes.md) and [feature fixtures](51-hil-diagnostic-fixtures.md)
provide the admitted commands, owner assertions, typed stops and saved-profile readback.
The exact-part ledger assigns zero new PHY procedures to prompt 52.

The runner admits a count and primary duration before it reserves evidence
or attaches a carrier. Each iteration uses the existing case procedure and
one shared boot, nonce and request sequence. The receipt distinguishes a
write attempt, a completed case decision, any required restoration, and
the remaining cases. Interrupted replay uses the number of consumed records
and exact outbound prefix to exclude bytes that arrived during teardown.
Setup before the nonce has separate write/terminal evidence. After an
uncertain write or lost framing, later bytes cannot authorize another
primary command. The one bounded recovery nonce remains available after an
eligible serial reply timeout. The [runner guide](../../hil-runner.md#finite-soak)
gives the arithmetic, exit statuses and commands.

Two implementation agents used the inherited high-reasoning model for
the repeated-case state and exhaustive interruption tests. Their writable
files were disjoint. The lead owned session and artifact contracts, the
primary/cleanup clock, test registration, integration and this record.

| Read-only sibling, clean revision | Source and selected convention |
| --- | --- |
| PCA9555 `b9297a45a6f18fa8251e95857aedea0588a32e27` | `tools/test_run_i2c_hil_parser.py::test_aggregate_destructive_failure_runs_recovery_before_stopping`, `tools/run_i2c_hil.py::run_aggregate_commands`, and the ESP-IDF CLI recovery image. Keep primary and restoration outcomes in wire order. DP8382x requires clean framing and continuity for any subsequent write. |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `examples/common/DiagnosticCore.h::restoreVerified`, the verified write path in `include/MB85RC/MB85RC.h` and `src/MB85RC.cpp`, `test/test_basic.cpp` and `tools/test_hil_runner.py`. Require independent saved-state readback and complete counter receipts through the existing owner. |
| TCA9548A `63b356afd6cb90642628e407548cacea3851aa96` | `tools/tca9548a_hil.py::run_soak` and `append_not_run_results`, `tools/test_tca9548a_hil.py`, `tools/test_hil_artifacts.py`, and the example CLI. Retain completed counts, unfinished rows and prior artifacts. Apply the primary deadline during each iteration and keep all-source frames separate from managed health. |

The telemetry schema follows `examples/common/Dp8382xCli.cpp` and the
collector in `src/DP8382x.cpp` and `src/DP8382xEvidence.h`. In particular,
the last complete false-carrier sum, its lower bound, software overflow
and the retained raw RECR word describe different observations. Native
pump loss counters are present only on the corresponding native console.
No new silicon timings, permissions or device variants enter this stage.

## Offline evidence

Windows 11 x86-64 build 26300, GNU C++ 15.1.0, Release with assertions,
Ninja, CMake 4.0.1 and Python 3.12.10. The compiled fixture used the real CLI
processor, operation service and simulated management bus.

| Layer | Phase (b) result |
| --- | --- |
| Host and offline replay | PASS, 16/16 focused Release CTests. The registrations cover repeated cases, duration and count boundaries, first failure, closure, saturation qualifiers, console backpressure, manifest changes and replay. |
| Portable or exported consumer | NOT RUN in this phase. The changes affect the Python runner and host test registration. |
| SDK link | NOT RUN in this phase. The native application and SDK inputs are unchanged. |
| Remote CI | NOT RUN. Prompt 71 owns first-execution CI. |
| Physical | NOT RUN. First-board INT, stock native feature admission and the deep-power exit gate retain their earlier dispositions. |

```powershell
cmake -S . -B build/p52-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/p52-release --parallel 4
ctest --test-dir build/p52-release -R '^cli_(hil_(runner|serial|campaign|features|soak|interruptions)|native_console_retention|console_writer_backpressure|processor_(retention|operation_edges|run_edges)|.*transcript)$' --output-on-failure --no-tests=error -j 3
python -S -m unittest discover -s test -p 'test_*.py' -q
python -S tools/check_doxygen.py
python -S scripts/generate_version.py check
python -S tools/verify_ledger.py
python -S tools/verify_references.py
git diff --check
```

The interruption suite cuts 2,927 actual command prefixes, including
287 after an operation admission, 18 external-action boundaries and 23
before-nonce setup boundaries. Every prefix checks the actual writes,
case outcomes, remaining iterations and offline replay. Compiled held-TDR
timeout and cancellation retain one terminal and service the separate stop
under a stalled output queue. Watch and stress retain a cancelled terminal
and one completed round. Publication failure leaves the earlier run intact
and preserves primary/restoration evidence in the diagnostic.

The mutation tests remove, corrupt and duplicate each required correlated
field occurrence, including in the second iteration. The mutating-case
matrix rejects 432,972 mutations. The soak suite rejects 19,059 across
three repeated compiled cases and 4,245 in the injected native serial stream.
A known failure followed by a stalled serial close exits with carrier status
2 and preserves the FAIL campaign verdict for replay.

The guide's compiled 10BASE-Te example and `verify-artifacts` both exited 0.
The `phase-b-10base-te` pair in `build/p52-release/evidence` records
two requested, attempted and completed iterations, zero remaining, and
PASS on both restorations. It contains 83 confirmed commands, 108 ms of
host elapsed time through closure, and 32,165 transcript bytes with SHA-256
`0353e05f4bd18fe170cabd5348593c290808c7695d70f368bc54a50db273eb87`.
Identity was simulated DP83826I ENHANCED, RMII address 1, firmware
prefix `abcdef0` and boot 3512936843. Its primary window was 8,000 ms.
All 7,011 required-field mutations of this saved transcript were rejected.
Python discovery ran 231 tests with 43 compiled-only skips. Doxygen checked
293 generated pages and rejected both negative fixtures. Version, reference,
ledger and whitespace checks passed.

Phase (c) reviews the wrapper's actual implementation commit for deadline
ownership, interrupted replay, authorization and worst-verdict behavior.
Phase (d) owns full local verification and completion.
