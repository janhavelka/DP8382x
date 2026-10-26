# Example CLI and Python runner contract

This specifies future example behavior. The shared processor belongs in
`examples/common/`; console and MDIO adapters belong to each example application.
The production core has no CLI, UART, ANSI, JSON or Python dependency.

## Familiar presentation, explicit semantics

Audit current TMP1x2, SHT3x, INA228, LDC1614 and TCA9548A dispatchers and runners.
Use `=== DP8382x CLI Help ===`, grouped help, aligned command/description columns,
prompt `> ` and optional sibling ANSI colors: cyan headings/commands, green ready,
yellow degraded/pending, red errors/offline, gray metadata, reset after styling.
Support color off and plain redirected output. Test behavior, not just help text.

Freeze exact grammar, aliases and output records in stage 18 before parser
implementation. Follow `help/?`, `version/ver`, `init/begin`, `drv/health`,
`cfg/settings` and register vocabulary where semantics agree. Avoid many aliases.
These are example CLI aliases, not extra C++ methods. Route initialization to
the single stage-04 operation API; [CONVENTIONS](CONVENTIONS.md) rejects duplicate
core `init`/`begin` facades. Use its Status/health field names consistently in
human output and machine records, while keeping link and trust separate.
Every command has arity, units, legal values, side effects, owner route, maximum
work, output and applicability. The following is a coverage requirement, not a
claim that these commands already exist:

| Group | Required capability |
| --- | --- |
| Local | Help/topics, version, capabilities, board/backend settings, color, quiet |
| Cached | Health, last link, config trust, last operation/result, MDIO counters |
| Connection | Explicit identity/probe, bounded address discovery, initialize, end |
| Configuration | Desired config, validation, readback, apply/verify, invalidate |
| Link | Live observation, advertised/resolved modes, partner evidence, finite watch |
| Owner | Start/poll/status/result/cancel for actual cooperative operations |
| Registers | Safe dump, Clause 22 read, extended read, explicit guarded raw write |
| Recovery | Distinct reset/restart/reinitialize commands with effects documented |
| Diagnostics | Read-only selftest, explicit loopback/BIST/TDR where supported |
| Optional | Capability-gated interrupts, LED/clock, wake/power/EEE controls |
| Test | Finite stress/soak, stop, run statistics, transfer evidence assertions |

This is a starting organization, not the maximum command set. Reconcile against
the complete register/field/procedure ledger in [FULL_COVERAGE](FULL_COVERAGE.md).
Every implemented programmable feature and legal setting needs a usable command
or grouped configuration path with help, state/effect reporting and tests. The
"Optional" group means caller opt-in; board-limited IRQ/clock/mode support does
not permit omission from the generic processor. Gate unsafe live changes through
the existing owner and report unavailable fixture/backend conditions accurately.

Address discovery is bounded MDIO identification over legal PHY addresses; it
is not an I2C ACK scan and does not choose a device/configuration silently. Reading
identity still requires bus ownership. Live reads that consume latches/counters
must be labeled and routed to the single collector. Ordinary health/status/help
is passive. A finite watch polls through the owner, never a second driver loop.

Malformed commands, extra tokens, overflow, out-of-range addresses, unsupported
features and invalid enums cause no transfer or state mutation. Use strict full
token parsing, fixed input/argument limits and defined CR/LF/backspace behavior.
Keep command admission distinct from completion. An accepted command cannot
return a success record before its operation result exists. Pending work and
retained results cannot be overwritten. Cancellation/stop report remaining
hardware effects and restoration failure separately.

Raw writes, resets, mode/clock/power changes and diagnostic stimulus can disrupt
the link. Require explicit mutating commands and an admitted maintenance state;
reject them while the application holds an active EtherCAT session. Merely
printing a warning is not ownership control. No hidden initialization, reset,
PHY scan or restoration writes on CLI startup. Do not issue servo commands.

Console servicing is bounded: fixed queue/line capacities, output budget, drop/
truncation counters and no blocking prints in driver callbacks or link-critical
tasks. Completion records cannot be silently lost; backpressure/rejection is
observable and runner truncation makes the case inconclusive. Explain where
board UART/task overhead limits timing; console success is not real-time proof.

## Automation protocol

Maintain one documented machine-readable contract alongside human output.
Choose bounded key/value records or JSON lines based on tested peer conventions,
not a new generic RPC framework. Include schema version, boot/session identity,
request/run identifier, operation identity if applicable, terminal status and
elapsed/deadline context. Keep health, link and application policy separate.

Firmware identity includes library version, firmware source revision/dirty or
unknown state, build/framework/target, configured variant/interface/address and
relevant board clock settings. Runner checkout SHA is never proof of flashed
firmware. Reboot during a case invalidates correlation; old results cannot satisfy
a new command. Avoid secrets and never print arbitrary environment values.

`tools/dp8382x_hil_runner.py` is the planned canonical Python entry point.
Use standard-library parsing/artifacts and an optional documented serial
dependency. Provide `--help`, dry-run, bounded timeout/duration, explicit port/
baud/target expectation, selected suites and output directory. Do not auto-flash,
auto-select a serial device, power-cycle hardware or reset active applications.
Expose additional fixture requirements instead of substituting a fake PASS.

Require exactly the records needed for each case, matching session/request IDs
and complete typed fields. Handle echo, prompts, ANSI, fragmentation, duplicate/
missing/out-of-order records, wrong firmware, serial loss, reboot, cancellation,
oversized output and unsupported capabilities. Parse failure, timeout or missing
fixtures cannot be PASS. Define exit codes for pass, failure and inconclusive.

Save bounded timestamped raw transcripts and structured per-case summaries,
runner/firmware identity, board/fixture settings, start/end times, exact commands,
skips/blocks and cleanup outcome. Use exclusive-create output bundles that retain
prior runs, with a manifest of artifact byte counts and SHA-256 hashes. Report
failed writes, flush/close and incomplete manifests as artifact failures, not
successful evidence. Separate host UTC timestamps from monotonic
device time; no fabricated timing correlation. Local fixtures get named manual
steps or separately configured control, never inferred cable/switch states.

Runner unit tests use recorded/synthetic transcripts and a compiled host CLI
fixture. Include output collision, partial-write and failed-close cases.
CLI contract guards verify grammar/help/dispatcher/runner agreement;
real execution tests prove rejected commands leave state and counters unchanged.
Text searches alone cannot prove behavior or hardware results.
