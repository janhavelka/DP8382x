# Prompt 40: ESP32-P4 ordinary Ethernet application

Entry: 2026-11-19, clean `main` equal to `origin/main` at
`022cccb7ffcffc211a81398764eb64380346dccb` after fetch. Prompt 39's
native link/notification implementation, actual P4 link fixture and
`native_rx_quiescence` lifetime restriction are the concrete prerequisites.
No ledger procedure has `implementationStage == 40`; this is application
integration, not a new chip register procedure. The shared core and native
adapter still implement only their previously supported feature routes.

## Source and architecture decisions

Read COMMON, CONVENTIONS, ENGINEERING, VERIFICATION, EVIDENCE, the prompt,
design decisions 2-7 and the operation contract, plus root/parent and relevant
TMP1x2/INA228 agent rules,
README, architecture, variants and reference index. The lead owns the new
application, shared adapter health accessor, guides, progress, integration and
Git. Agents used the current capable model for bounded peer and board reviews
and stronger lifecycle/failure analysis; this follows the available-model rule
without a model override. The board and peer reviewers were read-only; the
lifecycle agent owned only host test files and used its own build directory.
All findings were checked against source and the final diff.

| Current source, clean revision | Observed behavior | Decision |
| --- | --- | --- |
| `TMP1x2/examples/esp_idf/basic/main/main.cpp`, `main/CMakeLists.txt`, `main/BuildIdentity.cmake` at `afd50889312f6e218a6a34eca687e3e8547d8b40` | Native C++17 component, explicit resource acquisition, bounded CLI queue, build identity; I2C pins and a potentially blocking console loop | Adapt C++17/native ownership and build identity; reject sensor pins and console wait for P4 |
| `INA228/examples/esp_idf/basic/main/main.cpp` (`initExample`, `app_main`), `Ina228IdfI2cTransport.cpp` and component/identity CMake at `8e439d6c99ece1353cd9f555deed0314794412d9` | Explicit native init and cleanup evidence; continued CLI after init failure | Adapt error retention; avoid continuing failed Ethernet startup |
| `test/esp_idf/adapter_link/main/main.cpp`, `include/DP8382x/esp_idf/Phy.h`, `src/esp_idf/Phy.cpp` at entry `022cccb7` | Real P4 MAC and owner link, conditional board readiness, typed 100/full AN, start/stop and retained installed resources; no netif | Reuse one owner; add application netif/glue, DHCP and callback context; add passive health accessor |
| Pinned ESP-IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`: `components/esp_eth/src/esp_eth_netif_glue.c`, `components/esp_netif/lwip/esp_netif_lwip.c`, `components/esp_eth/src/esp_eth.c` | Glue registers five handlers. Attach publishes the netif RX pointer before later setup can fail. `esp_netif_deinit` is unsupported after lwIP init; unregister can wait on default-loop mutex | Retain netif/glue/driver and static callbacks after install; delete only owned preinstall objects; callbacks only copy atomics |
| [Board source audit](40-board-source-audit.md) | Latest saved netlist confirms P4 pins, but PHY `3V3` has no demonstrated supply source | Explicit mapping and startup check; default-closed runtime gate; physical networking NOT RUN |

There is no repository workflow yet; prompt 71 owns its creation and first-set
remote CI run. Sibling repositories, EasyEDA and SDK trees were not edited or
built. TMP1x2 registers CLI/console/HIL tests in its root CMake and builds a
native IDF image in `.github/workflows/ci.yml`; INA228's workflow builds its
S2/S3 native image. Neither has a registered test of its native `app_main`
lifecycle, and their I2C test matrices are not evidence of P4 Ethernet.

## Implemented ownership

`examples/esp_idf/basic/` contains a native project, component, dependency
metadata through the root component, default configuration, application-owned
BoardConfig and Git identity generation. The root component's pinned
`idf_component.yml` already specifies IDF 6.1/esp32p4. Every P4 SMI/RMII field
is set and checked, including unique signal/reset pins. Expected address 0,
ENHANCED mode, 100/full AN, 1000 ms framework poll and conditional controller
bounds are application inputs. Startup refuses any field mismatch. The selected
P4 IO_MUX and saved board netlist support the signal mapping; neither proves
power, strap or clock measurements.

The application acquires lwIP/default loop, netif, shared bus mutex, real P4
MAC, native PHY, board readiness, installed driver, glue/attachment and three
app handlers before typed initialization/configuration/SDK reconciliation and
explicit start. It handles a null MAC. The owner task alone advances operations,
stops/restarts once and calls `retire`; callbacks never print, wait for cleanup
or call MDIO. The SDK timer remains the only running link poller. DHCP is
`esp_netif` policy. Peer reachability is explicitly not requested. A cable or
DHCP failure does not change management health; a resolved 10/half link does
not satisfy the selected 100/full application profile.

Before installation, locally created MAC, PHY, bus, netif and owned loop are
released in dependency order where safe, while lwIP's process-wide facility
remains. After installation, all resources and callback contexts stay alive
until restart, including attach/handler/start/stop failures; failed `retire`
does not free them. This follows the native RX lifetime gate and accounts for
partial attach. `Phy::retire` fences timer callbacks and attempts finite stop.
App callbacks do bounded atomic updates; default-loop unregister/glue deletion
has no independently qualified deadline. The application does neither from a
callback. Primary SDK/core and cleanup errors remain distinct from the native
lifecycle journal.

The new native `healthSnapshot` copies managed MDIO health under the instance
gate without I/O. Its report stays separate from configuration trust, acquired
link/profile, SDK event carrier, IP state and unrequested reachability. The
application's generated build-directory identity refreshes on each build and
falls back to `unknown` outside a Git checkout; the committed release Version.h
stays independent. No portable API, driver wrapper, board registry, FieldCore,
EtherCAT master or shared CLI implementation was added.

## Verification and remaining gates

Verification uses Windows PowerShell, GNU 15.1.0 host, CMake 4.0.1/Ninja,
Python 3.12.10, Doxygen 1.13.2 and the pinned unmodified ESP-IDF v6.1/P4
GCC 15.2.0 through `scripts\idf.cmd`. Host test counts and final image hash
are filled from the final verified run in the handoff.

| Layer | Result | Evidence and limit |
| --- | --- | --- |
| Portable host | PASS | Release configure/build, 236/236 CTests |
| Portable consumer | PASS | Separate tests-OFF consumer, 2/2 CTests |
| Pinned native host | PASS | 318/318 Release and 318/318 UBSan-trap CTests: 35 application cases plus existing driver/adapter cases, three tick rates; 45-file actual SDK dependency closure |
| Actual P4 image | PASS | `scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 set-target esp32p4 build`, then incremental build; real image linked, default board gate off |
| Python/research | PASS | 103/103 unittest; ledger 97 registers/52 procedures/35 features; version check |
| Source docs/reference | PASS | Doxygen 291 pages and two rejected adverse fixtures; reference 29 vendor artifacts/27 extracts/1994 links in 247 Markdown files; diff check |
| Installed/exported package | NOT RUN | Later packaging stages own installed consumers |
| Remote CI | NOT RUN | First-execution prompt 71 rule |
| Physical link, DHCP, peer probe, reset/clock | NOT RUN | Board PHY power source unresolved; no fixture operation |
| Reusable installed teardown | BLOCKED | `native_rx_quiescence`; resources retained until system restart |

Acceptance regression input includes acquisition and each handler registration,
partial attach, start/stop/retirement failures, callback bursts and a withheld
console. The 35 registered `example_*` cases include null MAC, board reset and
driver allocation failures; five real glue and three application handler
registration failures; a failed attach after RX pointer publication; 1000
event callbacks without console; foreign lost-IP filtering; delayed expected
STOP and unexpected STOP signalling; two-cycle start/stop and failed event,
timer, MAC stop and MDIO paths. The application retains repaired first-cycle
STOP errors before a later start resets the native cycle journal. Independent
review also corrected stale link-mode reporting and foreign lost-IP effects.
Pinned-driver host tests execute actual checked SDK driver/glue source
with injected SDK boundaries; these do not measure real event-loop latency,
P4 controller waits or electrical behavior. The full linked image proves native
API and symbol integration, not physical networking.

The precommit linked image was 509,776 bytes with SHA-256
`fd5e5930ebd06798e494dc6f6fafd362cc13c5e3610943fb5be523c663646a71`.
Its embedded Git tree status is dirty because this implementation was still
uncommitted. The committed clean-identity rebuild and SHA are recorded in the
final handoff, avoiding a self-referential report commit.

Commands, from the repository root (all PASS):

```text
cmake -S . -B build/p40-host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/p40-host -j4
ctest --test-dir build/p40-host --output-on-failure --no-tests=error
cmake -S test/consumer -B build/p40-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF -DDP8382X_BUILD_PINNED_IDF_TESTS=OFF
cmake --build build/p40-consumer -j4
ctest --test-dir build/p40-consumer --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p40-agent-tests -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p40-agent-tests -j2
ctest --test-dir build/p40-agent-tests --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p40-native-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf '-DCMAKE_C_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error' '-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error'
cmake --build build/p40-native-ubsan -j4
ctest --test-dir build/p40-native-ubsan --output-on-failure --no-tests=error
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 set-target esp32p4 build
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_ledger.py
python tools/verify_references.py
python tools/check_doxygen.py
python scripts/generate_version.py check
git diff --check
```

README, CHANGELOG, example guide, native guide and progress were reviewed and
updated. No version increment, new package manifest, SDK fork or workflow is
appropriate for this stage. The source audit, stage code, tests and checks are
committed and synchronized on `main`; the final commit ID belongs in the
handoff to avoid a self-reference.
