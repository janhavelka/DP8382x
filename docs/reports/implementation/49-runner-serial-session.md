# Prompt 49 · Bounded serial sessions

Stage (a) assessed the sources and prepared the implementation brief on clean
`main` at `c55463678a12ab77ca497fa86eaa80cc2ff8bf1e`, the audited prompt-48
checkpoint. `origin/main` at entry was 0 ahead and 0 behind. The unattended
wrapper owns remote synchronization. The operator requested solo execution
with GPT-6.2 Sol at high effort for the lifetime, framing and deadline work.

## Prerequisites and source audit

Prompt 48 supplies `Session`, the plan and case classifier in
`tools/dp8382x_hil_runner.py`, `Decoder` and identity matching in
`tools/dp8382x_hil_protocol.py`, and `EvidenceFiles` plus read-only verification
in `tools/dp8382x_hil_artifacts.py`. `FixtureProcess` already owns the bounded
compiled child. The shared producer is `examples/common/Dp8382xCli.cpp`
(`Processor::feed`, `finishLine`, `identity` and record emission). Its
`session <nonce>` reply exposes boot, req and build context without startup
output or PHY operations. Its line state also explains why sending a delimiter
into an unknown input prefix is unsafe. [Prompt 48 report](48-runner-protocol-artifacts.md) ·
[Independent audit](47-48-independent-review.md) · [CLI contract](../../cli.md#session-handshake)

The P4 example identities in `examples/esp_idf/{basic,owner}/main/main.cpp`
report `esp32p4`, backends `p4-netlist-basic` and `P4-raw-frame`, respectively,
and `uart0` at their configured rate. Both defaults select UART0 at 115200.
The wire record carries board context through those declared build fields.
The operator supplies the physical board identifier and authorization separately
because the record has no individual board ID. No register procedure has
implementation owner 49. The repository has no active CI workflow or custom
hook at this checkpoint. [Board context](../../esp32p4-board.md)

The four sibling checkouts reported clean status. Their root `AGENTS.md`
and test registration were inspected as read-only references.

| Checkout | Evidence and decision |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `tools/hil_tmp1x2_runner.py::SerialSession` clips waits, retains a short-write framing failure, and keeps close errors separate. `EvidenceFiles` reserves an INCONCLUSIVE report and publishes last. `tools/test_hil_tmp1x2_runner.py` tests deadline clipping, fragmented markers, close, output reservation and cleanup suppression after framing loss. Root CMake registers it. `examples/common/Tmp1x2Cli.cpp` emits the marker and `Tmp1x2CliOutput.cpp` reports firmware metadata. Adopt failure and ownership semantics. Keep DP8382x's nonce reply in place of startup capture, control-character synchronization and prompt markers. |
| INA228 `8e439d6c99ece1353cd9f555deed0314794412d9` | `tools/run_i2c_hil.py::{run_step,read_until_hilrun_end}` checks full writes and a terminated, correlated trailer. `tools/test_run_i2c_hil_parser.py` tests short writes and stale input with CI registration. Both `examples/01_basic_bringup_cli/main.cpp` and `examples/esp_idf/basic/main/main.cpp` produce token/sequence begin and end lines. Adapt exact correlation and rejection of an incomplete numeric field. Consume and persist raw bytes through the existing decoder. |
| TCA9548A `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `tools/tca9548a_hil.py::SerialRunner` configures DTR/RTS before open, retires session loss and reserves reports before live work via `prepare_artifacts`. `tools/test_tca9548a_hil.py` and `tools/test_hil_artifacts.py` cover open configuration, short writes, stale input and failure receipt handling. `examples/common/Tca9548aCli.cpp` supplies a bus-silent marker. Adapt pre-open setup and failure retention. DP8382x uses two files, its own console-specific settings and a bounded close even if open partially fails. |
| 24Cxx `1bd9f969d1df55ca5819b0d290ff331869e7d5bc` | `tools/hil_identity.py::{parse_identity,check_identity}` and `tools/test_hil_identity.py` validate complete identity and preserve observed commit spelling and unknowns. `examples/common/Eeprom24CxxCli.cpp` emits the banner and build line, exercised through the compiled consumer in root CMake. Keep the existing DP8382x identity rules, including the requirement that the observed abbreviation prove the full requested prefix. |

## Serial backend and fixture admission

Use the optional pySerial 3.5 native backend. Windows Python 3.12.10
already imports version 3.5 from its installed site-packages, with
`serial.Serial.__module__ == "serial.serialwin32"`. The inspected package
sources were `serial/serialutil.py` (`SerialBase.__init__` and setters),
`serial/serialwin32.py` (`open`, `_reconfigure_port`, `read`, `write`, `close`)
and `serial/serialposix.py` (the same lifecycle and byte I/O). Only the
live path needs the optional import. [pySerial 3.5 API](https://pyserial.readthedocs.io/en/latest/pyserial_api.html)

Windows opens the device handle before it applies the requested DTR/RTS
states via the DCB. The POSIX implementation opens the descriptor before
applying ioctl-based states and suppresses EINVAL/ENOTTY for those ioctls.
Drivers may assert or glitch lines during open, and platform close can
change line levels. Windows pySerial itself purges the queues at open
and the POSIX implementation clears its input buffer. The runner cannot
promise preservation of bytes queued before attachment. Use finite settings
on a closed object and avoid reapplying port settings while open. The
runner calls no flush, purge, drain, reset-buffer or break method.
[Windows backend at v3.5](https://github.com/pyserial/pyserial/blob/v3.5/serial/serialwin32.py) ·
[POSIX backend at v3.5](https://github.com/pyserial/pyserial/blob/v3.5/serial/serialposix.py)

Require an explicit local port (Windows COM name or POSIX local tty path),
declared board and fixture authorization reference, declared console, and
operator confirmation that this exact driver, port and wiring permit
reset-silent open and close and exclusive attachment at a clean input line.
The receipt records those declarations, not measured line state. Reject
unknown or reset-capable setups without that evidence before port access.
Use DTR and RTS deasserted with flow control disabled. A UART bridge needs
physical isolation from reset/boot or evidence for its exact circuit and
driver. USB Serial/JTAG is a separate declaration with its own open/close
qualification and an inapplicable wire baud (`0`). The host library still
needs a nominal baud value. Espressif implements a distinct USB Serial/JTAG
control-line reset sequence, so native USB alone gives no reset assurance.
The shipped UART0 examples do not supply an image using that console.
[P4 boot-mode selection](https://docs.espressif.com/projects/esptool/en/latest/esp32p4/advanced-topics/boot-mode-selection.html) ·
[P4 USB Serial/JTAG console](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32p4/api-guides/usb-serial-jtag-console.html) ·
[esptool reset implementation](https://github.com/espressif/esptool/blob/v5.1.0/esptool/reset.py)

## Stage (b) implementation brief

1. Extend the canonical CLI with `run-serial` and a dry-run of the same
   validated plan. Require an explicit port, UART baud or USB inapplicable
   baud, board/fixture authorization, target/backend/part/interface/address,
   expected console, exact firmware or explicit prefix, suite and evidence
   destination. Keep the existing offline modes and artifact replay usable.
   The existing `passive` suite is the serial starting scope, including its
   deliberately out-of-range rejection and before/after proof. Keep
   `fixture-operations` scoped to the simulated producer. Parse and reject
   invalid or incompatible options and reserves before importing/opening a
   port. No real port is authorized for this unattended run.
2. Add a small serial I/O module alongside the fixture module. Start one
   absolute monotonic run deadline, with distinct active duration, per-command
   and cleanup limits, and a reserved finalization interval. Clip each deadline
   to its parent. Leave time for one close attempt inside cleanup even if
   resynchronization consumes its own interval. Use a single serialized port
   owner, fixed finite driver timeouts, bounded chunks and one backend
   `write` per complete line. Include setup and byte I/O in deadline checks.
   On Windows a finite driver setting still relies on overlapped I/O completion.
   Supervise backend calls that may overrun the host deadline with bounded
   daemon joins. A lingering operation owns the port until process exit.
3. Reserve and sync the incomplete JSON and reserve the transcript before
   creating `Serial(port=None, ...)`. Set DTR/RTS before assigning the port
   and opening it. Complete the open worker before borrowing the port for
   byte I/O. When that owner returns, close once on a daemon thread, including
   after a returned open failure with a constructed port. Do not race close
   against an active worker. If open or close stalls, record the unsuccessful
   lifecycle phase and finalize within the reserved interval, then use
   `os._exit` with a nonzero code. A read/write worker that remains active
   requires the same ownership treatment. If evidence publication fails,
   preserve the provisional pair as nonpassing and report the finalization
   error. Keep the port referenced until hard exit when backend destruction
   could otherwise repeat a stalled close.
4. Send a fresh nonce as the first line, compare the complete session identity
   and reuse the existing boot/req/token tracking for ordinary commands. Keep
   one outstanding request. Feed raw RX through the same decoder across read
   boundaries and enforce the byte cap including noise. Capture late bytes
   without promoting them to timely completions. Use a finite tail observation
   interval for malformed or surplus records, with every observed byte retained
   within the capture limit. A serial idle read is neither a terminal nor EOF.
5. A short, zero, failed or interrupted write can leave a prefix or an
   unconfirmed effect. Retire command admission immediately, retain that first
   failure and report restoration unavailable. Preserve the actual write
   attempt separately from commands confirmed sent. After a reply timeout
   following a timely full write, use the allocated recovery interval for at
   most one fresh `session <nonce>` query. Retain late/old receipts as evidence
   without letting them satisfy the old request. Require a complete nonce and
   matched identity and corroborate boot/req before declaring continuity.
   Record a changed boot, missing reply or mismatch separately. The
   passive suite has no admitted firmware restoration, so either recovery
   outcome closes the run with its original failure. A framing-unsafe session
   sends no recovery or restoration bytes.
6. Keep the two-file carrier, bounded JSON and verified raw transcript. Record
   live serial scope distinctly from host protocol, with typed serial metadata
   covering console/baud applicability, requested line states and limits,
   fixture declarations and observed lifecycle outcomes. Keep old host
   receipts readable and replay complete runs through the canonical case
   classifier. Preserve the primary case failure, the recovery/cleanup status
   and evidence failures independently. A bounded RX buffer capped by the
   existing allowance can keep filesystem writes off the active serial path
   and preserve the prefix on handled errors. Bound finalization by the reserved
   deadline and check expiry before atomic replacement. If interrupted before
   replacement, the provisional JSON is the published result. Document the
   filesystem boundary and the possible missing tail on abrupt process loss.

Stage (b) owns the runner, a serial I/O module, the serial extension to the
existing evidence carrier, focused Python tests and their CTest registration.
Extend `docs/hil-runner.md` with offline/live invocations, deadlines, passive
attach and teardown. Review `README.md`, `CHANGELOG.md`, `docs/cli.md`,
`docs/architecture.md` and the example/test guides, updating the affected
instructions with the implementation. Stage (c) independently traces the
landed code against the producer and the audited backend. Stage (d) updates
progress after verification. This planning stage changes no implementation
status.

## Acceptance and baseline receipts

Test invalid or dry-run arguments and output collisions with a port factory
that must never execute. Inject control-line and open failures, partial/zero
and uncertain writes, bytes split inside a numeric field, read timeout,
device disappearance, wrong firmware or reboot, cancellation, exhausted
budgets, recovery timeout and close/finalization errors. Assert one write per
attempt, no boot banner dependency, preserved bounded prefixes, and no cleanup
bytes after framing loss. Test a never-returning open and close in disposable
test processes with an outer timeout and inspect the nonzero exit and evidence
pair. This test containment is separate from the production daemon-thread
lifetime. Drive the serial path through an injected adapter connected to the
bounded compiled CLI fixture, and keep existing host replay cases.

Stage (a) baseline on Windows used Python 3.12.10, GNU 15.1.0, CMake 4.0.1 and
Ninja 1.12.1:

- PASS. `python -m unittest discover -s test -p 'test_hil*.py' -q` ran 56
  runner tests with one compiled-fixture case deferred to CTest.
- PASS. `cmake -S . -B build/p49-plan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`,
  `cmake --build build/p49-plan --target dp8382x_cli_fixture --parallel 4`
  and `ctest --test-dir build/p49-plan -R '^cli_hil_runner$' --output-on-failure --no-tests=error`
  built and ran the existing compiled producer check.
- PASS. `python -S tools/dp8382x_hil_runner.py --help` and the offline
  `dry-run --suite passive --target host --backend sim-mdio --variant DP83826I --interface RMII --address 1`
  ran with site-packages disabled.
- PASS. `python tools/verify_references.py` checked 29 vendor artifacts, 27
  extracts and 2147 local links. Git whitespace checks passed for the tree
  and the new report.
- Stage (b) through (d) should rerun focused runner, CTest and artifact checks,
  then the affected reference/link, version, Doxygen and diff checks.
  Portable/package consumer, SDK link, remote CI and physical verification
  were NOT RUN for this brief. No hardware receipt exists for open/close line
  behavior on an authorized fixture. Remote CI remains prompt 71.
