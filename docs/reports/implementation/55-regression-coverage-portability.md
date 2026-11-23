# Prompt 55 · Independent coverage, portability and test registration

## Phase (a) assessment

The optimized console framing test prints three failed checks and exits zero.
The P4 example host fixture also consumes 12 SDK headers outside the existing
manifest. Both findings reproduce on clean `main` at
`bb7d188549d41a61d08942e0c382493b65eb7a5d`. The wrapper verified this
prompt 54 completion commit against `origin/main`. The local comparison was
0 ahead and 0 behind. Prompt 55 is **IN PROGRESS**.

The prerequisites are the [transaction ownership tests](53-regression-transaction-ownership.md),
[lifecycle and deadline tests](54-regression-lifecycle-deadlines.md), primary
feature APIs and CLI, and the offline campaigns from prompts 48–52. The
[DP83826I ledger](../../registers/dp83826i.json) assigns zero procedures to
implementation stage 55. This stage audits their existing owners. Two
read-only agents used the session's high-reasoning model to trace field
ownership and test failure propagation independently. The lead reproduced
the framing failure, checked the configured registrations and SDK dependencies,
and built the ordinary consumer.

### Release assertions and shared fixtures

Run `build/p53-baseline/test/cli/dp8382x_native_console_tests.exe framing`.
The failed checks at `test/cli/native_console.cpp` lines 138–140 expect
`%0300d` to be dropped and the drop and truncation counters to advance.
The [CLI contract](../../cli.md#console-service) allows 384 rendered bytes.
Six prefix bytes and CR LF leave 376 body bytes. A 300-byte body fits.
`test/cli/native_console.cpp` and `test/esp_idf/host/uart_console.cpp`
both return zero after running their checks. A separately injected failed
check in each binary also returned zero. This affects four
`cli_native_console_*` registrations and `native_uart_error_latch`.

| Before consolidation, Windows x86-64 GNU 15.1.0 Release | Observed |
| --- | ---: |
| Static CHECK call sites in tracked test sources, excluding the definition | 6,683 (6,671 in .cpp and 12 in headers) |
| Positive CHECK registrations / evaluations in the second run | 239 / 5,018,382, including three failed evaluations |
| Positive registrations with the same counts in both runs | 237 / 5,016,062 |
| Schedule-dependent evaluations in the second run | `frame_load` 1,072 and `cli_native_console_concurrent_logs` 1,248 |
| Intentional WILL_FAIL probes | 27, each returned 1 with the expected diagnostic and one failed check |
| Other CTest registrations outside the CHECK count | 396 |
| Operations / reset | 342 / 267 static sites and 74,993 / 19,192 evaluations across 9 / 6 positive registrations |

The first run had 5,018,383 positive evaluations. The two scheduling cases
contributed 1,071 and 1,250 respectively. Of the five registrations with a
silent exit, framing evaluated 150 checks, input gap 171, retention 14,433,
concurrent logs 1,248 in run two, and UART 162. Python assertions, SDK host
`assert` calls and CLI transcripts have their own verdicts.

To reproduce the static count, enumerate `git ls-files test` with suffixes
`.c .cc .cpp .cxx .h .hpp`. Count each match of `\bCHECK\s*\(` on each
line after excluding `^\s*#\s*define\s+CHECK\b`. In top-level
`test/*.cpp`, `\bstruct Controller\b` finds 25 declarations including two
derived controllers. `\bCHECK\s*\([^\n;]*?\.bind\s*\(` finds 260 expressions
on 258 lines. These counts supersede the historical G12-2 inventory.

For execution counts, use the pinned Release configuration in the
[prompt 54 receipt](54-regression-lifecycle-deadlines.md#phase-d-final-verification).
The local reproduction command is `python build/p55a-harness/measure.py`.
It enumerates the generated CTest invocations, runs each CHECK suite once
with its registered arguments, and reads the check and failure totals.
For the two silent executables it links their original optimized objects
and libraries with GNU `-Wl,--wrap=main`. The wrapper calls
`__real_main`, prints `test::checks` and `test::failures`, and returns
the original exit code. The injected negative run is separate. Local
`build/p55a-harness/first.json` and `baseline.json` in the same directory
retain the per-suite counts for phase (b).

### Ledger and independent oracles

The [single matrix](../../feature-coverage.md) contains all 35 feature and
52 procedure IDs. Its 93 distinct host test names all resolve in
`ctest --test-dir build/p53-baseline --show-only=json-v1` (662 registrations).
The ledger has 97 registers, 283 fields with 30 distinct feature owners,
five printed EEE addresses, 47 write actions, 20 timing entries and 25 gate
records. Including the write-action and printed-address owners gives 32
distinct owner IDs. A reference to a registered test proves the name exists.
The assertion and field-to-command paths still need inspection.

- `test/test_cli_coverage.py` admits a `Pending 47` marker, groups fields by
  owner, checks nonempty physical cells and derives registration from CMake
  text. Test that a wrong field route, duplicate ID, pending cell, missing
  registration or unrelated physical case fails. Trace each field to the
  setting, observed payload, collector, or specific source disposition its row
  promises. The physical column names a case explicitly for 39 of 52
  procedures. The 13 prose-only rows are `identify`, `direct_access`,
  `mmd_access`, `pcs_reset`, `hard_reset`, `restart`, `external_reset`,
  `initialize_10base`, `configure_an`, `configure_forced`,
  `next_page_exchange`, `observe_link` and `extended_link_configure`.
  Map these and the feature rows to actual cases where available, including
  `pcs`, `hard`, `restart`, `cold-boot` and `10base-te`. Retain required
  independent instrument observations and Q10 partner exchange. Use the
  existing [case table](../../hardware-validation.md#case-table).
- The access and power ledger entries still carry partial evidence. Reconcile
  the three stable EEE read routes and the two collector routes against D2
  and the native receipts. Keep Q1 wire confirmation open. Deep-power exit
  remains source-gated. Keep native grants, first-board INT, RX quiescence
  and actual netif closure within their recorded evidence limits.
- The original [SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf)
  has SHA-256 `4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
  Tables 8-18 and 8-90 give W0C for RXFCFG1 bit 11. Compare ordinary
  neutral-one and explicit-zero write traces in `test/wol.cpp` against
  `src/DP8382xWake.cpp`. Tables 8-21/22 and `test/identity.cpp` distinguish
  revision-1 BASIC `0x2000:A111` and ENHANCED `0x2000:A131`. Section 8.3.13
  gives the 10BASE-Te write `0x030E=0x4A40`. Reopen description-only
  clear/latch effects, table 8-39 RECR width ambiguity, and timing qualifiers
  against the independent wire tests. Keep ID filters, DEVAD/offset and
  write masks in their respective namespaces. No silicon correction was
  established by the phase (a) samples.
- Use the [library catalogues](../../reference/library-evidence.md) and
  [independent vectors](../../register-vectors.md) for the remaining
  source comparisons. TI RTOS commit `94bf4343004732983b0b8e1eec123d88ff70453b`
  and its SDK copies share the same reset implementation blob. The inherited
  reset-mask-as-address error supplies one source discrepancy. Preserve
  the source-backed choice and independent literal in the owning tests.

### P4 example SDK dependency evidence

At ESP-IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`,
`test/esp_idf/CMakeLists.txt` verifies the 45 entries in
`host/sdk-files.json` at three tick rates. That verifier omits
`host/example.cpp`, which includes the basic application's `main.cpp`.
The actual example compile selects 37 SDK headers, of which 25 appear
in the existing manifest. The 12 additional headers include
`components/esp_driver_gpio/include/driver/gpio.h` and
`components/esp_eth/include/esp_eth_mac_esp.h`. All 37 working files
matched the pinned Git blobs after CRLF normalization.

To reproduce, select `host/example.cpp` in
`ninja -C build/p53-baseline -t compdb`. Run its compiler with the original
definitions and ordered includes, replacing object/dependency output with
`-MM -MT closure` and the same source. Restrict the dependencies to the
SDK root and compare relative paths with the manifest. The built object's
dependency list is also available via `ninja -C build/p53-baseline -t deps
test/esp_idf/CMakeFiles/dp8382x_example_host_tests.dir/host/example.cpp.obj`.
The owner CLI host translation unit selected the same 37 SDK paths. The
separately configured UART unit selected 42, including UART-specific headers.
State the scope of each manifest in the host guide.

### Phase (b) implementation brief

1. **Test exits and harness.** Route the two silent mains through counted
   verdicts. Reject a named CHECK suite that executes zero checks. Exercise a
   known failed assertion and distinguish its intended diagnostic/exit 1 from
   an unknown argument/exit 2. Preserve the 27 existing WILL_FAIL probes.
   Test formatted log acceptance at 376 bytes, whole-line refusal at 377,
   formatting-buffer overflow and independent counter deltas. Preserve the
   300-byte acceptance, framing and drain assertions.
2. **Shared setup.** Compare `test/operations.cpp` and `test/reset.cpp` first.
   Move common bounded trace, clock, frame/lock/release accounting, malformed
   batch and accepted-write fault setup into shared test support. Build on
   `test/support/FakeController.h` and generalize `test/access.cpp::BoundDriver`
   for ordinary successful binds. Preserve controller/driver lifetimes and
   backpointers. Keep custom and failing binds, rebinds and two-owner cases
   explicit. Operations needs the coarse clock and destructive counter model.
   Reset needs PHYRCR, optional HARD strap resampling and the cross-owner
   `ownerQuietUntil` floor. Keep feature-specific wire values and failure
   expectations local, including the wake W0C, BIST, cable and EEE bank
   models. Account for every consolidated site and compare per-suite
   execution counts, order, negative controls and failure boundaries.
3. **Example pin (F10).** Give the actual example configuration its own
   checked manifest. Share the target's ordered includes and definitions
   (`CONFIG_ETH_USE_ESP32_EMAC=1`, `CONFIG_IDF_TARGET_ESP32P4=1`) with the
   dependency scan. Verify at configuration and build, preserving the existing
   three-rate check. Register a negative test that keeps the pinned identity
   and paths, modifies one example-only header in disposable build inputs,
   and requires a rejection naming that file. Keep the installed SDK read-only.
4. **Coverage and verification.** Close the existing matrix in place. Link it
   from `test/README.md`, correct evidence and guide claims in the owning
   files and update Doxygen with any changed declarations. Inspect explicit
   portable, SDK, application and test source lists and the five standalone
   core header TUs. Run full registered Debug and optimized suites with
   strict warnings, both C and C++ UBSan trap builds, Python discovery plus
   compiled CLI/runner registrations, ordinary tests-OFF consumer, archive
   symbols, version, Doxygen, ledger, references and whitespace. The compiler
   accepted `-fsanitize=undefined -fsanitize-trap=undefined` in a link/run
   probe. Relink real P4 images if public or adapter inputs change.

For phase (b), one agent can own portable test fixtures and CHECK exit
paths, and a second can own the example manifest, verifier and drift test.
The lead owns shared build registration, public contracts, the matrix and
its checker, source dispositions, guides, integration and the commit request.
Coordinate edits to the UART test main with the harness owner.
Phase (c) requires two parallel read-only reviews of the implementation.

### Peer conventions and checks

Both peers were clean. TMP1x2 at `0791429f8fce1170e2082f1b011456fc461318b8`
registers `test/core_no_arduino/compile_main.cpp` in its root CMake. Its
public header, `src/TMP1x2Operations.cpp`, `examples/common/Tmp1x2CliDiagnostics.cpp`,
`tools/check_contracts.py`, `tools/check_doxygen.py` and
`tools/hil_tmp1x2_runner.py` establish the linked consumer, allocation/source
guards and operation result precedent. 24Cxx at
`139e76cd38ecdeec40be5fdb1ce5e633158ea59a` links its host application
from `test/cli_host/main.cpp` through `examples/common/Eeprom24CxxCli.cpp`.
Its CMake passes the built executable to `tools/test_cli_transcript.py`
and registers Python runner discovery. Its public config and
`src/EEPROM24Cxx.cpp` keep hardware transport outside the consumer.
Both AGENTS files and local workflow definitions were inspected. Adopt
explicit registrations, bounded real-producer tests and build-tree consumer
checks. PHY faults, timing vectors and supported MCU targets follow DP8382x
evidence. DP8382x has no tracked CI workflow at entry.

| Phase (a) evidence | Result |
| --- | --- |
| Host | **FAIL** for the three framing assertions and both silent exit paths. The two focused Python discovery suites `test_cli_coverage.py` and `test_access_policy.py` passed 4/4 and 2/2. The 662 count describes configured registrations. Full Debug, Release and sanitized acceptance runs remain due. |
| Portable build-tree consumer | **PASS** 2/2. Windows x86-64, GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1. The archive had all 14 explicit portable objects. All undefined symbol references from its members were in the DP8382x namespace. |
| Python and source documentation | **PASS**. Python 3.12.10 standalone discovery ran 238 tests with 49 compiled-fixture skips. Doxygen checked 293 generated pages and two negative fixtures. Reference integrity/local links, ledger structure, generated version and whitespace checks passed. |
| SDK link | **NOT RUN**. The SDK evidence above checks host compiler inputs. The prompt 54 link receipt identifies the previous three P4 images. |
| Exported package consumer | **NOT RUN**. Export and relocation belong to prompt 70. |
| Remote CI | **NOT RUN**. Prompt 71 owns remote execution, including the Linux compiler and sanitizer matrix. |
| Physical | **NOT RUN**. Board, partner and instrument evidence requires the identified authorized fixtures in prompts 72–73. |

The following commands reproduce the consumer build and symbol inspection.

```powershell
cmake -S test/consumer -B build/p55a-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF '-DCMAKE_CXX_FLAGS=-Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti'
cmake --build build/p55a-consumer --parallel 4
ctest --test-dir build/p55a-consumer --output-on-failure --no-tests=error
ar t build/p55a-consumer/dp8382x/libDP8382x.a
nm -g -C --undefined-only build/p55a-consumer/dp8382x/libDP8382x.a
```

## Phase (b) implementation

The run began on clean `main` at `826f9042dbcd4f6821a4f77a4fe99d8c13d5bc74`.
The wrapper recorded this phase (a) commit equal to `origin/main`. The lead
integrated two disjoint implementation assignments using the session's
high-reasoning model. One handled controller and verdict semantics. The other
traced the SDK inputs selected by the compiler. The lead owned the ledger,
matrix, registrations, source comparisons, guides and final checks.
The TMP1x2 and 24Cxx revisions and listed conventions in phase (a) still apply.
Neither peer had local changes.

### Executing test verdicts and counts

The console and UART mains now use the same counted verdict as the portable
suites. A failed assertion returns 1, an unknown argument returns 2, and a
recognized suite that executes no checks returns 1 with an empty-suite
diagnostic. The two new verdict registrations execute the affected binaries.
All 27 original WILL_FAIL registrations still run. Framing accepts formatted
300- and 376-byte bodies and checks the entire drained record. The 377-byte
rendered-line overflow and the 379-byte formatting-buffer overflow each drop
one whole log, increment both counters and emit zero bytes.

The [bound driver](../../../test/support/BoundDriver.h) owns its controller
before its nonmovable driver. It replaces 118 ordinary successful-bind sites
with one check at the same point in each fixture's lifetime. Custom binds,
failing binds, address changes and competitors remain explicit. The
[initialization controller](../../../test/support/InitializationController.h)
shares bounded traces, batch budgets, clock/lock/release accounting, accepted
writes with failed receipts and malformed completion injection. Operations
retains its coarse-clock and destructive counter wire model. Reset retains
the PHYRCR model, optional HARD strap resampling and shared-bus silence.
Wake, BIST, cable and EEE keep their own wire models and literal expectations.

| Windows GNU Release | Phase (a), second run | Phase (b) |
| --- | ---: | ---: |
| Static CHECK sites (.cpp + headers) | 6,671 + 12 = 6,683 | 6,516 + 35 = 6,551 |
| Positive registrations / evaluations | 239 / 5,018,382 | 239 / 5,018,403 |
| Positive evaluations with schedule-dependent suites excluded | 5,016,062 | 5,016,087 |
| Failed evaluations in positive registrations | 3 | 0 |
| Intentional probes / failed evaluations | 27 / 27 | 27 / 27 |
| Operations, nine registrations | 74,993 | 74,993 |
| Reset, six registrations | 19,192 | 19,192 |

The 132-site reduction is 117 duplicate bind checks plus 22 duplicate
controller/trace sites, offset by seven new framing sites. Compared by CTest
name, framing increased from 150 to 175 evaluations. Concurrent logs changed
from 1,248 to 1,244. Frame load executed 1,072 checks in both of these runs.
The other 263 counted registrations matched individually. Each intentional
probe produced its expected diagnostic. The four additional CTests check the
two verdicts, the example header drift and the configured matrix.

For the static count, use the phase (a) pattern with
`git ls-files -co --exclude-standard test` to include new headers before
commit. To reproduce the execution count, use the combined Release build
below and run `ctest --test-dir build/p53-baseline -V -E 'cli_hil_' -j 1`.
The excluded Python campaigns have no CHECK reports and run separately.
Join `Start <number>: <test name>` to
`<number>: <count> checks; <count> failures` by CTest number. Require one
summary for each of the 266 counted registrations. Retain the two variable
suites separately. The per-registration comparisons are in local
`build/p55a-harness/baseline.json` and `build/p55b-check-counts.json`.
The full verbose run is `build/p55-phase-b-count.log`.

### Fields, source decisions and SDK inputs

The [feature matrix](../../feature-coverage.md) now assigns all 283 field IDs,
47 write actions and five printed EEE tuples to an owner and command.
Read-only, internal-selector, collector and source-gated branches have separate
dispositions. The 35 feature and 52 procedure rows reference 95 distinct
executing CTests. The [matrix test](../../../test/test_cli_coverage.py)
resolves selectors, checks dispatch, and reads CTest's generated JSON in the
combined build. Negative cases cover a wrong mixed-register command, duplicate
row, placeholder, absent registration and a real but unrelated reset case.
Campaign IDs resolve against the runner's case catalogue. The direct C22 case
reads CR1 during cable autorun. The indirect 10BASE-Te case reads the programmed
VOD word. Separate instrumentation is still needed for alias frames, external
RESET_N, mirror/crossover, and the Q10 next-page partner exchange.

The exact-part source is the archived SNLS647I at the hash recorded in phase
(a). The [independent vectors](../../register-vectors.md) and
[catalogues](../../reference/library-evidence.md) retain the full numeric
evidence. The following comparisons govern the changed rows and descriptions.

| Source and independent execution | Disposition |
| --- | --- |
| SNLS647I Tables 8-21/22, `identity_policy`, `identity_decode` | BASIC 2000:A111 and ENHANCED 2000:A131 admit revision 1 for the declared DP83826I. The ID pair cannot establish the ordering suffix. |
| Table 8-48 and §8.3.13, `reset_traces`, `operation_initialization` | PHYRCR address 0x1F is distinct from HARD mask 0x8000. The copied TI reset code has one inherited address defect. Mandatory initialization writes 0x4A40 at vendor offset 0x030E. The pin minimum and software-reset readiness floors retain their own timing origins. |
| Tables 8-18/90, `wol_vectors`, `wol_acquisition`, `wol_failures` | RXFCFG1 bit 11 is W0C. The neutral write for stable 0x1181 is 0x1981, and acknowledgment emits 0x1181 after preharvest. The Bit Mask Flag at bit 6 is R/W with unresolved meaning and is preserved. Corrected the CLI guide's W1S claim. |
| Tables 8-25/34/38/39/41/44/45, `events_vectors`, `interface_collector`, `quality_collector`, `bist_measurements` | Retain ANER page and 10BTSCR polarity effects described outside the access column. RECR remains a raw word under the width/saturation conflict. The BICSR1 capture/clear write is distinct from reading its high byte. The acquisition guide now lists all 15 sources in collector order. |
| D2, SNLS647I §§8.3.3.2 and 8.3.11.1, `eee_status`, `eee_acquisition_failures`, `mmd_ownership`, `native_cli_cross_entry_boundaries` | Stable reads use 3/0014, 7/003C and 7/003D. Only the collector reads 3/0001 and 3/0016. Raw writes and printed high-prefix aliases are excluded. The access ledger now credits the implemented and host-tested policy and the historical native image link. Q1 wire confirmation remains open. |

Power retains its partial software/host evidence because deep entry/exit is
source-gated. Native feature grants, the first-board INT route, reusable SDK
RX teardown and actual netif session closure retain their existing limits.
No silicon mask, timing bound, public declaration or firmware source changed.

The basic P4 example's [manifest](../../../test/esp_idf/host/example-sdk-files.json)
checks 37 real headers at configure and build with its own compiler flags,
ordered includes and P4/EMAC definitions. The compiled object's Ninja dependency
list matched all 37. Of these, 25 overlap the three-rate driver manifest and 12
are additional. The negative test verifies a clean disposable copy first, then
requires a failure naming the modified GPIO header with the same SDK Git pin.
The [host guide](../../../test/esp_idf/host/README.md) identifies the separate
owner CLI and UART configurations. The original 45-file check still runs at
100, 333 and 1,000 Hz.

### Local verification

| Layer | Result |
| --- | --- |
| Host | **PASS** on Windows x86-64 with GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1 and Python 3.12.10. Debug, Release and Release with C/C++ UBSan trap each passed all 666 CTests. This includes the five standalone core header translation units in the build, both native console verdicts, the SDK drift case, all compiled CLI and offline runner campaigns. |
| Portable consumer | **PASS**, both tests-OFF registrations with exceptions and RTTI disabled and strict warnings. The archive contained precisely the 14 portable implementation objects. All 158 undefined-symbol entries from its members were DP8382x references. |
| Python and source checks | **PASS**. Standalone discovery ran 242 tests, including 50 skips needing a compiled fixture or generated registration inventory. The combined runs exercised those paths. Doxygen checked 293 pages and two negative fixtures. The ledger verified 97 registers, 52 procedures and 35 features. References verified 29 vendor payloads, 27 extracts and 2,271 local links. Generated version and whitespace checks passed. |
| SDK image link | **NOT RUN** this phase. This change affects the host test build. The clean three-image link receipt remains in prompt 54. |
| Exported package consumer | **NOT RUN**. Build-tree consumer evidence is above. |
| Remote CI and physical | **NOT RUN**. Prompt 71 owns the Linux/second-compiler and additional sanitizer checks. The hardware cases need their identified fixtures and authorization. |

The portable library, SDK adapter, examples and tests use explicit CMake source
lists. The consumer compiled without a framework, SDK path, Git, Python or
Doxygen build step. Use the consumer commands above with
`build/p55b-consumer`. For the three combined builds, run from the project
root with the process-local Git settings below.

```powershell
$env:GIT_CONFIG_COUNT = '1'
$env:GIT_CONFIG_KEY_0 = 'safe.directory'
$env:GIT_CONFIG_VALUE_0 = 'C:/esp/v6.1/esp-idf'
$options = '-G','Ninja','-DDP8382X_BUILD_TESTS=ON','-DDP8382X_BUILD_PINNED_IDF_TESTS=ON','-DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf'
$san = '-fsanitize=undefined -fsanitize-trap=undefined'
cmake -S . -B build/p53-baseline @options -DCMAKE_BUILD_TYPE=Release
cmake -S . -B build/p55-debug @options -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build/p55-ubsan @options -DCMAKE_BUILD_TYPE=Release "-DCMAKE_C_FLAGS=$san" "-DCMAKE_CXX_FLAGS=$san"
foreach ($dir in 'build/p53-baseline','build/p55-debug','build/p55-ubsan') {
  cmake --build $dir --parallel 3
  ctest --test-dir $dir -E 'cli_matrix_registrations|cli_hil_' -j 3 --output-on-failure --no-tests=error
  ctest --test-dir $dir -R 'cli_matrix_registrations|cli_hil_' -j 1 --output-on-failure --no-tests=error
}
```

The two CTest selections passed 659 and seven registrations respectively.
After final matrix and test-comment edits the 660 non-campaign registrations
were checked again in each build. Both agents finished. The checked phase (b)
commit is requested from the wrapper. Prompt 55 remains **IN PROGRESS** for
the two independent reviews at phase (c).
