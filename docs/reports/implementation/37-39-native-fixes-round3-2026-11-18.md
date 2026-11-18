# Native adapter third-round review, 2026-11-18

Maintenance of completed prompts 37-39; prompt 40 remains unstarted. Entry was
clean `main` at `5bee64074dc65d7ed2350ca849a585b9811644c4`, equal to
`origin/main` after fetch. Root/parent AGENTS, COMMON, prompts 37-39, engineering,
verification and evidence rules, architecture, variants and references were
rechecked. No SDK, sibling or vendor file is changed. Remote CI remains prompt 71;
`native_rx_quiescence` remains BLOCKED with installed resources retained.

This report answers the seven review items individually and supersedes the
affected transient/P4-busy and board-failure dispositions in the historical
[round-2 report](37-39-native-fixes-round2-2026-11-18.md). The
[native guide](../../esp-idf.md) and public Doxygen describe current behavior.

The lead owns production code, shared declarations, documentation, integration
and Git. Two independent gpt-6.2-astra high-reasoning reviewers inspected
trust/controller and board/deadline paths, respectively, then added disjoint
regressions. This model choice supports reasoning about SDK ordering, uncertain
effects and time domains. A gpt-6.2-sol high-reasoning reviewer performed the
read-only source/convention inventory. Build directories are separate and ignored.
The lead reviewed each contribution and the complete production/test diff.

## Current evidence

The supplied `review998/b/probe2.cpp` cases Y, U and V were rerun against the
entry production sources: Y retained UP for one poll after BMCR reset invalidated
trust; U treated persistent FAILED_CLOSED as transient; V demonstrated that
gate-denied polls do not advance the observation counter. Coherent reset plus
DOWN also wrongly reported ordinary successful observation. The separate
`review998/a/rvprobe.cpp` R1 confirmed conservative fence/MAC cleanup for a
never-started core operation still in CONTROL after forbidden uninstall.
These probes inform registered regressions, not standalone acceptance claims.

SDK evidence remains ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`, clean checkout at
`C:/esp/v6.1/esp-idf`. The host harness hashes its selected 45 source/header blobs
at every tick rate. Relevant original paths are:

- `components/esp_eth/src/esp_eth.c`: link polling ignores the PHY callback
  return; uninstall deletes its timer before PHY deinit and does not prove MAC
  idle when that callback rejects.
- `components/esp_eth/src/mac/esp_eth_mac_esp.c`, lines 211-250 at the pin:
  [P4 MDIO methods](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/mac/esp_eth_mac_esp.c)
  reject a busy controller before issuing a command, and successful issued
  commands wait for idle. This proves an unissued rejected frame, not that
  unexplained prior controller activity is safe to reuse.
- `components/soc/esp32p4/register/hw_ver1/soc/emac_reg.h` and its `hw_ver3`
  counterpart: EMAC_GB is set on command issue and cleared on completion.
- `components/esp_driver_gpio/src/gpio.c`, lines 249-254: GPIO validity is checked
  before writing. An arbitrary application callback can also perform other
  actions; its returned SDK error alone cannot attest no effect. The fixture
  writes a latch before configuring direction, so later failure can follow an effect.

Reopened original [SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf),
pages 15, 18-19, 34 and 54, and the current
[official TI PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf), still revision I.
BMCR table 8-19 identifies reset in progress. Section 8.3.11.1 requires shared
SMI silence for 50 ms after power-up and 2 ms after RESET release. The 25 us
pulse minimum excludes debounce capacitors; clock qualification remains board
owned. No register values or timing constants change in this correction.

Fresh sibling inventories were clean, with unchanged full revisions:

| Repository / revision | Reopened paths and chosen behavior |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `src/TMP1x2Operations.cpp:194-236`, `test/jobs/test_jobs.cpp:228-257`: separate aggregate deadline and callback count, retained late effects. Adapt to whole MDIO batches. |
| PCA9555 `0aec2212894910e0d872c3e511fbcd740837c5f7` | `include/PCA9555/Config.h`, `src/PCA9555.cpp:846-917`, `examples/espidf_basic/main/main.cpp:143-160,217-250`, `test/test_basic.cpp:705,774,2477`: distinguish proved non-attempt from possibly applied writes and independent cleanup. Reject the example's ignored old-handle removal error at 200-209. |
| TCA9548A `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `src/TCA9548A.cpp:99-131`, `test/test_driver/test_driver.cpp:1246-1270`, `examples/espidf_basic/main/main.cpp:181-221`: invalidate before status-only reset and retain uncertain effects/failed cleanup. |
| INA228 `8e439d6c99ece1353cd9f555deed0314794412d9` | `examples/esp_idf/basic/main/Ina228IdfI2cTransport.cpp:19-57,107-170,278-294`, `test/test_basic.cpp:4156,4922,5040`: exact native error provenance and retained handles/effects. Do not transfer temporary I2C-handle disposal to unsafe Ethernet destruction. |
| FieldCore-node `c40ea8341edf8d665460b902cafefbe857a7f73f` | `src/i2c/I2cTask.cpp:1305-1307,1660-1698,2019-2025`, `src/devices/i2c/Ina228Module.cpp:635-678`: distinguish route-cache faults from physical reset/recovery affecting multiple bindings; invalidate cached evidence without I/O. |

