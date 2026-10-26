# 43 - Implement the portable CLI, bounded console and record schema

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [8](8-operation-contracts.md), passive snapshots and the owner
contracts established in [41](41-owner-admission-example.md). Actual feature
handlers follow; expose only working commands here.

## Current evidence and ownership

Reopen TMP1x2's `examples/common/Tmp1x2Cli.h/.cpp` (`setup`, `feed`,
`processCommand`, `tick`, `printHelp`) and `ConsoleBuffer.h` (`format`, `drain`).
Compare 24Cxx's `discardInput`, `Platform::outputReady` and session/result
retention; inspect SHT3x dispatch, LDC1614 help/parser tests and INA228's native
framing with `tools/run_i2c_hil.py::{hilrun_end_re,strip_hilrun_frame}`. Record
current revisions, source/test/runner paths and adopted/adapted/rejected choices.

One lead owns grammar and callback signatures. After freezing them, delegate
disjoint parser/console tests or read-only admission/retention review; choose
available models by complexity. Shared dispatch/contracts and Git remain with
the lead. Do not create a private CLI-owned PHY competing with the adapter.

## Closed work unit

Freeze exact grammar, aliases and machine schema in `docs/cli.md` before coding.
Use `examples/common/Dp8382xCli.h/.cpp` and `ConsoleBuffer.h` with one small
framework-free processor including the public DP8382x API. Keep parsing/dispatch
together until real responsibilities justify output/diagnostic source files;
avoid per-command classes and generic RPC/registration frameworks.

Implement working help/topics, version/capabilities, local controls and cached
health/link/configuration views. Follow grouped aligned sibling presentation,
`> ` prompt and optional ANSI, with plain/color-off output and ANSI reset. Test
actual aliases and dispatch, not only help text. Snapshot rendering performs no
I/O and does not collapse health, link, trust and application policy.

Specify fixed line/token/output capacities, numeric bases/signs, full-token enum
parsing, extra tokens, CR/LF/CRLF/backspace, control characters and overflow
recovery. Input loss discards the entire affected line. A truncated mutation
cannot execute later. Define callback lifetime, feed/tick budgets, owner routing
and processor reconfiguration while work is pending.

The single terminated machine record schema includes version, boot/session,
request/operation identity, admission versus terminal state, printable `Err`,
signed backend detail, primary/cleanup status and valid time/deadline fields.
Keep color and message pointers out. Reserve capacity or retain terminal output
until accepted; ordinary help cannot erase it. Distinguish boot from sequence
reuse, local/firmware time and library/firmware identity; archives stay unknown
when provenance is unavailable. Pending admission is never terminal success.

## Acceptance and checkpoint

Register a host executable under `test/cli/` using real shared sources and a
clearly simulated backend. Test bytewise input, every malformed boundary,
passive/rejected zero-frame/state invariants, short/zero output writes, full
queue, terminal retention and pending reconfiguration. No UART/HIL claim follows.

Ship processor/platform Doxygen and schema/limits in CLI/example guides. Review
README/CHANGELOG and affected docs, run documentation and host checks, record
counts/gates and commit/sync checked units and completion on the recorded branch.
Remote CI remains exclusive to task 71.
