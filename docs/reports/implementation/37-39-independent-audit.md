# Independent audit of prompts 37-39

Audit date: 2026-11-17. Entry was clean `main`, synchronized after fetch at
`ec792d2ea5bee28d20092999c1ec6facad57e088`. Reviewed the complete original
[37](../../prompts/37-esp-idf-lifecycle.md),
[38](../../prompts/38-esp-idf-state-arbitration.md) and
[39](../../prompts/39-esp-idf-link-bounds.md), the actual source/tests and their
diff from pre-37 `cb5ffe5b5d21e700c3e9fbef434e192a50b1aeff`. Prior reports were
evidence pointers, not substitutes for current execution or inspection.

## Rules, ownership and sources

Reopened parent/root AGENTS, COMMON, CONVENTIONS, ENGINEERING, VERIFICATION,
README, architecture, variants, reference index, SDK evidence, ownership and
operation contracts. Preserve one chip implementation, application ownership,
whole MDIO batches, separate logical admission and physical locks, accepted
effects, retained cleanup and core isolation from SDK headers/allocation/time.
No new chip procedure belongs to these adapter stages. No sibling, SDK or vendor
payload was edited. Remote CI remains first-execution prompt71 only.

The lead owns production code, shared headers/contracts, documentation, integration
and Git. Parallel gpt-6.1-sol/high reviews covered requirements/source coverage
and tests. After review, their disjoint writable paths were `test/identity.cpp`
and the native CMake/host test files respectively. gpt-6-astra/high independently
reviewed ownership, lifetime, clock and failure cases read-only, then rebuilt the
native tests and real SDK image. These models were selected for source/test work
and stronger independent state/deadline reasoning. Separate build directories
avoided build mutation between agents. The lead checked every confirmed finding
against production source and reviewed each contribution. Final independent
review found no remaining confirmed in-scope defect.

Fresh source inspection retained the exact unmodified SDK at
`C:/esp/v6.1/esp-idf`, v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`.
It reopened `components/esp_eth/include/esp_eth_phy.h`, `esp_eth_driver.h`,
`esp_eth_phy_802_3.h`, `src/phy/esp_eth_phy_802_3.c`, `src/esp_eth.c`,
`src/mac/esp_eth_mac_esp.c`, `components/esp_timer/src/esp_timer.c` and the
P4 HAL/timer paths recorded in the [source audit](39-native-source-audit.md).
Actual builds verify the same 45 SDK source/header blobs for both tick rates.
The generic base does not execute. Its scan/reset loops and default pin/address
policies remain rejected. No supported SDK range is inferred from this pin.

Fresh peer reads found the same clean revisions and source/test boundaries:

| Peer relative to Projects | Revision | Native production path |
| --- | --- | --- |
| INA228 | `8e439d6c99ece1353cd9f555deed0314794412d9` | `examples/esp_idf/basic/main/Ina228IdfI2cTransport.h`, `.cpp`, `main.cpp` |
| PCA9555 | `0aec2212894910e0d872c3e511fbcd740837c5f7` | `examples/espidf_basic/main/main.cpp` |
| TCA9548A | `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `examples/espidf_basic/main/main.cpp` |
| TMP1x2 | `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/esp_idf/basic/main/main.cpp` |

The current AGENTS, Status/Config/facade/implementation, example/CLI, test and
Python runner paths listed in the [source inventory](39-native-source-audit.md)
were checked again. No relevant source or workflow change was found. PCA9555's
`test/test_basic.cpp` distinguishes transport timeout from owner deadline;
TCA9548A's `test/test_driver/test_driver.cpp` preserves backend errors on late
return. Both execute core fake-bus harnesses, not native I2C hardware. INA228's
native mapping/removal handles and TMP1x2's `mapError`, `finish` and console
startup were compared with their actual tests. Retain precise errors and effects;
reject I2C transaction granularity/NACK semantics and ignored cleanup returns as
MDIO precedents. No sibling tests or fixtures were executed. No repository CI
workflow exists yet; creation and remote execution remain prompt71.

