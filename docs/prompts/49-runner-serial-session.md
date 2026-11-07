# 49 - Implement bounded serial sessions without implicit hardware actions

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisite: [48](48-runner-protocol-artifacts.md). Reuse its parser,
classification and durable two-file records as one canonical runner; no second
protocol.

## Current evidence and ownership

Reopen current TMP1x2 `SerialSession`/`EvidenceFiles` and tests, INA228
`run_step`/`read_until_hilrun_end` plus short-write/stale-input tests, and
TCA9548A session/artifact handling. Compare their firmware producers and 24Cxx
identity expectations. Inspect optional serial library open/DTR/RTS behavior
and the chosen platform restrictions. Record exact revisions, paths,
adopted/adapted/ rejected conventions and actual dependencies; peer behavior
is not PHY evidence.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Implement

Extend `tools/dp8382x_hil_runner.py` with explicit port/baud, expected
firmware/ board/target context, selected suites, output directory and finite
command, session, duration and cleanup limits. Validate arguments, fixture
permissions and limits before opening anything. Give the total session an
absolute monotonic deadline, reserving a finite finalization allowance;
command/cleanup deadlines are clipped to its remaining budget. `--help` and
dry-run need neither pyserial nor SDK and must never open a port. No port
scanning/guessing, flashing, power-cycle, reset, reconnect or command replay
occurs automatically.

Reserve and write provisional incomplete evidence before hardware access.
Audit serial-open DTR/RTS behavior; choose a non-resetting configuration where
supported and report unavoidable restrictions before use rather than causing
an implicit reset. Borrow/close serial resources in a stated order and
preserve open/write/read/close errors in artifacts.

Use the passive session/identity handshake from prompt 43 when attaching to an
already-running device; do not require replay of its boot banner. Require
complete reported identity and correlated boot/session records before case
evaluation, comparing explicit expectations and preserving unknown fields as
unknown. Never substitute runner checkout SHA for flashed identity. Send one
correlated command with a total write deadline. A partial write fails the
attempt and is never replayed or completed by sending its remainder. Mark
input framing lost: appending cleanup could complete the partial mutation, so
issue no more commands on that session unless the frozen protocol provides a
separately proved safe resynchronization. Keep remaining hardware effects and
unavailable cleanup visible. Read under aggregate deadline, maximum
record/decoder state and total capture limits; fragmentation cannot turn an
unterminated numeric field into completion.

Use prompt 43's one-outstanding-command boot/req correlation and nonce
resynchronization after timeout. Partial input remains framing-unsafe: no appended
cleanup or replay. Preserve first failure separately from explicitly admitted
restoration. Create pyserial with port=None and finite timeout/write_timeout;
set DTR/RTS before open. Never flush/drain. Run open and close on daemon threads
joined under finite deadlines; if either stalls, mark INCONCLUSIVE, finalize
artifacts and exit via os._exit(nonzero). No serial child-process layer.
Test a backend whose open/close never returns; compiled fixture subprocess
management remains separate. Finalize the two artifacts as prompt 48 specifies.

Record console type, whether baud applies, and DTR/RTS behavior at open.
A USB Serial/JTAG session must not inherit UART-bridge reset assumptions.

## Acceptance

Inject open failures, DTR/RTS limitations, partial/zero writes, fragmented
reads, serial disappearance, cancellation, deadline exhaustion, cleanup
timeout and close/finalization failures. Assert no port access on
invalid/dry-run options and no hidden replay. Drive the compiled CLI fixture
with bounded process lifetime to prove actual producer/parser parity. Run
help/offline tests without pyserial. These tests qualify runner behavior, not
a physical PHY.

Document Python session lifetime, backend limits and failure behavior. Update
`docs/hil-runner.md` with real/offline invocations and the passive handshake.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
