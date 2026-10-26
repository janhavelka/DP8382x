# 18 - Build the shared portable CLI and console

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-17.

Before the audit, apply COMMON's short rule reminder: current sibling sources,
selected conventions, owned paths, source/general documentation, focused local
checks and same-branch commit/sync. Choose subagent models by task complexity.

## Fresh audit and owned work

Inspect actual TMP1x2/SHT3x/24Cxx shared CLI dispatch, LDC1614 topic help and host
parser tests, and INA228 framed command protocol. Compare current native-IDF front ends
with their runners.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Freeze exact command grammar and machine-readable framing in `docs/cli.md` using
CLI_CONTRACT. Build one framework-free processor under examples/common, with the
familiar grouped help, Status vocabulary and optional ANSI. Local
help/version/capabilities/cached status commands must work now; don't expose
unimplemented command stubs.

Implement strict token/number/enum parsing, bounded input and output, CR/LF/editing
behavior, invalid input rejection and optional color-off mode. Separate command metadata
from execution enough to keep help/dispatcher/runner consistent without a generic
command framework. Keep fixed capacities and explicit overflow/truncation outcomes.

Introduce owner-routing and rendering seams with lifetimes, so the console never issues
uncoordinated MDIO. Cached health/link/config views are bus-silent. The console polls
admitted operations within budgets and produces completion only from actual retained
results. Backpressure cannot silently erase terminal records.

Add boot/session, request/operation correlation and example-only build identity. Do not
borrow an unrelated repository's Git identity for source archives. Keep local and
firmware time domains distinct. Provide a host executable using real CLI code and a
clearly simulated backend for parser/runner integration tests.

## Concrete CLI modules and records

Apply [CONVENTIONS](CONVENTIONS.md). Reopen
`TMP1x2/examples/common/Tmp1x2Cli.h/.cpp` (`Cli::setup`, `feed`,
`processCommand`, `tick`, `printHelp`) and `ConsoleBuffer.h` (`format`, `drain`).
Compare `24Cxx/examples/common/Eeprom24CxxCli.h/.cpp` (`discardInput`,
`Platform::outputReady`, session/result retention). Adopt the recognizable small
processor API and bounded console servicing, adapted to an externally owned PHY.
Do not let a private CLI-owned driver compete with the IDF adapter's live object.

Use example-only `examples/common/Dp8382xCli.h/.cpp` and `ConsoleBuffer.h` as
proposed responsibility homes. Keep parsing/dispatch together until output or
diagnostic jobs warrant `Dp8382xCliOutput.cpp`/`Dp8382xCliDiagnostics.cpp`, as in
TMP1x2. Do not create one class/file per command. A short fixed metadata table
may generate grouped help and legal aliases; parsing and actual action tests
must still prove execution. Include `<DP8382x/DP8382x.h>` through the public API.

Freeze in `docs/cli.md` before coding:

- Complete grammar for `help/?`, `version/ver`, `drv/health`, `cfg/settings`,
  capabilities and implemented local controls, including extra-token handling.
- Line length, maximum tokens/token length, accepted numeric bases/signs,
  full-token parsing, CR/LF/CRLF/backspace rules and overflow recovery. An input
  loss notification discards the complete affected line; it cannot execute a
  truncated mutation later.
- Human output and the single machine record schema: version, boot/session ID,
  request ID, operation ID where used, admission versus terminal state, stable
  printable `Err`, signed backend `detail`, primary/cleanup status and valid
  elapsed/deadline fields. Never print a message pointer as protocol data.
- Which records are local/passive, which require owner admission and which must
  be retained until accepted by the output queue. A terminal record has reserved
  capacity or an explicit retained pending slot; regular help output cannot erase
  it. Print loss/truncation counters as evidence, not a replacement completion.

Inspect `INA228/examples/esp_idf/basic/main/main.cpp` framing and
`tools/run_i2c_hil.py::{hilrun_end_re,strip_hilrun_frame}`. Its `HIL_BEGIN`/
`HIL_END` token/sequence pattern is a comparison, not a mandatory byte format.
An `IN_PROGRESS` trailer cannot satisfy a DP8382x operation's terminal result.
Require terminated records and correlate a new boot separately from sequence
reuse. Keep color out of machine records and restore ANSI state after human text.

Add executable CLI fixture targets under `test/cli/` using the actual shared
sources and a simulated callback backend. Assert before/after frame counts,
configuration/operation state and retained results for every passive/rejected
command. Test byte-by-byte input, overlong/malformed numeric tokens, embedded
control characters, CRLF, output short writes/zero progress, full queue, retained
terminal record and processor reconfiguration while work is pending. Register
these sources explicitly as TMP1x2 CMake does; they are not a core dependency.

Doxygen must describe processor/platform callback lifetime, feed/tick work
bounds, owner routing and output acceptance. Update `docs/cli.md`, the example
guide, root README and CHANGELOG with only working commands. Run Doxygen and
host CLI/console checks together. Parser/console tests are suitable disjoint
agent work once grammar and callback signatures are frozen; a stronger reasoning
review is useful for admission/completion retention. Keep a single grammar owner.

## Acceptance and handoff

Execute help/alias/strict rejection tests and real command dispatch against the host
fixture. Cover extra tokens, overflow, input fragmentation, ANSI off, full output queue
and completion retention. Assert no MDIO/state change for rejected or cached/local
commands. No UART or hardware claim from host tests.

Expected output: Working shared CLI foundation and frozen automation grammar. Include
source documentation, relevant guides, feature/evidence matrix and progress updates.
Apply COMMON's completion rules; report actual commands, results and remaining gates.
Continue only when the concrete required inputs exist; COMMON explains how unrelated
gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
