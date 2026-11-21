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

## Independent review, 2026-11-21

Two read-only reviewers examined `93390a8..de7a27c` in parallel on the
inherited high-reasoning model. One traced clock ownership, framing,
mutations and restoration. The other reconstructed interruption prefixes,
aggregate decisions and file publication. They reproduced failures under
`build/p52-review-timing` and `build/p52-review-artifacts`. The lead owned
all changes and ran separate reproductions under `build/p52-review-lead`.
The implementation checkpoint was clean and wrapper-verified at
`de7a27cf3d4f5c01287dcd5aa810410b8e8e3e6b` on entry. The three sibling
revisions and adopted conventions in the table above were rechecked.

| Observed defect | Review fix and executing regression |
| --- | --- |
| Removing `rxfs_valid` or `physcr_valid` from the pre-nonce IRQ terminal could leave a passing pair with a correct hash. | Live setup and replay now run the feature's typed write and readback assertions, check timing and carry used tokens into the nonce session. The required-field matrix starts at the first setup record. The runner rejects a rehashed IRQ pair missing its setup field. |
| Primary expiry could leave the target profile active after a confirmed write, even when cleanup was already queued. | At a clean request boundary, the owner drops the unissued primary step and runs its guarded cleanup within the original remaining allowance. Replay checks the exact record and intervention counts at that boundary. The compiled tests expire after the target apply, at entry to restoration, and with an active IRQ test. They require one restore or stop, successful exit proof, a duration stop and a second NOT RUN row. |
| A blocked operator prompt held the serial owner beyond its deadline. | Console I/O has a bounded worker. After a stall, further operator prompts are declined, while independent cleanup and serial close proceed. The injected peer case verifies one prompt, two intervention outcomes and one close while that worker is still blocked. A separate subprocess with blocked final output exits with its published verdict. |
| Replay accepted an invented continuous session after a short write. | The verifier matches the fresh nonce write and response to the timed-out request. The live checker and replay share boot, request and identity checks. Injected tests cover a short write, continuous and unproven sessions, a changed boot and an incomplete recovery tail. |
| A declined final calibration intervention yielded a complete INCONCLUSIVE receipt that failed replay. | Replay consumes the last intervention before checking for outstanding work. The regression compares the entire live and replayed receipts. |
| A hard exit during temporary-manifest sync could leave a third file. | The timeout path withdraws the runner's exclusive temporary name under a finite allowance. Windows grants delete sharing to that handle. Both a compiled fixture and an injected serial subprocess stall inside manifest sync, exit 2 and leave the reserved JSON and transcript. External process kill and unavailable storage retain the limits in the runner guide. |

README and CHANGELOG still describe the same commands and scope. The runner
and hardware-validation guides now explain the clean expiry boundary and
the consequences of a stalled operator prompt. No silicon fact, permission,
native source or feature-coverage mapping changed.

### Phase (c) receipts

The phase (b) Release build and command block above reproduce the focused
checks on this Windows host. The build was current. All 16 registered CTests
passed in 144.79 seconds. The interruption matrix replayed 2,927 cuts and
rejected 436,290 field mutations including pre-nonce setup. The other soak
tests rejected 19,224 compiled and 4,245 injected-serial mutations.
Standalone Python discovery passed 189 tests and skipped 49 compiled-fixture
tests. Doxygen passed with 293 pages and two rejected negative fixtures.
Version, reference, ledger and diff checks passed.

Replaying the reviewers' original pairs in `build/p52-review-artifacts`
accepted `irq` as PASS, and `short` and `declined-final` as INCONCLUSIVE.
It rejected `missing`, `wrong` and `false-continuity`. The existing phase
(b) `phase-b-10base-te.json` also replayed as PASS.

| Evidence layer | Phase (c) |
| --- | --- |
| Host and offline replay | PASS on the compiled CLI and injected serial carriers. |
| Portable and exported package consumer | NOT RUN. |
| ESP-IDF image link | NOT RUN. |
| Remote CI | NOT RUN. First-execution CI belongs to prompt 71. |
| Physical | NOT RUN. |

The full Release suite, focused UBSan trap checks and portable consumer
remain assigned to phase (d). This review retains the existing first-board
INT, native feature and deep-power gates. No fixture was attached.
