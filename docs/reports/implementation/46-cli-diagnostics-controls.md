# Prompt 46 · Typed diagnostic and control CLI

The stage started on `main` from synchronized
`76d900bb9d4a01b996374c5a0f0015d09d9810`. Prompts 41, 43 and 45 provide
application admission, the shared processor, and retained owner operations.
The ledger has no procedure whose implementation stage is 46. This stage
exposes existing procedures and source gates through typed CLI routes.

## Current source audit

The four sibling checkouts were clean. The source paths and registered
evidence below were checked in their current working trees under parent and
local agent rules.

| Checkout | Checked behavior and decision |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/common/Tmp1x2CliDiagnostics.cpp::{startDiagnostic,tickDiagnostic,finishDiagnostic,finishConfigTestOperation}` runs bounded public steps and restores a copied baseline. `Tmp1x2CliOutput.cpp::printDiagnosticResult` exposes terminal and restore evidence. Root CMake registers `test/cli/test_cli.cpp` and `tools/test_hil_tmp1x2_runner.py`; `test_terminal_failures_and_cancelled_diagnostics` exercises final results. Adapt the bounded steps and independently reported restoration. Keep DP8382x selftest read-only and explicit restoration separately admitted. |
| INA228 `8e439d6c99ece1353cd9f555deed0314794412d9` | `examples/esp_idf/basic/main/main.cpp::{startOwnerJob,pollOwnerJob}` checks current ownership before consuming a result. `test/test_basic.cpp` covers triggered budget and cancellation state. The workflow runs `tools/test_run_i2c_hil_parser.py::test_expected_rejection_is_fail_closed` against `tools/run_i2c_hil.py::classify_step`. Adopt exact admission and failure correlation. Retain current evidence through queue backpressure and use the shared DP8382x schema. |
| TCA9548A `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `examples/common/Tca9548aCli.cpp::{processCommand,stop,finish}` gives cleanup precedence and keeps its outcome separate. `native_cli_commands` registers tests through `platformio.ini`. CI runs `tools/test_tca9548a_hil.py::test_stress_requires_full_count_and_verified_safe_off` against `tools/tca9548a_hil.py::classify`. Adapt cleanup priority and attempt accounting. I2C mask restoration belongs to that device. |
| MB85RC `3ace168eb01d687ab60e9fe88cc72322e5cae260` | `examples/espidf_basic/main/main.cpp` prints actual scratch restoration and transfer counts. CI runs `tools/test_hil_runner.py::test_diagnostic_prompt_requires_all_counters_and_restore_proof` against `tools/hil_runner.py::{classify,payload_failure_reason}`. Require distinct restoration evidence and preserve the original failure. The DP8382x runner will consume correlated machine records. |

