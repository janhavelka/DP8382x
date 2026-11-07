# 43 - Implement the portable CLI, bounded console and record schema

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [8](8-operation-contracts.md), passive snapshots and the owner
contracts established in [41](41-owner-admission-example.md). Actual feature
handlers follow; expose only working commands here.

## Current evidence and ownership

Reopen TMP1x2's `examples/common/Tmp1x2Cli.h` and `Tmp1x2Cli.cpp` (`setup`, `feed`,
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

Freeze tokenization, aliases, numeric parsing, group names, `<group> <verb> [args]`,
records and error vocabulary in `docs/cli.md`;44-46 append commands. Use
`examples/common/Dp8382xCli.h`, `Dp8382xCli.cpp` and `ConsoleBuffer.h`. One static
command table holds{name,alias,minArgs,maxArgs,flags,handler}; a cfg table holds
{group,name,enumValues or min/max,unit}. Dispatch/help share them. Each handler
calls a public API except bounded CLI sequencers. No dynamic registration or
virtual framework.

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
lifetime, feed/tick budgets and owner routing.

Application setup supplies a nonconstant u32 boot ID. `session <nonce>` passively
prints nonce verbatim, boot, current req, identity and schema. Every complete
input line, including rejected lines, increments a u64 firmware req from1;
there is no exhaustion branch. Every record includes boot/req. The runner has
one outstanding command and seeds its count from its own nonce reply; other
boot/req values are stale. After command timeout only `session <new nonce>` may
resynchronize; without its reply send nothing further. Session query never
reconfigures the processor or discards pending work.

The single terminated machine record schema includes version, boot/session,
request/operation identity, admission versus terminal state, printable `Err`,
signed backend detail, primary/cleanup status and valid time/deadline fields.
Keep records ASCII-safe, fully terminated and independent of human-output
filtering. Specify escaping, duplicate/unknown field policy, integer ranges
and record-byte capacity. A queued record is indivisible; external unqueued output may corrupt it and
the parser must reject that corruption. Reserve capacity before admitting work, or
retain one complete terminal result until its entire record is queued; the
queue owns bytes through partial writes. Queue acceptance is not host receipt.
Ordinary help or quiet/color settings cannot erase required protocol output.
Distinguish boot from sequence reuse, local/firmware time and library/firmware
identity; archives stay unknown when provenance is unavailable. Pending
admission is never terminal success.

Records are single uncolored lines at column0 with the fixed prefix. Track the
console cursor and queue CR LF first when prompt/partial human output is open.
Runner strips nothing from candidate records and discards non-record lines.
Queue records as indivisible units; prompt 47 routes ESP_LOGx through the same
queue, while ROM/panic/direct printf remain possible corruption sources.

## Acceptance

Register `test/cli/dp8382x_cli_fixture`, linking real CLI/core over a plain-array
simulated MDIO model. Feed stdin bytes unchanged to the CLI and its output to
stdout. Options: --identity, --partner, --fail-after N, --reboot-after N,
--partial-record. Test-only stdin controls $pause,$resume,$tick N,$output-stall N
are handled before CLI and absent in firmware. Cover malformed bytewise input,
zero-frame rejection, full/short/zero output and terminal retention. Campaign
classification uses recorded/synthetic transcripts; none claims UART/HIL.

Document processor/platform callbacks and source limits in Doxygen; maintain
`docs/cli.md` with grammar, machine schema, session handshake and example
records.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
