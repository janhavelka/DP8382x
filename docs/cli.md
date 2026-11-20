# Shared example CLI

The portable C++17 processor in [examples/common](../examples/common/Dp8382xCli.h)
accepts commands through one application owner. Setup queues a prompt. It does
not bind, initialize, probe or reset a PHY. Cached commands call the owner's
snapshot callback once. That callback copies values under the existing owner
admission and performs no MDIO. Health, link resolution and configuration trust
are separate. The application still decides whether a link suits its session.

This stage provides the commands below. Live configuration, diagnostics and
operations enter the table with their executing handlers in prompts 44–46.
Prompt 47 attaches the processor to the native examples.

## Input and commands

Use case-sensitive `<group> <verb> [args]` for grouped commands. The general
commands and listed aliases are single tokens. Spaces and tabs separate
tokens, including at the ends of a line. There is no quoting, escaping,
comment syntax, abbreviation or case folding.

The processor stores up to 192 edited bytes, 10 tokens including the command,
and 64 bytes per token. ASCII 0x20 through 0x7E and tab are accepted. Backspace
and DEL erase one stored byte. CR and LF terminate a line. A contiguous CRLF
terminates one line, including when it is split across calls. Every completed
line, including a blank, invalid or discarded line, consumes one request
number. A suffix at EOF without a delimiter is incomplete.

An overlong line, embedded NUL, high byte (including UTF-8), or other control
character poisons the line through its delimiter. Backspace cannot recover it.
After a transport input gap call `discardInput()` after delivering any older
retained bytes and before feeding newer ones. The gap breaks CRLF adjacency.
The next delimiter ends the discarded line, and its following line can run.
An argument or token failure rejects the entire line before dispatch.

| Command | Alias | Arguments and effect |
| --- | --- | --- |
| `help [topic]` | `?` | Show aligned help from the command and settings tables. Topics are `local`, `cached` and each canonical general command. |
| `version` | `ver` | Report library and application identity. |
| `session <nonce>` | | Echo a 1–64 byte protocol token and report identity for synchronization. |
| `capabilities` | `caps` | Show passive capability evidence from accepted silicon identity. |
| `local board` | | Show declared board/backend, interface, clock, address and provenance. |
| `local color [off\|on]` | | Query or set human help styling. Default off. Each styled title resets ANSI before the line ends. |
| `local quiet [0..1]` | | Query or mute human help and prompts. Default 0. Protocol records remain enabled. |
| `local console` | | Show queued bytes, dropped human chunks and zero-byte write count sampled before its own record. |
| `cached health` | `drv`, `health` | Copy managed MDIO health, counts and last error. |
| `cached link` | | Copy link validity, staleness, resolved speed/duplex and pause evidence. |
| `cached settings` | `cfg`, `settings` | Copy desired and observed link profile summaries and configuration trust. |
| `cached operation` | | Copy the owner slot and cleanup barrier. Does not poll or take a result. |
| `cached transport` | | Copy all-source MDIO batch/frame counts and controller flags. |

Unsigned input uses decimal or explicit `0x`/`0X` hex. Leading zeros do not
select octal. Require at least one digit, reject signs, partial conversions and
values above 4294967295, then check the command range before using the value.
The only numeric input here is `local quiet`, with range 0–1. The wire's signed
details use decimal with an optional minus and retain the whole int32 range.
Enums match their complete tokens. Missing or extra arguments reject.

## Console service

Serialize `setup`, `feed`, `discardInput` and `tick` in the application task.
The borrowed context and immutable identity strings live through its last call.
Setup succeeds once per processor. The snapshot, clock and write callbacks
return promptly, do not reenter the processor and do not retain buffer pointers.
Use `feed` to accept at most 128 bytes per call. Keep the unaccepted suffix for
another call. After a real transport overrun use the input-gap rule above.

