# Offline protocol and evidence runner

`tools/dp8382x_hil_runner.py` uses Python 3.10+ and the standard library.
It checks the shared CLI on a recorded byte stream or the compiled host
fixture. A PASS is host protocol evidence. Serial sessions and physical
qualification have their own stages.

## Commands

- `dry-run` prints the selected plan and expected target. It starts no
  process and writes no evidence files. Successful plan printing exits 0.
- `run-fixture` starts the explicitly supplied compiled CLI fixture for
  `host` and `sim-mdio`. It sends one complete command at a time and writes
  a new evidence pair. The default nonce is generated for this run.
- `verify-transcript` reads a raw recorded stdout file and writes a new
  evidence pair. Supply the suite, expected target and the session nonce
  actually used in the recording.
- `verify-artifacts` reads an existing JSON and its sibling transcript.
  It checks the file receipt and case aggregate, then replays complete
  runs against the known plan. It leaves both files untouched.

All plan modes require `--suite`, `--target`, `--backend`, `--variant`,
`--interface` and `--address`. Use `--help` on the runner or a subcommand
for its options. `--out-dir` and a unique `--run` basename are required
when creating evidence. Names start with an ASCII letter or digit followed
by up to 95 letters, digits, dots, underscores or hyphens.

The `passive` plan checks cached owner/transport evidence and submits a
register write with a syntactically out-of-range value. The exact expected rejection
needs the specified status, detail and reason, plus matching usable
before/after settings, health, slot and unsaturated transport counts.
A recorded native example can use this suite when its background
traffic is quiesced and the observed counts remain unchanged.

`fixture-operations` adds the host-only `connection initialize none`,
the two-sample passive selftest and a final idle-slot check. The operation
must complete on its admitted token and produce persistent replay evidence
without cleanup debt. Missing terminal evidence or a successful cleanup
cannot erase an observed primary failure.

```powershell
python tools/dp8382x_hil_runner.py --help
python tools/dp8382x_hil_runner.py dry-run --suite passive --target host --backend sim-mdio --variant DP83826I --interface RMII --address 1
cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host -R '^cli_hil_runner$' --output-on-failure --no-tests=error
python tools/dp8382x_hil_runner.py run-fixture build/host/test/cli/dp8382x_cli_fixture.exe --suite fixture-operations --target host --backend sim-mdio --variant DP83826I --interface RMII --address 1 --nonce demo-48 --out-dir build/evidence --run fixture-48
python tools/dp8382x_hil_runner.py verify-artifacts build/evidence/fixture-48.json
python tools/dp8382x_hil_runner.py verify-transcript build/evidence/fixture-48.txt --suite fixture-operations --target host --backend sim-mdio --variant DP83826I --interface RMII --address 1 --nonce demo-48 --out-dir build/evidence --run replay-48
```

Use the fixture without the `.exe` suffix on POSIX. A failed fixture launch
or transcript read gets an incomplete evidence pair after valid arguments
and successful reservation. Argument errors and output collisions return
an error immediately.

## Protocol and limits

The decoder accepts the exact schema-1 prefix at column zero and the
1024-byte machine record limit including CRLF. It keeps only a bounded
candidate across reads. It skips ordinary echo, prompts and ANSI output
as bytes. A malformed, duplicate-field or unterminated candidate fails
verification. The implemented suites require the declared enums, widths and
validity flags. [CLI schema](cli.md#schema-1)

The matching nonce establishes the boot and first request. Prior well-formed
unrelated traffic cannot establish that session. After synchronization,
each issued request must match the exact boot, request, command and
operation token. A valid admission is progress; a duplicate or conflicting
terminal, including a late one during closure, invalidates the run. Reboot,
reordering, request rollover and mismatched deadlines also fail correlation.
The offline verifier consumes the bounded file through EOF.

`--timeout` bounds the child lifetime from launch in seconds, with a default
of 5 and a maximum of 120. `--closure` bounds the final drain to EOF and exit.
It defaults to 0.5 seconds, is at most 5 seconds, and must be shorter than the
aggregate timeout. On failure the runner requests termination, then kills
if necessary and waits for the direct child in bounded steps. Stdout and
stderr are drained concurrently. A short stdin write ends the session with
no repeated command or suffix. Fixture selection is an explicit executable
path. The runner launches the direct child without a shell.

`--max-bytes` accepts 1 through 1,048,576 bytes and defaults to the upper
limit. For a child it bounds retained stdout and stderr combined. For raw
offline input it bounds stdout capture, including ordinary text. Reaching
the limit with additional bytes marks the capture truncated. The verifier
also enforces the 1 MiB combined limit and a 2 MiB JSON limit.

`--firmware` requires an exact 40- or 64-digit lowercase hexadecimal revision.
`--firmware-prefix` explicitly requests 7–64 lowercase hexadecimal digits.
An observed abbreviated revision can prove only a prefix that fits entirely
within it. The receipt keeps the observed spelling and records the strength
as `exact`, `prefix` or `unverified`. A known expectation fails on
`unknown` or an incompatible revision. `--fixture-identity` is an optional
value reported by the simulated producer for host tests. The runner's Git
revision and tracked tree state are recorded separately and are never used
as firmware identity. Host UTC timestamps and device monotonic microseconds
remain separate.

## Files and exit status

The runner exclusively reserves `<run>.json` with an INCONCLUSIVE
placeholder and `<run>.txt` for the raw stdout bytes. The finalized JSON
records the suite, target evidence and expected identity, fixture, UTC
timestamps, commands, case verdicts and primary/restoration outcomes. For
a subprocess, commands lists writes that completed before their deadline.
For a supplied transcript, commands records the declared replay plan.
The `stderr` field is base64 for the captured stderr bytes.

The runner flushes, syncs and closes the transcript before hashing it. It
writes and syncs a temporary JSON file and atomically replaces its placeholder.
Failed or interrupted finalization may leave the placeholder and captured
prefix, which remain nonpassing. Offline verification checks JSON schema,
byte length, SHA-256 and verdict, and replays complete protocol evidence.

| Exit | Meaning |
| --- | --- |
| `0` | Verified host protocol PASS, or successful help or dry-run output. |
| `1` | FAIL asserted by a case or its restoration evidence. |
| `2` | INCONCLUSIVE, invalid input, or unverifiable evidence. |
| `130` | Interrupted execution. |

FAIL dominates incomplete evidence. INCONCLUSIVE includes blocked, skipped,
zero-executed, unfinished, corrupt and truncated evidence. PASS requires
complete closure and a passing executed case set with any required
restoration proved.
