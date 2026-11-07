# Example CLI and Python runner contract

Put one portable CLI processor in `examples/common/`. Each example application
owns console transport, driver scheduling and maintenance admission. Core remains
independent of CLI, UART, ANSI, JSON and Python.

## Grammar and commands

Re-audit relevant TMP1x2, SHT3x, INA228, LDC1614 and TCA9548A dispatchers/runners.
Use `=== DP8382x CLI Help ===`, grouped aligned help and prompt `> `. Support
plain/color-off output; optional sibling colors reset after each styled field.
Prompt 43 freezes tokenization, aliases, numeric parsing, group names,
`<group> <verb> [args]`, records and errors; prompts 44-46 append commands.

Use familiar aliases where meanings agree: `help/?`, `version/ver`, `init/begin`,
`drv/health`, `cfg/settings`. Map aliases to one command implementation and one
core API, except the explicitly bounded CLI sequencers below. Define each command's arity, units, ranges, effect, owner route, maximum
work, output and applicability.

| Group | Required behavior |
| --- | --- |
| Local | Help/topics, version, capabilities, board/backend settings, color/quiet |
| Cached | Health, last link, configuration trust, operation/result, MDIO counters |
| Connection | Identity/probe, bounded explicit address discovery, initialize/end |
| Configuration | Desired settings, validation, apply, readback/verify, invalidation |
| Link | Live observation, advertisement/partner/resolved modes, finite watch |
| Owner | Start/poll/status/result/cancel for cooperative operations |
| Registers | Safe dump, C22/MMD read and guarded explicit raw writes |
| Recovery | Distinct reset/restart/reinitialize with documented effects |
| Diagnostics | Passive selftest, supported loopback/BIST/TDR and cleanup |
| Controls | Interrupts, LED/GPIO, clocks, wake/power/EEE and other ledger features |
| Test | Finite stress/soak, stop, run statistics, attributed transfer assertions |

Map every programmable feature/legal setting in FULL_COVERAGE to a command or
grouped configuration path, discoverable help and executing tests. Keep board
availability and silicon capability separate. Address discovery inspects legal
PHY addresses through the bus owner and returns candidates for explicit selection.
Route destructive live reads through the collector and finite watch through the
normal owner. Show cached snapshots without driver I/O.

Use fixed line/token/output capacities and strict whole-token parsing. Define
CR/LF, backspace, overflow recovery, signedness, numeric bases and enum handling.
Syntactically invalid or statically inapplicable requests perform zero I/O/state
changes. If a valid command needs fresh prerequisites, do bounded validation
reads before mutation and account for them explicitly.

Guard raw selectors/data and read-clear registers under ENGINEERING's transaction
policy. Require maintenance admission for disruptive writes, resets, clock/power
changes and stimulus. An active application session controls when maintenance
can begin. Attaching the CLI to a running example performs only local setup;
the example's documented startup independently controls network initialization.

Separate admission from terminal completion. Retain an operation/result until its
correlated outcome can be delivered; reject new work when that capacity is full.
Cancellation/stop reports pending effects and restoration separately. Attribute
zero-transfer checks to the command: SDK polling on the same bus remains visible
as separate traffic or is quiesced under owner control.

## Bounded console and machine records

Service input/output with finite per-call budgets and queues. Keep blocking
console calls outside driver callbacks/transaction locks. Expose overflow,
truncation and dropped ordinary output; reserve/retain terminal evidence until
emitted. A stalled console must leave explicit backpressure and retained state.

Use one bounded machine format (key/value records or JSON lines) with documented
framing/escaping, schema version, boot/session identity, request/run ID, optional
operation identity, terminal status and elapsed/deadline fields. Keep records
plain and line-delimited; define maximum bytes, required fields and handling of
unknown fields/schema versions. Records are single uncolored lines at column zero with the prefix. The console
tracks cursor state and queues CR LF before a record when a prompt/partial line
is open. The runner discards non-record lines and strips nothing from a candidate.
Human prompts and echo remain separate noise; bypass logging can corrupt a
record and must produce missing/invalid evidence, never parser repair. Test actual parser/dispatcher behavior as well as help agreement.

