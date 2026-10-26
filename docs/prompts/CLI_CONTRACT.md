# Example CLI and Python runner contract

Put one portable CLI processor in `examples/common/`. Each example application
owns console transport, driver scheduling and maintenance admission. Core remains
independent of CLI, UART, ANSI, JSON and Python.

## Grammar and commands

Re-audit relevant TMP1x2, SHT3x, INA228, LDC1614 and TCA9548A dispatchers/runners.
Use `=== DP8382x CLI Help ===`, grouped aligned help and prompt `> `. Support
plain/color-off output; optional sibling colors reset after each styled field.
Prompt 43 freezes grammar and record schema before implementation.

Use familiar aliases where meanings agree: `help/?`, `version/ver`, `init/begin`,
`drv/health`, `cfg/settings`. Map aliases to one command implementation and one
core API. Define each command's arity, units, ranges, effect, owner route, maximum
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
unknown fields/schema versions. Human prompts, echo and ANSI are separate input
noise. Test actual parser/dispatcher behavior as well as help agreement.

Include library version, firmware revision/dirty or unknown state, SDK/build/
target, declared variant/interface/address and relevant clock settings. Establish
an explicit session handshake when attaching after startup. Correlate every
result to that session/request; a reboot invalidates in-flight work. Define the
finite receive window used to classify duplicates, stale and late records.

## Runner and evidence artifacts

Use `tools/dp8382x_hil_runner.py` as canonical entry point. Provide `--help`,
dry-run, explicit port/baud/target expectations, selected suites, finite duration/
timeout and output directory. Use standard-library parsing/artifacts with a
small documented optional serial dependency. Require explicit fixture selection
and authorized hardware actions; runner startup establishes a session without
implicit flashing, device selection, reset or power cycling.

Bound serial reads/writes, receive bytes, pending records, console drain, retries,
subprocesses and cleanup/finalization. Account for backend flush/close behavior
in the bound. A partial command write leaves framing uncertain: stop further
commands on that connection and report cleanup unavailable until a new verified
session is established. Preserve primary failure and separate cleanup status.

Validate required typed fields and exactly the terminal evidence needed per case.
Exercise echo/ANSI, fragmentation, duplicates, late/out-of-order/missing records,
wrong firmware, serial loss, reboot, cancellation and oversized output. Distinguish
PASS, FAIL and inconclusive/blocked with documented exit codes. Missing fixtures,
invalid/truncated evidence and failed required restoration cannot produce PASS.

Create each artifact bundle exclusively, retaining previous runs. Save bounded
raw transcripts and structured case results, runner/firmware identity, fixture
settings, UTC start/end times, commands and cleanup outcome. Use device monotonic
time only within its documented domain. Track artifact byte counts and SHA-256
hashes. Finalize payloads first, then write the manifest last; the manifest lists
payloads, excluding itself. Publish a completed bundle only after required
writes/closes succeed. Validate required payload membership, unique bundle-relative
paths, schema/version and agreement with case verdicts, as well as hashes.
Retain failed/incomplete bundles with an explicit verdict.

Test protocol/artifact behavior offline with synthetic/recorded transcripts and
the compiled host CLI fixture. Include partial writes, output collisions,
truncation, failed close/finalization and evidence after cancellation. Physical
case results require the identified real fixture; host fixtures prove software
behavior and remain labeled accordingly.
