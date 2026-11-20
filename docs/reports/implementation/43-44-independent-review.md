# Independent review of prompts 43 and 44

The review started on 2026-11-20 with clean synchronized `main` at
`0f493d4220b0845fc280b431e240a1f415455b36`. It reread the complete
[CLI foundation](../../prompts/43-cli-foundation.md) and
[configuration and register](../../prompts/44-cli-configuration-registers.md)
prompts and checked the shipped diff from `468e269` through `0f493d4`.
The [CLI guide](../../cli.md) owns the grammar. The
[coverage matrix](../../feature-coverage.md) owns feature and test mapping.

Three read-only agents ran in parallel with the current model. One traced
parser, framing and backpressure. One reviewed core and pinned SDK ownership,
clock and failure behavior and built both host routes in separate directories.
One checked fixture options, executing transcripts, matrix coverage and source
documentation. The lead verified the findings against source, reproduced the
relevant cases, made the corrections and owned Git. The clean sibling revisions
from the [43](43-cli-foundation.md) and [44](44-cli-configuration-registers.md)
reports still match the local checkouts. The review retained their selected
strict framing, bounded service and separate restoration conventions.

## Findings and fixes

| Finding | Correction and regression |
| --- | --- |
| A range could detect clock regression and publish the regressed time as valid. A session could also see a later raw time than the core, leaving an admitted apply free to mutate after the CLI observed regression. | The processor uses one monotonic time check across records and live work. At a clock fault it publishes invalid timestamps, calls existing owner cancellation for a token that still needs polling, and retains the result and cleanup debt. `cli_processor_routes` and `native_cli_clock_fault` verify correlation, zero additional owner frames and no SDK reconciliation. |
| Range accounting inferred the number of attempted frames for a successful callback without a batch receipt. | Require fresh batch evidence for a successful scan or dump row. Stop on missing or malformed evidence, retain previously validated rows and mark the count as a known prefix. The processor suite checks this and exact exclusive deadline expiry. |
| The raw release code could be outside `Err` and leak into a write record. | Carry the core's audited release result through the portable and native receipts while retaining the original report, signed detail and violation. `transport_reports`, `cli_processor_routes` and `native_cli_raw_access` check those paths. |
| An out-of-contract null callback status message was accepted. Full help grouped the profile syntax under registers and long command names broke alignment. | Reject malformed terminal status, keep separate diagnostic detail, and render profile help with its group and legal combinations. The cached and grammar suites exercise both corrections. |
| The compiled fixture covered C22 dumps but had no positive MMD dump or live packed next-page apply transcript. | Added those cases, decoded-page assertions, and zero-frame mutation rejection after overflow or NUL. The real core reports next-page timeout and restoration debt with the fixture's simulated partner. The matrix now cites the exercised native raw suite. |

The review also reran the existing alias/line-loss, queue backpressure, session
and reboot, exact quiet-floor, shared quarantine, ownership and stopped SDK
AN/forced tests. The fixes reuse the existing parser and chip procedures.
Exact-part scope remains DP83826I. README's
support summary still applies. Changelog, CLI, direct-access, SDK and test guides
record the corrected behavior.

## Final local verification

Windows host with GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1, Python 3.12.10 and
Doxygen 1.13.2. Pinned ESP-IDF v6.1 is
`fff9895c82d744c7237be8847347bdd1b07c6643`. The pinned host build verified 45
SDK sources and headers. The P4 toolchain uses GCC 15.2.0.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable host | PASS | 251 Release CTests with tests required. The real CLI fixture ran 14 transcript cases. |
| Pinned SDK host | PASS | 363 Release CTests including the live native bridge and `native_cli_clock_fault`. |
| Focused UBSan trap | PASS | 11 portable CLI, transport and reset cases and 6 native CLI cases. |
| Tests-OFF consumer | PASS | 2 linked consumer CTests. |
| P4 firmware links | PASS | `dp8382x_p4_ethernet.bin` (515,552 bytes) and `dp8382x_p4_owner.bin` (401,392 bytes). Host tests exercise the CLI bridge. Stage 47 owns firmware console attachment. |
| Source and tools | PASS | 293 Doxygen pages with 2 rejected negative fixtures, strict CLI/bridge pages, 106 Python tests, 2,066 local links in 258 Markdown files, ledger, version and diff checks. |
| Installed package, remote CI, UART and physical PHY | NOT RUN | Later stages own those gates. |

```powershell
cmake -S . -B build/audit43-44-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/audit43-44-release
ctest --test-dir build/audit43-44-release --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/audit43-44-sdk -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/audit43-44-sdk
ctest --test-dir build/audit43-44-sdk --output-on-failure --no-tests=error
cmake -S test/consumer -B build/audit43-44-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/audit43-44-consumer
ctest --test-dir build/audit43-44-consumer --output-on-failure --no-tests=error
cmake --build build/p43-ubsan --target dp8382x_cli_fixture dp8382x_cli_tests dp8382x_console_tests dp8382x_reset_tests dp8382x_transport_tests
ctest --test-dir build/p43-ubsan -R '^cli_|^reset_failures$|^transport_reports$' --output-on-failure --no-tests=error
cmake --build build/p44-native-ubsan --target dp8382x_native_cli_tests
ctest --test-dir build/p44-native-ubsan -R '^native_cli_' --output-on-failure --no-tests=error
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 build
scripts\idf.cmd -C examples/esp_idf/owner -B C:\esp\build\dp8382x-p41 build
python -m unittest discover -s test -p 'test_*.py' -q
python tools/check_doxygen.py
$reviewDocs = Join-Path (Get-Location) 'build/audit43-44-doxygen'
(Get-Content -Raw Doxyfile) + "`nINPUT = examples/common examples/esp_idf/common`nOUTPUT_DIRECTORY = `"$reviewDocs`"`n" | doxygen -
python tools/verify_references.py
python tools/verify_ledger.py
python scripts/generate_version.py check
git diff --check
```

The UBSan builds use `-fsanitize=undefined -fsanitize-undefined-trap-on-error`.
Checkpoint identity and upstream equality are recorded in the handoff.
