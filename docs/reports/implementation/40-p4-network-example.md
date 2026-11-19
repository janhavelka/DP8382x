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
partial attach, start/stop/retirement failures and callback bursts. Testing
owner progress with a blocked output sink belongs to prompt 47. The 35
registered `example_*` cases include null MAC, board reset and
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

## Follow-up review, 2026-11-19

An independent OpenAI audit (`aud-20261119-093117-0ad388`, gpt-6.2-astra,
complex depth) of `c2023d9` against this prompt and the shared contracts was
compared with a separate Claude review of the same code before any change.
The first review's dispositions follow. Later checks against the preserved
ELF corrected the original assessment of F5. Executing regressions and
artifact checks are identified separately below.

| Finding | Disposition |
| --- | --- |
| F2 service loop ignored `stopRequired` and `eventLost` | Fixed. Each 100 ms step copies the journal (no I/O). `stopRequired` ends the session with an owner stop and no restart; `eventLost` uses the event-only `retryLinkEvent` and stops after 10 undelivered steps. Tests `running_stop_required`, `lost_event_retry`, `lost_event_unresolved`. |
| F3 a BUSY poll or failed take stranded the operation token | Fixed. BUSY is retried, never treated as terminal; cancel/take retry within finite counts; an undelivered token is kept, reported, and retirement is refused rather than claimed. Tests `operation_busy_retry`, `operation_result_owed`. |
| F1 pre-install unwind order and quarantined mutex | Fixed. The never-installed PHY (the borrower) is destroyed before the MAC; a failed PHY destruction keeps MAC and mutex; a quarantined bus keeps its possibly held mutex. Tests `mac_cleanup_failure`, `quarantined_mutex_retained`. |
| F5 the gate-closed image omitted operational symbols | False positive. The original clean `c2023d9` P4 ELF already contains all twelve symbols listed below. The volatile gate makes retention explicit across compiler optimizations. |
| F7 invented NO_MEM for null factories; typed board cause lost | Fixed. Null factories record their stage with an unknown cause; adapter steps copy the typed journal before the mapped return; the final line names the failing stage. A repaired stop's historical failure is still captured before the next start resets the journal. |
| F8 stale-but-copied link evidence printed CURRENT; BUSY phase printed STOPPED | Fixed. CURRENT requires a sample at most three poll periods old (`linkAgeMs`, else `AGED`); unavailable phase prints `BUSY`. Test `link_evidence_age`. |
| F11 cleanup allowance could not admit an owed DOWN | Fixed. `cleanupBudgetUs` is 500 ms, above the 101 ms notification allowance. |
| F9 some start/stop failure tests accept several outcomes | Partly fixed: the success path now requires MAINTENANCE and a completed session. Deferred to prompt 54: exact per-variant outcome oracles, attach sub-step failures and reset-release failure after assertion. |
| F4 ending the netif session before retirement | Deferred. STOP is queued. The glue handler calls a void netif action on later dispatch, and the application lacks confirmation of completed netif shutdown. Installed resources stay alive under `native_rx_quiescence`. Prompt 54 tracks delayed or withheld dispatch. |
| F6 console backpressure on the owner task | Deferred to prompt 47. The synchronous UART writer can wait on its lock or FIFO. The callback-burst case exercises callback independence. Owner progress with blocked output still needs the bounded writer and a withheld-sink test. |
| F10 example translation unit outside the SDK closure manifest | Deferred to prompt 55: give the example configuration its own pinned closure manifest. |

Auditor questions that remain open and are owned elsewhere: boot-time PHY
power and REF_CLK validity (board gate, physical prompt 72), main and timer
task stack margins (`native_wall_time`, prompt 72), and whether the optional
local-peer probe should be selectable (prompt 47 CLI). None was invented here.

The [subsequent independent review](40-42-independent-review.md) resolved
the F9 start/stop, reset-release and attach cases and corrected the main-task
stack configuration. It records fresh host and image results for both P4 examples.

## Second auditor follow-up