## Confirmed defects and corrections

1. A second PHY's board preparation could overwrite the shared silence Boolean
   while the first PHY owned initialization. Either owner could then reopen the
   bus too early. One atomic `BusAdmission` enum now claims BOARD_PREPARING or
   INITIALIZING before draining the controller. Reservations cannot replace one
   another; only the initializing owner issues its batches. No controller lock
   spans a GPIO callback, hardware wait or owner poll. Failed board readiness
   remains FAILED_CLOSED even after deleting a never-installed adapter, until
   explicit external disposition under quiescence or system restart.
2. Explicit discovery bypassed the common transport audit, counters and clock
   fault handling. The real portable `readIdentityAt` direct method now executes
   the existing two-ID batch through that boundary. It never retargets or accepts
   identity, updates another PHY's semantic collector, or counts managed health.
   Both ordinary ID outputs publish together on success. A callback receipt flag
   distinguishes new partial evidence from an earlier batch after local denial,
   including after saturated counters. Native discovery still retains successful
   partial words and has a whole-scan allowance; it never selects an address.
3. Positive controller lock waits that expired were classified as BUSY. They now
   return MDIO_TIMEOUT with ESP_ERR_TIMEOUT detail and count as managed transport
   failures when issued by managed traffic. Zero-tick contention remains BUSY.
   Neither case attempts MDIO or quarantines a sound controller.
4. SDK reconciliation and MAC/PHY flow-policy calls could exceed their declared
   individual callback bound while returning success inside the outer deadline.
   Each call now samples completion and checks that bound. Accepted SDK/MAC/PHY
   effects survive a late return, later work is stopped when allowance fails,
   and explicit retry is required for reconciliation. An actual backend error
   retains precedence over local timeout. No synchronous callback is preempted.
5. Common discovery auditing exposed native OPERATION_TIMEOUT results with the
   invalid detail zero. All affected budget checks now report DEADLINE,
   FRAME_BUDGET or BOUND_ADMISSION. Local empty-mask rejection also has its
   defined parameter detail. A valid timeout no longer looks like a malformed
   backend report or unnecessarily stops the binding.
6. The host timer model treated a dispatched one-shot as inactive before its
   callback returned. Its public active query now includes the running callback,
   matching the actual pinned timer implementation. Stop-on-unarmed behavior
   remains INVALID_STATE. This corrects the test seam, not SDK production code.

Simplicity review kept one shared admission enum, the existing core transport
boundary and a small common identity-batch builder. No registry, alternate chip
implementation, private SDK cache access, duplicate engine or success stub was
added. The new direct probe has a current native consumer and executable tests.

Rejected findings were also checked: forced10 with extendedFullDuplex is already
illegal in the portable source-backed validation; the native forced100/half
restriction remains necessary for resolved publication. Hypothetical SDK timer
stop errors outside the pinned implementation are not evidence of a production
failure. Pre-install application calls still obey the documented single-owner
serialization contract; the new shared reservation covers distinct PHY owners.

## Requirements and regression closure

| Scope | Actual paths and evidence |
| --- | --- |
| 37 factory, board readiness, installation and lifetime | `Phy.cpp`, pinned driver tests, raw/glue RX pause tests and real P4 fixture retain the declared address, local install callbacks, explicit 2ms/50ms readiness and terminal retained resources. New board reservation regressions cover competing owners, failed readiness and never-installed deletion. |
| 37 explicit discovery | `DP8382x.cpp::readIdentityAt`, `Phy.cpp::discover`, `identity_scan`, `native_discovery*`: address/mask/frame admission, two-word output preservation, partial receipt, transport accounting, late return, malformed report and clock faults. The scan never chooses an address. |
| 38 SDK state and raw arbitration | Existing AN/forced lifecycle, getter/cache, raw-BMCR, both cached-AN direct-start, four/five-frame boundary, release, collector and flow interference tests remain active. New callback-bound tests cover accepted then late controls and MAC policy, backend-error precedence and explicit retry. |
| 38 controller waits | `native_controller_wait_classification` at1000Hz and100Hz distinguishes timed expiry, zero-tick contention and managed health without illegal frames or quarantine. |
| 39 notifications and timing | All existing UP/DOWN and live-tuple, unresolved negotiation, full pause versus Boolean projection, partial publication, event loss, stop/start/fence and 100Hz readiness cases remain active. The method-by-method native conformance table is maintained in the adapter guide. |

