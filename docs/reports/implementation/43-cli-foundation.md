# Prompt 43 · Portable CLI foundation

Work began on 2026-11-20 from clean synchronized `main` at
`468e269f4ea254599b8d0d5cc7d1813c521f3180`. Prompt 8 supplies the retained
operation contract, the core supplies the passive snapshots, and prompt 41
supplies single-owner admission and cached evidence. No ledger procedure has
`implementationStage == 43`.

The lead owns the processor, schema, integration, docs and Git. Two coding
agents worked in parallel on the fixed console queue and the separate host
fixture. The source reviewer independently inspected sibling dispatch and
parser sources, then reviewed this processor and tested it in a separate
build. All used the current model. Console and fixture were distinct C++ work
units. The independent review concentrated on input loss, correlated output
and terminal evidence before and after backpressure.

## Current precedents

All sibling checkouts were clean at the recorded revisions. Their applicable
agent rules, native entry points, test registrations and workflow source were
read without building or editing the siblings.

| Checkout and revision | Source and exercising checks | Decision |
| --- | --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/common/Tmp1x2Cli.h`, `Tmp1x2Cli.cpp::{setup,feed,processCommand}`, `Tmp1x2CliDiagnostics.cpp::tick`, `Tmp1x2CliOutput.cpp::printHelp`, `ConsoleBuffer.h::{format,drain}`. Root CMake registers `cli`, `console` and `hil_runner` using `test/cli/test_cli.cpp`, `test/console/test_console.cpp`, `tools/test_hil_tmp1x2_runner.py`. | Adopt bounded queues, short writes, aligned help and reset ANSI. Adapt help and dispatch to the same table. Setup stays passive, and terminal records keep their own capacity. |
| 24Cxx `1bd9f969d1df55ca5819b0d290ff331869e7d5bc` | `examples/common/Eeprom24CxxCli.h::{discardInput,Platform::outputReady}`, `Eeprom24CxxCli.cpp::{feed,tick,finishSession}`. CMake registers `cli`, `console`, `hil_runner`, `cli_transcript`; see `test/test_cli/test_cli.cpp`, `test/test_console/test_console.cpp`, `test/cli_host/main.cpp` and `tools/test_cli_transcript.py`. | Adapt sticky input discard, retained output and the subprocess fixture. The CLI's write capacity does not control application operation polling. |
| SHT3x-main `7d04cd1a94a01eb9c144fd95d4ec95cff3d22ec3` | `examples/common/Sht3xCli.cpp::{validCommandArity,processCommandString,printHelp}`. Workflow executes `tools/test_cli_health_rate.py`, `tools/check_cli_contract.py`, `tools/test_run_sht3x_hil_parser.py`. | Adopt explicit arity and cached job checks. Use one table and typed terminal evidence in place of hand-synchronized arity and human prose. |
| LDC1614 `4b5b5f5d90940a1c1012ad1c05746973690af7a3` | `examples/common/Ldc1614Cli.cpp::{parseLine,resolveCommand,handleCommand,processCommand}`. Unity registers the fixed help, parser boundary and terminal session tests in `test/test_cli.cpp`; native in `platformio.ini`, with runner tests in `tools/test_ldc1614_hil_runner.py`. | Adopt actual alias and zero-I/O rejection tests. Adapt to one static command table. |
| INA228 `8e439d6c99ece1353cd9f555deed0314794412d9` | Native `examples/esp_idf/basic/main/main.cpp` hilrun branch and `tools/run_i2c_hil.py::{hilrun_end_re,strip_hilrun_frame}`. The workflow runs the parser self-test and `tools/test_run_i2c_hil_parser.py`, including wrong correlation and truncated framing. | Adopt strict correlation and complete termination. Select single-line ASCII records with exact prefix matching and no ANSI or whitespace repair. |
| TCA9548A `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `examples/common/{CliLineBuffer.h,CliArguments.h,Tca9548aCli.cpp}`, `test/test_cli_line_buffer/test_cli_line_buffer.cpp`, native filters in `platformio.ini`, `tools/tca9548a_hil.py` and `tools/test_tca9548a_hil.py`. | Adapt explicit control-byte, sign, overflow and numeric-tail checks. The PHY commands call cached owner APIs, with protocol and help driven by the same tables. |

