# Prompt 48 · Offline protocol and evidence

Work began on synchronized `main` at `37059a07742a6928ca7a5116a16222ad8f0cbb63`.
The reusable protocol, artifact and pipe units were checkpointed at `0a7477b`.
The schema-1 producer and compiled host fixture are the inputs to this stage.

Three agents used the active model in parallel for strict decoding and identity,
durable files and failure injection, and bounded fixture subprocess ownership.
Independent read-only reviews traced the actual producers and failure paths.
The lead owned the suite, correlation, command entry point, integration and Git.

## Source audit

All three sibling checkouts were clean. The review used their current
agent rules, source, tests and registration without executing remote CI.

| Checkout | Evidence and decision |
| --- | --- |
| INA228 `8e439d6c99ece1353cd9f555deed0314794412d9` | `tools/run_i2c_hil.py::read_until_hilrun_end` and `run_step` correlate a token/sequence, require the terminated trailer and catch stale or adverse data after a frame. `tools/test_run_i2c_hil_parser.py` exercises short writes, stale input, wrong identity and trailer failures. Its workflow runs the parser tests directly. Adapt exact receipt and final-drain checks to the DP8382x record grammar. |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `tools/hil_tmp1x2_runner.py::{one_record,operation_admission,response_complete,classify}` distinguish progress, result and failure. `EvidenceFiles` reserves provisional evidence and finishes the transcript first. `SerialSession` has independently bounded capture and writes. Root CMake registers `tools/test_hil_tmp1x2_runner.py`, including terminal/operation correlation, fixture and setup-failure file cases. Adopt strict completion and provisional files. Reserve the JSON itself exclusively and use a separate direct-child pipe boundary for the compiled fixture. |
| 24Cxx `1bd9f969d1df55ca5819b0d290ff331869e7d5bc` | `tools/hil_identity.py::{parse_identity,check_identity}` retain observed short revisions and compare identity. `tools/hil_artifacts.py::collect_provenance` separates host provenance from the producer and the renderer escapes text. Root CMake discovers the identity and artifact tests in `tools/`, including malformed identity and receipt/provenance cases. Adopt explicit identity strength and separate runner provenance. Use only the two requested raw/JSON artifacts. |

## Delivered behavior

- The canonical entry point prints a dry-run plan, executes one compiled
  host fixture, verifies supplied stdout, and verifies/replays an evidence
  pair. The implemented plans are passive context and exact rejection, plus
  bounded initialization, restoration evidence and selftest on sim-mdio.
- The decoder validates exact framing and typed fields. The session validates
  nonce, boot, request, token and closure, including late duplicate and
  malformed records. A captured failure in a baseline, post-rejection query
  or selftest sample holds its verdict when the remaining transcript is absent.
- Stdout remains raw. Bounded stderr is recorded in base64. Reservation and
  finalization preserve earlier runs. The receiver checks the persisted
  transcript against its capture digest before publishing and validates
  schema, length, hash, aggregate and replay on verification.
- The compiled child runs without a shell. A short write fails once without
  sending its suffix. Both streams drain under an aggregate deadline and a
  bounded final closure. Timeout and interruption preserve nonpassing evidence.

[Runner commands, limits and files](../../hil-runner.md)

## Local verification

Windows with Python 3.12.10, GNU 15.1.0, CMake 4.0.1 and Ninja.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable host and compiled producer | PASS | 262 Release CTests. `cli_hil_runner` executes 10 runner tests with the actual compiled C++ provider. The test includes exact firmware identity, artifact replay, token mismatch, admission-only output and a failed sample without a terminal. |
| Python | PASS | 152 discovered tests. The one fixture-dependent test skipped here executes under CTest. New unit cases cover every record split, malformed fields and identity, late duplicates, wrong boot/order, false PASS, subprocess faults and injected file failures. |
| Tests-OFF consumer | PASS | Two separately linked consumer CTests. |
| Source and tooling | PASS | Help, dry-run, compiled fixture, supplied transcript and artifact verification commands. Reference, ledger, version, Doxygen with its two negative fixtures, and diff checks. |
| Native SDK, serial/physical and remote CI | NOT RUN | This stage exercises the offline host protocol. Serial implementation follows in prompt 49. Remote CI remains prompt 71. |

The Release build used `cmake --build build/p46-release` and CTest with
`--output-on-failure --no-tests=error`. Pure Python checks used
`python -m unittest discover -s test -p "test_*.py" -q`.
