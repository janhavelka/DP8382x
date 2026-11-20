# Prompt 44 · Identity, configuration and register CLI

Work started on 2026-11-20 from clean synchronized `main` at
`d1cdb0c71cdeb81183d83438e4b2fda39fb53d89`. Prompt 43 supplies the parser,
console and schema. Prompt 38 supplies stopped native profile reconciliation
and raw arbitration. The existing core supplies both identity-read routes,
complete indirect transactions, configuration and verification. No silicon
procedure has `implementationStage == 44`.

The lead owns shared CLI contracts, parsing, sequencing, transcripts, core
receipt flags, integration and Git. Three parallel agents used the current
model for read-only precedent research and matrix checking, portable core
fixture/owner integration, and native owner failure analysis and tests. The
latter two used separate build directories. Each contribution was checked
against the executing APIs and final diff.

## Source decisions

All four sibling checkouts were clean. Applicable agent rules, the native
front ends and executing registrations were checked in place.

| Checkout and revision | Observed source and decisions |
| --- | --- |
| INA228 `8e439d6c99ece1353cd9f555deed0314794412d9` | `examples/esp_idf/basic/main/main.cpp::processCommand` guards `wreg16` and validates widths for raw reads/writes. `src/INA228.cpp::{readRegister16,writeRegister16,invalidateHardwareState}` retains possible effects in the core. `tools/run_i2c_hil.py::{classify_step,hilrun_end_re,strip_hilrun_frame}` enforces explicit results and correlation. CI runs `tools/test_run_i2c_hil_parser.py`, `tools/test_cli_regressions.py` and the CLI contract checker. Adapt full-width checks and effect receipts through the existing DP8382x owner. |
| TCA9548A `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `examples/common/Tca9548aCli.{h,cpp}::{Session,processCommand,verifyMask,tickCleanup,finish}` uses a bounded cursor and distinguishes absent, failed and unvisited. `examples/espidf_basic/main/main.cpp` shares that processor. `test/test_cli_commands/test_cli_commands.cpp` registers admission, discovery and cleanup regressions in PlatformIO `native_cli_commands`. `tools/tca9548a_hil.py::{classify,run_mask_checks}` and `tools/test_tca9548a_hil.py` require explicit readback and preserve restore failures. Adapt per-address evidence and bounded service. Do not infer MDIO acknowledgement or retarget a binding from a response. |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/common/Tmp1x2Cli.cpp::{processCommand,integer,status}` and `Tmp1x2CliOutput.cpp::{printHelp,printOperationResult}` demonstrate raw command parsing and uncertain write reporting. The native `examples/esp_idf/basic/main/main.cpp` uses the processor. CMake registers `test/cli/test_cli.cpp`, console and `tools/test_hil_tmp1x2_runner.py` cases. Adapt errors and transfer evidence. Replace the synchronous dump precedent with one safe row per tick and reuse the dispatch/help tables. |
| MB85RC `3ace168eb01d687ab60e9fe88cc72322e5cae260` | `examples/espidf_basic/main/main.cpp` scratch paths use `examples/common/DiagnosticCore.h::restoreVerified` and actual `MB85RC::writeVerify`. `tools/hil_runner.py::{payload_failure_reason,classify}` requires explicit restore/readback evidence for native diagnostic plans. The CI-run `tools/test_hil_runner.py` tests missing counters, readback and restore-only failures. Keep OP-9 restoration separately admitted and report effect, failure and remaining debt. |

The [TI core catalogue](../../reference/library-ti-core.md), SNLS647I tables
8-19, 8-21/22, 8-31/32 and 8-42/48, and the live core access policy distinguish
the C22 PHYRCR address 0x1F from 0x8000, the two ID reads and the four-frame
nonincrementing indirect route. The EEE raw tuple tests use literal
3:0x0014 and reject an incorrect 3:0x003C mapping. LEDCR uses only 0x0692 and
raw BMCR only 0x3100. No register, field or exact-part support changed.

## Implementation and acceptance

- `configuration validate` calls the same pure settings/options validator as
  the typed operation. Apply and read-only verify deliver one admitted token
  and a later terminal result. Report AN completion, next-page receipts,
  mutation receipts and restoration debt independently. Native apply also
  calls the public stopped SDK controls before reporting success.
