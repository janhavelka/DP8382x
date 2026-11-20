# Prompt 45 · Link, lifecycle and retained operations CLI

Work started on 2026-11-20 on `main` at synchronized
`110b6b0f990bd80765b2f5c3a4cd457f83018a03`. Prompt 44 supplies the shared
parser, owner routes and machine records. The core already implements link
observation, destructive event collection, reset and restoration. Stage 45
exposes those operations through the shared processor.

## Source audit

Each sibling checkout was clean. The audit read the parent and repository
agent rules, current native front ends, executing code and test registration.
The paths below are relative to the named sibling. Siblings were read-only.

| Checkout and revision | Observed behavior and DP8382x decision |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/common/Tmp1x2CliDiagnostics.cpp::startDiagnostic` records work, `tickDiagnostic` advances phases through public calls, and `finishDiagnostic` copies the result for `Tmp1x2CliOutput.cpp::printDiagnosticResult`. `restoreProfile` schedules a distinct operation. The native `examples/esp_idf/basic/main/main.cpp` feeds and ticks the shared processor. CMake registers `cli`, `console` and `hil_runner`. `test/cli/test_cli.cpp` checks bounded per-tick transfers, retained output across a new start, failure and cancellation. `tools/test_hil_tmp1x2_runner.py::test_terminal_failures_and_cancelled_diagnostics` checks verdicts. Adopt the small sequencer and distinct cleanup. Adapt result retention to preserve an undelivered terminal without overwriting it or relying on ordinary output. |
| INA228 `8e439d6c99ece1353cd9f555deed0314794412d9` | `examples/esp_idf/basic/main/main.cpp::startOwnerJob` checks active ownership before starting. `pollOwnerJob` compares operation IDs, consumes a matching result before printing and discards a mismatched terminal. `tools/run_i2c_hil.py::{run_step,read_until_hilrun_end,classify_step}` checks framed output and rejects unexpected faults in negative cases. CI runs `tools/test_run_i2c_hil_parser.py`, which covers expected rejection with a second fault, stale input and post-frame failure. Adopt ownership and fail-closed checks. Reject discarding a mismatched terminal and consuming a matching one before it has queue space. |
| TCA9548A `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `examples/common/Tca9548aCli.cpp::processCommand` handles stop before the active-work guard. `stop` leaves cleanup running and `finish` reports primary status, verified safe-off, logical completion, health delta and session transfers. `examples/espidf_basic/main/main.cpp` drives the same CLI. CI runs PlatformIO `native_cli_commands`, which includes `test/test_cli_commands/test_cli_commands.cpp::test_cancelled_sweep_restores_mask_and_rejects_competing_mutation`. CI also runs `tools/test_tca9548a_hil.py`, including `test_stress_requires_full_count_and_verified_safe_off` and `test_stress_requires_transfer_evidence_even_when_lifetime_counter_saturates` against `tools/tca9548a_hil.py::classify`. Adapt cleanup priority and separate accounting. Reject cancellation as `BUSY` and leave I2C safe-off policy with its own device. |
| MB85RC `3ace168eb01d687ab60e9fe88cc72322e5cae260` | `examples/espidf_basic/main/main.cpp` reports actual scratch restoration and exact run counts. `tools/hil_runner.py::{payload_failure_reason,classify}` requires complete counts and readback/restoration evidence under a finite response deadline. CI runs `tools/test_hil_runner.py`, including `test_diagnostic_prompt_requires_all_counters_and_restore_proof` and `test_generated_idf_confirmed_plans_require_complete_diagnostics`. Adopt explicit completion and cleanup proof. Reject a prompt or human substring as a DP8382x terminal record. |