Checked `aud-20261119-100555-4ccdbe` against synchronized `main` at
`9933109`. The existing netif definitions were already in the host test at
`0c50ed0`. Prompts 47, 54 and 55 already contained the planned follow-up
work at `9933109`. The earlier audit and this audit use independent finding
numbers. Second-review F5/F6 correspond to the deferred netif-session and
synchronous-output findings above. F1/F2/F8 already have executing STOP,
tenth-retry and negotiation regressions.

The original clean gate-closed image at `c2023d91c4984ee2b1c72b96d863a5bf87791f32`
is 509,776 bytes, SHA-256
`865813bd18cf046b7b4cff90d463f93eda2266c41bacfbac7128d785661d2f02`.
Its ELF contains six SDK and six native PHY entry points. Both this image
and the precommit image above have build identities independent of the
library version. To reproduce the symbol inspection in PowerShell, set
`$build` to the historical or follow-up P4 build directory and run

```powershell
$nm = (Select-String -Path "$build/CMakeCache.txt" -Pattern '^CMAKE_NM:FILEPATH=(.+)$').Matches[0].Groups[1].Value
& $nm -C --defined-only "$build/dp8382x_p4_ethernet.elf" | Select-String -CaseSensitive ' T (esp_eth_driver_install|esp_eth_start|esp_eth_new_netif_glue|esp_netif_attach|esp_eth_mac_new_esp32|esp_netif_new)$| T DP8382x::EspIdf::Phy::(create|install|prepareBoard|start|stop|retire)\('
Get-FileHash "$build/dp8382x_p4_ethernet.bin"
Get-Content "$build/basic_identity/ExampleIdentity.h"
(Get-Content "$build/config/sdkconfig.json" -Raw | ConvertFrom-Json).DP8382X_BOARD_READY
```

| Second review | Disposition and executing cases |
| --- | --- |
| F3 terminal status and delivery | A known terminal failure survives take-only contention. Report the owed token, its terminal status and the delivery failure separately. On owner expiry, preserve a subsequently delivered cancellation result. `terminal_delivery_failure`, `terminal_busy`, `owner_timeout_delivered`. |
| F4 journal attribution | A cycle's historical first error is identified as journal evidence. The direct adapter return and the observation ending service have their own receipts. Before a new start, allow three cached journal reads with at most two one-tick yields. Withhold restart on persistent contention. `lost_event_retry`, `running_failures`, `journal_retry`, `journal_unavailable`. |
| F7 null factories | The stage records an application failure with an unreported cause. Neither the SDK nor core error domain receives a fabricated diagnosis. Covered by the netif, MAC and glue allocation cases. |
| F9 profile verdict | Output uses yes/no for a fresh resolved match/mismatch and unknown for unavailable evidence. Four registered `profile_*` cases check the actual emitted line, including age and BUSY phase. |
| F10 pre-install unwind | `live_cleanup_order` observes destruction of the live PHY before MAC deletion. `live_cleanup_denied` denies PHY destruction, checks retention of both dependencies and exercises an explicit cleanup retry. Prompt 54 retains cross-module stop and dispatch coverage. |
| F11 reproducibility | The full pinned suite passed 352/352 in both Release and UBSan-trap. This includes 63 registered basic-example cases at the example's configured 1,000 Hz and the native adapter suites at 100, 333 and 1,000 Hz. Use `ctest --test-dir build/p40-native-ubsan -N -R "^example_"` to list the examples. |

Follow-up P4 link with ESP-IDF `fff9895c`: closed board gate, 16 KiB main
stack, source identity `9933109` with the follow-up edits present. The image
was 514,688 bytes, SHA-256
`7a7de6600125c0a8dc2084420925936e378d39606362607e141d36eb4e5ad9c2`.
The symbol command above returned all twelve entries. Build with
`scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-audit40 -D SDKCONFIG=C:/esp/build/dp8382x-audit40/sdkconfig -D IDF_TARGET=esp32p4 build`.
This run executed registered regressions and inspected artifacts. No physical
board or first-set remote CI was run.
