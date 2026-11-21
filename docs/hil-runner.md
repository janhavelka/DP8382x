# Console protocol and evidence runner

`tools/dp8382x_hil_runner.py` uses Python 3.10+ and the standard library for
help, dry-runs, recorded streams and the compiled host fixture. Live serial
attachment also uses the optional pySerial 3.5 native backend. A serial PASS
records the specified console cases for one session. PHY qualification needs
its own fixture cases and evidence.

## Commands

- `dry-run` prints the selected plan and expected target. It starts no
  process and writes no evidence files. Successful plan printing exits 0.
- `run-fixture` starts the explicitly supplied compiled CLI fixture for
  `host` and `sim-mdio`. It sends one complete command at a time and writes
  a new evidence pair. The default nonce is generated for this run.
- `run-serial` attaches once to an explicitly identified, already-running
  console using the `passive` or `campaign` suite. Add `--dry-run` to validate and print
  the same plan and declarations without importing pySerial or creating files.
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

The `campaign` suite selects one case from the [hardware validation table](hardware-validation.md)
with `--case`. The session and wire identity, full entry/target/restored
profiles, resolved link, and bounded peer action declarations form one
evidence pair. The same case definitions drive the compiled fixture,
serial execution and offline replay. Missing compatible equipment yields
a retained NOT RUN case. A dry-run prints the initial handshake and the
conditional case definition. Later commands depend on the responses and
saved entry profile.