The official TI RTOS repository still identified `main` as
`94bf4343004732983b0b8e1eec123d88ff70453b`. The pinned `dp83826.c` Git
blob is `e0b64710350d764fe16ffd089fbdc5246e600039`. `Dp83826_reset` and
`Dp83826_restart` select separate PHYRCR commands. `Dp83826_isResetComplete`
incorrectly passes the `0x8000` mask as the register address at line 349.
`Dp83826_isRestartComplete` reads PHYRCR at `0x1F`. Keep the exact core
procedures and reject the mask-as-address poll. [Pinned TI DP83826 source](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c#L318)

The pinned `generic_phy.c` Git blob is
`39414bd3b1004cdf6075bf0f4be30180babdc84d`. `GenericPhy_isLinkUp` only
reads BMSR a second time when the first link bit is high. A successful first
low leaves its output untouched. DP8382x retains the first-low history and
reads the current bit regardless of the first bit. It also avoids inferring
forced speed or duplex solely from PHYSTS. [Pinned TI generic PHY source](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/generic_phy.c#L1217)

TI's current SNLS647I PDF matched the archived 3,366,322 bytes at SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Tables 8-19 and 8-48 distinguish BMCR PCS reset, PHYRCR `0x8000` hard
reset and PHYRCR `0x4000` circuitry restart with register retention.
Table 8-20 specifies BMSR bit 2 as latched and cleared on read. Tables
8-36 and 8-37 specify separately consumed high bytes at MISR1 `0x12` and
MISR2 `0x13`. The collector retains the consumed MISR1 sample when MISR2
fails. The 2 ms hard-reset silence floor remains a software policy. Host
`reset_traces`, `reset_floor`, `reset_failures`, `link_read_plan`,
`link_transitions`, `link_failures` and `events_failures` exercise these
contracts. Physical reset, link and event cases remain open. [SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf), [TI core catalogue](../../reference/library-ti-core.md)

## Implementation and ownership

- The shared CLI dispatch and owner bridges reuse live core link observation
  and the sole event collector. The native bridge admits explicit acquisition
  while stopped and copies historical evidence behind the existing gate. Live
  link/partner acquisition is one coherent 8+2-frame core procedure. Bounded
  watch samples cached evidence, and bounded stress attempts one raw PHYIDR1
  frame per tick. An outstanding result or cleanup obligation blocks run
  admission. The terminal records separate successful rounds, attempted frames,
  and managed traffic from the chip counters available through the collector.
- The initiating request retains one terminal outcome. Manual poll and cancel
  replies have their own request, and the passive result view references the
  original request. Restoration reuses `startInitialize` under a new token and
  budget. The receipt distinguishes requested/effective reset, completed command
  frame, protocol completion, persistent restoration, strap capture and fresh
  link validity. Native end delegates to an application hook. Abandon requires
  independently established disposition. The pinned SDK retirement test keeps
  installed resources reachable when the SDK cannot safely release them.
- No ledger procedure has implementation stage 45. The stage exposes the
  existing procedures and updates the [feature matrix](../../feature-coverage.md),
  [CLI guide](../../cli.md), architecture, native guide and fixture instructions.
  README and changelog enumerate the delivered behavior. Early setup remains
  passive, and the existing discovery and dump routes keep their budgets.

Three subagents worked in parallel using the available current model. The
source reviewer owned precedent evidence and feature-matrix checks. The
owner reviewer implemented the portable and native routes and native tests,
including stopped admission and partial effects. The fixture reviewer implemented
literal reset and event behaviors and byte-level transcript cases. Each build
used its own directory. The lead owned shared types, dispatch and record shape,
integration, documentation, final verification and Git.

## Local verification

Windows with GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1, Python 3.12.10 and
Doxygen 1.13.2. The SDK baseline was ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`. Its host build verified 45 pinned
source/header blobs and exercised the real SDK driver at the configured tick rates.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable Release | PASS | 251 CTests. The registered fixture runs 28 transcript cases including exact reset literals and quiet-floor expiry, failed reset timing, consumed event prefixes, malformed and rejected input, cancellation, separate restoration failure and finite runs. |
| Pinned SDK host | PASS | 365 Release CTests including `native_cli_link_events`, `native_cli_lifecycle` and existing shared silence and clock-fault cases. |
| Focused UBSan trap | PASS | 30 portable CLI/core CTests and 8 native CLI CTests. |
| Separate tests-OFF consumer | PASS | 2 linked CTests. |
| P4 SDK links | PASS | `dp8382x_p4_ethernet.bin` (515,552 bytes) and `dp8382x_p4_owner.bin` (401,392 bytes). Firmware console attachment is stage 47. |
| Source and tools | PASS | Core Doxygen 293 pages and 2 rejected negative fixtures. Strict example Doxygen, 107 Python tests, references, ledger, version and diff checks. |
| Package, remote CI, UART and physical qualification | NOT RUN | Later package, CI and physical stages own this evidence. |

```powershell
cmake -S . -B build/p45-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/p45-release
ctest --test-dir build/p45-release --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p45-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p45-native
ctest --test-dir build/p45-native --output-on-failure --no-tests=error
cmake --build build/audit43-44-consumer
ctest --test-dir build/audit43-44-consumer --output-on-failure --no-tests=error
cmake --build build/p43-ubsan --target dp8382x_cli_fixture dp8382x_cli_tests dp8382x_console_tests dp8382x_reset_tests dp8382x_transport_tests dp8382x_link_tests dp8382x_events_tests
ctest --test-dir build/p43-ubsan -R '^cli_|^reset_|^transport_reports$|^link_|^events_' --output-on-failure --no-tests=error
cmake --build build/p44-native-ubsan --target dp8382x_native_cli_tests
ctest --test-dir build/p44-native-ubsan -R '^native_cli_' --output-on-failure --no-tests=error
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 build
scripts\idf.cmd -C examples/esp_idf/owner -B C:\esp\build\dp8382x-p41 build
python -m unittest discover -s test -p 'test_*.py' -q
python tools/check_doxygen.py
$cliDocs = Join-Path (Get-Location) 'build/p45-cli-doxygen'
(Get-Content -Raw Doxyfile) + "`nINPUT = examples/common examples/esp_idf/common`nOUTPUT_DIRECTORY = `"$cliDocs`"`n" | doxygen -
python tools/verify_references.py
python tools/verify_ledger.py
python scripts/generate_version.py check
git diff --check
```

The UBSan builds use `-fsanitize=undefined -fsanitize-undefined-trap-on-error`.
The handoff records the completion checkpoint and upstream equality.