- The existing raw access methods can now report current callback invocation.
  The standalone and native owner bridges only use a fresh receipt. This
  remains unambiguous when transport counters saturate. Keep backend primary
  and controller release errors, and avoid treating malformed receipt data as
  verified ordinary values. Raw and explicit invalidate paths preserve the
  core's trust and cleanup rules.
- Discovery and dump use one bounded cursor, one absolute allowance and one
  complete batch per serviced row. No second binding or direct controller
  route exists. Discovery shows 0000, FFFF, candidate and transport cases
  separately. Dumps skip protected rows through core policy. Prior complete or
  failed rows remain correlated even if a later row fails.
- Exact raw-time quiet floor tests exercise one microsecond before expiry, at
  expiry with the cached flag still set, and after expiry. The pinned SDK
  bridge tests two users of the same controller, fresh versus historical
  discovery and clock regression after an attempted hard reset without a
  valid anchor. Shared quarantine survives operation delivery in that case.
- Console backpressure does not repeat an operation or consume its retained
  result before record capacity exists. Already admitted core polling
  continues. The application must serve the owner and permit local terminal
  delivery. Permanent gate starvation cannot establish a terminal hardware
  outcome.

The same [CLI guide](../../cli.md) defines both the command grammar and schema.
The [feature matrix](../../feature-coverage.md) includes 35 feature IDs and
52 procedure IDs with a check for every field/recipe/printed-address owner.
Stage 45–47 routes remain individually marked. README, changelog,
architecture, native ownership, fixture and test guides were reviewed.
The exported API and example comments cover actual inputs and lifetimes.

## Verification

Host Windows, GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1, Python 3.12.10,
Doxygen 1.13.2. The native SDK baseline is v6.1 at
`fff9895c82d744c7237be8847347bdd1b07c6643`, P4 GCC 15.2.0.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable Release | PASS | 251 CTests including `cli_processor_routes`, `cli_fixture_transcript` and the exact core reset-silence vectors. |
| CLI/reset UBSan trap | PASS | 10 focused CTests. |
| Tests-OFF consumer | PASS | Two separate linked CTests. |
| Pinned SDK host | PASS | 362 Release CTests. Five focused UBSan-trap CTests for admission, SDK reconciliation, discovery, raw access and shared silence. The reconciliation case feeds the compiled CLI through the native bridge into the real pinned driver. |
| SDK images | PASS | Basic `dp8382x_p4_ethernet.bin` (515,488 bytes) and owner `dp8382x_p4_owner.bin` (401,328 bytes) linked on the pinned SDK. Console attachment follows at 47. |
| Source and tools | PASS | 293 core Doxygen pages and two rejected negative fixtures. Separate strict example Doxygen, 106 Python tests, 2058 local links in 257 Markdown files, ledger 97 registers/52 procedures/35 features, version and diff checks. |
| Installed package / remote CI | NOT RUN | Dedicated package stages and first-execution prompt 71. |
| UART/physical PHY | NOT RUN | Simulated MDIO and actual SDK link evidence only. |

```powershell
cmake --build build/p43-host -j4
ctest --test-dir build/p43-host --output-on-failure --no-tests=error
cmake --build build/p43-ubsan --target dp8382x_cli_fixture dp8382x_cli_tests dp8382x_console_tests dp8382x_reset_tests -j4
ctest --test-dir build/p43-ubsan -R '^cli_|^reset_failures$' --output-on-failure --no-tests=error
cmake --build build/p43-consumer -j4
ctest --test-dir build/p43-consumer --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p44-native-agent -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p44-native-agent -j4
ctest --test-dir build/p44-native-agent --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p44-native-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf '-DCMAKE_C_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error' '-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error'
cmake --build build/p44-native-ubsan --target dp8382x_native_cli_tests -j4
ctest --test-dir build/p44-native-ubsan -R '^native_cli_' --output-on-failure --no-tests=error
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 build
scripts\idf.cmd -C examples/esp_idf/owner -B C:\esp\build\dp8382x-p41 build
python -m unittest discover -s test -p 'test_*.py' -q
python tools/check_doxygen.py
$p44Docs = Join-Path (Get-Location) 'build/p44-cli-doxygen'
(Get-Content -Raw Doxyfile) + "`nINPUT = examples/common examples/esp_idf/common`nOUTPUT_DIRECTORY = `"$p44Docs`"`n" | doxygen -
python tools/verify_references.py
python tools/verify_ledger.py
python scripts/generate_version.py check
git diff --check
```

The checked stage uses the existing `main` branch. The final handoff records
the checkpoint and upstream equality.
