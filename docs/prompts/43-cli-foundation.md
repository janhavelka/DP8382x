# 43 - Implement the portable CLI, bounded console and record schema

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [8](8-operation-contracts.md), passive snapshots and the owner
contracts established in [41](41-owner-admission-example.md). Actual feature
handlers follow; expose only working commands here.

## Current evidence and ownership

Reopen TMP1x2's `examples/common/Tmp1x2Cli.h/.cpp` (`setup`, `feed`,
`processCommand`, `tick`, `printHelp`) and `ConsoleBuffer.h` (`format`,
`drain`). Compare 24Cxx's `discardInput`, `Platform::outputReady` and
session/result retention; inspect SHT3x dispatch, LDC1614 help/parser tests
and INA228's native framing with
`tools/run_i2c_hil.py::{hilrun_end_re,strip_hilrun_frame}`. Record current
revisions, source/test/runner paths and adopted/adapted/rejected choices.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Implement

Freeze exact grammar, aliases and machine schema in `docs/cli.md` before
coding. Use `examples/common/Dp8382xCli.h/.cpp` and `ConsoleBuffer.h` with one
small framework-free processor including the public DP8382x API. Keep
parsing/dispatch together until real responsibilities justify
output/diagnostic source files; avoid per-command classes and generic
RPC/registration frameworks.

Implement working help/topics, version/capabilities, local controls and cached
health/link/configuration views. Follow grouped aligned sibling presentation,
`> ` prompt and optional ANSI, with plain/color-off output and ANSI reset.
Test actual aliases and dispatch, not only help text. Snapshot rendering
performs no I/O and does not collapse health, link, trust and application
policy.

Specify fixed line/token/output capacities, numeric bases/signs, full-token
enum parsing, extra tokens, CR/LF/CRLF/backspace, control characters and
overflow recovery. Include embedded NUL, invalid encoding, integer overflow
and signed versus unsigned conversion. Input loss discards through the next
complete line boundary; backspace cannot clear discard mode. A truncated
mutation or its remaining suffix cannot execute later. Define callback
lifetime, feed/tick budgets, owner routing and processor reconfiguration while
work is pending.

Provide a passive session/identity query so a runner attaching after boot can
establish fresh correlation without resetting hardware or depending on a lost
boot banner. Define who supplies session/request identifiers, their legal
widths, non-reuse/exhaustion rules and how stale records from an earlier
attachment are rejected. Reconfiguring a session with pending work/result must
preserve or reject it explicitly; a new identifier cannot silently discard
work.

The single terminated machine record schema includes version, boot/session,
request/operation identity, admission versus terminal state, printable `Err`,
signed backend detail, primary/cleanup status and valid time/deadline fields.
Keep records ASCII-safe, fully terminated and independent of human-output
filtering. Specify escaping, duplicate/unknown field policy, integer ranges
and record-byte capacity. Human logs cannot impersonate record prefixes or
interleave inside a machine record. Reserve capacity before admitting work, or
retain one complete terminal result until its entire record is queued; the
queue owns bytes through partial writes. Queue acceptance is not host receipt.
Ordinary help or quiet/color settings cannot erase required protocol output.
Distinguish boot from sequence reuse, local/firmware time and library/firmware
identity; archives stay unknown when provenance is unavailable. Pending
admission is never terminal success.

## Acceptance

Register a host executable under `test/cli/` using real shared sources and a
clearly simulated backend. Test bytewise input, every malformed boundary,
passive/rejected command-attributed zero-frame/state invariants, short/zero
output writes, full queue, terminal retention and pending reconfiguration. No
UART/HIL claim follows.

Document processor/platform callbacks and source limits in Doxygen; maintain
`docs/cli.md` with grammar, machine schema, session handshake and example
records.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