Diagnostic and control case IDs use the same suite and artifact schema.
Choose the exact BASIC/ENHANCED mode and MII/RMII binding from the
[feature fixture guide](hardware-validation.md#feature-fixtures). Case
selection in `run-fixture` enables the compiled fixture's synthetic
feature attestations. When a case needs owned settings, a fixed prelude
of up to eight typed commands installs them before the nonce. That traffic
is retained as an uncorrelated prefix. The case then independently verifies
the owned entry baseline in the correlated session. `commands` records
only the nonce and later requests. The 64-command ceiling includes reserved
prelude slots. A case may request up to four interventions. Each feature
command exchange has a two-second host ceiling inside its primary or
cleanup allowance. Closure has its own remaining allowance.

For example, select `--case loopback-digital` with the compiled RMII fixture.
`compliance-*` and `calibration-recovery` additionally require
`--reinitialize` for separate saved-profile replay after verified HARD exit.
After an autorun enable is admitted, restore the saved disabled policy even
if the failed request completed no writes. Autorun cleanup debt requires
`--reinitialize` and verified retained intent before no-reset replay and
the typed disable. Pins, quality and analog use their own known-intent
checks before the corresponding no-reset replay.
The host diagnostic/control cases prove their typed software paths, with
simulated inputs for packet errors, calibration, interrupt and wake status.
They cannot qualify the cable, signals, timing, packet generator, power or
wake behavior of a real board. Stock native feature cases are BLOCKED
before stimulus. Their instrument requirements and controlled procedures
are in the [feature fixture guide](hardware-validation.md#feature-fixtures).
The runner blocks an incompatible operating mode or interface before sending
the fixture prelude. A feature case samples the ordinary link before stimulus
and verifies the same link state, including partner ability when valid, after
restoration. A correlated reply with malformed feature payload fails the
primary assertion. The runner may still attempt a typed stop after checking
the owner, slot and transport. Primary and restoration outcomes remain
independent. Lost framing or boot continuity stops further commands.

For a compiled offline example, first build `dp8382x_cli_fixture`, then run:

```powershell
$opts = @(
  '--suite', 'campaign', '--case', '10base-te', '--target', 'host', '--backend', 'sim-mdio',
  '--variant', 'DP83826I', '--interface', 'RMII', '--address', '1',
  '--clock', 'simulated', '--console', 'host-stdio', '--baud', '0',
  '--firmware-prefix', 'abcdef0', '--operating-mode', 'enhanced', '--polling', 'exclusive',
  '--board', 'compiled-fixture', '--authorization', 'offline-simulation',
  '--peer', 'simulated-peer', '--peer-profile', 'an-10-half', '--cable', 'simulated-cable'
)
python -S tools/dp8382x_hil_runner.py dry-run @opts
python -S tools/dp8382x_hil_runner.py run-fixture build/host/test/cli/dp8382x_cli_fixture.exe @opts --fixture-identity abcdef0 --out-dir build/evidence --run campaign-10base-te
python -S tools/dp8382x_hil_runner.py verify-artifacts build/evidence/campaign-10base-te.json
```

For offline replay of an intervention case via `verify-transcript`, pass
the original declarations and the captured `campaign.interventions` array
as `--interventions FILE`. It is bounded to 4096 bytes. `verify-artifacts`
reads the array from the existing manifest. The replay derives the expected
case, step order, actions, outcome and restoration from the case definition,
correlated records and acknowledgment receipts. A compiled fixture control
stays in the child harness. Serial intervention receipts are manual
declarations tied to an action, boot, preceding request and finite budget.
Enter the action and prompt code shown for that intervention. The code changes
even when cleanup repeats the same action. An acknowledgment at the deadline
is a timeout. If a late receive leaves an incomplete machine record, a
campaign closes the session without sending a resynchronization command.

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

## Live serial attachment

Supply an explicit local COM port on Windows or a local tty device path on
POSIX, a board or fixture identifier, and the record authorizing that attachment.
`--reset-silent` declares that open and close on this exact driver, port and
wiring are qualified to leave the device running. `--clean-input` declares
exclusive attachment while the firmware is at a clean CLI input line. Use both
only when the fixture record supports them. Firmware reports its build context
but has no per-board identifier. The operator links the board to the run.

`uart0` needs the firmware's actual baud rate. Qualify the bridge reset and
boot wiring, including drivers that assert or glitch DTR/RTS as they acquire
the handle. An image that reports `usb-serial-jtag` uses `--baud 0`, which
declares that baud does not apply on the wire. The driver receives a nominal
115200. Native USB control lines have their own reset behavior and require
separate open/close evidence. Both shipped P4 examples report `uart0`.

For an authorized P4 basic image, set `$FixturePort`, `$BoardId`,
`$AuthorizationId` and `$FirmwareRevision` from the fixture record. Confirm the
firmware's address, bus interface and console values before running.

```powershell
$serialPlan = @(
  'run-serial', '--suite', 'passive', '--target', 'esp32p4', '--backend', 'p4-netlist-basic',
  '--variant', 'DP83826I', '--interface', 'RMII', '--address', '0',
  '--console', 'uart0', '--baud', '115200', '--firmware', $FirmwareRevision,
  '--port', $FixturePort, '--board', $BoardId, '--authorization', $AuthorizationId,
  '--reset-silent', '--clean-input', '--out-dir', 'build/evidence', '--run', 'p4-passive-01'
)
python -S tools/dp8382x_hil_runner.py @serialPlan --dry-run
python tools/dp8382x_hil_runner.py @serialPlan
python tools/dp8382x_hil_runner.py verify-artifacts build/evidence/p4-passive-01.json
```

Install pySerial 3.5 in the Python environment for live runs. The runner
accepts only its platform-native Windows or POSIX serial backend. It reserves
the INCONCLUSIVE JSON and transcript before constructing a closed serial
object. It requests deasserted DTR and RTS, disables flow control, assigns the
port and opens once. The requested line states are software settings.
The OS acquires the handle before pySerial applies those settings, and
pySerial's own open sequence can purge queued bytes. The runner issues no
flush, drain, reset-buffer or break calls. Attachment itself has no reset or
power-cycle action. An explicitly selected campaign can issue the specified
CLI software reset after an admitted entry snapshot.
[pySerial 3.5 API](https://pyserial.readthedocs.io/en/latest/pyserial_api.html)

Attachment starts with `session <fresh-nonce>`. It needs no startup banner
and requires the matching reply and an explicitly verified firmware revision
or prefix. The cached before/after case requires a quiesced owner and usable
cached state. If a timely full write receives no terminal, the runner can
issue one new nonce query using its recovery allowance. A successful recovery
requires the same boot, full reported identity and the expected next request.
Late replies stay in the raw transcript. A fresh reply without an earlier
verified boot is recorded as unproven continuity. The run retains its original
failure either way. A short, zero or uncertain write leaves the input framing
unsafe, so the runner sends no further bytes. The passive suite has no firmware
restoration command. [Session handshake](cli.md#session-handshake)

| Serial option | Default | Scope |
| --- | --- | --- |
| `--session-timeout` | 30 s | Absolute monotonic budget from provenance collection through finalization |
| `--duration` | 20 s | Provenance, reservation, open, commands and receive tail, clipped to leave cleanup and finalization time |
| `--command-timeout` | 1.5 s | One write and its correlated response within the active budget |
| `--tail-timeout` | 0.15 s | Observe the stream after the last terminal without sending another command |
| `--cleanup-timeout` | 3 s | At most half for resynchronization, with the remainder available for close |
| `--finalize-timeout` | 2 s | Write, sync, hash and atomically publish the evidence pair |

All times are finite and at most 120 seconds. Cleanup and publication are
also clipped to their remaining session time. A single owner supervises
one daemon backend call at a time. Reads poll in chunks of at most 256 bytes
using a nonblocking driver setting. An empty read means idle. On Windows the
runner uses pySerial's positive write timeout to wait for the reported count
from one overlapped write. On POSIX it makes one nonblocking OS write to
pySerial's descriptor. A short count or a would-block error ends the session
without issuing another write. The absolute host deadline bounds either path.

The owner closes once after completed I/O, including a returned open failure if
a serial object was created. It never closes concurrently with a stalled I/O
call. A still-pending backend call is recorded as INCONCLUSIVE and the process
uses `os._exit(2)` after bounded artifact finalization. Console diagnostics on
this path use only remaining session time. Hard exit also avoids backend
destruction if a failed native close could run again. Use this runner as a
standalone Python command. The serial transport uses no child process.

## Protocol and limits

The decoder recognizes the schema-1 marker `@DP8382X` at column zero. A
record requires one space after the marker and fits in 1024 bytes including
CRLF. The decoder keeps only a bounded candidate across reads. It skips
ordinary echo, prompts and ANSI output as bytes. A malformed, duplicate-field
or unterminated candidate fails verification. The implemented suites require
the declared enums, widths and validity flags. [CLI schema](cli.md#schema-1)

The matching nonce establishes the boot and first request. Prior well-formed
unrelated traffic cannot establish that session. After synchronization,
each issued request must match the exact boot, request, command and
operation token. An admission only records progress. A duplicate or conflicting
terminal, including a late one during closure, invalidates the run. A machine
record that begins in received bytes before the next write ends live command
admission even if its delimiter is pending. Reboot, reordering, request
rollover and mismatched deadlines also fail correlation.
Admissions carry no cleanup result. Cached queries carry no command deadline.
Scheduled feature reads use the producer's 2-second bound. Scheduled writes,
starts, observations and resets use 5 seconds. Starts, observations and
resets require their admitted token when successful. Direct feature reads and
writes require a direct terminal. Clean stopped-owner, controller and slot
checks precede eligible restoration, with a separately verified feature exit.
The record clock may be unavailable, including throughout a passive run. A
valid timestamp cannot follow an unavailable clock within one boot. The offline
verifier consumes the bounded file through EOF.

`--timeout` bounds the child lifetime from launch in seconds, with a default
of 5 and a maximum of 120. `--closure` bounds the final drain to EOF and exit.
It defaults to 0.5 seconds, is at most 5 seconds, and must be shorter than the
aggregate timeout. On failure the runner requests termination, then kills
if necessary and waits for the direct child in bounded steps. A failed teardown
is recorded alongside the original error. Stdout and stderr are drained
concurrently. A short stdin write ends the session without repeating the
command or writing its suffix. Fixture selection is an explicit executable
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
value reported by the simulated producer for host tests. Older schema-1
recordings may omit both `console` and `baud`. A recording with one of the
two is malformed. An explicit console or baud expectation requires those
fields in the recording. The runner records its Git revision and tracked
tree state separately from firmware identity. Host UTC timestamps and device
monotonic microseconds remain separate.

## Files and exit status

The runner exclusively reserves `<run>.json` with an INCONCLUSIVE
placeholder and `<run>.txt` for the raw stdout bytes. The finalized JSON
records the suite, target evidence and expected identity, fixture, UTC
timestamps, commands, case verdicts and primary/restoration outcomes. For
a subprocess, commands lists writes that completed before their deadline.
For a supplied legacy transcript, commands records the declared replay plan.
For a campaign transcript it records the validated request prefix derived from
the case and retained records.
The `stderr` field is base64 for the captured stderr bytes.

The runner flushes, syncs and closes the transcript before hashing it. It
writes and syncs a temporary JSON file and atomically replaces its placeholder.
Failed or interrupted finalization may leave the placeholder and captured
prefix, which remain nonpassing. An interruption before transcript reservation
releases the new placeholder. Offline verification checks JSON schema,
byte length, SHA-256 and verdict, and replays complete protocol evidence.

| Exit | Meaning |
| --- | --- |
| `0` | Verified console evidence PASS, or successful help or dry-run output. |
| `1` | FAIL asserted by a case or its restoration evidence. |
| `2` | INCONCLUSIVE, invalid input, or unverifiable evidence. |
| `130` | Interrupted execution. |

FAIL dominates incomplete evidence. INCONCLUSIVE includes blocked, skipped,
zero-executed, unfinished, corrupt and truncated evidence. PASS requires
complete closure and a passing executed case set with any required
restoration proved.

Serial JSON uses scope `serial_session` and adds typed fixture declarations,
console and baud applicability, requested control-line states, limits, write
attempts, open/close and resynchronization results. `commands` contains only
timely full writes. A hard exit remains INCONCLUSIVE and retains any observed
case failure as a separate receipt. The in-memory RX prefix includes ordinary
text and machine records. It is persisted after serial teardown, so abrupt
process loss can leave only an empty transcript or a partial prefix. If final
publication fails or misses admission to atomic replacement, the INCONCLUSIVE
placeholder remains. Filesystem operations already in the kernel may complete
as the process exits, and abrupt termination may leave a temporary JSON file.
