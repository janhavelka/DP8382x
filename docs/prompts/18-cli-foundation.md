# 18 - Build the shared portable CLI and console

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-17.

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