The FIFO holds 4096 bytes and copies each accepted chunk into its storage.
Human chunks can be dropped whole when full. `tick` offers at most one
contiguous span of at most 256 bytes to the nonblocking writer. The writer
returns the length of its accepted prefix, including zero for backpressure.
The queue retains all remaining bytes. No controller lock spans output work.

A complete record occupies at most 1024 bytes including CRLF. The queue admits
the entire record and any needed leading CRLF together, using its logical tail
to detect an open prompt or partial human line even after short writes. When
the record cannot fit, the processor holds its serialized bytes in one slot
and stops accepting new input until the slot clears. A writer stall does not
repeat a command or overwrite its outcome. Separately admitted asynchronous
features must retain their own result until this delivery boundary. Queue
acceptance means local ownership of the bytes. Host receipt needs a matching,
complete record on the other end.

The prompt is `> ` and the title is `=== DP8382x CLI Help ===`. Human output
and machine output use the same cursor tracking. ROM, panic or other direct
output that bypasses the queue can corrupt a record. Native log routing enters
in prompt 47. The host must not reconstruct a corrupted record from fragments.

## Schema 1

A record starts at column zero with the exact ASCII prefix `@DP8382X ` and
ends with CR LF. Its body is space-separated `key=value` fields, with
one ASCII space between adjacent fields and no leading/trailing space in the
body. Field names are lowercase ASCII letters, digits and underscore, starting
with a letter. Values use ASCII letters, digits, underscore, period, colon or
hyphen. There are no escapes. Application identity tokens and session nonces
must be 1–64 bytes in this alphabet. Report unavailable archive identity and
its tree as `unknown`. Known firmware may have `clean`, `dirty` or `unknown`
tree evidence. The release value in `library` is independent of the firmware
build identity.

The receiver removes exactly the CRLF delimiter. It discards lines without a
column-zero prefix and does not strip whitespace, ANSI or prompt bytes from
a candidate. A prefixed record with invalid bytes, malformed/unknown/duplicate
fields, missing mandatory fields, bad types or an unsupported schema is
invalid evidence. Match the field set for the command and state. Field order
is stable on output and immaterial to validation.

| Common field | Meaning |
| --- | --- |
| `schema` | Decimal 1. |
| `boot`, `req` | uint32 application boot identifier and uint64 firmware request counter starting at 1. A fresh processor resets the counter. The application supplies a varying boot value. |
| `type` | `session` for a successful handshake, `result` otherwise. |
| `cmd` | Canonical command. Groups use a dot as separator, such as `cached.health`. `unknown` means dispatch could not select one. |
| `op` | uint64 operation token. Zero when the request has no operation. The cached slot token is a separate payload value. |
| `state` | `terminal` for a final outcome. `admitted` with `primary=IN_PROGRESS` and a nonzero token is reserved for later operation handlers. Admission is never success. |
| `primary`, `detail` | Exact printable `Err` and signed int32 detail. Only terminal `OK` is command success. Details never narrow a backend error. |
| `cleanup`, `cleanup_detail` | Separate printable cleanup outcome, or `none` and 0 when no cleanup was executed. A later cleanup cannot replace a primary error. |
| `time_valid`, `time_us` | 0/1 and uint64 firmware clock in microseconds, sampled when the record is created. With no clock, both are zero. With a valid clock zero is usable. |
| `deadline_valid`, `deadline_us` | 0/1 and uint64 exclusive operation deadline on that same clock. Both are zero for commands here. |
| `reason` | `none`, or the fixed local reason listed below. |

Decimal wire values have no sign except a leading minus for signed negative
details. Bounds are 0–4294967295 for uint32, 0–18446744073709551615 for uint64,
and −2147483648–2147483647 for int32. Flags are exactly 0 or 1. Firmware time
cannot be compared directly to the host's elapsed or UTC clock. Request numbers
belong to a boot, with no separate counter-exhaustion rule.

