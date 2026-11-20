# Prompt 47 · P4 CLI integration

Work began on synchronized `main` at `12085ea709b599b60898cfb0b100753e1067ba50`.
The bounded console and its tests were checkpointed at `97b3c65`. The basic
network, raw frame boundary and typed CLI prerequisites are present. The
board power, reset, clock and fixture qualification remain open.

Three agents worked in parallel with the active model. They handled the
bounded queues and their tests, the raw application's lifecycle and frame
policy, and the focused source/matrix review. The lead integrated the common
contract, UART pump, basic example and pinned native tests, and owned Git.

## Source evidence

These sibling checkouts were clean. The review read their agent rules,
executing producers, tests and workflow definitions.

| Checkout | Source and decision |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/esp_idf/basic/main/CMakeLists.txt` selects the shared CLI sources. In its `main.cpp`, `initializeConsole`, `writeConsole` and `inputTask` separate queued input and bounded write prefixes. `examples/common/ConsoleBuffer.h` counts dropped output. Root CMake and `.github/workflows/ci.yml` run CLI, console and runner checks. `tools/test_hil_tmp1x2_runner.py::test_console_integrity_rejects_new_or_saturated_loss` checks explicit loss. Adopt shared sources, bounded prefixes and accounting. Keep terminal capacity and discard gaps in order. |
| INA228 `8e439d6c99ece1353cd9f555deed0314794412d9` | `examples/esp_idf/basic/main/main.cpp::processCommand` produces framed results through synchronous stdout; the native CMake selects its own parser. `tools/run_i2c_hil.py::{read_until_hilrun_end,run_step}` correlate frame identity and check the full write. The workflow runs `tools/test_run_i2c_hil_parser.py` for short writes and stale or trailing frames. Adopt correlation and incremental build identity. Keep one DP8382x parser and the bounded writer. |
| TCA9548A `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `examples/espidf_basic/main/main.cpp` explicitly starts its device and discards a partial input on console loss. `examples/common/Tca9548aCli.cpp::{processCommand,stop,finish}` separates command cancellation and cleanup. The CI source runs PlatformIO `native_cli_commands`, including cancellation and restoration failures in `test/test_cli_commands/test_cli_commands.cpp`. Adopt explicit startup and gap recovery. Retain the PHY owner's separate results and maintenance policy. |
| 24Cxx `1bd9f969d1df55ca5819b0d290ff331869e7d5bc` | `test/cli_host/main.cpp` and `examples/esp_idf/basic/main/CMakeLists.txt` link the same `Eeprom24CxxCli.cpp`. Native `main.cpp::checkConsoleInput` checks UART errors and queue saturation around input. Root CMake and CI run `tools/test_cli_transcript.py` with the compiled producer and separate runner tests. Adapt producer parity and gap detection. Keep DP8382x's schema and ESP32-P4 target. |

The pinned IDF is v6.1 at `fff9895c82d744c7237be8847347bdd1b07c6643`.
`components/log/src/os/log_write.c` installs the callback atomically and
`components/log/src/log.c` uses one formatted call in V1. The pinned
`components/esp_driver_uart/src/uart.c::uart_tx_chars` accepts available
FIFO space and takes the driver's TX mutex. Only the UART task calls this
driver TX path. It holds no PHY or application gate. The event queue reports
observed RX faults and treats saturation as a gap when receipt order is lost.
The firmware pins UART0, 115200 bit/s, with the secondary USB console disabled.

## Ownership and coverage

- Both P4 components include `examples/esp_idf/common/ConsoleSources.cmake`.
  It selects `examples/common/Dp8382xCli.cpp` and `Dp8382xUartConsole.cpp`
  for the application. The real-core host fixture selects the same processor.
  The production library target has no console dependencies. The pinned
  cross-compiler also checks the width of the packed cable flags in the
  shared formatter, which stores them as the specified 16-bit value.
- The application task owns the processor, board gate, installed owner and
  application mode. The UART task exchanges bounded bytes. Reentrant V1 log
  calls format locally and try to enqueue a whole ordinary line. Terminal
  records keep separate capacity. Periodic status uses the same writer.
- The basic example keeps its explicit startup and timed demo. A commanded
  stop supersedes the remaining demo after stop/fence. The raw example replaces
  its single keys with shared application commands. It requires the current
  verified 100/full AN profile and closed frame queues before resuming from
  maintenance. Raw BMCR maintenance withdraws configuration/SDK proof before
  restart. END preserves installed contexts and reports the SDK restriction.
- Local-peer probing stays external to the basic demo. Pin and fixture
  attestation remain with the board. Missing IRQ wiring affects physical
  delivery cases; polled IRQ/event evidence uses the existing collector.
  The [coverage matrix](../../feature-coverage.md) closes CLI routing for
  the 35 feature IDs, 52 procedures and all 283 field owners while retaining
  source and physical gates.

## Verification

Windows host with GNU 15.1.0, CMake 4.0.1, Ninja, Python 3.12.10 and Doxygen
1.13.2. The native host build verifies 45 pinned SDK sources and headers.
The P4 builds use the pinned SDK's RISC-V GCC 15.2.0.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable host | PASS | 261 Release CTests, including the compiled fixture and four native-console queue tests. The tests exercise concurrent logs, fragmented input and a known gap, short/zero sink returns and terminal cleanup delivery under backpressure. |
| Pinned SDK host | PASS | 371 Release CTests. The production basic app tests cover attach/cached zero frames, active-session refusal, stopped AN/forced and BMCR reconciliation, stop failure and retry, and unexpected STOP during a callback burst with a withheld sink. The production raw app test covers application admission, release, queued frame service, busy/cancel and END. The native CLI tests also check deadline, accepted partial effects and cleanup obligations. |
| Focused UBSan | PASS | 19 portable CLI checks and 14 pinned SDK console/CLI integration checks. |
| Tests-OFF consumer | PASS | Two separately linked portable consumer checks. |
| P4 links | PASS | `dp8382x_p4_ethernet.bin` and `dp8382x_p4_owner.bin` linked. The ELF symbol tables contain the shared processor, native route, UART startup and log hook setter. Both builds use the default-closed board gate. The owner test-frame gate is off. |
| Source and tools | PASS | 293 public Doxygen pages, two rejected negative fixtures, strict CLI/platform Doxygen, 107 Python tests, version, references, ledger and diff checks. |
| Physical UART, PHY and remote CI | NOT RUN | No flash or live fixture operation. Remote CI remains stage 71. |

Portable build and tests used `build/p46-release`. The pinned SDK host used
`build/p45-native`. Both CTest invocations used `--output-on-failure` and
`--no-tests=error`. The P4 images used `scripts/idf.cmd` with
`examples/esp_idf/basic` and `examples/esp_idf/owner` and separate short
build paths under `C:/esp/build`.