Sibling AGENTS, Status headers, examples/CLIs, tests, Python runners and local
workflow definitions were reviewed read-only. No I2C NACK categories, transfer
granularity, offline latch or scheduler policy is imported. This repository has
no workflow directory yet; no remote CI operation was performed.

## Finding dispositions

| Item | Disposition and current behavior |
| --- | --- |
| 1 | **Confirmed and fixed.** After a successful live query, recheck agreement before unresolved UP, coherent DOWN or publication. A reset discovered in that query immediately records NOT_INITIALIZED, latches stopRequired and withdraws an earlier UP. A failing query retains its primary error and also uses the refreshed agreement. Both Y and reset-with-DOWN are regressions. |
| 2 | **Confirmed and fixed.** FAILED_CLOSED invalidates SDK agreement, independently of controller quarantine. An admitted running callback requires owner stop and withdraws UP; preflight records NOT_INITIALIZED with shared-readiness context. A raced failed query retains its actual failure with stopRequired. Temporary BOARD_PREPARING and mutex contention remain transient. Stop/restart alone does not clear a shared failed-readiness latch: external disposition or restart is required. |
| 3 | **Confirmed; proposed fix qualified by evidence.** Before any pin callback, with known timing and reusable controller, restore the prior admission. This is AVAILABLE only if it was AVAILABLE; untouched BOOT_CLOSED must stay closed. Merge the POR floor before draining, preserve every later quiet floor, and refresh time before requesting a lock wait. Any invoked pin callback, failed controller release or unknown readiness keeps FAILED_CLOSED. Tests cover both prior states, deadline/short-tick/drain/poll denial, applied/uncertain assertion and controller release failure. |
| 4 | **Necessary relation correct; factory rejection not selected.** An owed DOWN needs remaining allowance strictly greater than notificationUs. cleanupBudgetUs remains an explicitly positive attempt allowance, not guaranteed DOWN or completed cleanup. A tiny attempt can close framework admission or retain a fence, then exposes stopRequired and pending DOWN for later explicit stop. Requiring the total to exceed N would still not ensure sufficient remaining allowance after preceding work. Public contracts now state this necessary condition and limitation. Tests at 1/10/11 us with N=10 verify non-admitted versus admitted DOWN and one eventual accepted transition. |
| 5a | **Documented limitation, strengthened.** Gate-denied polls do not count; stale UP has no wall-time bound under persistent contention. This now sits directly beside maxObservationMisses and in the transient table/guide. No arbitrary upper cap is added: it cannot fix gate starvation. Application observation-age and scheduling policy remains external. V is registered. |
| 5b | **Documentation gap fixed.** Shared RESET_N requires invalidation under every affected peer's owner, quiescence before the external effect, then reinitialization/reconciliation. An ambiguous callback uses AMBIGUOUS_MUTATION; only a proved completed reset may attest EXTERNAL_RESET. Separate reset nets still share SMI silence without automatically resetting peer settings. No topology registry is introduced. |
| 5c | **Confirmed; stronger correction selected.** P4 busy under the complete-batch mutex is unproved controller idle. A mutex bypass is possible, not uniquely proved. Preserve successful samples, mark the rejected frame unattempted, retain exact ESP_ERR_INVALID_STATE as MDIO_ERROR, record failed idle assurance separately and quarantine all participants. It is no longer a transient observation miss. |
| 6a | **Not a defect in the error category; clarified.** MDIO_ERROR/ESP_FAIL in batch releaseStatus is a terminal transport ownership failure, independent of the primary frame result. It does not invent a frame. Its message now explicitly identifies mutex release. A pre-transaction initialization drain remains BUSY/CONTROLLER_QUARANTINED plus native journal evidence. Tests cover successful and failing frame prefixes and exact primary/release precedence. |
| 6b | **Confirmed and fixed.** Preserve DEADLINE, POLL_BUDGET and BOUND_ADMISSION at the observed decision, through board/cleanup receipts and synchronous SDK forwarding. No post-hoc clock guess relabels an actual SDK timeout. A disjoint tagged receipt retains the existing first/latest mailbox ordering. Tests cover gate/fence/board caps, sub-tick admission and SDK event timeout exactly at the owner deadline. |
| 7 | **Accepted conservative behavior; not a supported-route defect.** Forbidden uninstall during a never-started core CONTROL operation may require independent fence/direct MAC stop. SDK FSM STOP/timer deletion do not attest hardware idle, so skipping cleanup on this evidence would weaken safety. No START means no synthetic STOP event. The deleted SDK timer is never touched, accepted stop receipts are retained, and core cancel/result delivery remains independently owed. R1 behavior is asserted in registered tests. |