Include library version, firmware revision/dirty or unknown state, SDK/build/
target, declared variant/interface/address and relevant clock settings. Establish
an explicit session handshake when attaching after startup. Correlate every
result to that session/request; a reboot invalidates in-flight work. The application supplies a nonconstant u32 `boot` at setup. `session <nonce>`
passively returns the nonce verbatim, boot, current req, identity and schema.
Every record carries boot and a 64-bit req starting at one and incrementing for
every complete input line, including rejected lines; there is no exhaustion or
reconfiguration branch. The runner has one outstanding command, synchronizes
from its nonce reply and rejects other boot/req records as stale. After timeout
send only `session <new nonce>`; without its reply send no further commands.

## Runner and evidence artifacts

Use `tools/dp8382x_hil_runner.py` as canonical entry point. Provide `--help`,
dry-run, explicit port/baud/target expectations, selected suites, finite duration/
timeout and output directory. Use standard-library parsing/artifacts with a
small documented optional serial dependency. Require explicit fixture selection
and authorized hardware actions; runner startup establishes a session without
implicit flashing, device selection, reset or power cycling.

Bound serial reads/writes, receive bytes, pending records, retries, subprocesses
and finalization. Construct serial with `port=None`, finite read/write timeouts,
set DTR/RTS, then open. Never flush/drain. Run open/close on a daemon thread
joined with a finite timeout; expiry finalizes INCONCLUSIVE evidence and exits
nonzero with `os._exit`. The serial backend adds no child process; the compiled
fixture subprocess is separate. Test open/close that never return. A partial command write leaves framing uncertain: stop further
commands on that connection and report cleanup unavailable until a new verified
session is established. Preserve primary failure and separate cleanup status.

Validate required typed fields and exactly the terminal evidence needed per case.
Exercise echo/ANSI, fragmentation, duplicates, late/out-of-order/missing records,
wrong firmware, serial loss, reboot, cancellation and oversized output. Distinguish
PASS, FAIL and inconclusive/blocked with documented exit codes. Missing fixtures,
invalid/truncated evidence and failed required restoration cannot produce PASS.

Create exactly two exclusive files per run: `<run>.json` and `<run>.txt`.
JSON records verdict/cases, runner and firmware identity, fixture, UTC times,
commands, cleanup, transcript SHA-256/length and truncation. Create an
INCONCLUSIVE placeholder first. Close the transcript, then write JSON via temp
file, flush, fsync and `os.replace`. Offline verify checks hash, length, schema
and worst-case verdict: FAIL, then INCONCLUSIVE/BLOCKED/NOT RUN, then PASS.
Zero cases, skipped-only runs and failed restoration cannot pass. Preserve
failed/incomplete runs; there is no bundle tree or manifest.

Test protocol/artifact behavior offline with synthetic/recorded transcripts and
the compiled host CLI fixture. Include partial writes, output collisions,
truncation, failed close/finalization and evidence after cancellation. Physical
case results require the identified real fixture; host fixtures prove software
behavior and remain labeled accordingly.


## Concrete dispatch, coverage and console ownership

Use one static command table `{name, alias, minArgs, maxArgs, flags, handler}`
and a second cfg table `{group, name, enumValues or min/max, unit}` for dispatch
and help; no dynamic registration or virtual hierarchy. Discovery, dump,
selftest, watch and stress use small fixed index/count/deadline sequencers,
one public batch per tick, never direct MDIO or a second chip procedure.
Discovery uses a separate probe driver on the same transport, checking the
bound instance's RUNNING/busQuietValid admission before each ID-only batch.

The single `docs/feature-coverage.md` matrix has one row per ledger procedure
and feature ID: exact part/mode, CLI command, owner route, record type, host test,
physical case or reasoned N/A. A unit test checks all IDs and that every field's
owner resolves to a command/cfg route. Prompts 44/46/47 append, 47 closes CLI
columns, 55 closes primary coverage and family closure appends exact-part rows.
Keep docs/cli.md limited to grammar/records.

For P4, record console type and meaningful baud. Route ESP log callbacks through
a bounded whole-line queue using a try-lock and drop accounting; retain terminal
record capacity, pin the supported USB Serial/JTAG V1 path, and test concurrent
logging. ROM/panic/direct printf bypass remains an explicit corruption limit.
The compiled host fixture links real CLI/core with only required table-driven
MDIO behavior; it is software evidence, never hardware qualification.