Local syntax errors use `INVALID_PARAM` with detail 0. Reasons are `empty`,
`command`, `topic`, `arity`, `enum`, `number`, `nonce`, `token_count`,
`token_length`, `line_length`, `byte`, and `input_loss`. A nonterminal or
invalid cached callback status becomes `INVALID_CONFIG` with reason
`callback` and retained signed detail. An internal formatting failure uses
`INVALID_CONFIG` and reason `format`. Other callback errors are returned with
their actual status. Error records omit the command payload.

### Success payloads

| Command | Additional fields in a successful record |
| --- | --- |
| `help` | None. The human help remains optional output. |
| `session`, `version`, `local board` | `nonce library firmware tree sdk target backend clock variant interface address`. Non-session commands use `nonce=none`. The address is 0–31, and the interface is MII or RMII. |
| `local color`, `local quiet` | The setting's name with value 0 or 1. |
| `local console` | `queued dropped_human stalled_writes`. The first is bytes, the other two are saturating uint32 counters. |
| `capabilities` | `valid modes interfaces copper fiber gigabit`. Masks are the portable `LINK_*` and `INTERFACE_*` values and zero if invalid. |
| `cached health` | `health initialized successes failures consecutive last_error last_detail last_ok_valid last_ok_us last_error_valid last_error_us`. Health names are UNINIT, READY, DEGRADED and OFFLINE. Counts are uint32. |
| `cached link` | `valid stale up resolved speed_mbps duplex negotiation pause_valid pause_rx pause_tx observed_valid observed_us`. A current result requires valid=1 and stale=0. Unresolved fields use 0 or `unknown`; duplex is `half` or `full` when resolved. Negotiation is `disabled`, `in_progress`, `complete` or `unknown`. Observed time may be historical. |
| `cached settings` | `trust desired_valid desired_mode desired_modes desired_speed_mbps desired_duplex observed_valid observed_stale observed_mode observed_modes observed_speed_mbps observed_duplex`. Mode is `an`, `forced` or `unknown`. Only the active mode's corresponding data applies. Observed staleness is independent of desired intent. |
| `cached operation` | `slot token slot_status slot_detail cleanup_owed temporary slot_deadline_us`. Slot is `idle`, `running` or `result_pending`. An occupied slot carries its own token and deadline. The query's terminal outcome does not advance that slot. |
| `cached transport` | `batches attempted completed stopped clock_invalid controller_reusable`. Counts are uint32 and can include application startup or other owner traffic. |

For example, a successful session after boot 42 has these bytes followed by
CRLF:

```text
@DP8382X schema=1 boot=42 req=1 type=session cmd=session op=0 state=terminal primary=OK detail=0 cleanup=none cleanup_detail=0 time_valid=1 time_us=7600 deadline_valid=0 deadline_us=0 reason=none nonce=host-87 library=0.1.0 firmware=unknown tree=unknown sdk=unknown target=host backend=sim-mdio clock=simulated variant=DP83826I interface=RMII address=1
```

An invalid line at request 2 produces:

```text
@DP8382X schema=1 boot=42 req=2 type=result cmd=unknown op=0 state=terminal primary=INVALID_PARAM detail=0 cleanup=none cleanup_detail=0 time_valid=1 time_us=8700 deadline_valid=0 deadline_us=0 reason=byte
```

## Session handshake

Send `session <new-nonce>` and require its exact echoed nonce, a valid session
record, the expected identity and schema. The reply's boot and request number
seed the host counter. The nonce is echoed verbatim and is not stored as
processor state. Every other record correlates by boot/request and, for future
operations, by token. A record with another boot or request belongs to older
or unrelated work. A new boot requires a new synchronization before sending an
ordinary command.

Use one outstanding command. Increment the expected request for each complete
line sent, including lines the processor rejects. After a timeout send only
`session <new-nonce>` to resynchronize on an intact stream. Without a valid
matching reply, stop sending commands. A partial command write can leave the
input framing uncertain and needs a newly verified connection. Session queries
do not reconfigure the processor or discard pending work.

The [host fixture](../test/cli/README.md) produces software transcripts from the
real processor and core. Its fault controls are confined to the executable.