Independent review also found a directly related release race: another owner
could quarantine the controller after successful unlock but before this callback
sampled controllerReusable. That produced an internally invalid OK/OK/nonreusable
receipt for the earlier successful transaction. Capture this transaction's idle
assurance while still locked; actual release failure overrides it. A registered
two-owner regression makes the peer fail immediately after unlock, preserves the
first read output and verifies the next request is blocked by shared quarantine.

Final independent review also reproduced invalidation succeeding in the gap
between successful observation and publication admission. The latter could then
publish stale UP with stopRequired already true. Observation validation now
hands the held gate directly to the same publication procedure; it stages its
in-flight marker before releasing for SDK callbacks. Invalidation during those
callbacks returns BUSY and must be retried before external mutation. A regression
fails on the old interleaving and proves the retry/administrative-DOWN path.
The same review separated a short STARTING notification reserve from actual
deadline expiry using BOUND_ADMISSION, with no frame or UP on rejection.

## Verification and remaining gates

Windows GNU 15.1.0/Ninja Release retains assertions. Native suites compile
production adapter/core and actual hash-verified SDK driver/glue sources, with
instrumented MAC/RTOS boundaries. UBSan uses both C and C++ flags
`-fsanitize=undefined -fsanitize-undefined-trap-on-error`. These tests establish
software ordering and receipts, not physical concurrency or timing.

| Layer / command | Result |
| --- | --- |
| Native Release: `cmake -S test/esp_idf -B build/r3-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf`, build, CTest | PASS, 283/283; 92 suites at each 1000/100/333 Hz plus 7 pinned-driver suites |
| Native UBSan: same source/options in `build/r3-ubsan` with the flags above, build, CTest | PASS, 283/283 |
| Portable Release: prescribed `build/host` configure/build/CTest | PASS, 236/236 |
| Tests-OFF consumer: `test/consumer` separately configured/built in `build/r3-consumer`, CTest | PASS, 2/2 |
| P4 SDK link: `scripts\idf.cmd -C test/esp_idf/adapter_link -B C:\esp\build\dp8382x-fix3 set-target esp32p4 build`, subsequent `build` | PASS, actual ESP32-P4 image, SDK v6.1, RISC-V GCC 15.2.0; build only, no flash. Final clean-commit identity recorded in the handoff. |
| Doxygen: `python tools/check_doxygen.py` | PASS, 291 HTML pages and 2 rejected negative fixtures |
| References / ledger: `python tools/verify_references.py`, `python tools/verify_ledger.py` | PASS, 29 artifacts/27 extracts; 97 registers/52 procedures/35 features |
| Python: `python -m unittest discover -s test -p "test_*.py" -v` | PASS, 103/103 |
| Version: `python scripts/generate_version.py check`; `git diff --check` | PASS |
| Remote CI | NOT RUN; first-execution checkpoint remains prompt 71 |
| Physical fixture / wall-time measurements | NOT RUN; no hardware operation requested or performed |
| Reusable installed teardown | BLOCKED, unchanged native_rx_quiescence; retain driver/MAC/glue/RX contexts until system restart |

README, CHANGELOG, architecture, ownership decisions, native guide, Doxygen and
progress are reconciled. API/feature signatures, register ledger, source gates,
example plans and generated version need no change: no new feature or support
claim is introduced. The existing fixture links the corrected adapter unchanged.
First-run test failures in the new board test were fixture placement errors:
at 100/1000 Hz the POR floor needed an additional tick before the intended
assertion. The corrected scenario isolates post-assertion failure explicitly.
Removing the observation/publication gate gap also invalidated an old test's
injection point. That test now covers both real orderings: stop wins before the
timer enters, or publication is already admitted and stop retains its drain
obligation. Both terminate with the required DOWN, fence and observed MAC stop.

Completed checkpoint IDs and verified upstream equality are recorded in the
handoff, avoiding a commit that cites its own hash.