The new portable scan suite makes285 checks, including address boundaries0/31,
invalid inputs before callbacks, output sentinels, active/result/cleanup admission,
partial and malformed reports, late return and clock latching. Existing identity
and transport suites retain release-failure and backend-error precedence coverage.
Eight new native CTests raise the native total from55 to63; existing cases also
gain malformed-timeout and correct timer-active assertions. Assertions remain
active in Release. Deterministic scheduling models the selected boundaries; it
does not establish physical concurrency or unconditional wall-clock bounds.

Reviewed and updated README/CHANGELOG, Doxygen declarations, API/identity and
ownership/architecture guides, the native guide, example notes, test guides,
feature coverage and progress together. Chip ledger facts, variants, component
metadata/source selection, version and SDK dependency manifest need no change:
no new silicon feature, support pin or packaging behavior is introduced.

## Final local verification

Host: Windows/PowerShell, GNU15.1.0, CMake4.0.1/Ninja, Python3.12.10. Actual P4 SDK:
GCC15.2.0 (esp-15.2.0_20251204), SDK-environment CMake4.0.3, Ninja1.12.1,
esptool5.6.0, accessed only through `scripts/idf.cmd`.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable host | PASS |236/236 Release CTests in build/host |
| Portable consumer | PASS |2/2 separately configured tests-OFF consumer CTests |
| Native host | PASS |63/63 Release; independent reviewer63/63;63/63 UBSan trap, complete45-file SDK closure at both tick rates |
| Actual P4 SDK link | PASS | Real fixture image0x5c3b0 bytes; no SDK replacement or flash |
| Python/research/version | PASS |103 tests;97 registers/52 procedures/35 features; generated version current |
| Source documentation/reference/diff | PASS | Doxygen289 pages and two rejected negative fixtures;29 vendor artifacts/27 extracts/1889 local links in239 Markdown files; diff check |
| Installed/exported package | NOT RUN | Build-tree consumer exercised; packaging stages68-71 |
| Remote CI | NOT RUN | First-execution prompt71 rule |
| Physical timing/link | NOT RUN | No flash or fixture operation; native_wall_time qualification remains open |
| Reusable installed teardown | BLOCKED | native_rx_quiescence; all installed resources remain retained until restart |

Commands used (from repository root; configure each before building):

```sh
cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/host -j2
ctest --test-dir build/host --output-on-failure --no-tests=error
cmake -S test/consumer -B build/audit3739-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/audit3739-consumer -j2
ctest --test-dir build/audit3739-consumer --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/audit3739-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/audit3739-native -j2
ctest --test-dir build/audit3739-native --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/audit3739-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf "-DCMAKE_C_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error" "-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error"
cmake --build build/audit3739-ubsan -j2
ctest --test-dir build/audit3739-ubsan --output-on-failure --no-tests=error
scripts\idf.cmd -C test/esp_idf/adapter_link -B C:\esp\build\dp8382x-p39 build
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_references.py
python tools/verify_ledger.py
python scripts/generate_version.py check
python tools/check_doxygen.py
git diff --check
```

The inspected SDK image before commit identifies entry SHA ec792d2 as dirty;
its SHA-256 is `fc94181f93330d0d2b0f6eacad19c0c1fa46ff7c906476c1efe87980ec86e4e6`.
Final commit/sync and the clean post-commit firmware identity belong to the
handoff, avoiding a self-referencing commit. Hardware gates remain unchanged.