The current official SNLS647I matched the archived PDF SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
The TI MCU+ branch `next` resolves to
`4b9a66f4e18e02a834961f5f834155c846cab041`, with source/header blobs
`b320d30c92f7f49885a5249b560744f8b063814d` and
`7aab53d4eb7bf507684d7e5e7f894dcdc7ebb777`. Its odd-nibble helper at source
line 535 sets CR2[1], which table 8-29 defines as disable. TI's combined
repository `main` was `450af1144b8caeb58724bcbae081b409f6bf5c3a` and its
additional DP83826A comparison stays subject to exact-part selection.
[Pinned MCU+ source](https://github.com/TexasInstruments/mcupsdk-core/blob/4b9a66f4e18e02a834961f5f834155c846cab041/source/board/ethphy/icss_emac/ethphy_dp83826e.c), [TI feature catalogue](../../reference/library-ti-features.md), [SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf)

| Exact-part source | Independent behavior checked |
| --- | --- |
| SNLS647I tables 8-19, 8-40, 8-45–47 and 8-50, sections 8.3.14–16 and SNLA266B tables 5-1–5 | Loopback companion/restart/restore order, BISCR generator/checker, BIST acquisition, separate COMPT stimulus and stop, collision and calibration. Unsupported `0428` COMPT stays source-gated. |
| SNLS647I tables 8-54–71, SNLA428 tables 2-1/2-5 | Raw and recipe TDR field bounds, ordered manual capture, guarded fresh words, raw autorun and separate HARD restore. Cable distance needs matching recipe evidence and supplied propagation calibration. |
| SNLS647I tables 8-29, 8-41, 8-49, 8-73, 8-85 and 8-89 | LED0 source 8 produces literal `0x0041` with the enable bit. LED0 width is `0x0078` including bit 6. Reserved LED3 drive rejects before frames. CR2 bit 1 zero enables odd-nibble detection. Repeater enable uses one owner path per PHY. |
| SNLS647I tables 8-30, 8-52/53, 8-72, 8-79–83 and 8-86 | Fast-drop code limits, raw MSE tuple and the split amplitude transition. Gain and unresolved MSE interpretation stay gated. The safe bridge is exercised before target amplitude changes. |
| SNLS647I 8.3.3, 8.3.6 and tables 8-19, 8-35–37 | IRQ role and full enable bytes, single destructive collector, ordered PHY/BMCR power writes, EEE HARD and replay, and separate legacy LPI request/clear receipts. Deep power-down remains gated by the exit procedure. |
| SNLS647I tables 8-90–97 | WoL status mask `0x0800` differs from the W0C acknowledgement command. The transcript checks acknowledge word `0x11A1` and MAC words `0x2211`, `0x4433`, `0x6655`. The independently registered `wol_vectors` proves Secure-ON placement. The CLI and fixture trace omit password bytes. |

## Implementation and verification

Three agents worked concurrently with the current model. One developed the
diagnostic routes and transcript suite, one implemented IRQ/power/EEE/LPI/WoL
and the common fixture, and one owned pins/interface/quality/analog, matrix and
coverage assertions. Each agent ran a distinct build directory and did not
mutate Git. The lead reviewed source and diffs, integrated dispatch, record and
native owner behavior, ran final checks and owned Git.

The shared static selectors drive help and dispatch. Handlers create existing
typed settings, and fixture prerequisites require a separate application
attestation. The native bridge calls the stopped instance and the same
cooperative slot. All reset-bearing operations share the initialization
silence gate. Peer access stays quarantined if an attempted EEE HARD reset
loses its clock anchor. The fixed record and queue preserve partial results;
already-admitted work and restoration continue under stalled output. Selftest
uses one identity batch, then one safe PHYRCR batch on a later tick. The
[CLI guide](../../cli.md) owns the command and record grammar. The one
[feature coverage matrix](../../feature-coverage.md) resolves ledger owners to
the typed CLI paths and preserves source/physical gates.

Windows host with GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1, Python 3.12.10 and
Doxygen 1.13.2. The pinned SDK is ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`. The host SDK build verified 45
source/header blobs. Both P4 links use the installed GCC 15.2.0 toolchain.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable Release | PASS | 255 CTests, including four new registered transcript suites with 31 cases. Existing fixture transcripts also passed. |
| Pinned SDK host | PASS | 366 Release CTests. `native_cli_features` exercises stopped admission, successful power apply, EEE cancellation and shared quarantine after failed reset clock. |
| Focused UBSan trap | PASS | 20 registered portable CLI, reset and transport cases and 9 native CLI cases. |
| Tests-OFF consumer | PASS | Two linked CTests. |
| ESP32-P4 SDK link | PASS | Basic image 515,792 bytes and owner image 401,632 bytes. Firmware console attachment is stage 47. |
| Source and tools | PASS | Doxygen 293 pages and two rejected negative fixtures, strict CLI/bridge documentation, 107 Python tests, reference, ledger, version and diff checks. |
| Package, remote CI, UART and physical/electrical qualification | NOT RUN | Later stages own those gates. |

```powershell
cmake -S . -B build/p46-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/p46-release
ctest --test-dir build/p46-release --output-on-failure --no-tests=error
cmake --build build/p45-native
ctest --test-dir build/p45-native --output-on-failure --no-tests=error
cmake --build build/p43-ubsan --target dp8382x_cli_fixture dp8382x_cli_tests dp8382x_console_tests dp8382x_reset_tests dp8382x_transport_tests
ctest --test-dir build/p43-ubsan -R '^cli_|^reset_|^transport_reports$' --output-on-failure --no-tests=error
cmake --build build/p44-native-ubsan --target dp8382x_native_cli_tests
ctest --test-dir build/p44-native-ubsan -R '^native_cli_' --output-on-failure --no-tests=error
cmake --build build/audit43-44-consumer
ctest --test-dir build/audit43-44-consumer --output-on-failure --no-tests=error
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 build
scripts\idf.cmd -C examples/esp_idf/owner -B C:\esp\build\dp8382x-p41 build
python -m unittest discover -s test -p 'test_*.py' -q
python tools/check_doxygen.py
$cliDocs = Join-Path (Get-Location) 'build/p46-cli-doxygen'
(Get-Content -Raw Doxyfile) + "`nINPUT = examples/common examples/esp_idf/common`nOUTPUT_DIRECTORY = `"$cliDocs`"`n" | doxygen -
python tools/verify_references.py
python tools/verify_ledger.py
python scripts/generate_version.py check
git diff --check
```

The native host uses `DP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf`. Both
sanitizer builds use `-fsanitize=undefined -fsanitize-undefined-trap-on-error`.
The completion handoff records the synchronized checkpoint.