## Delivered behavior and review

The portable example owns input, static dispatch/help, local config and a
fixed queue. It copies one selected cache through its application callback.
The processor holds a terminal record when the writer cannot accept it and
resumes input only after queue ownership transfers. The session nonce is
verbatim and boot/request identity remains attached to all terminal records.
Separate status, timestamp, operation and cleanup fields provide the wire
contract for later handlers. This stage executes no live feature commands.

Independent review caught CRLF adjacency surviving an input gap, duplicated
enum choices between help and parsing, and CR/LF/CRLF differences in the fixture
reboot timing. Those paths now share the defined semantics. Processor copies
are prohibited to protect queued bytes and request identity. Under the
minimal simulated boot, DP83826I initialization plus link observation can
yield READY and a resolved 10/half link while profile trust is DIRTY. The
transcript test preserves those separate facts and verifies unchanged MDIO
frame and batch totals across CLI calls.

The maintained CLI, architecture, test, README, changelog and progress guides
were reviewed. The new example headers have Doxygen inputs, lifetime, bounds
and result comments. Core API and silicon evidence did not change. The
feature coverage matrix still assigns live command and physical cases to
their owning stages.

## Verification

Windows / PowerShell, GNU 15.1.0, CMake 4.0.1 / Ninja 1.12.1, Python
3.12.10, and Doxygen 1.13.2.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable Release | PASS | 250/250 CTests. Includes four processor suites, three console suites and a subprocess transcript suite. |
| CLI UBSan trap | PASS | 8/8 focused CTests. Covers raw invalid bytes, all line endings, alias and arity checks, independent typed views, zero MDIO, short and stalled writes, terminal retention, reboot and malformed/truncated output. |
| Tests-OFF consumer | PASS | 2/2 separate linked CTests. |
| Source documentation | PASS | 291 generated core pages, two rejected negative fixtures. Strict Doxygen with input overridden to `examples/common` generated CLI pages. |
| Research and tooling checks | PASS | 103 Python unit tests. Ledger: 97 registers, 52 procedures and 35 features. References: 29 vendor artifacts, 27 extracts and 2045 links in 256 Markdown files. Version and diff checks pass. |
| SDK firmware / installed package | NOT RUN | Native CLI integration belongs to 47. The changes affect portable example and host test targets. |
| Remote CI | NOT RUN | First-execution prompt 71 rule. |
| UART / physical behavior | NOT RUN | The corpus contains host recordings and synthetic corruptions. The simulated partner, time and MDIO errors are software evidence. |

```powershell
cmake -S . -B build/p43-host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/p43-host -j4
ctest --test-dir build/p43-host --output-on-failure --no-tests=error
cmake -S . -B build/p43-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON '-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error'
cmake --build build/p43-ubsan --target dp8382x_cli_tests dp8382x_cli_fixture dp8382x_console_tests -j4
ctest --test-dir build/p43-ubsan -R '^cli_' --output-on-failure --no-tests=error
cmake -S test/consumer -B build/p43-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF -DDP8382X_BUILD_PINNED_IDF_TESTS=OFF
cmake --build build/p43-consumer -j4
ctest --test-dir build/p43-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p 'test_*.py' -q
python tools/check_doxygen.py
$p43Docs = Join-Path (Get-Location) 'build/p43-cli-doxygen'
(Get-Content -Raw Doxyfile) + "`nINPUT = examples/common`nOUTPUT_DIRECTORY = `"$p43Docs`"`n" | doxygen -
python tools/verify_ledger.py
python tools/verify_references.py
python scripts/generate_version.py check
git diff --check
```

The checked stage is committed and synchronized on the same `main` execution
branch. The final handoff records the commit ID and upstream comparison.
